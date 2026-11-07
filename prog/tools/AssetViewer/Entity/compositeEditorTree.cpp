// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "compositeEditorTree.h"
#include "../av_appwnd.h"
#include "compositeEditorTreeData.h"
#include <winGuiWrapper/wgw_input.h>
#include <assets/asset.h>
#include <propPanel/propPanel.h>
#include <gui/dag_imgui.h>

static const E3DCOLOR MISSING_ROW_COLOR(222, 76, 71, 77);
static const E3DCOLOR CONTAINS_MISSING_ROW_COLOR(217, 122, 7, 77);

CompositeEditorTree::CompositeEditorTree(PropPanel::ITreeViewEventHandler *event_handler, void *phandle, int x, int y, unsigned w,
  unsigned h, const char caption[]) :
  TreeBaseWindow(event_handler, phandle, x, y, hdpi::_pxActual(w), hdpi::_pxActual(h), caption, true, true, true)
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
  alertImageId = addImage("alert");
  setTreeRenderEx(this);
}

PropPanel::IconId CompositeEditorTree::getImageId(CompositeEditorTreeDataNode &treeDataNode) const
{
  const auto it = nodeStates.find(&treeDataNode);
  if (it != nodeStates.end() && it->second == NodeState::Missing)
    return alertImageId;

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

bool CompositeEditorTree::collectMissingStates(const CompositeEditorTreeDataNode &treeDataNode)
{
  // A disabled subtree loads no assets, so a missing asset in it is not an error.
  if (treeDataNode.isDisabled())
    return false;

  bool containsMissing = false;
  for (const eastl::unique_ptr<CompositeEditorTreeDataNode> &subNode : treeDataNode.nodes)
    containsMissing |= collectMissingStates(*subNode);

  if (treeDataNode.isAssetMissing())
  {
    nodeStates[&treeDataNode] = NodeState::Missing;
    return true;
  }

  if (containsMissing)
    nodeStates[&treeDataNode] = NodeState::ContainsMissing;

  return containsMissing;
}

void CompositeEditorTree::fillInternal(CompositeEditorTreeDataNode &treeDataNode, PropPanel::TLeafHandle parent,
  CompositeEditorTreeDataNode *treeDataNodeToSelect, const dag::Vector<CompositeEditorTreeDataNode *> &dataBlocksToMultiSelect,
  eastl::hash_map<const void *, bool> &closedNodes, bool parentDisabled)
{
  for (int nodeIndex = 0; nodeIndex < treeDataNode.nodes.size(); ++nodeIndex)
  {
    CompositeEditorTreeDataNode &treeDataSubNode = *treeDataNode.nodes[nodeIndex];

    const bool nodeIsDisabled = parentDisabled || treeDataSubNode.isDisabled();
    const char *name = treeDataSubNode.getName();
    const PropPanel::IconId imageId = nodeIsDisabled ? PropPanel::IconId::Invalid : getImageId(treeDataSubNode);
    PropPanel::TLeafHandle subBlockTreeNode = addItem(name, imageId, parent, &treeDataSubNode);

    if (treeDataSubNode.isEntBlock() && !nodeIsDisabled)
      changeItemStateImage(subBlockTreeNode, gameObjImageId);

    if (nodeIsDisabled)
    {
      setColor(subBlockTreeNode, E3DCOLOR(128, 128, 128, 255));
      nodeStates[&treeDataSubNode] = parentDisabled ? NodeState::ParentDisabled : NodeState::Disabled;
    }

    fillInternal(treeDataSubNode, subBlockTreeNode, treeDataNodeToSelect, dataBlocksToMultiSelect, closedNodes, nodeIsDisabled);

    if (closedNodes.find(&treeDataSubNode) == closedNodes.end())
      expand(subBlockTreeNode);

    if (treeDataNodeToSelect == &treeDataSubNode)
      setSelectedItem(subBlockTreeNode, dataBlocksToMultiSelect.size() > 1);
    else if (
      eastl::find(dataBlocksToMultiSelect.begin(), dataBlocksToMultiSelect.end(), &treeDataSubNode) != dataBlocksToMultiSelect.end())
      setSelectedItem(subBlockTreeNode, true);
  }
}

void CompositeEditorTree::fill(CompositeEditorTreeData &treeData, CompositeEditorTreeDataNode *treeDataNodeToSelect,
  const dag::Vector<CompositeEditorTreeDataNode *> &dataBlocksToMultiSelect, bool keepNodeExpansionState)
{
  eastl::hash_map<const void *, bool> closedNodes;
  if (keepNodeExpansionState)
    getClosedNodes(closedNodes);

  clear();
  nodeStates.clear();

  if (!treeData.isComposite)
    return;

  collectMissingStates(treeData.rootNode);

  PropPanel::TLeafHandle itemRootNode = addItem(treeData.assetName, compositImageId, nullptr, &treeData.rootNode);
  fillInternal(treeData.rootNode, itemRootNode, treeDataNodeToSelect, dataBlocksToMultiSelect, closedNodes);

  if (closedNodes.find(&treeData.rootNode) == closedNodes.end())
    expand(itemRootNode);

  if (treeDataNodeToSelect == &treeData.rootNode)
    setSelectedItem(itemRootNode, dataBlocksToMultiSelect.size() > 1);
  else if (
    eastl::find(dataBlocksToMultiSelect.begin(), dataBlocksToMultiSelect.end(), &treeData.rootNode) != dataBlocksToMultiSelect.end())
    setSelectedItem(itemRootNode, true);

  PropPanel::TLeafHandle selectedNode = getSelectedItem();
  if (selectedNode)
    ensureVisible(selectedNode);
}

void CompositeEditorTree::selectByTreeDataNode(const CompositeEditorTreeDataNode *treeDataNodeToSelect, bool multiSelect)
{
  if (treeDataNodeToSelect != nullptr)
  {
    PropPanel::TLeafHandle rootItem = getRoot();
    PropPanel::TLeafHandle item = rootItem;
    while (true)
    {
      if (getItemData(item) == treeDataNodeToSelect)
      {
        setSelectedItem(item, multiSelect);
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

void CompositeEditorTree::deselectByTreeDataNode(const CompositeEditorTreeDataNode *treeDataNodeToDeselect)
{
  if (treeDataNodeToDeselect != nullptr)
  {
    PropPanel::TLeafHandle rootItem = getRoot();
    PropPanel::TLeafHandle item = rootItem;
    while (true)
    {
      if (getItemData(item) == treeDataNodeToDeselect)
      {
        deselectItem(item);
        return;
      }

      item = getNextNode(item, true);
      if (!item || item == rootItem)
        break;
    }
  }
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
    get_app().getCompositeEditor().deleteSelectedNodes();
    setFocus();
  }

  return 0;
}

void CompositeEditorTree::updateImgui(float control_height)
{
  CompositeEditor &compositeEditor = get_app().getCompositeEditor();
  if (compositeEditor.isEditingSubComposite())
  {
    const ImVec2 fontSizedIconSize = PropPanel::ImguiHelper::getFontSizedIconSize();
    const ImVec2 padding = ImGui::GetStyle().FramePadding;

    const ImVec2 lineTopLeft = ImGui::GetCursorScreenPos();
    ImVec2 pos = lineTopLeft;
    ImGui::SetCursorScreenPos(ImVec2(pos.x, pos.y + padding.y));
    ImGui::Image(get_im_texture_id_from_icon_id(alertImageId), fontSizedIconSize);

    ImGui::SameLine();

    pos = ImGui::GetCursorScreenPos();
    ImGui::SetCursorScreenPos(ImVec2(pos.x, pos.y - padding.y));
    PropPanel::ImguiHelper::labelOnly("Editing sub-composite of", nullptr, true);

    ImGui::SameLine();

    ImGui::PushFont(imgui_get_bold_font(), 0.0f);
    dag::ConstSpan<CompositeEditorSubContext> stack = compositeEditor.getSubCompositeStack();
    const DagorAsset *parentAsset = compositeEditor.getParentCompositeAsset();
    if (stack.size() > 1)
    {
      pos = ImGui::GetCursorScreenPos();
      ImGui::SetCursorScreenPos(ImVec2(pos.x, pos.y - padding.y));
      PropPanel::ImguiHelper::labelOnly(" ... > ", nullptr, true);
      ImGui::SameLine();
    }
    pos = ImGui::GetCursorScreenPos();
    ImGui::SetCursorScreenPos(ImVec2(pos.x, pos.y - padding.y));
    PropPanel::ImguiHelper::labelOnly(parentAsset != nullptr ? parentAsset->getName() : "", nullptr, true);
    const int stackSize = (int)stack.size();
    if (stackSize > 1 && ImGui::IsMouseHoveringRect(lineTopLeft, ImGui::GetItemRectMax()))
    {
      if (headerTooltipDepth != stackSize)
      {
        headerTooltip.clear();
        for (int i = 0; i < stackSize; ++i)
        {
          if (i > 0)
            headerTooltip += " >\n";
          if (stack[i].parentAsset)
            headerTooltip += stack[i].parentAsset->getName();
        }
        headerTooltipDepth = stackSize;
      }
      ImGui::SetTooltip("%s", headerTooltip.str());
    }
    ImGui::PopFont();
  }

  TreeBaseWindow::updateImgui(control_height);
}

eastl::optional<E3DCOLOR> CompositeEditorTree::getBorderColor() const
{
  if (!get_app().getCompositeEditor().isEditingSubComposite())
    return eastl::nullopt;
  return E3DCOLOR(255, 128, 0, 255);
}

void CompositeEditorTree::drawTreeNode(PropPanel::TreeNodeDrawContext &ctx)
{
  const auto it = nodeStates.find(getItemData(ctx.getHandle()));
  if (it == nodeStates.end())
  {
    ctx.drawDefault();
    return;
  }

  const char *tooltip = nullptr;
  switch (it->second)
  {
    case NodeState::Missing:
      ctx.setNormalBackgroundColor(MISSING_ROW_COLOR);
      tooltip = "Asset not found.";
      break;
    case NodeState::ContainsMissing:
      ctx.setNormalBackgroundColor(CONTAINS_MISSING_ROW_COLOR);
      tooltip = "Contains missing asset.";
      break;
    case NodeState::Disabled: tooltip = "Disabled."; break;
    case NodeState::ParentDisabled: tooltip = "An ancestor is disabled."; break;
  }
  ctx.drawDefault();

  if (!PropPanel::should_show_imgui_control_tooltip(ImGui::GetItemID()))
    return;

  if (!ImGui::BeginTooltipEx(ImGuiTooltipFlags_OverridePrevious, ImGuiWindowFlags_None))
    return;

  if (ctx.isLabelClipped())
  {
    ImGui::TextUnformatted(ctx.getLabel(), ctx.getLabelEnd());
    ImGui::NewLine();
  }

  ImGui::PushFont(imgui_get_bold_font(), 0.0f);
  ImGui::TextUnformatted(tooltip);
  ImGui::PopFont();

  ImGui::EndTooltip();
}
