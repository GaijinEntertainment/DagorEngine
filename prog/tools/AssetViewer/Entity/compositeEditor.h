// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "compositeEditorGizmoClient.h"
#include "compositeEditorRefreshType.h"
#include "compositeEditorToolbar.h"
#include "compositeEditorTreeData.h"
#include <EASTL/unique_ptr.h>
#include <EditorCore/ec_interface.h>
#include <propPanel/c_control_event_handler.h>
#include <propPanel/commonWindow/treeviewPanel.h>
#include <propPanel/control/menu.h>

class CompositeEditorTree;
class CompositeEditorPanel;
class DagorAsset;
class IEntityViewPluginInterface;

class CompositeEditor : public PropPanel::ControlEventHandler,
                        public PropPanel::IMenuEventHandler,
                        public PropPanel::ITreeViewEventHandler
{
public:
  bool begin(DagorAsset *asset, IObjEntity *entity);
  bool end();
  bool expectingAssetReload() const { return assetExpectedToReload.length() > 0; }
  void onCompositeEditorVisibilityChanged(bool shown);
  void fillCompositeTree();

  const CompositeEditorTreeDataNode *getSelectedTreeDataNode() const;
  unsigned getSelectedTreeNodeDataBlockId() const;
  void selectTreeNodeByDataBlockId(unsigned dataBlockId);
  void deleteSelectedNode(bool needsConfirmation = true);
  void updateSelectedNodeTransform(const TMatrix &tm);
  void cloneSelectedNode();

  void setGizmo(IEditorCoreEngine::ModeType mode);
  bool getPreventUiUpdatesWhileUsingGizmo() const { return preventUiUpdatesWhileUsingGizmo; }
  void setPreventUiUpdatesWhileUsingGizmo(bool prevent, bool wasCloning = false);

  void toggleSnapMode(int pcb_id);

  void setEntityViewPluginInterface(IEntityViewPluginInterface &inEntityViewPluginInterface);
  IEntityViewPluginInterface &getEntityViewPluginInterface() const;

  void beginUndo();
  void endUndo(const char *operation_name, bool accept = true);
  void saveForUndo(DataBlock &dataBlock);
  void loadFromUndo(const DataBlock &dataBlock);

  eastl::unique_ptr<CompositeEditorTree> compositeTreeView;
  eastl::unique_ptr<CompositeEditorPanel> compositePropPanel;
  bool autoShow = false;

private:
  // This is mostly for sanity check to ensure that after requesting (or expecting) a reload it really happens.
  class ReloadHelper
  {
  public:
    ReloadHelper(CompositeEditor &editor, const DagorAsset &asset, CompositeEditorRefreshType inRefreshType);
    ~ReloadHelper();

  private:
    CompositeEditor &compositeEditor;
    const String assetName;
    const CompositeEditorRefreshType refreshType;
  };

  // ControlEventHandler
  void onChange(int pcb_id, PropPanel::ContainerPropertyControl *panel) override;
  void onClick(int pcb_id, PropPanel::ContainerPropertyControl *panel) override;

  // IMenuEventHandler
  int onMenuItemClick(unsigned id) override;

  // ITreeViewEventHandler
  void onTvSelectionChange(PropPanel::TreeBaseWindow &tree, PropPanel::TLeafHandle new_sel) override;
  bool onTvContextMenu(PropPanel::TreeBaseWindow &tree_base_window, PropPanel::ITreeInterface &tree) override;

  void fillCompositeTreeInternal(bool keepExpansionState);
  void fillCompositePropPanel();
  bool saveComposit();
  void revertChanges();
  bool canSwitchToAnotherAsset();
  void updateAssetFromTree(CompositeEditorRefreshType refreshType);
  void updateGizmo();
  void updateToolbarVisibility();

  void onDelayedRefresh(const DagorAsset *asset, CompositeEditorRefreshType refreshType);
  static void onDelayedRefreshEntity(const DagorAsset *asset);
  static void onDelayedRefreshEntityAndTransformation(const DagorAsset *asset);
  static void onDelayedRefreshEntityAndCompositEditor(const DagorAsset *asset);
  static void onDelayedRevert(const DagorAsset *asset);
  static void onDelayedSelectAsset(const DagorAsset *asset);

  static void focusViewport();

  IEntityViewPluginInterface *entityViewPluginInterface = nullptr;
  DagorAsset *editedAsset = nullptr;
  CompositeEditorTreeData treeData;
  CompositeEditorTreeDataNode *selectedTreeDataNode = nullptr;
  String assetExpectedToReload;
  CompositeEditorRefreshType reloadRefreshType = CompositeEditorRefreshType::Unset;
  bool ignoreTreeSelectionChangePanelRefresh = false;
  bool modified = false;

  CompositeEditorGizmoClient gizmoClient;
  CompositeEditorToolbar toolbar;
  IEditorCoreEngine::ModeType lastSelectedGizmoMode = IEditorCoreEngine::ModeType::MODE_None;
  bool preventUiUpdatesWhileUsingGizmo = false;
};
