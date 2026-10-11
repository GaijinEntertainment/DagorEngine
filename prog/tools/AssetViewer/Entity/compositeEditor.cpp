// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "compositeEditor.h"
#include "../assetBuildCache.h"
#include "../av_appwnd.h"
#include "compositeEditorPanel.h"
#include "compositeEditorTree.h"
#include "compositeEditorUndo.h"
#include "entity_cm.h"
#include "dataBlockIncludeChecker.h"
#include "saveAsCompositeDlg.h"
#include <assets/asset.h>
#include <de3_objEntity.h>
#include <de3_composit.h>
#include <de3_dataBlockIdHolder.h>
#include <de3_interface.h>
#include <EditorCore/ec_cm.h>
#include <EditorCore/ec_ViewportWindow.h>
#include <assets/assetUtils.h>
#include <assetsGui/av_assetTreeDragHandler.h>
#include <assetsGui/av_selObjDlg.h>
#include <libTools/util/strUtil.h>
#include <libTools/util/blkUtil.h>
#include <osApiWrappers/dag_clipboard.h>
#include <propPanel/commonWindow/dialogWindow.h>
#include <propPanel/control/treeInterface.h>
#include <propPanel/control/menu.h>
#include <util/dag_delayedAction.h>
#include <winGuiWrapper/wgw_dialogs.h>
#include <imgui/imgui.h>
#include <EASTL/sort.h>
#include <EASTL/hash_map.h>
#include <EASTL/hash_set.h>

using PropPanel::DragAndDropResult;
using PropPanel::ROOT_MENU_ITEM;

extern void rimgr_set_force_lod_no(int lod_no);

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

  const bool wasEnteringSubCompositeEditing = subCompositeStack.isEntering();
  const bool wasReturningFromNestedEditing = subCompositeStack.isReturning();
  subCompositeStack.clearEntering();
  editedAsset = asset;
  gizmoClient.setEntity(entity);
  treeData.convertAssetToTreeData(editedAsset, &editedAsset->props);

  if (autoShow && treeData.isComposite)
    get_app().showCompositeEditor(true);

  updateToolbarVisibility();
  updateGizmo();
  rimgr_set_force_lod_no(-1);

  treeDropHandler.reset(new EntityTreeDropHandler(*this));
  childDropHandler.reset(new ChildEntityDragAndDropHandler(*this));

  // Apply before ghost creation so the tree is patched before any refresh.
  if (subCompositeStack.hasPendingUniqueSwap())
    applyPendingUniqueAssetSwap();
  subCompositeStack.clearPendingUniqueSwap();

  if (wasEnteringSubCompositeEditing)
  {
    CompositeEditorSubContext &ctx = subCompositeStack.back();
    G_ASSERT(!ctx.parentGhostEntity);
    ctx.parentGhostEntity = DAEDITOR3.createEntity(*ctx.parentAsset);
    if (ctx.parentGhostEntity)
    {
      ctx.parentGhostEntity->setTm(inverse(ctx.subCompositeTm));
      if (!wasReturningFromNestedEditing)
      {
        const TMatrix invNewTm = inverse(ctx.subCompositeTm);
        dag::ConstSpan<CompositeEditorSubContext> fullCtx = subCompositeStack.getFullContext();
        for (int i = 0; i < (int)fullCtx.size() - 1; ++i)
          if (fullCtx[i].parentGhostEntity)
          {
            TMatrix ancestorTm;
            fullCtx[i].parentGhostEntity->getTm(ancestorTm);
            fullCtx[i].parentGhostEntity->setTm(invNewTm * ancestorTm);
          }
      }
    }
  }

  return true;
}

bool CompositeEditor::end()
{
  if (expectingAssetReload())
    return true;

  if (!editedAsset)
    return true;

  if (!canSwitchToAnotherAsset())
  {
    if (subCompositeStack.isEntering())
      subCompositeStack.abortEnter();
    return false;
  }

  // Unexpected exit: user switched to an unrelated asset without going through
  // exitSubCompositeEditing(). Destroy any live ghost entities and clear the stack.
  // On a normal exit, exitSubCompositeEditing() already destroyed the ghost and
  // popped the stack, so the loop is a no-op in that case.
  if (!subCompositeStack.isEntering())
  {
    while (!subCompositeStack.isEmpty())
    {
      CompositeEditorSubContext ctx = subCompositeStack.pop();
      if (ctx.parentGhostEntity)
        ctx.parentGhostEntity->destroy();
    }
  }

  const bool wasEditingComposite = CompositeEditorTreeData::isCompositeAsset(editedAsset);

  editedAsset = nullptr;
  selectedTreeDataNode = nullptr;
  selectedTreeDataNodes.clear();
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

  undoEditGesture.end();

  UndoSystem *undoSystem = get_app().getUndoSystem();
  G_ASSERT(undoSystem);
  if (undoSystem)
    undoSystem->clear();

  return true;
}

void CompositeEditor::enterSubCompositeEditing()
{
  G_ASSERT(selectedTreeDataNode && selectedTreeDataNode->isCompositeAsset());

  DagorAsset *subAsset = DAEDITOR3.getAssetByName(selectedTreeDataNode->getAssetName());
  if (!subAsset)
    return;

  if (!canSwitchToAnotherAsset())
    return;

  const TMatrix subCompositeTm = calcParentMatrix(selectedTreeDataNode);
  subCompositeStack.push(editedAsset, subCompositeTm, selectedTreeDataNode->dataBlockId);

  // Remap the camera into sub-composite local space, applied after the asset switch. No auto zoom here. The user keeps
  // editing the same area, so the view must not move on screen.
  if (IGenViewportWnd *vp = EDITORCORE->getCurrentViewport())
  {
    TMatrix worldCamTm;
    vp->getCameraTransform(worldCamTm);
    subCompositeStack.setPendingCameraTransform(inverse(subCompositeTm) * worldCamTm);
  }

  add_delayed_callback((delayed_callback)&onDelayedSelectAsset, subAsset);
}

static bool showRevertConfirmDialog()
{
  PropPanel::DialogWindow dlg(nullptr, hdpi::_pxScaled(400), hdpi::_pxScaled(105), "Revert Changes", false);
  dlg.setDialogButtonText(PropPanel::DIALOG_ID_OK, "Revert");
  PropPanel::ContainerPropertyControl *panel = dlg.getPanel();
  panel->createStatic(PropPanel::DIALOG_ID_FIRST_FREE, "Are you sure you want to revert all changes to the original version?", false,
    true, true);
  dlg.setInitialFocus(PropPanel::DIALOG_ID_CANCEL);
  return dlg.showDialog() == PropPanel::DIALOG_ID_OK;
}

static bool showSaveConfirmDialog(const char *assetName)
{
  PropPanel::DialogWindow dlg(nullptr, hdpi::_pxScaled(400), hdpi::_pxScaled(105), "Save changes", false);
  dlg.setDialogButtonText(PropPanel::DIALOG_ID_OK, "Save");
  PropPanel::ContainerPropertyControl *panel = dlg.getPanel();
  panel->createStatic(PropPanel::DIALOG_ID_FIRST_FREE, String(0, "Do you want to save the changes of %s?", assetName), false, true,
    true);
  dlg.setInitialFocus(PropPanel::DIALOG_ID_OK);
  return dlg.showDialog() == PropPanel::DIALOG_ID_OK;
}

void CompositeEditor::saveSubCompositeEditing()
{
  G_ASSERT(isEditingSubComposite());
  if (!showSaveConfirmDialog(editedAsset->getName()))
    return;
  saveComposit();
  exitSubCompositeEditing();
}

void CompositeEditor::saveSubCompositeAsUnique()
{
  G_ASSERT(isEditingSubComposite());

  SaveAsNewCompositeDialog dlg(editedAsset, "Save as a unique composite", /*show_replace_option=*/false);
  if (dlg.showDialog() != PropPanel::DIALOG_ID_OK)
    return;

  DataBlock block;
  treeData.convertTreeDataToDataBlock(treeData.rootNode, block);
  block.setStr("className", "composit");
  String fullPath = dlg.getFullPath();
  if (!block.saveToTextFile(fullPath))
  {
    wingw::message_box(wingw::MBS_EXCL | wingw::MBS_OK, "Error", "Failed to create new composite '%s'.", fullPath.str());
    return;
  }

  // The immediate change tracking run usually misses a freshly written file.
  get_app().trackChangesContinuous(-1, true);
  const String newAssetName = dlg.getAssetName();
  int attempt = 0;
  while (get_app().getAssetMgr().findAsset(newAssetName) == nullptr && attempt < 2)
  {
    sleep_msec(100);
    get_app().trackChangesContinuous(-1, true);
    ++attempt;
  }

  // Restore the original sub-composite props so the parent composite renders the
  // unmodified version after exit. callAssetChangeNotifications (via onDelayedRevert)
  // must fire after end() to flush the entity cache that was updated during editing.
  DagorAsset *subAsset = editedAsset;
  subAsset->props.load(subAsset->getSrcFilePath());
  modified = false;

  // The node in the parent composite that references the old sub-composite will be
  // updated to reference the new unique asset in begin() once the parent is reloaded.
  // Captured before exitSubCompositeEditing() pops the context off the stack.
  subCompositeStack.setPendingUniqueSwap(newAssetName);

  exitSubCompositeEditing();
  add_delayed_callback((delayed_callback)&onDelayedRevertAfterUniqueSwap, subAsset);
}

