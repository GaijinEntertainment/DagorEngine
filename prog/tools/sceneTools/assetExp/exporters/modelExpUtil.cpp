#include "modelExp.h"
#include <assets/asset.h>
#include <assets/assetMgr.h>
#include <assets/assetMsgPipe.h>
#include <libTools/dagFileRW/dagUtil.h>
#include <libTools/dagFileRW/dagFileNode.h>
#include <libTools/util/makeBinDump.h>
#include <libTools/shaderResBuilder/processMat.h>
#include <libTools/shaderResBuilder/globalVertexDataConnector.h>
#include <shaders/dag_shaders.h>
#include <ioSys/dag_dataBlock.h>
#include <osApiWrappers/dag_direct.h>
#include <util/dag_texMetaData.h>
#include <util/dag_simpleString.h>
#include <util/dag_strUtil.h>

extern bool shadermeshbuilder_strip_d3dres;

BEGIN_DABUILD_PLUGIN_NAMESPACE(modelExp)

DataBlock appBlkCopy;
DagorAsset *cur_asset = NULL;
ILogWriter *cur_log = NULL;
Tab<IDagorAssetRefProvider::Ref> tmp_refs(tmpmem);
namespace modelexp
{
SimpleString texRefNamePrefix, texRefNameSuffix;
}
using namespace modelexp;

static bool find_broken_ref(dag::ConstSpan<IDagorAssetRefProvider::Ref> r, const char *name)
{
  for (int i = 0; i < r.size(); i++)
    if (r[i].getBrokenRef() && stricmp(name, r[i].getBrokenRef()) == 0)
      return true;
  return false;
}
static bool find_valid_ref(dag::ConstSpan<IDagorAssetRefProvider::Ref> r, DagorAsset *a)
{
  for (int i = 0; i < r.size(); i++)
    if (r[i].getAsset() && a == r[i].refAsset)
      return true;
  return false;
}

