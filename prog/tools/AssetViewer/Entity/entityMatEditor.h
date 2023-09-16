#pragma once

#include <libTools/dagFileRW/dagMatRemapUtil.h>
#include <propPanel2/c_control_event_handler.h>
#include <assets/asset.h>
#include <shaders/dag_shMaterialUtils.h>
#include <3d/dag_materialData.h>
#include <EASTL/string.h>
#include <EASTL/array.h>
#include <de3_objEntity.h>
#include "shaderPropSeparators.h"
#include <ska_hash_map/flat_hash_map2.hpp>
#include <dag/dag_vector.h>
#include "entityMatFileResource.h"

class EntityMaterialEditor : public ControlEventHandler
{
public:
  void begin(DagorAsset *asset, IObjEntity *asset_entity);
  bool end();

  void saveAllChanges();
  void fillPropPanel(PropertyContainerControlBase &panel);

private:
  struct EntityLodMatData
  {
    eastl::unique_ptr<DagMatFileResourcesHandler> fileRes;
    dag::Vector<EntityMatProperties> matProperties;
    dag::Vector<ShaderMaterial *> matShaders;

    dag::Vector<int> getChangedMatIds() const;
    bool isMatChanged(int mat_id) const;
    void save(const dag::Vector<int> &mats_to_save_ids);
  };

  void fillMatPropPanel(int lod, int mat_id, PropertyContainerControlBase &mat_panel);
  void refillMatPropPanel(int lod, int mat_id, PropertyContainerControlBase &editor_panel);
  static void addShaderProps(int mat_gui_elements_baseid, PropertyContainerControlBase &mat_panel, const dag::Vector<MatVarDesc> &vars,
    const dag::Vector<int> &var_indices = {});
  void addShaderPropsCategories(int mat_gui_elements_baseid, PropertyContainerControlBase &mat_panel,
    const dag::Vector<MatVarDesc> &vars, const ShaderSeparatorToPropsType &shaderSeparatorToProps);
  void updateAssetShaderMaterial(int lod, int mat_id);
  bool applyMaterialOverride(int lod, int mat_id);
  void clearMaterialOverrides(int lod);
  void performAddOrRemoveScriptVars(int lod, int mat_id, bool is_to_add);
  const char *performTextureSelection(int lod, int mat_id, int tex_slot);
  const char *performShaderClassSelection(int lod, int mat_id);
  const char *performProxyMatSelection(int lod, int mat_id);
  bool setMatTexture(int lod, int mat_id, int tex_slot, const char *tex_name);
  void replaceMatShaderClass(int lod, int mat_id, const char *new_shclass_name);

  void copyMatProperties(int lod, int mat_id);
  void pasteMatProperties(int lod, int mat_id);

  void reloadCurrentAsset();
  bool notifyCurrentAssetNeedsReload();
  void onReloaded();

  bool hasChanges() const;
  Tab<String> getAvailableShaderClasses() const;
  DataBlock makeCurMatPropertiesBlk(int lod, int mat_id) const;

  void onChange(int pcb_id, PropertyContainerControlBase *panel) override;
  void onClick(int pcb_id, PropertyContainerControlBase *panel) override;

  static dag::Vector<DagorAsset *> getLoadedRendInstAssets(const DagorAssetMgr &assetMgr);
  static bool isAssetDependsOnProxyMat(const DagorAssetMgr &assetMgr, const DagorAsset &asset, const char *proxy_mat_asset_file_name);
  void modifyAllAffectedLoadedAssets(const char *proxy_mat_Name);

  bool active = false;
  bool isReloading = false;

  dag::Vector<EntityLodMatData> matDataPerLod;
  String assetSrcFolderPath;
  String assetName;

  IObjEntity *entity;
  DataBlock appBlk;
  const DataBlock *shRemapBlk;
  ska::flat_hash_map<eastl::string, ShaderSeparatorToPropsType> shaderPropSeparators;

  void transferChangesToLods(int lod, int mat_id, PropertyContainerControlBase *panel);

  static void applyToCommonVars(dag::Vector<MatVarDesc> &dst_vars, const dag::Vector<MatVarDesc> &src_vars,
    eastl::function<void(MatVarDesc &, const MatVarDesc &)> common_var_cb);

  static EntityMatProperties propertiesCopyBuffer; // Should stay alive between assets. So, it's static.
};
