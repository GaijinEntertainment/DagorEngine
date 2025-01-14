// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "compositeEditor.h"
#include "../av_cm.h"
#include "../av_appwnd.h"
#include "compositeEditorPanel.h"
#include "compositeEditorTree.h"
#include "compositeEditorUndo.h"
#include "dataBlockIncludeChecker.h"
#include <assets/asset.h>
#include <de3_dataBlockIdHolder.h>
#include <de3_interface.h>
#include <EditorCore/ec_cm.h>
#include <assets/assetUtils.h>
#include <libTools/util/strUtil.h>
#include <osApiWrappers/dag_clipboard.h>
#include <propPanel/control/treeInterface.h>
#include <propPanel/control/menu.h>
#include <util/dag_delayedAction.h>
#include <winGuiWrapper/wgw_dialogs.h>

extern void rimgr_set_force_lod_no(int lod_no);

enum
{
  CM_ADD_NODE = CM_PLUGIN_BASE + 1,
  CM_ADD_RANDOM_ENTITY,
  CM_CHANGE_ASSET,
  CM_OPEN_ASSET,
  CM_COPY_ASSET_FILEPATH_TO_CLIPBOARD,
  CM_COPY_ASSET_NAME_TO_CLIPBOARD,
  CM_DELETE_NODE,
  CM_REVEAL_ASSET_IN_EXPLORER,
};

CompositeEditor::ReloadHelper::ReloadHelper(CompositeEditor &editor, const DagorAsset &asset,
  CompositeEditorRefreshType inRefreshType) :
  compositeEditor(editor), assetName(asset.getName()), refreshType(inRefreshType)
{
  G_ASSERT(compositeEditor.assetExpectedToReload.empty());
  G_ASSERT(compositeEditor.reloadRefreshType == CompositeEditorRefreshType::Unset);

  compositeEditor.assetExpectedToReload = assetName;
  compositeEditor.reloadRefreshType = inRefreshType;
}

CompositeEditor::ReloadHelper::~ReloadHelper()
{
  G_ASSERT(compositeEditor.assetExpectedToReload == assetName);
  G_ASSERT(compositeEditor.reloadRefreshType == refreshType);

  compositeEditor.assetExpectedToReload.clear();
  compositeEditor.reloadRefreshType = CompositeEditorRefreshType::Unset;
}

bool CompositeEditor::begin(DagorAsset *asset, IObjEntity *entity)
{
  if (expectingAssetReload())
  {
    G_ASSERT(asset->getName() == assetExpectedToReload);
    return true;
  }

  G_ASSERT(editedAsset == nullptr);
  G_ASSERT(!selectedTreeDataNode);
  G_ASSERT(!modified);

  editedAsset = asset;
  gizmoClient.setEntity(entity);
  treeData.convertAssetToTreeData(editedAsset, &editedAsset->props);

  if (autoShow && treeData.isComposite)
    get_app().showCompositeEditor(true);

  updateToolbarVisibility();
  updateGizmo();
  rimgr_set_force_lod_no(-1);

  return true;
}

bool CompositeEditor::end()
{
  if (expectingAssetReload())
    return true;

  if (!canSwitchToAnotherAsset())
    return false;

  const bool wasEditingComposite = CompositeEditorTreeData::isCompositeAsset(editedAsset);

  editedAsset = nullptr;
  selectedTreeDataNode = nullptr;
  modified = false;
  gizmoClient.setEntity(nullptr);
  treeData.convertAssetToTreeData(nullptr, nullptr);

  // When end() gets called AssetViewerApp::curAsset is already set to the new asset.
  // We (ab)use that to reduce the flickering by avoiding hiding and showing the Composite
  // Editor related windows when the new asset is also a composite.
  const DagorAsset *newAsset = get_app().getCurAsset();
  if (!CompositeEditorTreeData::isCompositeAsset(newAsset))
  {
    if (autoShow)
      get_app().showCompositeEditor(false);

    if (wasEditingComposite)
      fillCompositeTree();

    updateToolbarVisibility();
  }

  updateGizmo();

  UndoSystem *undoSystem = get_app().getUndoSystem();
  G_ASSERT(undoSystem);
  if (undoSystem)
    undoSystem->clear();

  return true;
}

