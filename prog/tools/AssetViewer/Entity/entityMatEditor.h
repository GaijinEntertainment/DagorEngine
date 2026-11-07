// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <libTools/dagFileRW/dagMatRemapUtil.h>
#include <propPanel/c_control_event_handler.h>
#include <propPanel/control/dragAndDropHandler.h>
#include <assets/asset.h>
#include <shaders/dag_shMaterialUtils.h>
#include <3d/dag_materialData.h>
#include <util/dag_string.h>
#include <EASTL/string.h>
#include <EASTL/array.h>
#include <EASTL/unique_ptr.h>
#include <de3_objEntity.h>
#include "shaderPropSeparators.h"
#include <ska_hash_map/flat_hash_map2.hpp>
#include <dag/dag_vector.h>
#include "entityMatFileResource.h"
#include "materialHighlightRenderer.h"

class Node;

class ProxymatAssetDragAndDropHandler;
class TextureAssetDragAndDropHandler;

String get_asset_dag_file_path(const DagorAsset &asset, int lod, const DataBlock **out_lod_props = nullptr);

class EntityMaterialEditor : public PropPanel::ControlEventHandler
{
public:
  void loadSettings(const DataBlock &settings);
  void saveSettings(DataBlock &settings) const;

  void begin(DagorAsset *asset, IObjEntity *asset_entity);
  bool end();

  // Returns false when the user refuses to save. The material overrides are cleared in both cases.
  bool saveAllChanges();
  void fillPropPanel(PropPanel::ContainerPropertyControl &panel);

  int swapTexture(int for_lod, const char *old_tex_name, const DagorAsset *new_tex, PropPanel::ContainerPropertyControl &panel);

  void saveForUndo(int lod, int material_index, EntityMatProperties &undo_material_properties);
  void loadFromUndo(int lod, int material_index, const EntityMatProperties &undo_material_properties);

  // The material is valid during the frame of the hover event and the frame after it
  ShaderMaterial *getHoveredShaderMaterial(int *out_lod = nullptr) const;
  // riex_override replaces the entity TM and seed when set (rendinst entities only)
  void renderMaterialHighlight(rendinst::riex_handle_t riex_override = rendinst::RIEX_HANDLE_NULL);

  friend class ProxymatAssetDragAndDropHandler;
  friend class TextureAssetDragAndDropHandler;

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

  void fillMatPropPanel(int lod, int mat_id, PropPanel::ContainerPropertyControl &mat_panel);
  void refillMatPropPanel(int lod, int mat_id, PropPanel::ContainerPropertyControl &editor_panel);
  static void addShaderProps(int mat_gui_elements_baseid, PropPanel::ContainerPropertyControl &mat_panel,
    const dag::Vector<MatVarDesc> &vars, const dag::Vector<int> &var_indices = {},
    dag::ConstSpan<const ShaderVariableLimits *> var_limits = {});
  void addShaderPropsCategories(int mat_gui_elements_baseid, PropPanel::ContainerPropertyControl &mat_panel,
    const dag::Vector<MatVarDesc> &vars, const ShaderSeparatorToPropsType &shaderSeparatorToProps);
  void updateAssetShaderMaterialInternal(int lod, int mat_id);
  void updateAssetShaderMaterial(int lod, int mat_id);
  bool applyMaterialOverride(int lod, int mat_id);
  void clearMaterialOverrides(int lod);
  void performAddOrRemoveScriptVars(int lod, int mat_id, bool is_to_add);
  const char *performTextureSelection(int lod, int mat_id, int tex_slot);
  const char *performShaderClassSelection(int lod, int mat_id);
  const char *performProxyMatSelection(int lod, int mat_id);
  void performSwitchToNewProxyMat(int lod, int mat_id, const char *new_proxy_mat_name);
  bool setMatTexture(int lod, int mat_id, int tex_slot, const char *tex_name);
  void replaceMatShaderClass(int lod, int mat_id, const char *new_shclass_name);

  void copyMatProperties(int lod, int mat_id);
  void pasteMatProperties(int lod, int mat_id, PropPanel::ContainerPropertyControl &editor_panel);

  void reloadCurrentAsset();
  bool notifyCurrentAssetNeedsReload();
  void onReloaded();
  E3DCOLOR getMaterialHighlightColor() const;

