// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "compositeEditorGizmoClient.h"
#include "compositeEditorRefreshType.h"
#include "compositeEditorSubStack.h"
#include "compositeEditorToolbar.h"
#include "compositeEditorTreeData.h"
#include <dag/dag_vector.h>
#include <generic/dag_span.h>
#include <EASTL/unique_ptr.h>
#include <EditorCore/ec_interface.h>
#include <propPanel/c_control_event_handler.h>
#include <propPanel/messageQueue.h>
#include <propPanel/commonWindow/treeviewPanel.h>
#include <propPanel/control/menu.h>
#include <propPanel/control/dragAndDropHandler.h>

class CompositeEditorTree;
class CompositeEditorPanel;
class DagorAsset;
class IEntityViewPluginInterface;
class EntityTreeDropHandler;
class ChildEntityDragAndDropHandler;

class CompositeEditor : public PropPanel::ControlEventHandler,
                        public PropPanel::IMenuEventHandler,
                        public PropPanel::ITreeViewEventHandler,
                        public PropPanel::IDelayedCallbackHandler
{
public:
  bool begin(DagorAsset *asset, IObjEntity *entity);
  bool end();
  bool expectingAssetReload() const { return assetExpectedToReload.length() > 0; }
  void onCompositeEditorVisibilityChanged(bool shown);
  void onSnapSettingChanged();
  void fillCompositeTree();
  void updateAssetFromTree(CompositeEditorRefreshType refreshType);
  const CompositeEditorTreeDataNode *getRootTreeDataNode() const { return &treeData.rootNode; }
  CompositeEditorTreeDataNode *getTreeNodeByDataBlockId(unsigned dataBlockId);
  CompositeEditorTreeDataNode *getTreeDataNodeParent(const CompositeEditorTreeDataNode *treeDataNode, int &nodeIndex);
  bool isTreeDataNodeRootNode(CompositeEditorTreeDataNode *treeDataNode);
  TMatrix calcParentMatrix(CompositeEditorTreeDataNode *parentTreeDataNode);

  const CompositeEditorTreeDataNode *getSelectedTreeDataNode() const;
  unsigned getSelectedTreeNodeDataBlockId() const;
  bool areMultipleNodesSelected() const;
  void getSelectedTreeNodeDataBlockIds(dag::Vector<unsigned> &dataBlockIds, unsigned &parentDataBlockId) const;
  void selectTreeNodeByDataBlockId(unsigned dataBlockId, bool multiSelect, bool selectAsParent);

  bool hasSelectedParentRelation() const;
  bool canParentSelectedTreeDataNodes() const;
  void makeSelectedParentRelation();
  void clearSelectedParentRelation();
  bool canSaveSelectedAsComposite();

  void createNode();
  void deleteSelectedNodes(bool needsConfirmation = true);
  void updateSelectedNodeTransform(const TMatrix &tm);
  void updateMultipleNodesTransforms(const dag::Vector<CompositeEditorTreeDataNode *> &nodes, const dag::Vector<TMatrix> &tms);
  void cloneSelectedNode();
  void splitSelectedCompositeNode(bool recursive);

  IObjEntity *getSubEntityByDataBlockId(unsigned dataBlockId);

  void copySelectedNodeParams();
  static bool getNodeParamsFromClipboard(DataBlock &block);
  void pasteParamsToSelectedNode();
  void duplicateSelectedNode();

  void enterSubCompositeEditing();
  void exitSubCompositeEditing();
  void saveSubCompositeEditing();
  void saveSubCompositeAsUnique();
  void revertSubCompositeEditing();
  void applyPendingUniqueAssetSwap();
  bool isEditingSubComposite() const { return !subCompositeStack.isEmpty(); }
  bool isModified() const { return modified; }
  const DagorAsset *getParentCompositeAsset() const
  {
    return subCompositeStack.isEmpty() ? nullptr : subCompositeStack.back().parentAsset;
  }
  IObjEntity *getParentEntityForEditing() const
  {
    return subCompositeStack.isEmpty() ? nullptr : subCompositeStack.back().parentGhostEntity;
  }
  unsigned getSavedSubCompositeDataBlockId() const
  {
    return subCompositeStack.isEmpty() ? 0 : subCompositeStack.back().subCompositeDataBlockId;
  }
  dag::ConstSpan<CompositeEditorSubContext> getSubCompositeStack() const { return subCompositeStack.getFullContext(); }
  // Returns true if a pending transform was applied; false if nothing was pending.
  bool applyPendingCameraTransform();

  void setGizmo(IEditorCoreEngine::ModeType mode);
  bool getPreventUiUpdatesWhileUsingGizmo() const { return preventUiUpdatesWhileUsingGizmo; }
  void setPreventUiUpdatesWhileUsingGizmo(bool prevent, bool wasCloning = false);

  void toggleSnapMode(int pcb_id);
  void openGridSettings();

  void setEntityViewPluginInterface(IEntityViewPluginInterface &inEntityViewPluginInterface);
  IEntityViewPluginInterface &getEntityViewPluginInterface() const;

  CompositeEditorToolbar &getToolbar() { return toolbar; }

  void beginUndo(bool save_selection = false);
  void endUndo(const char *operation_name, bool accept = true);
  void saveForUndo(DataBlock &dataBlock) const;
  void loadFromUndo(const DataBlock &dataBlock, unsigned selected_tree_node_data_block_id,
    const dag::Vector<unsigned> &multi_selected_tree_node_data_block_ids);

  eastl::unique_ptr<CompositeEditorTree> compositeTreeView;
  eastl::unique_ptr<CompositeEditorPanel> compositePropPanel;
  bool autoShow = false;

  friend class ChildEntityDragAndDropHandler;
  friend class EntityTreeDropHandler;

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

  // IDelayedCallbackHandler
  void onImguiDelayedCallback(void *user_data) override;

  void fillCompositeTreeInternal(bool keepExpansionState);
  void fillCompositePropPanel();
  bool saveComposit();
  void revertChanges();
  bool saveSelectedAsNewComposite();
  void splitCompositeInternal(CompositeEditorTreeDataNode *treeDataNode, bool recursive, bool startUndo);
  bool canSwitchToAnotherAsset();
  void updateGizmo();
  void updateToolbarVisibility();

  bool isTreeDataNodeSelected(CompositeEditorTreeDataNode *treeDataNode);
  void getSelectedTreeDataNodes(dag::Vector<CompositeEditorTreeDataNode *> &selectTreeDataNodes);

  void createNode(CompositeEditorTreeDataNode &parent, bool show_dialog = true);

  void cloneSelectedNodeInternal(CompositeEditorRefreshType refreshType);

  void recalcMatrixInParentBase(CompositeEditorTreeDataNode *oldParent, CompositeEditorTreeDataNode *treeDataNode,
    CompositeEditorTreeDataNode *newParent);

  void onDelayedRefresh(const DagorAsset *asset, CompositeEditorRefreshType refreshType);
  static void onDelayedRefreshEntity(const DagorAsset *asset);
  static void onDelayedRefreshEntityAndTransformation(const DagorAsset *asset);
  static void onDelayedRefreshEntityAndCompositEditor(const DagorAsset *asset);
  static void onDelayedRevert(const DagorAsset *asset);
  static void onDelayedRevertAfterUniqueSwap(const DagorAsset *sub_asset);
  static void onDelayedSelectAsset(const DagorAsset *asset);
  static void onDelayedUpdateAssetFromTree(CompositeEditorRefreshType refreshType);

  static void focusViewport();

  IEntityViewPluginInterface *entityViewPluginInterface = nullptr;
  DagorAsset *editedAsset = nullptr;
  CompositeEditorSubStack subCompositeStack;
  CompositeEditorTreeData treeData;
  CompositeEditorTreeDataNode *selectedTreeDataNode = nullptr;
  dag::Vector<CompositeEditorTreeDataNode *> selectedTreeDataNodes;

  int getSelectedTreeDataNodeIndex(unsigned dataBlockId);

  String assetExpectedToReload;
  CompositeEditorRefreshType reloadRefreshType = CompositeEditorRefreshType::Unset;
  bool ignoreTreeSelectionChangePanelRefresh = false;
  bool modified = false;

  CompositeEditorGizmoClient gizmoClient;
  CompositeEditorToolbar toolbar;
  IEditorCoreEngine::ModeType lastSelectedGizmoMode = IEditorCoreEngine::ModeType::MODE_None;
  bool preventUiUpdatesWhileUsingGizmo = false;

  eastl::unique_ptr<EntityTreeDropHandler> treeDropHandler;
  eastl::unique_ptr<ChildEntityDragAndDropHandler> childDropHandler;
};

class EntityTreeDropHandler : public PropPanel::ITreeDropHandler
{
public:
  EntityTreeDropHandler(CompositeEditor &e) : editor(e) {}
  PropPanel::DragAndDropResult onDropTargetDirect(PropPanel::TLeafHandle leaf) override;

private:
  CompositeEditor &editor;
};

class ChildEntityDragAndDropHandler : public PropPanel::IDropTargetHandler
{
public:
  ChildEntityDragAndDropHandler(CompositeEditor &e) : editor(e) {}
  PropPanel::DragAndDropResult handleDropTarget(int pcb_id, PropPanel::ContainerPropertyControl *panel) override;

private:
  CompositeEditor &editor;
};