void CompositeEditor::revertSubCompositeEditing()
{
  if (!showRevertConfirmDialog())
    return;
  DagorAsset *subAsset = editedAsset;
  subAsset->props.load(subAsset->getSrcFilePath());
  modified = false;
  exitSubCompositeEditing();
  add_delayed_callback((delayed_callback)&onDelayedRevert, subAsset);
}

void CompositeEditor::applyPendingUniqueAssetSwap()
{
  G_ASSERT(subCompositeStack.hasPendingUniqueSwap());
  const String newAssetName = subCompositeStack.getPendingUniqueName();
  const unsigned pendingUniqueDataBlockId = subCompositeStack.getPendingUniqueDataBlockId();
  subCompositeStack.clearPendingUniqueSwap();

  CompositeEditorTreeDataNode *node =
    CompositeEditorTreeData::getTreeDataNodeByDataBlockId(treeData.rootNode, pendingUniqueDataBlockId);
  if (!node)
    return;

  beginUndo();
  node->params.setStr("name", newAssetName);
  endUndo("Composit Editor: Save sub-composite as unique");

  // Call synchronously so editedAsset->props is updated before any queued callbacks
  // (e.g. onDelayedRevert for the sub-composite) fire and potentially trigger a
  // parent entity reload that would rebuild treeData from the stale props.
  updateAssetFromTree(CompositeEditorRefreshType::EntityAndCompositeEditor);
}

void CompositeEditor::exitSubCompositeEditing()
{
  G_ASSERT(isEditingSubComposite());

  // Check for unsaved changes before touching any state. end() will call this again
  // via the delayed asset switch, but modified will be false by then (saved or reverted).
  if (!canSwitchToAnotherAsset())
    return;

  CompositeEditorSubContext ctx = subCompositeStack.pop();

  // Save the current camera in parent world space so the transition back is seamless.
  if (IGenViewportWnd *vp = EDITORCORE->getCurrentViewport())
  {
    TMatrix subCamTm;
    vp->getCameraTransform(subCamTm);
    subCompositeStack.setPendingCameraTransform(ctx.subCompositeTm * subCamTm);
  }

  // Undo the transform shift applied to ancestor ghosts when this level was entered.
  if (!subCompositeStack.isEmpty())
  {
    dag::ConstSpan<CompositeEditorSubContext> remaining = subCompositeStack.getFullContext();
    for (const CompositeEditorSubContext &ancestorCtx : remaining)
      if (ancestorCtx.parentGhostEntity)
      {
        TMatrix ancestorTm;
        ancestorCtx.parentGhostEntity->getTm(ancestorTm);
        ancestorCtx.parentGhostEntity->setTm(ctx.subCompositeTm * ancestorTm);
      }
    // Destroy the ghost of the level we're returning to; begin() will recreate it.
    CompositeEditorSubContext &newBack = subCompositeStack.back();
    if (newBack.parentGhostEntity)
    {
      newBack.parentGhostEntity->destroy();
      newBack.parentGhostEntity = nullptr;
    }
    // Signal begin() to recreate the ghost; ancestor TMs are already restored above.
    subCompositeStack.setReturningEntering();
  }

  // Destroy before the asset switch so the extra entity doesn't interfere with selectAsset().
  if (ctx.parentGhostEntity)
  {
    ctx.parentGhostEntity->destroy();
    ctx.parentGhostEntity = nullptr;
  }
  add_delayed_callback((delayed_callback)&onDelayedSelectAsset, ctx.parentAsset);
}

bool CompositeEditor::applyPendingCameraTransform()
{
  if (!subCompositeStack.hasPendingCameraTransform())
    return false;
  if (IGenViewportWnd *vp = EDITORCORE->getCurrentViewport())
    vp->setCameraTransform(subCompositeStack.getPendingCameraTransform());
  subCompositeStack.clearPendingCameraTransform();
  return true;
}

void CompositeEditor::onCompositeEditorVisibilityChanged(bool shown)
{
  if (shown)
    fillCompositeTree();

  updateToolbarVisibility();
  updateGizmo();
}

void CompositeEditor::onSnapSettingChanged()
{
  if (toolbar.isInited())
    toolbar.updateSnapToolbarButtons();
}

