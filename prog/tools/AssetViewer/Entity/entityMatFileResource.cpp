// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "entityMatFileResource.h"

#include <obsolete/dag_cfg.h>
#include <shaders/dag_shMaterialUtils.h>
#include <de3_interface.h>
#include "../av_appwnd.h"

dag::Vector<MatVarDesc> extract_used_available_vars(const dag::Vector<MatVarDesc> &vars, const char *shclass)
{
  dag::Vector<MatVarDesc> result;
  dag::Vector<ShaderVarDesc> shClassVarList = get_shclass_script_vars_list(shclass);
  for (int varId = 0; varId < vars.size(); ++varId)
  {
    const MatVarDesc &var = vars[varId];
    if (!var.usedInMaterial)
      continue;

    // Do not include twosided among the final script vars. Its value belongs to the material flag.
    if (varId == MAT_VAR_TWOSIDED)
      continue;

    if (varId < MAT_SPECIAL_VAR_COUNT)
      result.push_back(var);
    else if (eastl::find_if(shClassVarList.begin(), shClassVarList.end(),
               [&var](auto &shclass_var) { return shclass_var.name == var.name; }) != shClassVarList.end())
      result.push_back(var);
  }
  return result;
}

MatFileResourceDagMat::MatFileResourceDagMat(DagData &dag_data, int dag_mat_id, DataBlock &sh_remap_blk) :
  dagData(dag_data), dagMatId(dag_mat_id), shRemapBlk(sh_remap_blk)
{}

SimpleString MatFileResourceDagMat::getShaderClass() const
{
  return SimpleString(shRemapBlk.getStr(dagData.matlist[dagMatId].classname, dagData.matlist[dagMatId].classname));
}

dag::Vector<MatVarDesc> MatFileResourceDagMat::getVars() const
{
  dag::Vector<MatVarDesc> vars = make_mat_vars_from_script(dagData.matlist[dagMatId].script, getShaderClass());

  if ((dagData.matlist[dagMatId].mater.flags & DAG_MF_2SIDED) != 0)
  {
    MatVarDesc &var = vars[MAT_VAR_TWOSIDED];
    var.usedInMaterial = true;
    var.value.i = 1;
  }

  return vars;
}

eastl::array<SimpleString, MAXMATTEXNUM> MatFileResourceDagMat::getTextureNames() const
{
  unsigned usedTexMask = get_shclass_used_tex_mask(getShaderClass());
  eastl::array<SimpleString, MAXMATTEXNUM> textures;
  for (int texSlot = 0; texSlot < MAXMATTEXNUM; ++texSlot)
  {
    int dagTexId = dagData.matlist[dagMatId].mater.texid[texSlot];
    textures[texSlot] =
      dagTexId != DAGBADMATID && (usedTexMask & (1 << texSlot)) ? DagorAsset::fpath2asset(dagData.texlist[dagTexId]).c_str() : "";
  }
  return textures;
}

DataBlock MatFileResourceDagMat::getPropertiesBlk() const
{
  DataBlock propsBlk = make_dag_one_mat_blk(dagData, dagMatId);
  propsBlk.setStr("class", getShaderClass().c_str());
  return propsBlk;
}

void MatFileResourceDagMat::setDataAndSave(const EntityMatProperties *mat_properties)
{
  dagData.matlist[dagMatId].classname = mat_properties->shClassName;
  dagData.matlist[dagMatId].script = make_mat_script_from_vars(extract_used_available_vars(mat_properties->vars, getShaderClass()));

  unsigned usedTexMask = get_shclass_used_tex_mask(getShaderClass());
  for (int texSlot = 0; texSlot < DAGTEXNUM; ++texSlot)
  {
    dagData.matlist[dagMatId].mater.texid[texSlot] = DAGBADMATID;
    if (!(usedTexMask & (1 << texSlot)))
      continue;

    String texFilePath;
    if (DagorAsset *texAsset = DAEDITOR3.getAssetByName(mat_properties->textures[texSlot]))
    {
      // Assets under #gameRes/... (in a CDK build) do not have source file names.
      if (texAsset->getSrcFileName())
        texFilePath = texAsset->getTargetFilePath();
      else
        texFilePath.printf(64, "%s*", texAsset->getName());
    }

    int dagTexId = find_value_idx(dagData.texlist, texFilePath);
    if (dagTexId < 0 && !texFilePath.empty())
    {
      dagTexId = dagData.texlist.size();
      dagData.texlist.push_back(texFilePath);
    }
    if (dagTexId >= 0)
      dagData.matlist[dagMatId].mater.texid[texSlot] = dagTexId;
  }

  remove_unused_textures(dagData);

  if (mat_properties->vars[MAT_VAR_TWOSIDED].usedInMaterial && mat_properties->vars[MAT_VAR_TWOSIDED].value.i != 0)
    dagData.matlist[dagMatId].mater.flags |= DAG_MF_2SIDED;
  else
    dagData.matlist[dagMatId].mater.flags &= ~DAG_MF_2SIDED;
}

MatFileResourceProxyMat::MatFileResourceProxyMat(DagorAsset *proxymat_asset, DataBlock &sh_remap_blk) :
  proxyMatAsset(proxymat_asset), proxyMatBlk(proxymat_asset->props), shRemapBlk(sh_remap_blk)
{}

SimpleString MatFileResourceProxyMat::getShaderClass() const
{
  const char *origShClassName = proxyMatBlk.getStr("class", "");
  return SimpleString(shRemapBlk.getStr(origShClassName, origShClassName));
}