void CompositeEditor::onCompositeEditorVisibilityChanged(bool shown)
{
  if (shown)
    fillCompositeTree();

  updateToolbarVisibility();
  updateGizmo();
}

void CompositeEditor::fillCompositeTreeInternal(bool keepExpansionState)
{
  G_ASSERT(!ignoreTreeSelectionChangePanelRefresh);
  ignoreTreeSelectionChangePanelRefresh = true;

  compositeTreeView->fill(treeData, selectedTreeDataNode, keepExpansionState);

  G_ASSERT(ignoreTreeSelectionChangePanelRefresh);
  ignoreTreeSelectionChangePanelRefresh = false;
}

void CompositeEditor::fillCompositeTree()
{
  if (!compositeTreeView)
    return;

  if (expectingAssetReload())
  {
    if (reloadRefreshType == CompositeEditorRefreshType::EntityAndCompositeEditor)
    {
      fillCompositeTreeInternal(true);
      fillCompositePropPanel();
    }
    else if (reloadRefreshType == CompositeEditorRefreshType::EntityAndTransformation && compositePropPanel)
      compositePropPanel->updateTransformParams(treeData, selectedTreeDataNode);
  }
  else
  {
    selectedTreeDataNode = treeData.isComposite ? &treeData.rootNode : nullptr;
    fillCompositeTreeInternal(false);
    fillCompositePropPanel();
    updateGizmo();
  }
}

void CompositeEditor::fillCompositePropPanel()
{
  if (compositePropPanel)
    compositePropPanel->fill(treeData, selectedTreeDataNode);
}

const CompositeEditorTreeDataNode *CompositeEditor::getSelectedTreeDataNode() const
{
  return compositeTreeView ? selectedTreeDataNode : nullptr;
}

unsigned CompositeEditor::getSelectedTreeNodeDataBlockId() const
{
  if (!compositeTreeView || !selectedTreeDataNode)
    return IDataBlockIdHolder::invalid_id;

  return selectedTreeDataNode->dataBlockId;
}

void CompositeEditor::selectTreeNodeByDataBlockId(unsigned dataBlockId)
{
  if (!compositeTreeView)
    return;

  if (dataBlockId == IDataBlockIdHolder::invalid_id)
    selectedTreeDataNode = nullptr;
  else
    selectedTreeDataNode = CompositeEditorTreeData::getTreeDataNodeByDataBlockId(treeData.rootNode, dataBlockId);

  compositeTreeView->selectByTreeDataNode(selectedTreeDataNode);
  updateGizmo();
}

void CompositeEditor::onChange(int pcb_id, PropPanel::ContainerPropertyControl *panel)
{
  if (!compositePropPanel || !selectedTreeDataNode)
    return;

  beginUndo();

  const CompositeEditorRefreshType refreshType = compositePropPanel->onChange(*selectedTreeDataNode, pcb_id);
  updateAssetFromTree(refreshType);

  endUndo("Composit Editor: Property editing", /*accept = */ refreshType != CompositeEditorRefreshType::Nothing);
}

bool CompositeEditor::saveComposit()
{
  G_ASSERT(editedAsset);

  if (DataBlockIncludeChecker::DoesBlkUseIncludes(editedAsset->getSrcFilePath()))
  {
    const int dialogResult = wingw::message_box(wingw::MBS_EXCL | wingw::MBS_YESNO, "WARNING",
      "The original composit contains an #include block. Saving the asset might cause data loss. Continue?");

    if (dialogResult != PropPanel::DIALOG_ID_YES)
      return false;
  }

  // ReloadHelper would not work here because file change notifications do not arrive instantly (before the helper gets
  // out of the scope), so we disable file change tracking while saving.
  const_cast<DagorAssetMgr &>(get_app().getAssetMgr()).enableChangesTracker(false);

  DataBlock block;
  treeData.convertTreeDataToDataBlock(treeData.rootNode, block);
  const bool succeeded = block.saveToTextFile(editedAsset->getTargetFilePath());

  const_cast<DagorAssetMgr &>(get_app().getAssetMgr()).enableChangesTracker(true);

  if (succeeded)
    modified = false;

  return succeeded;
}

