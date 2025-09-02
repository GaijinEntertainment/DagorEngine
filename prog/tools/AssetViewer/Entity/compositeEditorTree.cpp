// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "compositeEditorTree.h"
#include "../av_appwnd.h"
#include "compositeEditorTreeData.h"
#include <winGuiWrapper/wgw_input.h>

CompositeEditorTree::CompositeEditorTree(PropPanel::ITreeViewEventHandler *event_handler, void *phandle, int x, int y, unsigned w,
  unsigned h, const char caption[]) :
  TreeBaseWindow(event_handler, phandle, x, y, hdpi::_pxActual(w), hdpi::_pxActual(h), caption, true, true)
{
  animCharImageId = addImage("asset_animChar");
  compositImageId = addImage("asset_composit");
  dynModelImageId = addImage("asset_dynModel");
  efxImageId = addImage("asset_efx");
  folderImageId = addImage("folder");
  fxImageId = addImage("asset_effects");
  gameObjImageId = addImage("asset_gameObj");
  prefabImageId = addImage("asset_prefab");
  rendinstImageId = addImage("res_rendInst");
}

PropPanel::IconId CompositeEditorTree::getImageId(CompositeEditorTreeDataNode &treeDataNode) const
{
  if (treeDataNode.hasEntBlock())
    return gameObjImageId;

  const char *type = treeDataNode.getAssetTypeName();
  if (type == nullptr)
    return gameObjImageId;

  if (strcmp(type, "rendInst") == 0)
    return rendinstImageId;
  if (strcmp(type, "animChar") == 0)
    return animCharImageId;
  if (strcmp(type, "composit") == 0)
    return compositImageId;
  if (strcmp(type, "dynModel") == 0)
    return dynModelImageId;
  if (strcmp(type, "efx") == 0)
    return efxImageId;
  if (strcmp(type, "fx") == 0)
    return fxImageId;
  if (strcmp(type, "gameObj") == 0)
    return gameObjImageId;
  if (strcmp(type, "prefab") == 0)
    return prefabImageId;

  return gameObjImageId;
}

void CompositeEditorTree::fillInternal(CompositeEditorTreeDataNode &treeDataNode, PropPanel::TLeafHandle parent,
  CompositeEditorTreeDataNode *treeDataNodeToSelect, eastl::hash_map<const void *, bool> &closedNodes)
{
  for (int nodeIndex = 0; nodeIndex < treeDataNode.nodes.size(); ++nodeIndex)
  {
    CompositeEditorTreeDataNode &treeDataSubNode = *treeDataNode.nodes[nodeIndex];

    const char *name = treeDataSubNode.getName();
    const PropPanel::IconId imageId = getImageId(treeDataSubNode);
    PropPanel::TLeafHandle subBlockTreeNode = addItem(name, imageId, parent, &treeDataSubNode);

    if (treeDataSubNode.isEntBlock())
      changeItemStateImage(subBlockTreeNode, gameObjImageId);

    fillInternal(treeDataSubNode, subBlockTreeNode, treeDataNodeToSelect, closedNodes);

    if (closedNodes.find(&treeDataSubNode) == closedNodes.end())
      expand(subBlockTreeNode);

    if (treeDataNodeToSelect == &treeDataSubNode)
      setSelectedItem(subBlockTreeNode);
  }
}

void CompositeEditorTree::fill(CompositeEditorTreeData &treeData, CompositeEditorTreeDataNode *treeDataNodeToSelect,
  bool keepNodeExpansionState)
{
  eastl::hash_map<const void *, bool> closedNodes;
  if (keepNodeExpansionState)
    getClosedNodes(closedNodes);

  clear();

  if (!treeData.isComposite)
    return;

  PropPanel::TLeafHandle itemRootNode = addItem(treeData.assetName, compositImageId, nullptr, &treeData.rootNode);
  fillInternal(treeData.rootNode, itemRootNode, treeDataNodeToSelect, closedNodes);

  if (closedNodes.find(&treeData.rootNode) == closedNodes.end())
    expand(itemRootNode);

  if (treeDataNodeToSelect == &treeData.rootNode)
    setSelectedItem(itemRootNode);

  PropPanel::TLeafHandle selectedNode = getSelectedItem();
  if (selectedNode)
    ensureVisible(selectedNode);
}

void CompositeEditorTree::selectByTreeDataNode(const CompositeEditorTreeDataNode *treeDataNodeToSelect)
{
  if (treeDataNodeToSelect != nullptr)
  {
    PropPanel::TLeafHandle rootItem = getRoot();
    PropPanel::TLeafHandle item = rootItem;
    while (true)
    {
      if (getItemData(item) == treeDataNodeToSelect)
      {
        setSelectedItem(item);
        ensureVisible(item);
        return;
      }

      item = getNextNode(item, true);
      if (!item || item == rootItem)
        break;
    }
  }

  setSelectedItem(nullptr);
}

void CompositeEditorTree::getClosedNodes(eastl::hash_map<const void *, bool> &closedNodes)
{
  PropPanel::TLeafHandle rootItem = getRoot();
  PropPanel::TLeafHandle item = rootItem;
  while (true)
  {
    const bool itemClosed = !isOpen(item);
    if (itemClosed)
    {
      const void *itemData = getItemData(item);
      closedNodes.insert(eastl::pair<const void *, bool>(itemData, itemClosed));
    }

    item = getNextNode(item, true);
    if (!item || item == rootItem)
      break;
  }
}

long CompositeEditorTree::onWcKeyDown(WindowBase *source, unsigned v_key)
{
  if (v_key == wingw::V_DELETE)
  {
    get_app().getCompositeEditor().deleteSelectedNode();
    setFocus();
  }

  return 0;
}
