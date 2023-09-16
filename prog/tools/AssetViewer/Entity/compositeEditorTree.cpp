#include "compositeEditorTree.h"
#include "../av_appwnd.h"
#include "compositeEditorTreeData.h"
#include <winGuiWrapper/wgw_input.h>

CompositeEditorTree::CompositeEditorTree(ITreeViewEventHandler *event_handler, void *phandle, int x, int y, unsigned w, unsigned h,
  const char caption[]) :
  TreeBaseWindow(event_handler, phandle, x, y, w, h, caption, /*icons_show = */ true, /*state_icons_show = */ true)
{
  animCharImageIndex = addIconImage("asset_animchar");
  compositImageIndex = addIconImage("asset_composit");
  dynModelImageIndex = addIconImage("asset_dynmodel");
  efxImageIndex = addIconImage("asset_efx");
  folderImageIndex = addIconImage("folder");
  fxImageIndex = addIconImage("asset_effects");
  gameObjImageIndex = addIconImage("asset_gameobj");
  prefabImageIndex = addIconImage("asset_prefab");
  rendinstImageIndex = addIconImage("res_rendinst");

  // The first state image is the no state image. It won't be drawn.
  const String gameObjStateImage(256, "%s%s%s", p2util::get_icon_path(), "asset_gameobj", ".bmp");
  addStateImage(gameObjStateImage);
  gameObjStateImageIndex = addStateImage(gameObjStateImage);
}

int CompositeEditorTree::addIconImage(const char *fileName)
{
  String bmp(256, "%s%s%s", p2util::get_icon_path(), fileName, ".bmp");
  return addImage(bmp);
}

int CompositeEditorTree::getImageIndex(CompositeEditorTreeDataNode &treeDataNode) const
{
  if (treeDataNode.hasEntBlock())
    return gameObjImageIndex;

  const char *type = treeDataNode.getAssetTypeName();
  if (type == nullptr)
    return gameObjImageIndex;

  if (strcmp(type, "rendInst") == 0)
    return rendinstImageIndex;
  if (strcmp(type, "animChar") == 0)
    return animCharImageIndex;
  if (strcmp(type, "composit") == 0)
    return compositImageIndex;
  if (strcmp(type, "dynModel") == 0)
    return dynModelImageIndex;
  if (strcmp(type, "efx") == 0)
    return efxImageIndex;
  if (strcmp(type, "fx") == 0)
    return fxImageIndex;
  if (strcmp(type, "gameObj") == 0)
    return gameObjImageIndex;
  if (strcmp(type, "prefab") == 0)
    return prefabImageIndex;

  return gameObjImageIndex;
}

void CompositeEditorTree::fillInternal(CompositeEditorTreeDataNode &treeDataNode, TLeafHandle parent,
  CompositeEditorTreeDataNode *treeDataNodeToSelect, eastl::hash_map<const void *, bool> &closedNodes)
{
  for (int nodeIndex = 0; nodeIndex < treeDataNode.nodes.size(); ++nodeIndex)
  {
    CompositeEditorTreeDataNode &treeDataSubNode = *treeDataNode.nodes[nodeIndex];

    const char *name = treeDataSubNode.getName();
    const int imageIndex = getImageIndex(treeDataSubNode);
    TLeafHandle subBlockTreeNode = addItem(name, imageIndex, parent, &treeDataSubNode);

    if (treeDataSubNode.isEntBlock())
      this->changeItemStateImage(subBlockTreeNode, gameObjStateImageIndex);

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

  TLeafHandle rootNode = addItem(treeData.assetName, compositImageIndex, nullptr, &treeData.rootNode);
  fillInternal(treeData.rootNode, rootNode, treeDataNodeToSelect, closedNodes);

  if (closedNodes.find(&treeData.rootNode) == closedNodes.end())
    expand(rootNode);

  if (treeDataNodeToSelect == &treeData.rootNode)
    setSelectedItem(rootNode);

  TLeafHandle selectedNode = getSelectedItem();
  if (selectedNode)
    ensureVisible(selectedNode);
}

void CompositeEditorTree::selectByTreeDataNode(const CompositeEditorTreeDataNode *treeDataNodeToSelect)
{
  if (treeDataNodeToSelect != nullptr)
  {
    TLeafHandle rootItem = getRoot();
    TLeafHandle item = rootItem;
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
  TLeafHandle rootItem = getRoot();
  TLeafHandle item = rootItem;
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