void CompositeEditor::revertChanges()
{
  G_ASSERT(editedAsset);

  editedAsset->props.load(editedAsset->getSrcFilePath());
  modified = false;

  // See the comment in updateAssetFromTree for reason of using delayed callback.
  add_delayed_callback((delayed_callback)&onDelayedRevert, editedAsset);
}

void CompositeEditor::deleteSelectedNode(bool needsConfirmation)
{
  if (!selectedTreeDataNode || selectedTreeDataNode == &treeData.rootNode)
    return;

  int nodeIndex = -1;
  CompositeEditorTreeDataNode *nodeParent =
    CompositeEditorTreeData::getTreeDataNodeParent(*selectedTreeDataNode, treeData.rootNode, nodeIndex);
  G_ASSERT(nodeParent);
  if (!nodeParent)
    return;

  if (needsConfirmation)
  {
    const int dialogResult = wingw::message_box(wingw::MBS_EXCL | wingw::MBS_YESNO, "Delete node?",
      "Are you sure that you want to delete the selected node?");

    if (dialogResult != PropPanel::DIALOG_ID_YES)
      return;
  }

  beginUndo();
  endUndo("Composit Editor: Deleting node");

  nodeParent->nodes.erase(nodeParent->nodes.begin() + nodeIndex);
  nodeParent->convertSingleRandomEntityNodeToRegularNode(nodeParent == &treeData.rootNode);

  if (nodeParent->nodes.size() > 0)
    selectedTreeDataNode = nodeParent->nodes[nodeIndex >= nodeParent->nodes.size() ? (nodeIndex - 1) : nodeIndex].get();
  else
    selectedTreeDataNode = nodeParent;

  updateAssetFromTree(CompositeEditorRefreshType::EntityAndCompositeEditor);
}

void CompositeEditor::onDelayedRefresh(const DagorAsset *asset, CompositeEditorRefreshType refreshType)
{
  if (asset != editedAsset)
    return;

  ReloadHelper reloadHelper(*this, *asset, refreshType);
  get_app().getAssetMgr().callAssetChangeNotifications(*asset, asset->getNameId(), asset->getType());
}

void CompositeEditor::onDelayedRefreshEntity(const DagorAsset *asset)
{
  get_app().getCompositeEditor().onDelayedRefresh(asset, CompositeEditorRefreshType::Entity);
}

void CompositeEditor::onDelayedRefreshEntityAndTransformation(const DagorAsset *asset)
{
  get_app().getCompositeEditor().onDelayedRefresh(asset, CompositeEditorRefreshType::EntityAndTransformation);
}

void CompositeEditor::onDelayedRefreshEntityAndCompositEditor(const DagorAsset *asset)
{
  get_app().getCompositeEditor().onDelayedRefresh(asset, CompositeEditorRefreshType::EntityAndCompositeEditor);
}

void CompositeEditor::onDelayedRevert(const DagorAsset *asset)
{
  get_app().getAssetMgr().callAssetChangeNotifications(*asset, asset->getNameId(), asset->getType());

  // end() must have been called and that clears undo.
  UndoSystem *undoSystem = get_app().getUndoSystem();
  G_ASSERT(undoSystem && undoSystem->undo_level() == 0);
}

bool CompositeEditor::canSwitchToAnotherAsset()
{
  G_ASSERT(editedAsset);

  if (!modified)
    return true;

  const int dialogResult = wingw::message_box(wingw::MBS_QUEST | wingw::MBS_YESNOCANCEL, "Composit properties",
    "You have changed the composit. Do you want to save the changes?");

  if (dialogResult == PropPanel::DIALOG_ID_YES)
  {
    return saveComposit();
  }
  else if (dialogResult == PropPanel::DIALOG_ID_NO)
  {
    revertChanges();
    return true;
  }

  G_ASSERT(dialogResult == PropPanel::DIALOG_ID_CANCEL);
  return false;
}