void CompositeEditor::fillCompositeTreeInternal(bool keepExpansionState)
{
  G_ASSERT(!ignoreTreeSelectionChangePanelRefresh);
  ignoreTreeSelectionChangePanelRefresh = true;

  compositeTreeView->fill(treeData, selectedTreeDataNode, selectedTreeDataNodes, keepExpansionState);
  selectedTreeDataNodes.clear();
  compositeTreeView->setDropHandler(treeDropHandler.get());

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
      getSelectedTreeDataNodes(selectedTreeDataNodes);
      fillCompositePropPanel();
      updateGizmo();
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
  // The property panel controls are about to be destroyed, and destroyed controls do not send the change finished event
  undoEditGesture.end();

  if (compositePropPanel)
    compositePropPanel->fill(treeData, selectedTreeDataNode, childDropHandler.get());
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

bool CompositeEditor::canParentSelectedTreeDataNodes() const
{
  const PropPanel::TreeBaseWindow *tree = compositeTreeView.get();
  if (!tree)
    return false;

  dag::Vector<PropPanel::TLeafHandle> items;
  tree->getSelectedItems(items);
  const unsigned int selectedDataBlockId = getSelectedTreeNodeDataBlockId();
  bool hasReparentableNode = false;
  for (int i = 0; i < items.size(); ++i)
  {
    CompositeEditorTreeDataNode *multiSelectedNode = static_cast<CompositeEditorTreeDataNode *>(tree->getItemData(items[i]));
    if (multiSelectedNode && multiSelectedNode != selectedTreeDataNode)
    {
      if (multiSelectedNode->isAncestorOfNode(selectedDataBlockId))
        return false;

      if (!multiSelectedNode->isEntBlock())
        hasReparentableNode = true;
    }
  }
  return hasReparentableNode;
}

namespace
{

struct NodeInfo
{
  CompositeEditorTreeDataNode *parent = nullptr;
  int indexInParent = -1;
};

} // namespace

void CompositeEditor::makeSelectedParentRelation()
{
  if (!canParentSelectedTreeDataNodes())
    return;

  CompositeEditorTreeDataNode *newParent = selectedTreeDataNode;
  if (!newParent)
    return;
  int newIndex = newParent->nodeCount();

  beginUndo();

  int count = selectedTreeDataNodes.size();
  dag::Vector<CompositeEditorTreeDataNode *> nodesReparented(count, nullptr);
  dag::Vector<NodeInfo> nodeInfo(count);
  eastl::hash_map<CompositeEditorTreeDataNode *, int> nodeSelectionOrder;
  for (int i = 0; i < count; ++i)
  {
    CompositeEditorTreeDataNode *node = selectedTreeDataNodes[i];
    if (!node || node == newParent || node->isEntBlock())
      continue;

    int index = -1;
    CompositeEditorTreeDataNode *parent = CompositeEditorTreeData::getTreeDataNodeParent(*node, treeData.rootNode, index);
    if (!parent)
      continue;

    nodesReparented[i] = node;
    nodeInfo[i] = {parent, index};
    nodeSelectionOrder[node] = i;
  }

  dag::Vector<int> reparentOrder;
  CompositeEditorTreeData::traverseDepthFirst(treeData.rootNode, [&](CompositeEditorTreeDataNode &node, int /*order*/) {
    auto it = nodeSelectionOrder.find(&node);
    if (it != nodeSelectionOrder.end())
      reparentOrder.push_back(it->second);
  });

  dag::Vector<CompositeEditorTreeDataNode *> parentsToConvert;
  int reparentCount = 0;
  // Reverse pre-order: children and higher-index siblings first, keeping indexInParent valid for each erase.
  for (int i = reparentOrder.size() - 1; i >= 0; --i)
  {
    int idx = reparentOrder[i];
    CompositeEditorTreeDataNode *node = nodesReparented[idx];
    CompositeEditorTreeDataNode *oldParent = nodeInfo[idx].parent;
    if (oldParent == newParent)
      continue;

    int nodeIndex = nodeInfo[idx].indexInParent;

    recalcMatrixInParentBase(oldParent, node, newParent);

    // Move the node itself to its new parent, keeping the original pointer valid.
    newParent->nodes.insert(newParent->nodes.begin() + newIndex, std::move(oldParent->nodes[nodeIndex]));

    oldParent->nodes.erase(oldParent->nodes.begin() + nodeIndex);
    parentsToConvert.push_back(oldParent);

    reparentCount++;
  }

  for (CompositeEditorTreeDataNode *parent : parentsToConvert)
  {
    // Skip conversion if the sole remaining ent child is still selected.
    if (parent->nodeCount() == 1 && parent->nodes[0]->isEntBlock())
    {
      const CompositeEditorTreeDataNode *entNode = parent->nodes[0].get();
      if (selectedTreeDataNode == entNode ||
          eastl::find(selectedTreeDataNodes.begin(), selectedTreeDataNodes.end(), entNode) != selectedTreeDataNodes.end())
        continue;
    }
    parent->convertSingleRandomEntityNodeToRegularNode(parent == &treeData.rootNode);
  }

  const bool reparented = reparentCount > 0;
  if (reparented)
    updateAssetFromTree(CompositeEditorRefreshType::EntityAndCompositeEditor);

  endUndo("Composit Editor: Making parent relation(s)", /*accept = */ reparented);
}

bool CompositeEditor::hasSelectedParentRelation() const
{
  if (selectedTreeDataNode == nullptr)
    return false;

  for (CompositeEditorTreeDataNode *node : selectedTreeDataNodes)
  {
    // TODO: entity parent relation limitation, it is a complicated case to remove an entity block!
    if (node && node != &treeData.rootNode && !node->isEntBlock())
    {
      if (!treeData.rootNode.hasChildTreeDataNode(node))
        return true;
    }
  }
  return false;
}

void CompositeEditor::clearSelectedParentRelation()
{
  if (!hasSelectedParentRelation())
    return;

  beginUndo();

  int count = selectedTreeDataNodes.size();
  dag::Vector<CompositeEditorTreeDataNode *> nodesReparented(count, nullptr);
  dag::Vector<NodeInfo> nodeOldInfo(count);
  dag::Vector<NodeInfo> nodeNewInfo(count);
  eastl::hash_map<CompositeEditorTreeDataNode *, int> nodeSelectionOrder;
  for (int i = 0; i < count; ++i)
  {
    CompositeEditorTreeDataNode *node = selectedTreeDataNodes[i];
    if (!node || node->isEntBlock())
      continue;

    int index = -1;
    CompositeEditorTreeDataNode *parent = CompositeEditorTreeData::getTreeDataNodeParent(*node, treeData.rootNode, index);
    if (!parent)
      continue;

    int newIndex = -1;
    CompositeEditorTreeDataNode *newParent = CompositeEditorTreeData::getTreeDataNodeParent(*parent, treeData.rootNode, newIndex);
    if (!newParent)
      continue;

    nodesReparented[i] = node;
    nodeOldInfo[i] = {parent, index};
    nodeNewInfo[i] = {newParent, newIndex};
    nodeSelectionOrder[node] = i;
  }

  dag::Vector<int> reparentOrder;
  CompositeEditorTreeData::traverseDepthFirst(treeData.rootNode, [&](CompositeEditorTreeDataNode &node, int /*order*/) {
    auto it = nodeSelectionOrder.find(&node);
    if (it != nodeSelectionOrder.end())
      reparentOrder.push_back(it->second);
  });

  dag::Vector<CompositeEditorTreeDataNode *> parentsToConvert;
  // Reverse pre-order: children and higher-index siblings first, keeping indexInParent valid for each erase.
  for (int i = reparentOrder.size() - 1; i >= 0; --i)
  {
    int idx = reparentOrder[i];
    CompositeEditorTreeDataNode *node = nodesReparented[idx];
    const NodeInfo &oldInf = nodeOldInfo[idx];
    const NodeInfo &newInf = nodeNewInfo[idx];

    recalcMatrixInParentBase(oldInf.parent, node, newInf.parent);

    // Move the node itself to its new parent, keeping the original pointer valid.
    int newParentIndex = newInf.indexInParent + 1;
    newInf.parent->nodes.insert(newInf.parent->nodes.begin() + newParentIndex, std::move(oldInf.parent->nodes[oldInf.indexInParent]));

    oldInf.parent->nodes.erase(oldInf.parent->nodes.begin() + oldInf.indexInParent);
    parentsToConvert.push_back(oldInf.parent);
  }

  for (CompositeEditorTreeDataNode *parent : parentsToConvert)
  {
    // Skip conversion if the sole remaining ent child is still selected.
    if (parent->nodeCount() == 1 && parent->nodes[0]->isEntBlock())
    {
      const CompositeEditorTreeDataNode *entNode = parent->nodes[0].get();
      if (selectedTreeDataNode == entNode ||
          eastl::find(selectedTreeDataNodes.begin(), selectedTreeDataNodes.end(), entNode) != selectedTreeDataNodes.end())
        continue;
    }
    parent->convertSingleRandomEntityNodeToRegularNode(parent == &treeData.rootNode);
  }

  const bool reparented = reparentOrder.size() > 0;
  if (reparented)
    updateAssetFromTree(CompositeEditorRefreshType::EntityAndCompositeEditor);

  endUndo("Composit Editor: Clearing parent relation(s)", /*accept = */ reparented);
}

bool CompositeEditor::isTreeDataNodeSelected(CompositeEditorTreeDataNode *treeDataNode)
{
  PropPanel::TreeBaseWindow *tree = compositeTreeView.get();
  if (!tree)
    return false;

  dag::Vector<PropPanel::TLeafHandle> items;
  tree->getSelectedItems(items);
  for (int i = 0; i < items.size(); ++i)
  {
    CompositeEditorTreeDataNode *multiSelectedNode = static_cast<CompositeEditorTreeDataNode *>(tree->getItemData(items[i]));
    if (multiSelectedNode && multiSelectedNode == treeDataNode)
      return true;
  }

  return false;
}

void CompositeEditor::getSelectedTreeNodeDataBlockIds(dag::Vector<unsigned> &dataBlockIds, unsigned &parentDataBlockId) const
{
  PropPanel::TreeBaseWindow *tree = compositeTreeView.get();
  if (!tree)
    return;

  dag::Vector<PropPanel::TLeafHandle> items;
  tree->getSelectedItems(items);
  parentDataBlockId = IDataBlockIdHolder::invalid_id;
  for (int i = 0; i < items.size(); ++i)
  {
    CompositeEditorTreeDataNode *multiSelectedNode = static_cast<CompositeEditorTreeDataNode *>(tree->getItemData(items[i]));
    if (multiSelectedNode)
    {
      dataBlockIds.push_back(multiSelectedNode->dataBlockId);
      if (selectedTreeDataNode == multiSelectedNode)
        parentDataBlockId = multiSelectedNode->dataBlockId;
    }
  }
}

void CompositeEditor::getSelectedTreeDataNodes(dag::Vector<CompositeEditorTreeDataNode *> &selectTreeDataNodes)
{
  PropPanel::TreeBaseWindow *tree = compositeTreeView.get();
  if (!tree)
    return;

  dag::Vector<PropPanel::TLeafHandle> items;
  tree->getSelectedItems(items);
  selectTreeDataNodes.clear();
  for (int i = 0; i < items.size(); ++i)
  {
    CompositeEditorTreeDataNode *multiSelectedNode = static_cast<CompositeEditorTreeDataNode *>(tree->getItemData(items[i]));
    if (multiSelectedNode)
      selectTreeDataNodes.push_back(multiSelectedNode);
  }
}

TMatrix CompositeEditor::calcParentMatrix(CompositeEditorTreeDataNode *parentTreeDataNode)
{
  TMatrix parentMatrix = parentTreeDataNode->getTransformationMatrix();
  while (parentTreeDataNode != &treeData.rootNode)
  {
    int nodeIndex;
    parentTreeDataNode = CompositeEditorTreeData::getTreeDataNodeParent(*parentTreeDataNode, treeData.rootNode, nodeIndex);
    if (parentTreeDataNode == nullptr)
      break;

    parentMatrix = parentTreeDataNode->getTransformationMatrix() * parentMatrix;
  }
  return parentMatrix;
}

void CompositeEditor::recalcMatrixInParentBase(CompositeEditorTreeDataNode *oldParent, CompositeEditorTreeDataNode *treeDataNode,
  CompositeEditorTreeDataNode *newParent)
{
  recalcMatrixInParentBase(treeDataNode, calcParentMatrix(oldParent), calcParentMatrix(newParent));
}

void CompositeEditor::recalcMatrixInParentBase(CompositeEditorTreeDataNode *treeDataNode, const TMatrix &oldParentMatrix,
  const TMatrix &newParentMatrix)
{
  TMatrix matrix = TMatrix::IDENT;
  if (treeDataNode->getUseTransformationMatrix())
    matrix = treeDataNode->getTransformationMatrix();

  matrix = inverse(newParentMatrix) * oldParentMatrix * matrix;

  treeDataNode->setIdentityTransformationMatrix();
  const int paramIndex = treeDataNode->params.findParam("tm");
  if (paramIndex >= 0 && treeDataNode->params.getParamType(paramIndex) == DataBlock::TYPE_MATRIX)
    treeDataNode->params.setTm(paramIndex, matrix);

  treeDataNode->updateTransformParamsFromMatrix();
}

CompositeEditorTreeDataNode *CompositeEditor::getTreeNodeByDataBlockId(unsigned dataBlockId)
{
  if (!compositeTreeView || dataBlockId == IDataBlockIdHolder::invalid_id)
    return nullptr;

  return CompositeEditorTreeData::getTreeDataNodeByDataBlockId(treeData.rootNode, dataBlockId);
}

CompositeEditorTreeDataNode *CompositeEditor::getTreeDataNodeParent(const CompositeEditorTreeDataNode *treeDataNode, int &nodeIndex)
{
  if (!compositeTreeView || !treeDataNode)
    return nullptr;

  nodeIndex = -1;
  return CompositeEditorTreeData::getTreeDataNodeParent(*treeDataNode, treeData.rootNode, nodeIndex);
}

bool CompositeEditor::isTreeDataNodeRootNode(CompositeEditorTreeDataNode *treeDataNode)
{
  if (!compositeTreeView || !treeDataNode)
    return false;

  return treeDataNode == &treeData.rootNode;
}

void CompositeEditor::selectTreeNodeByDataBlockId(unsigned dataBlockId, bool multiSelect, bool selectAsParent)
{
  if (!compositeTreeView)
    return;

  CompositeEditorTreeDataNode *treeDataNode = nullptr;
  if (dataBlockId != IDataBlockIdHolder::invalid_id)
    treeDataNode = CompositeEditorTreeData::getTreeDataNodeByDataBlockId(treeData.rootNode, dataBlockId);
  else
    selectedTreeDataNode = nullptr;

  if (selectedTreeDataNode && selectedTreeDataNode == treeDataNode && selectAsParent)
    compositeTreeView->deselectByTreeDataNode(treeDataNode);
  else
  {
    bool isSelected = (getSelectedTreeDataNodeIndex(dataBlockId) > -1);
    if (treeDataNode && isSelected && multiSelect && !selectAsParent)
      compositeTreeView->deselectByTreeDataNode(treeDataNode);
    else
    {
      if (selectedTreeDataNode == nullptr || !multiSelect || selectAsParent)
      {
        selectedTreeDataNode = treeDataNode;
        if (treeDataNode)
          selectedTreeDataNodes.push_back(treeDataNode);
      }

      compositeTreeView->selectByTreeDataNode(treeDataNode, multiSelect);
    }
  }

  updateGizmo();
}

void CompositeEditor::onChange(int pcb_id, PropPanel::ContainerPropertyControl *panel)
{
  // Gizmo basis/center combos change only toolbar state, not asset data - no undo needed.
  if (pcb_id == CM_GIZMO_BASIS || pcb_id == CM_GIZMO_CENTER)
  {
    toolbar.onChange(pcb_id);
    return;
  }

  if (!compositePropPanel || !selectedTreeDataNode)
    return;

  if (UndoSystem *undoSystem = get_app().getUndoSystem())
    undoEditGesture.begin(*undoSystem, pcb_id, "Composit Editor: Property editing");

  beginUndo();

  const CompositeEditorRefreshType refreshType = compositePropPanel->onChange(*selectedTreeDataNode, pcb_id);
  updateAssetFromTree(refreshType);

  endUndo("Composit Editor: Property editing", /*accept = */ refreshType != CompositeEditorRefreshType::Nothing);
}

void CompositeEditor::onChangeFinished(int pcb_id, PropPanel::ContainerPropertyControl *panel) { undoEditGesture.end(pcb_id); }

bool CompositeEditor::saveComposit()
{
  G_ASSERT(editedAsset);

  if (DataBlockIncludeChecker::DoesBlkUseIncludes(editedAsset->getSrcFilePath()))
  {
    const int dialogResult = wingw::message_box(wingw::MBS_EXCL | wingw::MBS_YESNO, "WARNING",
      "The original composit contains an #include block. Saving the asset might cause data loss. Continue?");

    if (dialogResult != wingw::MB_ID_YES)
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

bool CompositeEditor::saveIfModified()
{
  if (!editedAsset || !modified)
    return true;

  return saveComposit();
}

namespace
{

class CompositeEditorAddNodeDlg : public SelectAssetDlg
{
public:
  CompositeEditorAddNodeDlg(DagorAssetMgr *mgr, dag::ConstSpan<int> type_filter) :
    SelectAssetDlg(nullptr, mgr, "Add node", "Create node with asset", "Add empty node", type_filter)
  {
    setManualModalSizingEnabled();
    setInitialFocus(PropPanel::DIALOG_ID_NONE);
    // Replace side-by-side Cancel with a stacked full-width button below OK.
    removeDialogButton(PropPanel::DIALOG_ID_CANCEL);
    buttonsPanel->createButton(PropPanel::DIALOG_ID_CANCEL, "Add empty node");
  }

  float calculateButtonPanelHeight() const override
  {
    return SelectAssetDlg::calculateButtonPanelHeight() + ImGui::GetFrameHeightWithSpacing();
  }
};

} // namespace

void CompositeEditor::createNode() { createNode(selectedTreeDataNode ? *selectedTreeDataNode : treeData.rootNode); }

void CompositeEditor::createNode(CompositeEditorTreeDataNode &parent, bool show_dialog)
{
  SimpleString assetName;
  if (show_dialog)
  {
    dag::ConstSpan<int> genObjTypes = DAEDITOR3.getGenObjAssetTypes();
    CompositeEditorAddNodeDlg dlg(&const_cast<DagorAssetMgr &>(get_app().getAssetMgr()), genObjTypes);
    const int dialogResult = dlg.showDialog();
    if (dialogResult != PropPanel::DIALOG_ID_OK && dialogResult != PropPanel::DIALOG_ID_CANCEL)
      return;

    if (dialogResult == PropPanel::DIALOG_ID_OK)
      assetName = dlg.getSelObjName();
  }

  beginUndo();
  endUndo("Composite Editor: Adding node");

  parent.insertNodeBlock(-1);

  if (*assetName)
  {
    CompositeEditorTreeDataNode *newNode = parent.nodes.back().get();
    newNode->params.setStr("name", assetName);
  }

  updateAssetFromTree(CompositeEditorRefreshType::EntityAndCompositeEditor);
}

IObjEntity *CompositeEditor::getSubEntityByDataBlockId(unsigned dataBlockId)
{
  IObjEntity *entity = gizmoClient.getEntity();
  if (!entity || dataBlockId == IDataBlockIdHolder::invalid_id)
    return nullptr;

  ICompositObj *compositObj = entity->queryInterface<ICompositObj>();
  if (!compositObj)
    return nullptr;

  const int subEntityCount = compositObj->getCompositSubEntityCount();
  for (int subEntityIndex = 0; subEntityIndex < subEntityCount; ++subEntityIndex)
  {
    IObjEntity *subEntity = compositObj->getCompositSubEntity(subEntityIndex);
    if (!subEntity)
      continue;

    IDataBlockIdHolder *dbih = subEntity->queryInterface<IDataBlockIdHolder>();
    if (dbih && dbih->getDataBlockId() == dataBlockId)
      return subEntity;
  }

  return nullptr;
}

void CompositeEditor::splitCompositeInternal(CompositeEditorTreeDataNode *treeDataNode, bool recursive, bool startUndo)
{
  G_ASSERT(treeDataNode && treeDataNode->isCompositeAsset());

  DagorAsset *asset = get_app().getAssetMgr().findAsset(treeDataNode->getAssetName());
  if (!asset)
    return;

  int nodeIndex = -1;
  CompositeEditorTreeDataNode *nodeParent = getTreeDataNodeParent(treeDataNode, nodeIndex);
  if (!nodeParent)
    return;

  if (startUndo)
    beginUndo();

  // parent transform
  TMatrix tm = TMatrix::IDENT;
  const bool useTransformationMatrix = treeDataNode->getUseTransformationMatrix();
  if (useTransformationMatrix)
    tm = treeDataNode->getTransformationMatrix();

  CompositeEditorTreeData subTreeData;
  subTreeData.convertAssetToTreeData(asset, &asset->props);

  // save old child nodes before erase!
  dag::Vector<eastl::unique_ptr<CompositeEditorTreeDataNode>> oldChildNodes = eastl::move(treeDataNode->nodes);

  // erase old composite node
  nodeParent->nodes.erase(nodeParent->nodes.begin() + nodeIndex);
  nodeParent->convertSingleRandomEntityNodeToRegularNode(nodeParent == &treeData.rootNode);
  if (startUndo && treeDataNode->dataBlockId != IDataBlockIdHolder::invalid_id)
  {
    int idx = getSelectedTreeDataNodeIndex(treeDataNode->dataBlockId);
    if (idx > -1)
      selectedTreeDataNodes.erase(selectedTreeDataNodes.begin() + idx);
  }

  // transform split nodes
  for (int i = 0; i < subTreeData.rootNode.nodeCount(); ++i)
  {
    const int at = nodeIndex + i;
    nodeParent->nodes.insert(nodeParent->nodes.begin() + at, std::move(subTreeData.rootNode.nodes[i]));
    CompositeEditorTreeDataNode *splitNode = nodeParent->nodes[at].get();
    if (splitNode && splitNode->canTransform())
    {
      TMatrix splitTm = TMatrix::IDENT;
      if (splitNode->getUseTransformationMatrix())
        splitTm = splitNode->getTransformationMatrix();
      splitNode->params.setTm("tm", tm * splitTm);
      splitNode->updateTransformParamsFromMatrix();
    }
  }

  if (oldChildNodes.size() > 0)
  {
    // add the old child nodes to a new empty node (cleaner and retains transforms) right after split result
    const int at = nodeIndex + subTreeData.rootNode.nodeCount();
    nodeParent->insertNodeBlock(at);
    CompositeEditorTreeDataNode *newParent = nodeParent->nodes[at].get();
    if (useTransformationMatrix)
      newParent->params.setTm("tm", tm);
    newParent->nodes = std::move(oldChildNodes);
  }

  if (recursive)
  {
    for (int i = subTreeData.rootNode.nodeCount() - 1; i >= 0; --i)
    {
      const int at = nodeIndex + i;
      CompositeEditorTreeDataNode *splitNode = nodeParent->nodes[at].get();
      if (splitNode && splitNode->isCompositeAsset())
        splitCompositeInternal(splitNode, recursive, false);
    }
  }

  if (startUndo)
    endUndo("Composit Editor: Splitting composite");
}

void CompositeEditor::splitSelectedCompositeNode(bool recursive)
{
  G_ASSERT(selectedTreeDataNode && selectedTreeDataNode->isCompositeAsset());
  if (!selectedTreeDataNode || !selectedTreeDataNode->isCompositeAsset())
    return;

  splitCompositeInternal(selectedTreeDataNode, recursive, true);

  selectedTreeDataNode = nullptr;
}

bool CompositeEditor::canSaveSelectedAsComposite()
{
  int selectedCount = 0;
  bool siblings = true;
  CompositeEditorTreeDataNode *firstParent = nullptr;
  for (CompositeEditorTreeDataNode *node : selectedTreeDataNodes)
  {
    if (node)
    {
      int nodeIndex;
      firstParent = getTreeDataNodeParent(node, nodeIndex);
      if (firstParent)
        break;
    }
  }
  if (firstParent)
  {
    for (CompositeEditorTreeDataNode *node : selectedTreeDataNodes)
    {
      if (!node || node->dataBlockId == IDataBlockIdHolder::invalid_id)
        continue;

      if (!firstParent->hasChildNode(node->dataBlockId))
      {
        siblings = false;
        break;
      }

      selectedCount++;
    }
  }
  else
    siblings = false;

  return siblings && selectedCount > 1;
}

bool CompositeEditor::saveSelectedAsNewComposite()
{
  G_ASSERT(editedAsset);
  G_ASSERT(canSaveSelectedAsComposite());

  SaveAsNewCompositeDialog dlg(editedAsset);
  if (dlg.showDialog() != PropPanel::DIALOG_ID_OK)
    return false;

  CompositeEditorTreeDataNode *nodeParent = nullptr;
  dag::Vector<int> indices;

  CompositeEditorTreeData newCompositeTreeData;
  for (CompositeEditorTreeDataNode *node : selectedTreeDataNodes)
  {
    if (node == nullptr)
      continue;

    int nodeIndex = -1;
    CompositeEditorTreeDataNode *parent = CompositeEditorTreeData::getTreeDataNodeParent(*node, treeData.rootNode, nodeIndex);
    if (!parent)
      continue;

    nodeParent = parent;
    indices.push_back(nodeIndex);

    DataBlock cloneDataBlock;
    CompositeEditorTreeData::convertTreeDataToDataBlock(*node, cloneDataBlock);

    eastl::unique_ptr<CompositeEditorTreeDataNode> clonedTreeDataNode = eastl::make_unique<CompositeEditorTreeDataNode>();
    CompositeEditorTreeData::convertDataBlockToTreeData(cloneDataBlock, *clonedTreeDataNode);

    newCompositeTreeData.rootNode.nodes.push_back(std::move(clonedTreeDataNode));
  }

  DataBlock block;
  treeData.convertTreeDataToDataBlock(newCompositeTreeData.rootNode, block);
  block.setStr("className", "composit");
  String fullPath = dlg.getFullPath();
  const bool succeeded = block.saveToTextFile(fullPath);
  if (!succeeded)
  {
    wingw::message_box(wingw::MBS_EXCL | wingw::MBS_OK, "Error", "Failed to create new composite '%s'.", fullPath.str());
    return false;
  }

  get_app().trackChangesContinuous(-1, true);

  if (dlg.shouldReplaceSelection())
  {
    int attempt = 0;
    String assetName = dlg.getAssetName();
    DagorAsset *newAsset = get_app().getAssetMgr().findAsset(assetName);
    while (newAsset == nullptr && attempt < 2)
    {
      sleep_msec(100); // The immediate change tracking run usually misses it...

      get_app().trackChangesContinuous(-1, true);

      newAsset = get_app().getAssetMgr().findAsset(assetName);
      attempt++;
    }

    if (newAsset == nullptr)
    {
      wingw::message_box(wingw::MBS_EXCL | wingw::MBS_OK, "Error", "Failed to load new composite '%s'.", fullPath.str());
      return false;
    }

    beginUndo();
    endUndo("Composit Editor: Replace selection with new composit");

    eastl::sort(indices.begin(), indices.end());
    for (int i = (indices.size() - 1); i >= 0; --i)
    {
      nodeParent->nodes.erase(nodeParent->nodes.begin() + indices[i]);
      nodeParent->convertSingleRandomEntityNodeToRegularNode(nodeParent == &treeData.rootNode);
    }

    if (indices.size() > 0)
    {
      int index = indices[0];
      nodeParent->insertNodeBlock(index);

      if (!assetName.empty())
        nodeParent->nodes[index]->params.setStr("name", assetName);
      else
        nodeParent->nodes[index]->params.removeParam("name");

      // Remove the unused type parameter from old .blk files.
      nodeParent->nodes[index]->params.removeParam("type");
    }

    selectedTreeDataNode = nullptr;
    selectedTreeDataNodes.clear();

    add_delayed_callback((delayed_callback)&CompositeEditor::onDelayedUpdateAssetFromTree,
      (void *)CompositeEditorRefreshType::EntityAndCompositeEditor);
  }

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

int CompositeEditor::getSelectedTreeDataNodeIndex(unsigned dataBlockId)
{
  for (int i = 0; i < selectedTreeDataNodes.size(); ++i)
  {
    CompositeEditorTreeDataNode *node = selectedTreeDataNodes[i];
    if (node != nullptr && node->dataBlockId == dataBlockId)
      return i;
  }
  return -1;
}

bool CompositeEditor::areMultipleNodesSelected() const
{
  int selectionCount = 0;
  for (CompositeEditorTreeDataNode *node : selectedTreeDataNodes)
    if (node != nullptr)
      selectionCount++;
  return selectionCount > 1;
}

void CompositeEditor::deleteSelectedNodes()
{
  if (!selectedTreeDataNode || isTreeDataNodeSelected(&treeData.rootNode))
    return;

  beginUndo();
  endUndo("Composit Editor: Deleting node");

  // Collect in pre-order DFS, then reverse: descendants before ancestors.
  eastl::hash_set<const CompositeEditorTreeDataNode *> toDeleteSet;
  for (const CompositeEditorTreeDataNode *node : selectedTreeDataNodes)
    if (node)
      toDeleteSet.insert(node);
  dag::Vector<CompositeEditorTreeDataNode *> nodesToDelete;
  CompositeEditorTreeData::traverseDepthFirst(treeData.rootNode, [&](CompositeEditorTreeDataNode &node, int) {
    if (toDeleteSet.count(&node))
      nodesToDelete.push_back(&node);
  });
  eastl::reverse(nodesToDelete.begin(), nodesToDelete.end());

  const bool isMultiSelection = nodesToDelete.size() > 1;

  // Save the cache index before the node pointer becomes dangling after deletion.
  const int selectedNodeCacheIdx = !isMultiSelection ? getSelectedTreeDataNodeIndex(selectedTreeDataNode->dataBlockId) : -1;
  G_ASSERT(isMultiSelection || selectedNodeCacheIdx > -1);

  dag::Vector<CompositeEditorTreeDataNode *> parentsToConvert;
  int nodeIndex = -1;
  CompositeEditorTreeDataNode *nodeParent = nullptr;
  for (CompositeEditorTreeDataNode *node : nodesToDelete)
  {
    nodeParent = CompositeEditorTreeData::getTreeDataNodeParent(*node, treeData.rootNode, nodeIndex);
    if (!nodeParent)
      continue;

    // Reparent non-ent children into the parent right after this node.
    // Selected children are already gone at this point (bottom-up sort ensures they are processed first).
    if (!node->nodes.empty())
    {
      const TMatrix nodeParentMatrix = calcParentMatrix(node);
      const TMatrix newParentMatrix = calcParentMatrix(nodeParent);
      int insertOffset = 1;
      for (auto &childPtr : node->nodes)
      {
        if (childPtr->isEntBlock())
          continue;
        recalcMatrixInParentBase(childPtr.get(), nodeParentMatrix, newParentMatrix);
        nodeParent->nodes.insert(nodeParent->nodes.begin() + nodeIndex + insertOffset, std::move(childPtr));
        ++insertOffset;
      }
    }

    nodeParent->nodes.erase(nodeParent->nodes.begin() + nodeIndex);
    // Defer the conversion of non-deleted parents until every node is processed
    // (converting mid-loop may free still-scheduled ent siblings).
    if (!toDeleteSet.count(nodeParent))
      parentsToConvert.push_back(nodeParent);
  }

  for (CompositeEditorTreeDataNode *parent : parentsToConvert)
    parent->convertSingleRandomEntityNodeToRegularNode(parent == &treeData.rootNode);

  if (isMultiSelection)
  {
    selectedTreeDataNode = nullptr;
    selectedTreeDataNodes.clear();
  }
  else
  {
    if (selectedNodeCacheIdx > -1)
      selectedTreeDataNodes.erase(selectedTreeDataNodes.begin() + selectedNodeCacheIdx);

    if (nodeParent && nodeParent->nodes.size() > 0)
      selectedTreeDataNode = nodeParent->nodes[nodeIndex >= nodeParent->nodes.size() ? (nodeIndex - 1) : nodeIndex].get();
    else
      selectedTreeDataNode = nodeParent;

    if (selectedTreeDataNode != nullptr)
      selectedTreeDataNodes.push_back(selectedTreeDataNode);
  }

  updateAssetFromTree(CompositeEditorRefreshType::EntityAndCompositeEditor);
}

void CompositeEditor::onDelayedRefresh(const DagorAsset *asset, CompositeEditorRefreshType refreshType)
{
  if (asset != editedAsset)
    return;

  DagorAssetMgr::WriteGuard wg(asset->getMgr().mutex);
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

void CompositeEditor::onDelayedUpdateAssetFromTree(CompositeEditorRefreshType refreshType)
{
  get_app().getCompositeEditor().updateAssetFromTree(refreshType);
}

void CompositeEditor::onDelayedRevert(const DagorAsset *asset)
{
  CompositeEditor &editor = get_app().getCompositeEditor();

  if (editor.isEditingSubComposite())
  {
    // The reverted sub-asset's reload notification triggers end()+begin() for editedAsset (its
    // parent in the stack). Without a ReloadHelper, end() would clear the sub-composite stack.
    DagorAssetMgr::WriteGuard wg(asset->getMgr().mutex);
    ReloadHelper reloadHelper(editor, *editor.editedAsset, CompositeEditorRefreshType::EntityAndCompositeEditor);
    get_app().getAssetMgr().callAssetChangeNotifications(*asset, asset->getNameId(), asset->getType());
    return;
  }

  get_app().getAssetMgr().callAssetChangeNotifications(*asset, asset->getNameId(), asset->getType());

  // end() must have been called and that clears undo.
  UndoSystem *undoSystem = get_app().getUndoSystem();
  G_ASSERT(undoSystem && undoSystem->undo_level() == 0);
}

void CompositeEditor::onDelayedRevertAfterUniqueSwap(const DagorAsset *sub_asset)
{
  CompositeEditor &editor = get_app().getCompositeEditor();
  DagorAssetMgr::WriteGuard wg(sub_asset->getMgr().mutex);
  if (editor.editedAsset)
  {
    // The parent entity cache still depends on sub_asset even though the parent's props were
    // already updated to reference the new unique asset. Suppress the resulting
    // end() -> canSwitchToAnotherAsset() dialog since the reload is an internal side effect.
    ReloadHelper reloadHelper(editor, *editor.editedAsset, CompositeEditorRefreshType::EntityAndCompositeEditor);
    get_app().getAssetMgr().callAssetChangeNotifications(*sub_asset, sub_asset->getNameId(), sub_asset->getType());
    return;
  }
  get_app().getAssetMgr().callAssetChangeNotifications(*sub_asset, sub_asset->getNameId(), sub_asset->getType());
}

bool CompositeEditor::canSwitchToAnotherAsset()
{
  G_ASSERT(editedAsset);

  if (!modified)
    return true;

  const int dialogResult = wingw::message_box(wingw::MBS_QUEST | wingw::MBS_YESNOCANCEL, "Composit properties",
    "You have changed the composit. Do you want to save the changes?");

  if (dialogResult == wingw::MB_ID_YES)
  {
    return saveComposit();
  }
  else if (dialogResult == wingw::MB_ID_NO)
  {
    revertChanges();
    return true;
  }

  G_ASSERT(dialogResult == wingw::MB_ID_CANCEL);
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

  get_app().onSnapSettingChanged();
}

void CompositeEditor::openGridSettings()
{
  if (ViewportWindow *viewport = static_cast<ViewportWindow *>(EDITORCORE->getCurrentViewport()))
  {
    viewport->showGridSettingsDialog();
  }
}

DragAndDropResult EntityTreeDropHandler::onDropTargetDirect(PropPanel::TLeafHandle leaf)
{
  CompositeEditorTree *compositeTreeView = editor.compositeTreeView.get();
  if (!compositeTreeView)
    return DragAndDropResult::NONE;

  auto treeDataNode = (CompositeEditorTreeDataNode *)compositeTreeView->getItemData(leaf);
  if (!leaf || !treeDataNode)
    return DragAndDropResult::NONE;

  auto rootNodeHandle = compositeTreeView->getRoot();
  auto rootTreeDataNode = (CompositeEditorTreeDataNode *)compositeTreeView->getItemData(rootNodeHandle);
  if (treeDataNode == rootTreeDataNode || !treeDataNode->canEditAssetName(false))
    return DragAndDropResult::NOT_ALLOWED;

  const ImGuiPayload *dragAndDropPayload = PropPanel::acceptDragDropPayloadBeforeDelivery(ASSET_DRAG_AND_DROP_TYPE);
  if (!dragAndDropPayload)
    return DragAndDropResult::NONE;

  AssetDragData dragData;
  PropPanel::getDragAndDropPayloadData<AssetDragData>(dragAndDropPayload, &dragData);

  // Only allow dropping into the same asset type.
  DagorAsset *asset = dragData.asset;
  if (asset != nullptr)
  {
    const int type = asset->getType();
    dag::ConstSpan<int> allowedTypes = DAEDITOR3.getGenObjAssetTypes();
    if (eastl::find(allowedTypes.begin(), allowedTypes.end(), type) != allowedTypes.end())
    {
      if (dragAndDropPayload->IsDelivery())
      {
        const char *assetName = asset->getName();
        if (!assetName)
          return DragAndDropResult::NONE;

        editor.beginUndo();
        editor.endUndo("Composit Editor: Property editing");

        if (*assetName)
          treeDataNode->params.setStr("name", assetName);
        else
          treeDataNode->params.removeParam("name");

        // Remove the unused type parameter from old .blk files.
        treeDataNode->params.removeParam("type");

        PropPanel::request_delayed_callback(editor, (void *)CompositeEditorRefreshType::EntityAndCompositeEditor);

        return DragAndDropResult::ACCEPTED;
      }
      return DragAndDropResult::NONE;
    }
  }

  return DragAndDropResult::NOT_ALLOWED;
}

DragAndDropResult ChildEntityDragAndDropHandler::handleDropTarget(int pcb_id, PropPanel::ContainerPropertyControl *panel)
{
  CompositeEditorPanel *compositePropPanel = editor.compositePropPanel.get();
  CompositeEditorTree *compositeTreeView = editor.compositeTreeView.get();
  CompositeEditorTreeDataNode *selectedTreeDataNode = editor.selectedTreeDataNode;
  if (!compositePropPanel || !compositeTreeView || !selectedTreeDataNode)
    return DragAndDropResult::NONE;

  // Only allow dropping into active composite edit buttons
  auto rootNode = compositeTreeView->getRoot();
  auto rootTreeDataNode = (CompositeEditorTreeDataNode *)compositeTreeView->getItemData(rootNode);
  const bool isRootNode = (selectedTreeDataNode == rootTreeDataNode);

  // Only allow dropping into the 'selector' and '+' entity/child buttons
  if (pcb_id == ID_COMPOSITE_EDITOR_ENTITIES_ADD ||
      (pcb_id >= ID_COMPOSITE_EDITOR_ENTITIES_ENTITY_SELECTOR_FIRST && pcb_id <= ID_COMPOSITE_EDITOR_ENTITIES_ENTITY_SELECTOR_LAST))
  {
    if (!selectedTreeDataNode->canEditRandomEntities(isRootNode))
      return DragAndDropResult::NONE;
  }
  else if (pcb_id == ID_COMPOSITE_EDITOR_CHILDREN_ADD)
  {
    if (!selectedTreeDataNode->canEditChildren())
      return DragAndDropResult::NONE;
  }
  else if (pcb_id >= ID_COMPOSITE_EDITOR_CHILDREN_ENTITY_SELECTOR_FIRST && pcb_id <= ID_COMPOSITE_EDITOR_CHILDREN_ENTITY_SELECTOR_LAST)
  {
    const int childIndex = pcb_id - ID_COMPOSITE_EDITOR_CHILDREN_ENTITY_SELECTOR_FIRST;
    if (!selectedTreeDataNode->nodes[childIndex]->canEditAssetName(false))
      return DragAndDropResult::NONE;
  }
  else if (pcb_id != ID_COMPOSITE_EDITOR_ENTITY_SELECTOR)
  {
    return DragAndDropResult::NONE;
  }

  const ImGuiPayload *dragAndDropPayload = PropPanel::acceptDragDropPayloadBeforeDelivery(ASSET_DRAG_AND_DROP_TYPE);
  if (!dragAndDropPayload)
    return DragAndDropResult::NONE;

  AssetDragData dragData;
  PropPanel::getDragAndDropPayloadData<AssetDragData>(dragAndDropPayload, &dragData);

  // Only allow dropping into the same asset type.
  DagorAsset *asset = dragData.asset;
  if (asset != nullptr)
  {
    const int type = asset->getType();
    dag::ConstSpan<int> allowedTypes = DAEDITOR3.getGenObjAssetTypes();
    if (eastl::find(allowedTypes.begin(), allowedTypes.end(), type) != allowedTypes.end())
    {
      if (dragAndDropPayload->IsDelivery())
      {
        const CompositeEditorRefreshType refreshType = compositePropPanel->onDragAndDropAsset(*selectedTreeDataNode, pcb_id, asset);

        PropPanel::request_delayed_callback(editor, (void *)refreshType);

        return DragAndDropResult::ACCEPTED;
      }
      return DragAndDropResult::NONE;
    }
  }

  return DragAndDropResult::NOT_ALLOWED;
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
  else if (pcb_id == ID_COMPOSITE_EDITOR_EDIT_SUB_COMPOSITE)
  {
    enterSubCompositeEditing();
  }
  else if (pcb_id == CM_COMPOSITE_EDITOR_SUB_COMPOSITE_SAVE)
  {
    saveSubCompositeEditing();
  }
  else if (pcb_id == CM_COMPOSITE_EDITOR_SUB_COMPOSITE_SAVE_UNIQUE)
  {
    saveSubCompositeAsUnique();
  }
  else if (pcb_id == CM_COMPOSITE_EDITOR_SUB_COMPOSITE_REVERT)
  {
    revertSubCompositeEditing();
  }
  else if (pcb_id == ID_COMPOSITE_EDITOR_SAVE_AS_NEW_CMP)
  {
    saveSelectedAsNewComposite();
  }
  else if (pcb_id == ID_COMPOSITE_EDITOR_DELETE_NODE)
  {
    deleteSelectedNodes();
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
  else if (pcb_id == CM_OPTIONS_GRID)
  {
    openGridSettings();
  }
  else if (pcb_id == CM_COMPOSITE_EDITOR_CREATE_NODE)
  {
    createNode(selectedTreeDataNode ? *selectedTreeDataNode : treeData.rootNode);
  }
  else if (compositePropPanel && selectedTreeDataNode)
  {
    if (pcb_id == ID_COMPOSITE_EDITOR_ENTITY_ACTIONS)
    {
      const int action = compositePropPanel->getInt(pcb_id);
      if (action == PropPanel::EXT_BUTTON_REMOVE)
        deleteSelectedNodes();
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
  if (id == CM_COMPOSITE_EDITOR_ADD_NODE)
  {
    G_ASSERT(selectedTreeDataNode);
    if (selectedTreeDataNode)
      createNode(*selectedTreeDataNode, false);

    return 0;
  }
  else if (id == CM_COMPOSITE_EDITOR_ADD_RANDOM_ENTITY)
  {
    G_ASSERT(selectedTreeDataNode);
    if (!selectedTreeDataNode)
      return 0;

    beginUndo();
    endUndo("Composit Editor: Adding entity");

    selectedTreeDataNode->insertEntBlock(-1);

    updateAssetFromTree(CompositeEditorRefreshType::EntityAndCompositeEditor);

    return 0;
  }
  else if (id == CM_COMPOSITE_EDITOR_CHANGE_ASSET)
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
  else if (id == CM_COMPOSITE_EDITOR_DELETE_NODE)
  {
    deleteSelectedNodes();
    return 0;
  }
  else if (id == CM_COMPOSITE_EDITOR_OPEN_ASSET)
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
  else if (id == CM_COMPOSITE_EDITOR_COPY_ASSET_FILEPATH_TO_CLIPBOARD)
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
  else if (id == CM_COMPOSITE_EDITOR_COPY_ASSET_NAME_TO_CLIPBOARD)
  {
    G_ASSERT(selectedTreeDataNode);
    if (!selectedTreeDataNode)
      return 0;

    DagorAsset *asset = DAEDITOR3.getAssetByName(selectedTreeDataNode->getAssetName());
    if (asset)
      clipboard::set_clipboard_ansi_text(asset->getName());

    return 0;
  }
  else if (id == CM_COMPOSITE_EDITOR_REVEAL_ASSET_IN_EXPLORER)
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

  selectedTreeDataNodes.clear();
  dag::Vector<PropPanel::TLeafHandle> items;
  tree.getSelectedItems(items);
  CompositeEditorTreeDataNode *lastSelectedTreeDataNode = nullptr;
  for (PropPanel::TLeafHandle item : items)
  {
    CompositeEditorTreeDataNode *multiSelectedNode = static_cast<CompositeEditorTreeDataNode *>(tree.getItemData(item));
    if (multiSelectedNode)
      selectedTreeDataNodes.push_back(multiSelectedNode);

    if (multiSelectedNode == selectedTreeDataNode)
      lastSelectedTreeDataNode = multiSelectedNode;
    else if (lastSelectedTreeDataNode == nullptr)
      lastSelectedTreeDataNode = multiSelectedNode;
  }
  selectedTreeDataNode = lastSelectedTreeDataNode;

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

  const bool isRootNodeSelected = isTreeDataNodeSelected(&treeData.rootNode);
  const bool isMultiSelection = areMultipleNodesSelected();
  if (isRootNodeSelected && isMultiSelection)
    return false;

  bool separateDelete = false;

  PropPanel::IMenu &menu = tree.createContextMenu();
  menu.setEventHandler(this);

  if (!isMultiSelection)
  {
    if (selectedTreeDataNode->canEditChildren())
    {
      menu.addItem(ROOT_MENU_ITEM, CM_COMPOSITE_EDITOR_ADD_NODE, "Add node");
      separateDelete = true;
    }

    if (selectedTreeDataNode->canEditRandomEntities(isRootNodeSelected))
    {
      menu.addItem(ROOT_MENU_ITEM, CM_COMPOSITE_EDITOR_ADD_RANDOM_ENTITY, "Add entity");
      separateDelete = true;
    }

    if (selectedTreeDataNode->canEditAssetName(isRootNodeSelected))
    {
      menu.addItem(ROOT_MENU_ITEM, CM_COMPOSITE_EDITOR_CHANGE_ASSET, "Change asset");
      separateDelete = true;
    }
  }

  if (!isRootNodeSelected)
  {
    if (separateDelete)
      menu.addSeparator(ROOT_MENU_ITEM);

    const char *deleteTitle = isMultiSelection ? "Delete selected nodes\tDelete" : "Delete node\tDelete";
    menu.addItem(ROOT_MENU_ITEM, CM_COMPOSITE_EDITOR_DELETE_NODE, deleteTitle);

    if (selectedTreeDataNode->hasNameParameter() && !isMultiSelection)
    {
      menu.addSeparator(ROOT_MENU_ITEM);
      menu.addItem(ROOT_MENU_ITEM, CM_COMPOSITE_EDITOR_COPY_ASSET_FILEPATH_TO_CLIPBOARD, "Copy filepath");
      menu.addItem(ROOT_MENU_ITEM, CM_COMPOSITE_EDITOR_COPY_ASSET_NAME_TO_CLIPBOARD, "Copy name");
      menu.addItem(ROOT_MENU_ITEM, CM_COMPOSITE_EDITOR_OPEN_ASSET, "Open asset");
      menu.addSeparator(ROOT_MENU_ITEM);
      menu.addItem(ROOT_MENU_ITEM, CM_COMPOSITE_EDITOR_REVEAL_ASSET_IN_EXPLORER, "Reveal in Explorer");
    }
  }

  return true;
}

void CompositeEditor::onImguiDelayedCallback(void *user_data)
{
  auto refreshType = CompositeEditorRefreshType::Nothing;

  if (user_data)
    refreshType = (CompositeEditorRefreshType)((intptr_t)user_data);

  updateAssetFromTree(refreshType);
}

void CompositeEditor::updateMultipleNodesTransforms(const dag::Vector<CompositeEditorTreeDataNode *> &nodes,
  const dag::Vector<TMatrix> &tms)
{
  G_ASSERT(nodes.size() == tms.size());
  if (nodes.empty())
    return;

  // Only used by CompositeEditorGizmoClient, which handles undo.
  UndoSystem *undoSystem = get_app().getUndoSystem();
  G_ASSERT(undoSystem && undoSystem->is_holding());

  for (int i = 0; i < (int)nodes.size(); ++i)
  {
    G_ASSERT(nodes[i] && nodes[i]->getUseTransformationMatrix());
    nodes[i]->params.setTm("tm", tms[i]);
    nodes[i]->updateTransformParamsFromMatrix();
  }

  if (compositePropPanel)
    compositePropPanel->updateTransformParams(treeData, selectedTreeDataNode);

  updateAssetFromTree(CompositeEditorRefreshType::Entity);
}

bool CompositeEditor::cloneSelectedNodesInternal(CompositeEditorRefreshType refreshType,
  dag::Vector<CompositeEditorTreeDataNode *> *out_clones_by_selection_idx)
{
  // This function is only used internally, and undo should be handled by the caller!
  UndoSystem *undoSystem = get_app().getUndoSystem();
  G_ASSERT(undoSystem && undoSystem->is_holding());

  // Capture originals before modifying selectedTreeDataNodes.
  dag::Vector<CompositeEditorTreeDataNode *> originalNodes = selectedTreeDataNodes;
  CompositeEditorTreeDataNode *savedSelectedNode = selectedTreeDataNode;
  selectedTreeDataNode = nullptr;
  selectedTreeDataNodes.clear();

  const int count = (int)originalNodes.size();
  dag::Vector<CompositeEditorTreeDataNode *> clonesBySelectionIndex(count, nullptr);
  dag::Vector<eastl::unique_ptr<CompositeEditorTreeDataNode>> cloneStorage(count);

  // Shallow-clone each selected node (no children) and collect their original parent.
  dag::Vector<NodeInfo> nodeInfos(count);
  for (int i = 0; i < count; ++i)
  {
    CompositeEditorTreeDataNode *node = originalNodes[i];
    if (!node)
      continue;
    int nodeIndex = -1;
    CompositeEditorTreeDataNode *parent = CompositeEditorTreeData::getTreeDataNodeParent(*node, treeData.rootNode, nodeIndex);
    if (!parent)
      continue;
    nodeInfos[i] = {parent, nodeIndex};
    auto clone = eastl::make_unique<CompositeEditorTreeDataNode>();
    clone->params = node->params;
    const char *blockName = node->params.getBlockName();
    if (blockName && *blockName)
      clone->params.changeBlockName(blockName);
    clonesBySelectionIndex[i] = clone.get();
    cloneStorage[i] = std::move(clone);
  }

  // Record which parent clone (if any) owns given cloned nodes.
  dag::Vector<CompositeEditorTreeDataNode *> clonedParent(count, nullptr);
  for (int i = 0; i < count; ++i)
  {
    if (!clonesBySelectionIndex[i])
      continue;
    for (int j = 0; j < count; ++j)
      if (originalNodes[j] == nodeInfos[i].parent)
      {
        clonedParent[i] = clonesBySelectionIndex[j];
        break;
      }
  }

  // Attach each clone to its parent (clone) in descending indexInParent order.
  // This re-attach to parent ordering ensures proper positioning in cloned and original parents as well.
  dag::Vector<int> intoParentOrder;
  for (int i = 0; i < count; ++i)
    if (clonesBySelectionIndex[i])
      intoParentOrder.push_back(i);
  eastl::sort(intoParentOrder.begin(), intoParentOrder.end(),
    [&](int a, int b) { return nodeInfos[a].indexInParent > nodeInfos[b].indexInParent; });

  int treeModifications = 0;
  for (int i : intoParentOrder)
  {
    if (clonedParent[i])
    {
      clonedParent[i]->nodes.insert(clonedParent[i]->nodes.begin(), std::move(cloneStorage[i]));
    }
    else
    {
      nodeInfos[i].parent->nodes.insert(nodeInfos[i].parent->nodes.begin() + nodeInfos[i].indexInParent + 1,
        std::move(cloneStorage[i]));
      ++treeModifications;
    }
  }

  for (int i = 0; i < count; ++i)
  {
    if (!clonesBySelectionIndex[i])
      continue;
    selectedTreeDataNodes.push_back(clonesBySelectionIndex[i]);
    if (originalNodes[i] == savedSelectedNode)
      selectedTreeDataNode = clonesBySelectionIndex[i];
  }

  if (out_clones_by_selection_idx)
    *out_clones_by_selection_idx = clonesBySelectionIndex;

  if (treeModifications > 0)
    updateAssetFromTree(refreshType);
  else
  {
    selectedTreeDataNode = savedSelectedNode;
    selectedTreeDataNodes = originalNodes;
  }

  return treeModifications > 0;
}

void CompositeEditor::cloneSelectedNodes(dag::Vector<CompositeEditorTreeDataNode *> &out_clones_by_selection_idx)
{
  G_ASSERT(selectedTreeDataNode);
  if (!selectedTreeDataNode)
    return;

  // Undo is handled by the caller (CompositeEditorGizmoClient).
  cloneSelectedNodesInternal(CompositeEditorRefreshType::Entity, &out_clones_by_selection_idx);
}

void CompositeEditor::copySelectedNodeParams()
{
  if (!selectedTreeDataNode)
    return;

  SimpleString text = blk_util::blkTextData(selectedTreeDataNode->params);
  clipboard::set_clipboard_utf8_text(text.c_str());
}

/*static*/
bool CompositeEditor::getNodeParamsFromClipboard(DataBlock &block)
{
  // It's unlikely that a composit blk file would be larger than 128 KB.
  const int maxLength = 128 * 1024;
  Tab<char> buffer;
  buffer.resize(maxLength);
  if (!clipboard::get_clipboard_utf8_text(&buffer[0], buffer.size()))
    return false;

  if (!block.loadText(&buffer[0], strlen(&buffer[0])))
    return false;

  return true;
}

void CompositeEditor::pasteParamsToSelectedNode()
{
  if (!selectedTreeDataNode)
    return;

  DataBlock block;
  if (!getNodeParamsFromClipboard(block))
    return;

  beginUndo();

  selectedTreeDataNode->params.setParamsFrom(&block);
  updateAssetFromTree(CompositeEditorRefreshType::EntityAndCompositeEditor);

  endUndo("Composit Editor: paste node params");
}

void CompositeEditor::duplicateSelectedNodes()
{
  if (!selectedTreeDataNode)
    return;

  if (selectedTreeDataNodes.size() == 1 && isTreeDataNodeRootNode(selectedTreeDataNode))
    return;

  beginUndo();

  const bool cloned = cloneSelectedNodesInternal(CompositeEditorRefreshType::EntityAndCompositeEditor, nullptr);

  endUndo("Composit Editor: duplicate node", cloned);
}

void CompositeEditor::setGizmo(IEditorCoreEngine::ModeType mode)
{
  lastSelectedGizmoMode = mode;
  switch (mode)
  {
    case IEditorCoreEngine::MODE_None: gizmoClient.setEditMode(CM_OBJED_MODE_SELECT); break;
    case IEditorCoreEngine::MODE_Move: gizmoClient.setEditMode(CM_OBJED_MODE_MOVE); break;
    case IEditorCoreEngine::MODE_MoveSurface: gizmoClient.setEditMode(CM_OBJED_MODE_SURF_MOVE); break;
    case IEditorCoreEngine::MODE_Rotate: gizmoClient.setEditMode(CM_OBJED_MODE_ROTATE); break;
    case IEditorCoreEngine::MODE_Scale: gizmoClient.setEditMode(CM_OBJED_MODE_SCALE); break;
  }
  toolbar.setGizmoClientType(mode);
  updateGizmo();
}

void CompositeEditor::updateGizmo()
{
  if (get_app().isGizmoOperationStarted())
    return;

  gizmoClient.refreshAffectedNodes(selectedTreeDataNodes);
  const bool canTransform = gizmoClient.hasAnyTransformableNode();
  const IEditorCoreEngine::ModeType oldMode = IEditorCoreEngine::get()->getGizmoModeType();
  const IEditorCoreEngine::ModeType newMode = canTransform ? lastSelectedGizmoMode : IEditorCoreEngine::ModeType::MODE_None;

  if (newMode != oldMode)
    IEditorCoreEngine::get()->setGizmo(&gizmoClient, newMode);

  toolbar.updateGizmoToolbarButtons(canTransform);
}

void CompositeEditor::updateToolbarVisibility()
{
  if (treeData.isComposite && get_app().isCompositeEditorShown())
    toolbar.initUi(*this, &gizmoClient, GUI_PLUGIN_TOOLBAR_ID);
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

  undoSystem->begin(true);
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

void CompositeEditor::saveForUndo(DataBlock &dataBlock, unsigned &primary_data_block_id, dag::Vector<unsigned> &data_block_ids) const
{
  treeData.convertTreeDataToDataBlock(treeData.rootNode, dataBlock);

  data_block_ids.clear();
  primary_data_block_id = IDataBlockIdHolder::invalid_id;

  PropPanel::TreeBaseWindow *tree = compositeTreeView.get();
  if (!tree)
    return;

  dag::Vector<PropPanel::TLeafHandle> items;
  tree->getSelectedItems(items);
  for (PropPanel::TLeafHandle item : items)
  {
    const CompositeEditorTreeDataNode *node = static_cast<CompositeEditorTreeDataNode *>(tree->getItemData(item));
    if (!node)
      continue;

    // The root has no data block ID, so the undo selection stores it as ROOT_NODE_UNDO_ID.
    const unsigned dataBlockId = node == &treeData.rootNode ? ROOT_NODE_UNDO_ID : node->dataBlockId;
    if (dataBlockId == IDataBlockIdHolder::invalid_id)
      continue;

    data_block_ids.push_back(dataBlockId);
    if (node == selectedTreeDataNode)
      primary_data_block_id = dataBlockId;
  }
}

CompositeEditorTreeDataNode *CompositeEditor::getTreeDataNodeForUndo(unsigned data_block_id)
{
  if (data_block_id == ROOT_NODE_UNDO_ID)
    return &treeData.rootNode;
  if (data_block_id == IDataBlockIdHolder::invalid_id)
    return nullptr;
  return CompositeEditorTreeData::getTreeDataNodeByDataBlockId(treeData.rootNode, data_block_id);
}

void CompositeEditor::loadFromUndo(const DataBlock &dataBlock, unsigned primary_data_block_id,
  const dag::Vector<unsigned> &data_block_ids)
{
  G_ASSERT(editedAsset);
  treeData.convertAssetToTreeData(editedAsset, &dataBlock);

  selectedTreeDataNode = getTreeDataNodeForUndo(primary_data_block_id);

  selectedTreeDataNodes.clear();
  for (unsigned dataBlockId : data_block_ids)
    if (CompositeEditorTreeDataNode *node = getTreeDataNodeForUndo(dataBlockId))
      selectedTreeDataNodes.push_back(node);

  // The saved primary can have no data block ID. Use the first restored node then.
  if (!selectedTreeDataNode && !selectedTreeDataNodes.empty())
    selectedTreeDataNode = selectedTreeDataNodes.front();

  updateAssetFromTree(CompositeEditorRefreshType::EntityAndCompositeEditor);
}
