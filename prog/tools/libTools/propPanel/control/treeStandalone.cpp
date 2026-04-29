// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "treeStandalone.h"

namespace PropPanel
{

namespace
{
class DefaultTreeRender final : public ITreeRenderEx
{
public:
  bool treeNodeStart(const NodeStartData &data) override
  {
    return ImguiHelper::treeNodeWithSpecialHoverBehaviorStart(data.id, data.flags, data.label, data.labelEnd, data.endData,
      data.allowBlockedHover);
  }
  void treeNodeRender(PropPanel::ImguiHelper::TreeNodeWithSpecialHoverBehaviorEndData &end_data, bool show_arrow) override
  {
    PropPanel::ImguiHelper::treeNodeWithSpecialHoverBehaviorRender(end_data, show_arrow);
  }

  void treeNodeEnd(PropPanel::ImguiHelper::TreeNodeWithSpecialHoverBehaviorEndData &end_data) override
  {
    PropPanel::ImguiHelper::treeNodeWithSpecialHoverBehaviorEnd(end_data);
  }

  void treeItemRenderEx([[maybe_unused]] const ItemRenderData &data) override {}
};

DefaultTreeRender defaultTreeRender;
} // namespace

TreeControlStandalone::TreeControlStandalone(bool selection_change_events_by_code, bool has_checkboxes, bool multi_select) :
  selectionChangeEventsByCode(selection_change_events_by_code),
  hasCheckboxes(has_checkboxes),
  multiSelectionEnabled(multi_select),
  treeRenderEx(&defaultTreeRender)
{
  markLinearizedTreeNodesDirty();
  resetMaximumSeenWidth();
}

template <>
inline const TreeNode *TreeHierarchyLineDrawer<TreeNode>::nodeGetParent(const TreeNode &node)
{
  // If a node has no parent then it must be the root node.
  const TreeNode *parent = node.getParent();
  const bool isRoot = parent->getParent() == nullptr;
  return isRoot ? nullptr : parent;
}

template <>
inline bool TreeHierarchyLineDrawer<TreeNode>::nodeHasChildren(const TreeNode &node)
{
  if (!node.getFirstChildFiltered())
  {
    return false;
  }

  return true;
}

template <>
inline bool TreeHierarchyLineDrawer<TreeNode>::nodeHasPreviousSibling(const TreeNode &node)
{
  return node.getPrevSiblingFiltered();
}

template <>
inline bool TreeHierarchyLineDrawer<TreeNode>::nodeHasNextSibling(const TreeNode &node)
{
  return node.getNextSiblingFiltered();
}

void TreeControlStandalone::drawNode(TreeNode *node, bool &double_clicked_on_item, bool &right_clicked_on_item,
  bool &clicked_on_state_icon, TreeNode *&collapsed_item, bool focus_selected, float &cursor_screen_pos_max_x)
{
  G_ASSERT(node);
  G_ASSERT(treeRenderEx);

  ImDrawList *drawList = ImGui::GetWindowDrawList();
  ImGuiContext &g = *ImGui::GetCurrentContext();
  const bool hasChild = node->children.size() != 0;
  const bool wasSelected = multiSelectionEnabled ? node->getFlagValue(TreeNode::SELECTED) : node == lastSelected;
  ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;

  if (!hasChild)
    flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;

  if (wasSelected)
  {
    flags |= ImGuiTreeNodeFlags_Selected;

    if (focus_selected)
      ImGui::SetKeyboardFocusHere();
  }

  ImGui::SetNextItemOpen(node->getFlagValue(TreeNode::EXPANDED));

  G_STATIC_ASSERT(sizeof(ImGuiSelectionUserData) == sizeof(TreeNode *));
  ImGui::SetNextItemSelectionUserData((ImGuiSelectionUserData)node);

  const int nodeLevel = getNodeLevel(*node);
  const float totalIndentSpace = ImGui::GetStyle().IndentSpacing * nodeLevel;
  if (nodeLevel > 0)
    ImGui::Indent(totalIndentSpace);

  bool clickedOnThisStateIcon = false;

  ImguiHelper::TreeNodeWithSpecialHoverBehaviorEndData endData;
  const ImGuiID nodeId = ImGui::GetCurrentWindow()->GetID(node);

  const bool isOpen = treeRenderEx->treeNodeStart(ITreeRenderEx::NodeStartData{.id = nodeId,
    .flags = flags,
    .label = node->title.begin(),
    .labelEnd = node->title.end(),
    .endData = endData,
    .allowBlockedHover = false});

  bool isItemHovered = false;

  if (endData.draw) // -V547
  {
    handleDragAndDrop(node, endData);

    treeRenderEx->treeNodeRender(endData, false);

    const ImVec2 fontSizedIconSize = PropPanel::ImguiHelper::ImguiHelper::getFontSizedIconSize();
    const float iconWidthWithSpacing = fontSizedIconSize.x + ImGui::GetStyle().ItemInnerSpacing.x;
    const float iconOrTextStartX = endData.textPos.x;
    ImVec2 iconPos = endData.textPos;

    IconId stateIcon = node->stateIcon;
    if (hasCheckboxes && node->getFlagValue(TreeNode::CHECKBOX_ENABLED))
    {
      if (node->getFlagValue(TreeNode::CHECKBOX_VALUE))
        stateIcon = checkedIcon;
      else
        stateIcon = uncheckedIcon;
    }

    if (stateIcon != IconId::Invalid)
      endData.textPos.x += iconWidthWithSpacing;
    if (node->icon != IconId::Invalid)
      endData.textPos.x += iconWidthWithSpacing;

    const bool hasColor = node->textColor != E3DCOLOR(0);
    if (hasColor)
      ImGui::PushStyleColor(ImGuiCol_Text, node->textColor);

    treeRenderEx->treeNodeEnd(endData);

    if (hasColor)
      ImGui::PopStyleColor();

    if (stateIcon != IconId::Invalid)
    {
      const ImVec2 iconRectRightBottom(iconPos.x + fontSizedIconSize.x, iconPos.y + fontSizedIconSize.y);
      drawList->AddImage(image_helper.getImTextureIdFromIconId(stateIcon), iconPos, iconRectRightBottom);

      clickedOnThisStateIcon = endData.hovered && ImGui::IsItemClicked(ImGuiMouseButton_Left) &&
                               ImRect(iconPos, iconRectRightBottom).Contains(ImGui::GetMousePos());
      iconPos.x += iconWidthWithSpacing;
    }

    if (node->icon != IconId::Invalid)
    {
      const ImVec2 iconRectRightBottom(iconPos.x + fontSizedIconSize.x, iconPos.y + fontSizedIconSize.y);
      drawList->AddImage(image_helper.getImTextureIdFromIconId(node->icon), iconPos, iconRectRightBottom);
    }

    const ImRect itemRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
    TreeHierarchyLineDrawer<TreeNode>::draw(*node, *drawList, itemRect, iconOrTextStartX, isOpen,
      getOverriddenColorU32(ColorOverride::TREE_HIERARCHY_LINE), getOverriddenColorU32(ColorOverride::TREE_OPEN_CLOSE_ICON_INNER));

    const float currentMaxX = endData.textPos.x + endData.labelSize.x;
    if (currentMaxX > cursor_screen_pos_max_x)
      cursor_screen_pos_max_x = currentMaxX;

    isItemHovered = ImGui::IsItemHovered();
    treeRenderEx->treeItemRenderEx(
      ITreeRenderEx::ItemRenderData{.handle = nodeAsLeafHandle(node), .textPos = endData.textPos, .isHovered = endData.hovered});
  }
  else
  {
    isItemHovered = ImGui::IsItemHovered();
  }

  if (node->getFlagValue(TreeNode::EXPANDED) != isOpen)
  {
    setExpanded(nodeAsLeafHandle(node), isOpen);

    if (!isOpen)
    {
      G_ASSERT(!collapsed_item);
      collapsed_item = node;
    }
  }

  if (isItemHovered)
  {
    if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
      double_clicked_on_item = true;
    // Imitate Windows where the context menu comes up on mouse button release. (BeginPopupContextItem also does this.)
    else if (ImGui::IsMouseReleased(ImGuiMouseButton_Right))
      right_clicked_on_item = true;
  }

  if (clickedOnThisStateIcon && hasCheckboxes && node->getFlagValue(TreeNode::CHECKBOX_ENABLED))
  {
    PropPanel::send_immediate_focus_loss_notification();
    node->setFlagValue(TreeNode::CHECKBOX_VALUE, !node->getFlagValue(TreeNode::CHECKBOX_VALUE));
    clicked_on_state_icon = true;
  }

  if (node == ensureVisibleRequested)
  {
    ensureVisibleRequested = nullptr;
    ImGui::SetScrollHereY();
  }

  if (isOpen && hasChild)
    ImGui::TreePop();

  if (nodeLevel > 0)
    ImGui::Unindent(totalIndentSpace);
}

void TreeControlStandalone::setTreeRenderEx(ITreeRenderEx *interface) { treeRenderEx = interface ? interface : &defaultTreeRender; }

void TreeControlStandalone::handleDragAndDrop(TreeNode *node, ImguiHelper::TreeNodeWithSpecialHoverBehaviorEndData &endData)
{
  if (dragHandler != nullptr && ImGui::BeginDragDropSource(dragDropFlags))
  {
    TLeafHandle leaf = nodeAsLeafHandle(node);
    dragHandler->onBeginDrag(leaf);

    ImGui::EndDragDropSource();
  }

  if (dropHandler != nullptr)
  {
    ImGuiContext &g = *ImGui::GetCurrentContext();

    if (dropHandler->canDropBetween())
    {
      const ImGuiID nodeId = ImGui::GetCurrentWindow()->GetID(node);

      ImRect extendedRect = g.LastItemData.DisplayRect;
      const float verticalSpacing = ImGui::GetStyle().ItemSpacing.y / 2.f;
      extendedRect.Expand(ImVec2(0.f, verticalSpacing));

      if (ImGui::BeginDragDropTargetCustom(extendedRect, nodeId))
      {
        auto mousePos = ImGui::GetMousePos();
        float betweenDropArea = g.LastItemData.DisplayRect.GetHeight() * 0.15f;
        const TLeafHandle leaf = nodeAsLeafHandle(node);

        const auto drawBetweenDropLine = [&g, verticalSpacing](bool top) {
          ImDrawList *drawList = ImGui::GetWindowDrawList();
          ImVec2 lineStart;
          ImVec2 lineEnd;
          if (top)
          {
            lineStart = g.LastItemData.DisplayRect.GetTL();
            lineStart.y -= verticalSpacing;
            lineEnd = g.LastItemData.DisplayRect.GetTR();
            lineEnd.y -= verticalSpacing;
          }
          else
          {
            lineStart = g.LastItemData.DisplayRect.GetBL();
            lineStart.y += verticalSpacing;
            lineEnd = g.LastItemData.DisplayRect.GetBR();
            lineEnd.y += verticalSpacing;
          }

          const ImVec2 oldCursorPos = ImGui::GetCursorScreenPos();
          ImGui::SetCursorScreenPos(lineStart);
          drawList->AddLine(lineStart, lineEnd, ImGui::GetColorU32(ImGuiCol_DragDropTarget), ImGui::GetStyle().ItemSpacing.y);
          ImGui::SetCursorScreenPos(oldCursorPos);
        };

        if (mousePos.y <= extendedRect.Min.y + betweenDropArea + verticalSpacing)
        {
          const TLeafHandle parentLeaf = getParentLeaf(leaf);
          auto result = dropHandler->onDropTargetBetween(parentLeaf, getChildIndex(leaf));
          PropPanel::handleDragAndDropNotAllowed(result);
          endData.hovered = false;

          if (g.DragDropPayload.Preview)
          {
            drawBetweenDropLine(true);
          }
        }
        else if (mousePos.y >= extendedRect.Max.y - betweenDropArea - verticalSpacing)
        {
          const TLeafHandle parentLeaf = getParentLeaf(leaf);
          auto result = dropHandler->onDropTargetBetween(parentLeaf, getChildIndex(leaf) + 1);
          PropPanel::handleDragAndDropNotAllowed(result);
          endData.hovered = false;

          if (g.DragDropPayload.Preview)
          {
            drawBetweenDropLine(false);
          }
        }
        else
        {
          auto result = dropHandler->onDropTargetDirect(leaf);
          PropPanel::handleDragAndDropNotAllowed(result);

          // Draw Drag&Drop rect manually, because we disable it for drag&drop with drop in between
          if (g.DragDropPayload.Preview)
          {
            ImGui::RenderDragDropTargetRectForItem(g.DragDropTargetRect);
          }
        }
        ImGui::EndDragDropTarget();
      }
    }
    else if (ImGui::BeginDragDropTarget())
    {
      TLeafHandle leaf = nodeAsLeafHandle(node);
      auto result = dropHandler->onDropTargetDirect(leaf);
      PropPanel::handleDragAndDropNotAllowed(result);

      ImGui::EndDragDropTarget();
    }
  }
}

} // namespace PropPanel