void CompositeEditor::updateAssetFromTree(CompositeEditorRefreshType refreshType)
{
  G_ASSERT(editedAsset);

  G_ASSERT(refreshType != CompositeEditorRefreshType::Unset);
  if (refreshType == CompositeEditorRefreshType::Nothing)
    return;

  DataBlock block;
  treeData.convertTreeDataToDataBlock(treeData.rootNode, block);
  treeData.setDataBlockIds(); // The asset will change, so we have to regenerate the data block IDs to match it.
  editedAsset->props.setFrom(&block);
  modified = true;

  updateGizmo();

  // We can't refresh the property panel directly here because it would destroy the notifier control's instance,
  // and cause a crash in WindowBaseHandler::controlProc, hence the use of the delayed callbacks. (We could also
  // use setPostEvent and onPostEvent to avoid the problem.)

  if (refreshType == CompositeEditorRefreshType::Entity)
    add_delayed_callback((delayed_callback)&CompositeEditor::onDelayedRefreshEntity, editedAsset);
  else if (refreshType == CompositeEditorRefreshType::EntityAndTransformation)
    add_delayed_callback((delayed_callback)&CompositeEditor::onDelayedRefreshEntityAndTransformation, editedAsset);
  else if (refreshType == CompositeEditorRefreshType::EntityAndCompositeEditor)
    add_delayed_callback((delayed_callback)&CompositeEditor::onDelayedRefreshEntityAndCompositEditor, editedAsset);
}

void CompositeEditor::toggleSnapMode(int pcb_id)
{
  GridObject &grid = IEditorCoreEngine::get()->getGrid();

  if (pcb_id == CM_VIEW_GRID_MOVE_SNAP)
    grid.setMoveSnap(!grid.getMoveSnap());
  else if (pcb_id == CM_VIEW_GRID_ANGLE_SNAP)
    grid.setRotateSnap(!grid.getRotateSnap());
  else if (pcb_id == CM_VIEW_GRID_SCALE_SNAP)
    grid.setScaleSnap(!grid.getScaleSnap());
  else
    G_ASSERT(false);

  toolbar.updateSnapToolbarButtons();
}

void CompositeEditor::onClick(int pcb_id, PropPanel::ContainerPropertyControl *panel)
{
  if (pcb_id == ID_COMPOSITE_EDITOR_COMPOSIT_SAVE_CHANGES)
  {
    saveComposit();
  }
  else if (pcb_id == ID_COMPOSITE_EDITOR_COMPOSIT_RESET_TO_FILE)
  {
    revertChanges();
  }
  else if (pcb_id == ID_COMPOSITE_EDITOR_DELETE_NODE)
  {
    deleteSelectedNode();
  }
  else if (pcb_id == CM_OBJED_MODE_SELECT)
  {
    setGizmo(IEditorCoreEngine::ModeType::MODE_None);
    focusViewport(); // Make hovering over the newly activated gizmo work without the need of an additional click in the viewport.
  }
  else if (pcb_id == CM_OBJED_MODE_MOVE)
  {
    setGizmo(IEditorCoreEngine::ModeType::MODE_Move);
    focusViewport();
  }
  else if (pcb_id == CM_OBJED_MODE_ROTATE)
  {
    setGizmo(IEditorCoreEngine::ModeType::MODE_Rotate);
    focusViewport();
  }
  else if (pcb_id == CM_OBJED_MODE_SCALE)
  {
    setGizmo(IEditorCoreEngine::ModeType::MODE_Scale);
    focusViewport();
  }
  else if (pcb_id == CM_VIEW_GRID_MOVE_SNAP || pcb_id == CM_VIEW_GRID_ANGLE_SNAP || pcb_id == CM_VIEW_GRID_SCALE_SNAP)
  {
    toggleSnapMode(pcb_id);
    focusViewport();
  }
  else if (compositePropPanel && selectedTreeDataNode)
  {
    if (pcb_id == ID_COMPOSITE_EDITOR_ENTITY_ACTIONS)
    {
      const int action = compositePropPanel->getInt(pcb_id);
      if (action == PropPanel::EXT_BUTTON_REMOVE)
        deleteSelectedNode(false);
    }
    else
    {
      const CompositeEditorRefreshType refreshType = compositePropPanel->onClick(*selectedTreeDataNode, pcb_id);
      updateAssetFromTree(refreshType);
    }
  }
}

