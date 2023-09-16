#pragma once

#include <libTools/dagFileRW/dagMatRemapUtil.h>
#include <util/dag_simpleString.h>
#include <dag/dag_vector.h>
#include <shaders/dag_shaderCommon.h>
#include <shaders/dag_shaderMatData.h>
#include <EASTL/array.h>
#include <EASTL/unique_ptr.h>
#include <assets/asset.h>
#include <3d/dag_materialData.h>
#include <libTools/shaderResBuilder/processMat.h>
#include "entityMatProperties.h"

class MatFileResource
{
public:
  virtual ~MatFileResource() {}

  // Returns remapped shader name (can be different with what is stored in a physical file).
  virtual SimpleString getShaderClass() const { return SimpleString{}; }
  // Returns N + MAT_SPECIAL_VAR_COUNT vars (N is the number of the shader class static vars). Check 'usedInMaterial' if it's used in
  // material.
  virtual dag::Vector<MatVarDesc> getVars() const { return dag::Vector<MatVarDesc>{}; }
  // Returns texture names (not file paths).
  virtual eastl::array<SimpleString, MAXMATTEXNUM> getTextureNames() const { return eastl::array<SimpleString, MAXMATTEXNUM>{}; }
  virtual DataBlock getPropertiesBlk() const { return DataBlock{}; }

  virtual void setDataAndSave(const EntityMatProperties *mat_properties) {}
};

class MatFileResourceDagMat : public MatFileResource
{
public:
  MatFileResourceDagMat(DagData &dag_data, int dag_mat_id, DataBlock &sh_remap_blk);

  virtual SimpleString getShaderClass() const override;
  virtual dag::Vector<MatVarDesc> getVars() const override;
  virtual eastl::array<SimpleString, MAXMATTEXNUM> getTextureNames() const override;
  virtual DataBlock getPropertiesBlk() const override;

  virtual void setDataAndSave(const EntityMatProperties *mat_properties) override;

private:
  DagData &dagData;
  int dagMatId;
  DataBlock &shRemapBlk;
};

class MatFileResourceProxyMat : public MatFileResource
{
public:
  MatFileResourceProxyMat(DagorAsset *proxymat_asset, DataBlock &sh_remap_blk);

  virtual SimpleString getShaderClass() const override;
  virtual dag::Vector<MatVarDesc> getVars() const override;
  virtual eastl::array<SimpleString, MAXMATTEXNUM> getTextureNames() const override;
  virtual DataBlock getPropertiesBlk() const override;

  virtual void setDataAndSave(const EntityMatProperties *mat_properties) override;

private:
  DagorAsset *proxyMatAsset;
  DataBlock proxyMatBlk;
  DataBlock &shRemapBlk;
};

class DagMatFileResourcesHandler
{
public:
  EA_NON_COPYABLE(DagMatFileResourcesHandler); // MatFileResource* classes work with dagData and shRemapBlk.

  DagMatFileResourcesHandler(const char *dag_file_name, const DagorAsset *asset);

  inline int getMaterialCount() const { return dagData.matlist.size(); }
  inline SimpleString getMaterialName(int dag_mat_id) const { return dagData.matlist[dag_mat_id].name; }
  inline SimpleString getShaderClass(int dag_mat_id) const { return matFileResources[dag_mat_id]->getShaderClass(); }
  inline dag::Vector<MatVarDesc> getVars(int dag_mat_id) const { return matFileResources[dag_mat_id]->getVars(); }
  inline eastl::array<SimpleString, MAXMATTEXNUM> getTextureNames(int dag_mat_id) const
  {
    return matFileResources[dag_mat_id]->getTextureNames();
  }
  inline DataBlock getPropertiesBlk(int dag_mat_id) const { return matFileResources[dag_mat_id]->getPropertiesBlk(); }

  inline SimpleString getCurProxyMatName(int dag_mat_id) const { return curProxyMatNames[dag_mat_id]; }
  inline SimpleString getOrigProxyMatName(int dag_mat_id) const
  {
    return SimpleString(detect_proxymat_classname(dagData.matlist[dag_mat_id].classname.c_str()).c_str());
  }
  void switchToProxyMat(int dag_mat_id, const char *proxymat_asset_name);

  void setDataAndSave(const dag::Vector<const EntityMatProperties *> &mat_properties);

private:
  dag::Vector<eastl::unique_ptr<MatFileResource>> matFileResources;
  dag::Vector<SimpleString> curProxyMatNames;
  SimpleString dagFileName;
  DagData dagData;
  DataBlock shRemapBlk;
  const DagorAsset *asset;
};