  bool hasChanges() const;
  bool reportBadEmissionColor(int lod, int mat_id) const;
  Tab<String> getAvailableShaderClasses() const;
  DataBlock makeCurMatPropertiesBlk(int lod, int mat_id) const;

  void onChange(int pcb_id, PropPanel::ContainerPropertyControl *panel) override;
  void onClick(int pcb_id, PropPanel::ContainerPropertyControl *panel) override;
  void onHover(int pcb_id, PropPanel::ContainerPropertyControl *panel) override;

  void handleDragAndDrop(int pcb_id, PropPanel::ContainerPropertyControl *panel);

  static dag::Vector<DagorAsset *> getLoadedRendInstAssets(const DagorAssetMgr &assetMgr);
  static bool isAssetDependsOnProxyMat(const DagorAssetMgr &assetMgr, const DagorAsset &asset, const char *proxy_mat_asset_file_name);
  void modifyAllAffectedLoadedAssets(const char *proxy_mat_Name);

  const ShaderVariableLimits *getShaderVariableLimits(const EntityMatProperties &mat_props, const MatVarDesc &var) const;

  void beginUndo(int lod, int material_index, const EntityMatProperties &material_properties);
  void endUndo(const char *operation_name, bool accept);

  static int getMaterialIndexByName(const DagMatFileResourcesHandler &file_res, const char *name);
  static void createSubMaterialToMaterialIndexMap(const char *dag_name, const Node &node, const DagMatFileResourcesHandler &file_res,
    dag::Vector<int> &map);
  static void countMaterialUsage(const char *dag_name, const Node &node, const DagMatFileResourcesHandler &file_res,
    dag::Span<int> materials);

  static const E3DCOLOR DEFAULT_HIGHLIGHT_COLOR;

  bool active = false;
  bool isReloading = false;
  bool showTriangleCount = false;
  MaterialHighlightRenderer::Mode materialHighlightMode = MaterialHighlightRenderer::Mode::None;
  bool hasCustomHighlightColor = false;
  E3DCOLOR customHighlightColor = DEFAULT_HIGHLIGHT_COLOR;
  MaterialHighlightRenderer materialHighlightRenderer;
  bool reportedShaderMaterialMatchError = false;

  int hoveredMatLod = -1;
  int hoveredMatId = -1;
  unsigned hoveredFrameNo = 0;

  dag::Vector<EntityLodMatData> matDataPerLod;
  String assetSrcFolderPath;
  String assetName;

  IObjEntity *entity;
  DataBlock appBlk;
  const DataBlock *shRemapBlk;
  ska::flat_hash_map<eastl::string, ShaderDescriptor> shaderPropSeparators;

  eastl::unique_ptr<ProxymatAssetDragAndDropHandler> proxymatDropHandler;
  eastl::unique_ptr<TextureAssetDragAndDropHandler> textureDropHandler;

  void transferChangesToLods(int lod, int mat_id, PropPanel::ContainerPropertyControl *panel);

  static void applyToCommonVars(dag::Vector<MatVarDesc> &dst_vars, const dag::Vector<MatVarDesc> &src_vars,
    eastl::function<void(MatVarDesc &, const MatVarDesc &)> common_var_cb);

  static EntityMatProperties propertiesCopyBuffer; // Should stay alive between assets. So, it's static.
  static String propertiesCopyBufferProxyMatName;  // ProxyMat name of the copied material. Empty if not a ProxyMat.
};

class ProxymatAssetDragAndDropHandler : public PropPanel::IDropTargetHandler
{
public:
  ProxymatAssetDragAndDropHandler(EntityMaterialEditor *me) : matEditor(me) {}
  PropPanel::DragAndDropResult handleDropTarget(int pcb_id, PropPanel::ContainerPropertyControl *panel) override;

private:
  EntityMaterialEditor *matEditor;
};

class TextureAssetDragAndDropHandler : public PropPanel::IDropTargetHandler
{
public:
  TextureAssetDragAndDropHandler(EntityMaterialEditor *me) : matEditor(me) {}
  PropPanel::DragAndDropResult handleDropTarget(int pcb_id, PropPanel::ContainerPropertyControl *panel) override;

private:
  EntityMaterialEditor *matEditor;
};