int CompositeEditor::onMenuItemClick(unsigned id)
{
  if (id == CM_ADD_NODE)
  {
    G_ASSERT(selectedTreeDataNode);
    if (!selectedTreeDataNode)
      return 0;

    beginUndo();
    endUndo("Composit Editor: Adding node");

    selectedTreeDataNode->insertNodeBlock(-1);
    selectedTreeDataNode = selectedTreeDataNode->nodes.back().get();
    updateAssetFromTree(CompositeEditorRefreshType::EntityAndCompositeEditor);

    return 0;
  }
  else if (id == CM_ADD_RANDOM_ENTITY)
  {
    G_ASSERT(selectedTreeDataNode);
    if (!selectedTreeDataNode)
      return 0;

    beginUndo();
    endUndo("Composit Editor: Adding entity");

    selectedTreeDataNode->insertEntBlock(-1);
    selectedTreeDataNode = selectedTreeDataNode->nodes.back().get();

    updateAssetFromTree(CompositeEditorRefreshType::EntityAndCompositeEditor);

    return 0;
  }
  else if (id == CM_CHANGE_ASSET)
  {
    G_ASSERT(selectedTreeDataNode);
    if (!selectedTreeDataNode)
      return 0;

    const char *oldAssetName = selectedTreeDataNode->getAssetName();
    const char *assetName = DAEDITOR3.selectAsset(oldAssetName, "Select asset", DAEDITOR3.getGenObjAssetTypes());
    if (!assetName)
      return 0;

    beginUndo();
    endUndo("Composit Editor: Changing asset");

    if (*assetName)
      selectedTreeDataNode->params.setStr("name", assetName);
    else
      selectedTreeDataNode->params.removeParam("name");

    // Remove the unused type parameter from old .blk files.
    selectedTreeDataNode->params.removeParam("type");

    updateAssetFromTree(CompositeEditorRefreshType::EntityAndCompositeEditor);

    return 0;
  }
  else if (id == CM_DELETE_NODE)
  {
    deleteSelectedNode();
    return 0;
  }
  else if (id == CM_OPEN_ASSET)
  {
    G_ASSERT(selectedTreeDataNode);
    if (!selectedTreeDataNode)
      return 0;

    const char *assetName = selectedTreeDataNode->getAssetName();
    DagorAsset *asset = DAEDITOR3.getAssetByName(assetName);
    if (asset && canSwitchToAnotherAsset())
    {
      // See the comment in updateAssetFromTree for reason of using delayed callback.
      add_delayed_callback((delayed_callback)&onDelayedSelectAsset, asset);
    }

    return 0;
  }
  else if (id == CM_COPY_ASSET_FILEPATH_TO_CLIPBOARD)
  {
    G_ASSERT(selectedTreeDataNode);
    if (!selectedTreeDataNode)
      return 0;

    DagorAsset *asset = DAEDITOR3.getAssetByName(selectedTreeDataNode->getAssetName());
    if (asset)
    {
      String path(asset->isVirtual() ? asset->getTargetFilePath() : asset->getSrcFilePath());
      clipboard::set_clipboard_ansi_text(make_ms_slashes(path));
    }

    return 0;
  }
  else if (id == CM_COPY_ASSET_NAME_TO_CLIPBOARD)
  {
    G_ASSERT(selectedTreeDataNode);
    if (!selectedTreeDataNode)
      return 0;

    DagorAsset *asset = DAEDITOR3.getAssetByName(selectedTreeDataNode->getAssetName());
    if (asset)
      clipboard::set_clipboard_ansi_text(asset->getName());

    return 0;
  }
  else if (id == CM_REVEAL_ASSET_IN_EXPLORER)
  {
    G_ASSERT(selectedTreeDataNode);
    if (!selectedTreeDataNode)
      return 0;

    DagorAsset *asset = DAEDITOR3.getAssetByName(selectedTreeDataNode->getAssetName());
    if (asset)
      dag_reveal_in_explorer(asset);

    return 0;
  }

  return 1;
}

void CompositeEditor::onTvSelectionChange(PropPanel::TreeBaseWindow &tree, PropPanel::TLeafHandle new_sel)
{
  if (ignoreTreeSelectionChangePanelRefresh)
    return;

  selectedTreeDataNode = new_sel == nullptr ? nullptr : static_cast<CompositeEditorTreeDataNode *>(tree.getItemData(new_sel));
  fillCompositePropPanel();
  updateGizmo();
}