dag::Vector<MatVarDesc> MatFileResourceProxyMat::getVars() const
{
  dag::Vector<MatVarDesc> vars = make_mat_vars_from_script(make_mat_script_from_props(proxyMatBlk), getShaderClass());

  if (proxyMatBlk.getBool("twosided", false))
  {
    MatVarDesc &var = vars[MAT_VAR_TWOSIDED];
    var.usedInMaterial = true;
    var.value.i = 1;
  }

  return vars;
}

eastl::array<SimpleString, MAXMATTEXNUM> MatFileResourceProxyMat::getTextureNames() const
{
  unsigned usedTexMask = get_shclass_used_tex_mask(getShaderClass());
  eastl::array<SimpleString, MAXMATTEXNUM> textures;
  for (int texSlot = 0; texSlot < MAXMATTEXNUM; ++texSlot)
  {
    const char *texPath = (usedTexMask & (1 << texSlot)) ? proxyMatBlk.getStr(String(0, "tex%d", texSlot), "") : "";
    textures[texSlot] = *texPath ? DagorAsset::fpath2asset(texPath).c_str() : texPath;
  }
  return textures;
}

DataBlock MatFileResourceProxyMat::getPropertiesBlk() const { return proxyMatAsset->props; }

void MatFileResourceProxyMat::setDataAndSave(const EntityMatProperties *mat_properties)
{
  replace_mat_shclass_in_props_blk(proxyMatBlk, mat_properties->shClassName);
  replace_mat_vars_in_props_blk(proxyMatBlk, extract_used_available_vars(mat_properties->vars, getShaderClass()));

  const MatVarDesc &var = mat_properties->vars[MAT_VAR_TWOSIDED];
  const bool propVarTwoSided = var.usedInMaterial && var.value.i != 0;
  const int paramIndex = proxyMatBlk.findParam("twosided");
  if (paramIndex >= 0) // Overwrite if it is already set and they differ.
  {
    if (proxyMatBlk.getParamType(paramIndex) != DataBlock::TYPE_BOOL || proxyMatBlk.getBool(paramIndex) != propVarTwoSided)
      proxyMatBlk.setBool("twosided", propVarTwoSided);
  }
  else if (propVarTwoSided) // Set if it is not the default value.
  {
    proxyMatBlk.setBool("twosided", propVarTwoSided);
  }

  eastl::array<SimpleString, MAXMATTEXNUM> filteredTextures;
  unsigned usedTexMask = get_shclass_used_tex_mask(getShaderClass());
  for (int texSlot = 0; texSlot < DAGTEXNUM; ++texSlot)
    filteredTextures[texSlot] = (usedTexMask & (1 << texSlot)) ? mat_properties->textures[texSlot] : "";
  replace_mat_textures_in_props_blk(proxyMatBlk, filteredTextures);

  proxyMatAsset->props.setFrom(&proxyMatBlk);
  proxyMatBlk.saveToTextFileCompact(proxyMatAsset->getTargetFilePath());
}

DagMatFileResourcesHandler::DagMatFileResourcesHandler(const char *dag_file_name, const DagorAsset *asset) : asset(asset)
{
  dagFileName = dag_file_name;
  load_scene(dag_file_name, dagData);

  DataBlock appBlk = DataBlock(::get_app().getWorkspace().getAppPath());
  const DataBlock *curTypeBlk = appBlk.getBlockByNameEx("assets")->getBlockByNameEx("build")->getBlockByNameEx(asset->getTypeStr());
  shRemapBlk = *curTypeBlk->getBlockByNameEx("remapShaders");

  int dagMatCnt = dagData.matlist.size();
  matFileResources.resize(dagMatCnt);
  curProxyMatNames.resize(dagMatCnt);
  for (int dagMatId = 0; dagMatId < dagMatCnt; ++dagMatId)
    switchToProxyMat(dagMatId, detect_proxymat_classname(dagData.matlist[dagMatId].classname).c_str());
}

void DagMatFileResourcesHandler::switchToProxyMat(int dag_mat_id, const char *proxymat_asset_name)
{
  curProxyMatNames[dag_mat_id] = proxymat_asset_name;
  if (!curProxyMatNames[dag_mat_id].empty())
  {
    DagorAsset *proxyMatAsset =
      asset->getMgr().findAsset(DagorAsset::fpath2asset(proxymat_asset_name), asset->getMgr().getAssetTypeId("proxymat"));
    if (proxyMatAsset)
    {
      matFileResources[dag_mat_id] = eastl::make_unique<MatFileResourceProxyMat>(proxyMatAsset, shRemapBlk);
    }
    else
    {
      logerr("Can't resolve proxymat: %s for %s", proxymat_asset_name, asset->getName());
      matFileResources[dag_mat_id] = eastl::make_unique<MatFileResource>();
    }
  }
  else
  {
    if (dagData.matlist[dag_mat_id].classname.empty())
      matFileResources[dag_mat_id] = eastl::make_unique<MatFileResource>();
    else
      matFileResources[dag_mat_id] = eastl::make_unique<MatFileResourceDagMat>(dagData, dag_mat_id, shRemapBlk);
  }
}

void DagMatFileResourcesHandler::setDataAndSave(const dag::Vector<const EntityMatProperties *> &mat_properties)
{
  bool needWriteDag = false;
  for (auto props : mat_properties)
  {
    int dagMatId = props->dagMatId;
    matFileResources[dagMatId]->setDataAndSave(props);

    SimpleString curProxyMatName = getCurProxyMatName(dagMatId);
    SimpleString origProxyMatName = getOrigProxyMatName(dagMatId);
    needWriteDag |= curProxyMatName.empty() || curProxyMatName != origProxyMatName;
    if (!curProxyMatName.empty())
      dagData.matlist[dagMatId].classname = String(0, "%s:proxymat", curProxyMatNames[dagMatId].c_str());
  }
  if (needWriteDag)
    write_dag(dagFileName, dagData);
}
