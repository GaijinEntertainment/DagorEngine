// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <generic/dag_tab.h>
#include <generic/dag_ptrTab.h>

class MaterialData;
class DataBlock;
class DagorAssetMgr;
class ILogWriter;


struct IProcessMaterialData
{
  //! optionally processes material data (not in-place) and returns pointer to mat (no changes made) or cloned and altered mat
  virtual MaterialData *processMaterial(MaterialData *mat, bool need_tex = true) = 0;
};

struct GenericTexSubstProcessMaterialData : public IProcessMaterialData
{
  struct TexReplRec;
  Tab<TexReplRec *> trr;
  DagorAssetMgr *mgrForProxyMat = nullptr;
  ILogWriter *log = nullptr;
  int proxymat_atype = -1;
  const char *assetName = nullptr;

  GenericTexSubstProcessMaterialData(const char *a_name, const DataBlock &props, DagorAssetMgr *mgr_allow_proxy_mat = nullptr,
    ILogWriter *l = nullptr);
  ~GenericTexSubstProcessMaterialData();
  virtual MaterialData *processMaterial(MaterialData *mat, bool need_tex) override;
  bool mayProcess() const { return trr.size() || mgrForProxyMat; }
};

String detect_proxymat_classname(const char *mat_classname);
String make_mat_script_from_props(const DataBlock &mat_props);
const char *replace_asset_name(const char *mat_tex, String &tmp_stor, const char *a_name);
void override_material_with_props(MaterialData &mat, const DataBlock &mat_props, const char *a_name);
void override_materials(PtrTab<MaterialData> &mat_list, const DataBlock &material_overrides, const char *a_name);

// Gets the name of the texture assets that a proxy material uses.
// proxymat_props: properties of the the proxy material
// owner_asset_name: name of the owner asset (for example a rendInst) that uses the proxy material.
//   Proxy materials can have texture references like this: "$(ASSET_NAME)_pivot_pos.dds", owner_asset_name is used in those.
// asset_names: output containing the referred texture asset names
void get_textures_used_by_proxymat(const DataBlock &proxymat_props, const char *owner_asset_name, dag::Vector<String> &asset_names);