void CompositeEditor::onDelayedSelectAsset(const DagorAsset *asset)
{
  G_ASSERT(asset);
  if (asset)
    get_app().selectAsset(*asset);
}

bool CompositeEditor::onTvContextMenu(PropPanel::TreeBaseWindow &tree_base_window, PropPanel::ITreeInterface &tree)
{
  if (!selectedTreeDataNode)
    return false;

  PropPanel::IMenu &menu = tree.createContextMenu();
  menu.setEventHandler(this);

  const bool isRootNode = selectedTreeDataNode == &treeData.rootNode;
  bool separateDelete = false;

  if (selectedTreeDataNode->canEditChildren())
  {
    menu.addItem(ROOT_MENU_ITEM, CM_ADD_NODE, "Add node");
    separateDelete = true;
  }

  if (selectedTreeDataNode->canEditRandomEntities(isRootNode))
  {
    menu.addItem(ROOT_MENU_ITEM, CM_ADD_RANDOM_ENTITY, "Add entity");
    separateDelete = true;
  }

  if (selectedTreeDataNode->canEditAssetName(isRootNode))
  {
    menu.addItem(ROOT_MENU_ITEM, CM_CHANGE_ASSET, "Change asset");
    separateDelete = true;
  }

  if (!isRootNode)
  {
    if (separateDelete)
      menu.addSeparator(ROOT_MENU_ITEM);

    menu.addItem(ROOT_MENU_ITEM, CM_DELETE_NODE, "Delete node\tDelete");

    if (selectedTreeDataNode->hasNameParameter())
    {
      menu.addSeparator(ROOT_MENU_ITEM);
      menu.addItem(ROOT_MENU_ITEM, CM_COPY_ASSET_FILEPATH_TO_CLIPBOARD, "Copy filepath");
      menu.addItem(ROOT_MENU_ITEM, CM_COPY_ASSET_NAME_TO_CLIPBOARD, "Copy name");
      menu.addItem(ROOT_MENU_ITEM, CM_OPEN_ASSET, "Open asset");
      menu.addSeparator(ROOT_MENU_ITEM);
      menu.addItem(ROOT_MENU_ITEM, CM_REVEAL_ASSET_IN_EXPLORER, "Reveal in Explorer");
    }
  }

  return true;
}

void CompositeEditor::updateSelectedNodeTransform(const TMatrix &tm)
{
  G_ASSERT(selectedTreeDataNode);
  if (!selectedTreeDataNode)
    return;

  // This function is only used by CompositeEditorGizmoClient, and that handles undo.
  UndoSystem *undoSystem = get_app().getUndoSystem();
  G_ASSERT(undoSystem && undoSystem->is_holding());

  G_ASSERT(selectedTreeDataNode->getUseTransformationMatrix());
  selectedTreeDataNode->params.setTm("tm", tm);

  // updateAssetFromTree(CompositeEditorRefreshType::EntityAndTransformation) would not work here, it
  // would not update the transformation properties on the panel because when this function is called
  // preventUiUpdatesWhileUsingGizmo is set to true.
  if (compositePropPanel)
    compositePropPanel->updateTransformParams(treeData, selectedTreeDataNode);

  updateAssetFromTree(CompositeEditorRefreshType::Entity);
}

void CompositeEditor::cloneSelectedNode()
{
  G_ASSERT(selectedTreeDataNode);
  if (!selectedTreeDataNode)
    return;

  // This function is only used by CompositeEditorGizmoClient, and that handles undo.
  UndoSystem *undoSystem = get_app().getUndoSystem();
  G_ASSERT(undoSystem && undoSystem->is_holding());

  int nodeIndex = -1;
  CompositeEditorTreeDataNode *nodeParent =
    CompositeEditorTreeData::getTreeDataNodeParent(*selectedTreeDataNode, treeData.rootNode, nodeIndex);
  G_ASSERT(nodeParent);
  if (!nodeParent)
    return;

  DataBlock cloneDataBlock;
  CompositeEditorTreeData::convertTreeDataToDataBlock(*selectedTreeDataNode, cloneDataBlock);

  eastl::unique_ptr<CompositeEditorTreeDataNode> clonedTreeDataNode = eastl::make_unique<CompositeEditorTreeDataNode>();
  CompositeEditorTreeData::convertDataBlockToTreeData(cloneDataBlock, *clonedTreeDataNode);

  selectedTreeDataNode = clonedTreeDataNode.get();
  nodeParent->nodes.insert(nodeParent->nodes.begin() + nodeIndex + 1, std::move(clonedTreeDataNode));

  updateAssetFromTree(CompositeEditorRefreshType::Entity);
}