static bool get_dag_tex_and_proxymat_list(const char *dag_fn, Tab<String> &list, Tab<String> &proxy_mat_list, IProcessMaterialData *pm)
{
  if (!pm)
    return ::get_dag_textures(dag_fn, list);

  AScene sc;
  if (!load_ascene(dag_fn, sc, LASF_MATNAMES | LASF_NOMESHES | LASF_NOSPLINES | LASF_NOLIGHTS, false))
    return false;
  struct GatherMats : public Node::NodeEnumCB
  {
    PtrTab<MaterialData> mats;
    virtual int node(Node &n) override
    {
      if (n.mat)
        for (unsigned int materialNo = 0; materialNo < n.mat->subMatCount(); materialNo++)
          if (MaterialData *subMat = n.mat->getSubMat(materialNo))
            if (find_value_idx(mats, subMat) < 0)
              mats.push_back(subMat);
      return 0;
    }
  } gm;
  sc.root->enum_nodes(gm);

  String tmp_stor;
  for (int i = 0; i < gm.mats.size(); i++)
  {
    for (int j = 0; j < MAXMATTEXNUM; j++)
      if (gm.mats[i]->mtex[j] != BAD_TEXTUREID)
      {
        String nm(DagorAsset::fpath2asset(TextureMetaData::decodeFileName(get_managed_texture_name(gm.mats[i]->mtex[j]), &tmp_stor)));
        if (nm.length() && *nm.end(1) == '*')
          nm.pop_back();
        gm.mats[i]->mtex[j] = add_managed_texture(String(0, "%s%s%s*", texRefNamePrefix.str(), nm, texRefNameSuffix.str()));
      }

    if (gm.mats[i])
    {
      const String proxyMatName = detect_proxymat_classname(gm.mats[i]->className);
      if (!proxyMatName.empty())
        proxy_mat_list.emplace_back(proxyMatName);
    }

    if (MaterialData *m = pm->processMaterial(gm.mats[i]))
      gm.mats[i] = m;
    else if (gm.mats[i])
      return false;
    for (int j = 0; j < MAXMATTEXNUM; j++)
      if (gm.mats[i]->mtex[j] != BAD_TEXTUREID)
        list.push_back() = get_managed_texture_name(gm.mats[i]->mtex[j]);
  }
  return true;
}
void add_dag_texture_and_proxymat_refs(const char *dag_fn, Tab<IDagorAssetRefProvider::Ref> &tmpRefs, DagorAssetMgr &mgr,
  IProcessMaterialData *pm)
{
  if (shadermeshbuilder_strip_d3dres)
    return;
  static Tab<String> list(tmpmem);
  static Tab<String> proxyMatList(tmpmem);
  static String s, tmp_stor;
  IDagorAssetMsgPipe &pipe = mgr.getMsgPipe();

  list.clear();
  proxyMatList.clear();
  if (get_dag_tex_and_proxymat_list(dag_fn, list, proxyMatList, pm))
  {
    for (int i = 0; i < list.size(); i++)
    {
      s.printf(64, "%s%s%s", texRefNamePrefix.str(), DagorAsset::fpath2asset(TextureMetaData::decodeFileName(list[i], &tmp_stor)),
        texRefNameSuffix.str());
      DagorAsset *tex_a = mgr.findAsset(s, mgr.getTexAssetTypeId());
      if (!tex_a)
      {
        if (*s.str())
        {
          s += ":tex";
          post_error(pipe, "%s: broken tex reference %s", dag_fn, list[i].str());
        }
        if (!find_broken_ref(tmpRefs, s))
        {
          IDagorAssetRefProvider::Ref &r = tmpRefs.push_back();
          r.flags = 0;
          r.setBrokenRef(s);
          if (s.empty())
            r.flags |= IDagorAssetRefProvider::RFLG_OPTIONAL;
        }
      }
      else if (!find_valid_ref(tmpRefs, tex_a))
      {
        IDagorAssetRefProvider::Ref &r = tmpRefs.push_back();
        r.flags = 0;
        r.refAsset = tex_a;
      }
    }

    const int proxyMatAssetTypeId = mgr.getAssetTypeId("proxymat");

    for (const String &proxyMatName : proxyMatList)
    {
      s = DagorAsset::fpath2asset(proxyMatName);
      DagorAsset *proxyMatAsset = mgr.findAsset(s, proxyMatAssetTypeId);
      if (!proxyMatAsset)
      {
        s += ":proxymat";
        post_error(pipe, "%s: broken proxymat reference %s", dag_fn, proxyMatName.str());

        if (!find_broken_ref(tmpRefs, s))
        {
          IDagorAssetRefProvider::Ref &r = tmpRefs.push_back();
          r.flags = 0;
          r.setBrokenRef(s);
        }
      }
      else if (!find_valid_ref(tmpRefs, proxyMatAsset))
      {
        IDagorAssetRefProvider::Ref &r = tmpRefs.push_back();
        r.flags = 0;
        r.refAsset = proxyMatAsset;
      }
    }
  }
}
const DataBlock &get_process_mat_blk(const DataBlock &a_props, const char *asset_type)
{
  static DataBlock processMat_stor;
  const DataBlock *ri_blk = appBlkCopy.getBlockByNameEx("assets")->getBlockByNameEx("build")->getBlockByNameEx(asset_type);

  const DataBlock *processMat = ri_blk->getBlockByName("processMat");
  if (const DataBlock *b = a_props.getBlockByName("processMat"))
  {
    if (processMat)
    {
      processMat_stor = *processMat;
      processMat = &processMat_stor;
      for (int i = 0; i < b->blockCount(); i++)
        processMat_stor.addNewBlock(b->getBlock(i), b->getBlock(i)->getBlockName());
      return processMat_stor;
    }
    return *b;
  }
  return processMat ? *processMat : DataBlock::emptyBlock;
}
bool add_proxymat_dep_files(const char *dag_fn, Tab<SimpleString> &files, DagorAssetMgr &mgr)
{
  AScene sc;
  if (!load_ascene(dag_fn, sc, LASF_MATNAMES | LASF_NOMESHES | LASF_NOSPLINES | LASF_NOLIGHTS, false))
    return false;
  struct GatherMats : public Node::NodeEnumCB
  {
    PtrTab<MaterialData> mats;
    virtual int node(Node &n) override
    {
      if (n.mat)
        for (unsigned int materialNo = 0; materialNo < n.mat->subMatCount(); materialNo++)
          if (MaterialData *subMat = n.mat->getSubMat(materialNo))
            if (find_value_idx(mats, subMat) < 0)
              mats.push_back(subMat);
      return 0;
    }
  } gm;
  sc.root->enum_nodes(gm);

  int proxymat_atype = mgr.getAssetTypeId("proxymat");
  for (int i = 0; i < gm.mats.size(); i++)
  {
    String proxyMatName = detect_proxymat_classname(gm.mats[i]->className);
    if (proxyMatName.empty())
      continue;
    DagorAsset *mat_a = mgr.findAsset(DagorAsset::fpath2asset(proxyMatName), proxymat_atype);
    if (!mat_a)
    {
      logerr("cannot resolve proxymat: %s for %s", gm.mats[i]->className, dag_fn);
      return false;
    }
    files.push_back() = mat_a->getSrcFilePath();
  }
  return true;
}

void setup_tex_subst(const DataBlock &a_props)
{
  texRefNamePrefix = a_props.getStr("texref_prefix", NULL);
  texRefNameSuffix = a_props.getStr("texref_suffix", NULL);
}
void reset_tex_subst()
{
  texRefNamePrefix = NULL;
  texRefNameSuffix = NULL;
}
END_DABUILD_PLUGIN_NAMESPACE(modelExp)
