// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <libTools/shaderResBuilder/processMat.h>
#include <libTools/dagFileRW/textureNameResolver.h>
#include <libTools/util/iLogWriter.h>
#include <assets/asset.h>
#include <assets/assetMgr.h>
#include <regExp/regExp.h>
#include <3d/dag_materialData.h>
#include <math/integer/dag_IPoint3.h>
#include <ioSys/dag_dataBlock.h>
#include <util/dag_string.h>
#include <util/dag_strUtil.h>
#include <util/dag_texMetaData.h>

static inline D3dColor dagcolor(const IPoint3 &i) { return D3dColor(i[0] / 255.0, i[1] / 255.0, i[2] / 255.0); }

struct GenericTexSubstProcessMaterialData::TexReplRec
{
  RegExp reClassName;
  RegExp reSrcTexName;
  RegExp reCheckParams;
  SimpleString destTexNamePattern;
  int srcSlot = -1;
  int destSlot = -1;
};

GenericTexSubstProcessMaterialData::GenericTexSubstProcessMaterialData(const char *a_name, const DataBlock &props,
  DagorAssetMgr *mgr_allow_proxy_mat, ILogWriter *l)
{
  assetName = a_name;
  mgrForProxyMat = mgr_allow_proxy_mat;
  log = l;
  proxymat_atype = mgrForProxyMat ? mgrForProxyMat->getAssetTypeId("proxymat") : -1;
  for (int i = 0; i < props.blockCount(); i++)
  {
    TexReplRec *r = new TexReplRec;
    bool rec_ok = true;

    r->srcSlot = props.getBlock(i)->getInt("fromTexSlot", -1);
    r->destSlot = props.getBlock(i)->getInt("toTexSlot", -1);
    if (r->srcSlot < 0 || r->srcSlot >= MAXMATTEXNUM || r->destSlot < 0 || r->destSlot >= MAXMATTEXNUM)
    {
      logerr("bad fromTexSlot=%d or toTexSlot=%d in block %d", r->srcSlot, r->destSlot, i);
      rec_ok = false;
    }

    if (!r->reClassName.compile(props.getBlock(i)->getBlockName(), "i"))
    {
      logerr("invalid regexp for shader name in block %d: %s", i, props.getBlock(i)->getBlockName());
      rec_ok = false;
    }
    if (!r->reSrcTexName.compile(props.getBlock(i)->getStr("texRE", ""), "i"))
    {
      logerr("invalid regexp in block %d: %s=%s", i, "texRE", props.getBlock(i)->getStr("texRE", ""));
      rec_ok = false;
    }
    if (!r->reCheckParams.compile(props.getBlock(i)->getStr("testScriptRE", ""), "i"))
    {
      logerr("invalid regexp in block %d: %s=%s", i, "testScriptRE", props.getBlock(i)->getStr("testScriptRE", ""));
      rec_ok = false;
    }
    r->destTexNamePattern = props.getBlock(i)->getStr("repl", "$0");
    if (r->destTexNamePattern.empty())
    {
      logerr("empty repl in block %d", i);
      rec_ok = false;
    }
    if (rec_ok)
      trr.push_back(r);
    else
      delete r;
  }
}
GenericTexSubstProcessMaterialData::~GenericTexSubstProcessMaterialData() { clear_all_ptr_items(trr); }

MaterialData *GenericTexSubstProcessMaterialData::processMaterial(MaterialData *mat, bool need_tex)
{
  if (mat)
  {
    String proxyMatName = detect_proxymat_classname(mat->className);
    if (proxyMatName.size())
    {
      if (mgrForProxyMat)
      {
        DagorAsset *mat_a = mgrForProxyMat->findAsset(DagorAsset::fpath2asset(proxyMatName), proxymat_atype);
        if (!mat_a)
        {
          if (log)
            log->addMessage(log->ERROR, "cannot resolve proxymat: %s", mat->className);
          else
            logerr("cannot resolve proxymat: %s", mat->className);
          mat->className = "null";
          return mat;
        }

        override_material_with_props(*mat, mat_a->props, assetName);
      }
      else
      {
        // The need_tex condition is only here to lessen the number of times this message is shown for the same asset and material.
        if (need_tex)
          logerr("Asset \"%s\" uses a proxymat in material \"%s\" but allowProxyMat is not enabled in the .folder.blk file.",
            assetName, mat->matName.c_str());
      }
    }
  }

  if (!need_tex || !mat || !trr.size()) // GenericTexSubstProcessMaterialData processes/replaces only textures for now
    return mat;

  MaterialData *new_mat = mat;
  ITextureNameResolver *scene_tex_resolve_cb = get_global_tex_name_resolver();
  String realTexName;
  for (int i = 0; i < trr.size(); i++)
  {
    TexReplRec &r = *trr[i];
    if (r.reClassName.test(mat->className) && r.reCheckParams.test(mat->matScript))
    {
      if (mat->mtex[r.srcSlot] == BAD_TEXTUREID)
      {
        mat->mtex[r.destSlot] = BAD_TEXTUREID;
        continue;
      }

      if (!r.reSrcTexName.test(get_managed_texture_name(mat->mtex[r.srcSlot])))
        continue;
      if (new_mat == mat)
      {
        // clone material
        new_mat = new MaterialData;
        new_mat->matName = mat->matName;
        new_mat->className = mat->className;
        new_mat->matScript = mat->matScript;
        new_mat->flags = mat->flags;
        memcpy(new_mat->mtex, mat->mtex, sizeof(new_mat->mtex));
      }

      char *repl = r.reSrcTexName.replace2(r.destTexNamePattern);
      if (scene_tex_resolve_cb && scene_tex_resolve_cb->resolveTextureName(repl, realTexName))
      {
        memfree(repl, strmem);
        repl = str_dup(realTexName, strmem);
      }
      mat->mtex[r.destSlot] = add_managed_texture(repl);
      memfree(repl, strmem);
      // debug("processMaterial(%p, %s, %s) repl(%d,%d) %s -> %s", mat, mat->matName, mat->className, r.srcSlot, r.destSlot,
      //   get_managed_texture_name(mat->mtex[r.srcSlot]), get_managed_texture_name(mat->mtex[r.destSlot]));
    }
  }
  return mat;
}