void CompositeEditor::setGizmo(IEditorCoreEngine::ModeType mode)
{
  lastSelectedGizmoMode = mode;
  updateGizmo();
}

void CompositeEditor::updateGizmo()
{
  if (get_app().isGizmoOperationStarted())
    return;

  const CompositeEditorTreeDataNode *node = getSelectedTreeDataNode();
  const bool canTransform = node && node->canTransform();
  const IEditorCoreEngine::ModeType oldMode = IEditorCoreEngine::get()->getGizmoModeType();
  const IEditorCoreEngine::ModeType newMode = canTransform ? lastSelectedGizmoMode : IEditorCoreEngine::ModeType::MODE_None;

  if (newMode != oldMode)
    IEditorCoreEngine::get()->setGizmo(&gizmoClient, newMode);

  toolbar.updateGizmoToolbarButtons(canTransform);
}

void CompositeEditor::updateToolbarVisibility()
{
  if (treeData.isComposite && get_app().isCompositeEditorShown())
    toolbar.initUi(*this, GUI_PLUGIN_TOOLBAR_ID);
  else
    toolbar.closeUi();
}

void CompositeEditor::setPreventUiUpdatesWhileUsingGizmo(bool prevent, bool wasCloning)
{
  if (preventUiUpdatesWhileUsingGizmo == prevent)
    return;

  preventUiUpdatesWhileUsingGizmo = prevent;

  if (!prevent)
    updateAssetFromTree(
      wasCloning ? CompositeEditorRefreshType::EntityAndCompositeEditor : CompositeEditorRefreshType::EntityAndTransformation);
}

void CompositeEditor::setEntityViewPluginInterface(IEntityViewPluginInterface &inEntityViewPluginInterface)
{
  G_ASSERT(!entityViewPluginInterface);
  entityViewPluginInterface = &inEntityViewPluginInterface;
}

IEntityViewPluginInterface &CompositeEditor::getEntityViewPluginInterface() const
{
  G_ASSERT(entityViewPluginInterface);
  return *entityViewPluginInterface; //-V522
}

void CompositeEditor::focusViewport()
{
  IGenViewportWnd *viewport = IEditorCoreEngine::get()->getCurrentViewport();
  if (viewport)
    viewport->activate();
}

void CompositeEditor::beginUndo()
{
  UndoSystem *undoSystem = get_app().getUndoSystem();
  G_ASSERT(undoSystem);
  if (!undoSystem)
    return;

  CompositeEditorUndoParams *undoParams = new CompositeEditorUndoParams();
  undoParams->saveUndo();

  undoSystem->begin();
  undoSystem->put(undoParams);
}

void CompositeEditor::endUndo(const char *operation_name, bool accept)
{
  UndoSystem *undoSystem = get_app().getUndoSystem();
  G_ASSERT(undoSystem);
  if (!undoSystem)
    return;

  if (accept)
    undoSystem->accept(operation_name);
  else
    undoSystem->cancel();
}

void CompositeEditor::saveForUndo(DataBlock &dataBlock) { treeData.convertTreeDataToDataBlock(treeData.rootNode, dataBlock); }

void CompositeEditor::loadFromUndo(const DataBlock &dataBlock)
{
  const unsigned selectedDataBlockId = getSelectedTreeNodeDataBlockId();

  G_ASSERT(editedAsset);
  treeData.convertAssetToTreeData(editedAsset, &dataBlock);

  selectedTreeDataNode = nullptr;
  if (selectedDataBlockId != IDataBlockIdHolder::invalid_id)
    selectedTreeDataNode = CompositeEditorTreeData::getTreeDataNodeByDataBlockId(treeData.rootNode, selectedDataBlockId);

  updateAssetFromTree(CompositeEditorRefreshType::EntityAndCompositeEditor);
}