String detect_proxymat_classname(const char *mat_classname)
{
  int slen = (int)strlen(mat_classname);
  const char *checkSuffix[] = {":proxymat", ".proxymat.blk"};
  for (int i = 0; i < sizeof(checkSuffix) / sizeof(*checkSuffix); ++i)
  {
    int sufxLen = (int)strlen(checkSuffix[i]);
    if (str_ends_with_c(mat_classname, checkSuffix[i], slen, sufxLen))
      return String::mk_str(mat_classname, slen - sufxLen);
  }
  return String();
}

String make_mat_script_from_props(const DataBlock &mat_props)
{
  String script;
  dblk::iterate_params_by_name_and_type(mat_props, "script", DataBlock::TYPE_STRING, [&](int pidx) {
    if (!script.empty())
      script += '\n';
    script += mat_props.getStr(pidx);
  });
  return script;
}

const char *replace_asset_name(const char *mat_tex, String &tmp_stor, const char *a_name)
{
  if (!a_name || !strchr(mat_tex, '$'))
    return mat_tex;
  tmp_stor = mat_tex;
  return tmp_stor.replaceAll("$(ASSET_NAME)", a_name);
}

void override_material_with_props(MaterialData &mat, const DataBlock &mat_props, const char *a_name)
{
  mat.className = mat_props.getStr("class", "null");

  mat.flags = 0;
  if (mat_props.getBool("twosided", false))
    mat.flags |= MATF_2SIDED;

  for (auto &t : mat.mtex)
    t = BAD_TEXTUREID;
  String nm, tmp_stor, tmp_stor2;
  for (int ti = 0, ti_num = mat_props.getBool("tex16support", false) ? 16 : 8; ti < ti_num; ti++)
  {
    const char *mat_tex = mat_props.getStr(String(0, "tex%d", ti), "");
    mat_tex = replace_asset_name(mat_tex, tmp_stor2, a_name);
    nm = DagorAsset::fpath2asset(TextureMetaData::decodeFileName(mat_tex, &tmp_stor));
    if (nm.empty() || *nm.end(1) == '*')
      continue;
    nm += "*";
    mat.mtex[ti] = add_managed_texture(nm);
  }

  mat.mat.power = mat_props.getReal("power", 8);
  mat.mat.amb = dagcolor(mat_props.getIPoint3("amb", IPoint3(255, 255, 255)));
  mat.mat.diff = dagcolor(mat_props.getIPoint3("diff", IPoint3(255, 255, 255)));
  mat.mat.spec = dagcolor(mat_props.getIPoint3("spec", IPoint3(255, 255, 255)));
  mat.mat.emis = dagcolor(mat_props.getIPoint3("emis", IPoint3(0, 0, 0)));

  if (mat.mat.spec == D3dColor(0.0f, 0.0f, 0.0f))
    mat.mat.power = 0.0f;

  mat.matScript = make_mat_script_from_props(mat_props);
}

void override_materials(PtrTab<MaterialData> &mat_list, const DataBlock &material_overrides, const char *a_name)
{
  for (int i = 0; const DataBlock *matOverrideProps = material_overrides.getBlock(i); ++i)
  {
    const char *name = matOverrideProps->getStr("name");
    auto *matPos =
      eastl::find_if(mat_list.begin(), mat_list.end(), [name](auto &matData) { return strcmp(name, matData->matName.c_str()) == 0; });
    if (matPos != mat_list.end())
      override_material_with_props(*matPos->get(), *matOverrideProps, a_name);
  }
}

void get_textures_used_by_proxymat(const DataBlock &proxymat_props, const char *owner_asset_name, dag::Vector<String> &asset_names)
{
  String assetName, tempString1, tempString2;
  const int textureCount = proxymat_props.getBool("tex16support", false) ? 16 : 8;
  for (int i = 0; i < textureCount; ++i)
  {
    tempString1.printf(8, "tex%d", i);
    const char *textureName = proxymat_props.getStr(tempString1, "");
    textureName = replace_asset_name(textureName, tempString1, owner_asset_name);
    assetName = DagorAsset::fpath2asset(TextureMetaData::decodeFileName(textureName, &tempString2));
    if (assetName.empty() || *assetName.end(1) == '*')
      continue;

    asset_names.push_back(assetName);
  }
}
