// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "treeStandalone.h"

namespace PropPanel
{

template <>
inline const TreeControlStandalone::TreeNode *TreeHierarchyLineDrawer<TreeControlStandalone::TreeNode>::nodeGetParent(
  const TreeControlStandalone::TreeNode &node)
{
  // If a node has no parent then it must be the root node.
  const TreeControlStandalone::TreeNode *parent = node.parent;
  const bool isRoot = parent->parent == nullptr;
  return isRoot ? nullptr : parent;
}

template <>
inline bool TreeHierarchyLineDrawer<TreeControlStandalone::TreeNode>::nodeHasChildren(const TreeControlStandalone::TreeNode &node)
{
  return node.children.size() > 0;
}

template <>
inline bool TreeHierarchyLineDrawer<TreeControlStandalone::TreeNode>::nodeHasPreviousSibling(
  const TreeControlStandalone::TreeNode &node)
{
  return node.parent->children[0] != &node;
}

template <>
inline bool TreeHierarchyLineDrawer<TreeControlStandalone::TreeNode>::nodeHasNextSibling(const TreeControlStandalone::TreeNode &node)
{
  const TreeControlStandalone::TreeNode *parent = node.parent;
  return parent->children[parent->children.size() - 1] != &node;
}

void TreeControlStandalone::fillTree(TreeNode &parent_node, bool &double_clicked_on_item, bool &right_clicked_on_item,
  bool &clicked_on_state_icon, TreeNode *&collapsed_item, bool focus_selected, float &cursor_screen_pos_max_x)
{
  ImDrawList *drawList = ImGui::GetWindowDrawList();
  ImGuiWindow *window = ImGui::GetCurrentWindow();

  for (TreeNode *node : parent_node.children)
  {
    // SetNextItemOpen does nothing if SkipItems is set, so to avoid unintended openness change we do nothing too.
    if (window->SkipItems)
      break;

    const bool hasChild = node->children.size() != 0;
    const bool wasSelected = multiSelectionEnabled ? node->multiSelected : node == lastSelected;
    ImGuiTreeNodeFlags flags =
      ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;

    if (!hasChild)
      flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;

    if (wasSelected)
    {
      flags |= ImGuiTreeNodeFlags_Selected;

      if (focus_selected)
        ImGui::SetKeyboardFocusHere();
    }

    ImGui::SetNextItemOpen(node->expanded);

    G_STATIC_ASSERT(sizeof(ImGuiSelectionUserData) == sizeof(TreeNode *));
    ImGui::SetNextItemSelectionUserData((ImGuiSelectionUserData)node);

    bool clickedOnThisStateIcon = false;

    ImguiHelper::TreeNodeWithSpecialHoverBehaviorEndData endData;
    const bool isOpen = ImguiHelper::treeNodeWithSpecialHoverBehaviorStart(node->title, flags, endData,
      /*allow_blocked_hover = */ false, /*show_arrow = */ false);

    if (endData.draw)
    {
      const ImVec2 fontSizedIconSize = PropPanel::ImguiHelper::ImguiHelper::getFontSizedIconSize();
      const float iconWidthWithSpacing = fontSizedIconSize.x + ImGui::GetStyle().ItemInnerSpacing.x;
      const float iconOrTextStartX = endData.textPos.x;
      ImVec2 iconPos = endData.textPos;

      IconId stateIcon = node->stateIcon;
      if (hasCheckboxes)
      {
        if (node->checkboxState == CheckboxState::Checked)
          stateIcon = checkedIcon;
        else if (node->checkboxState == CheckboxState::Unchecked)
          stateIcon = uncheckedIcon;
      }

      if (stateIcon != IconId::Invalid)
        endData.textPos.x += iconWidthWithSpacing;
      if (node->icon != IconId::Invalid)
        endData.textPos.x += iconWidthWithSpacing;

      const bool hasColor = node->textColor != E3DCOLOR(0);
      if (hasColor)
        ImGui::PushStyleColor(ImGuiCol_Text, node->textColor);

      PropPanel::ImguiHelper::treeNodeWithSpecialHoverBehaviorEnd(endData);

      if (hasColor)
        ImGui::PopStyleColor();

      if (stateIcon != IconId::Invalid)
      {
        const ImVec2 iconRectRightBottom(iconPos.x + fontSizedIconSize.x, iconPos.y + fontSizedIconSize.y);
        window->DrawList->AddImage(image_helper.getImTextureIdFromIconId(stateIcon), iconPos, iconRectRightBottom);

        clickedOnThisStateIcon = endData.hovered && ImGui::IsItemClicked(ImGuiMouseButton_Left) &&
                                 ImRect(iconPos, iconRectRightBottom).Contains(ImGui::GetMousePos());
        iconPos.x += iconWidthWithSpacing;
      }

      if (node->icon != IconId::Invalid)
      {
        const ImVec2 iconRectRightBottom(iconPos.x + fontSizedIconSize.x, iconPos.y + fontSizedIconSize.y);
        window->DrawList->AddImage(image_helper.getImTextureIdFromIconId(node->icon), iconPos, iconRectRightBottom);
      }

      const ImRect itemRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
      TreeHierarchyLineDrawer<TreeNode>::draw(*node, *drawList, itemRect, iconOrTextStartX, isOpen,
        getOverriddenColorU32(ColorOverride::TREE_HIERARCHY_LINE), getOverriddenColorU32(ColorOverride::TREE_OPEN_CLOSE_ICON_INNER));

      const float currentMaxX = endData.textPos.x + endData.labelSize.x;
      if (currentMaxX > cursor_screen_pos_max_x)
        cursor_screen_pos_max_x = currentMaxX;
    }

    if (node->expanded != isOpen)
    {
      node->expanded = isOpen;
      resetMaximumSeenWidth();

      if (!isOpen)
      {
        G_ASSERT(!collapsed_item);
        collapsed_item = node;
      }
    }

    if (ImGui::IsItemHovered())
    {
      if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
        double_clicked_on_item = true;
      // Imitate Windows where the context menu comes up on mouse button release. (BeginPopupContextItem also does this.)
      else if (ImGui::IsMouseReleased(ImGuiMouseButton_Right))
        right_clicked_on_item = true;
    }

    if (clickedOnThisStateIcon && hasCheckboxes && node->checkboxState != CheckboxState::NoCheckbox)
    {
      PropPanel::send_immediate_focus_loss_notification();
      node->checkboxState = node->checkboxState == CheckboxState::Checked ? CheckboxState::Unchecked : CheckboxState::Checked;
      clicked_on_state_icon = true;
    }

    if (node == ensureVisibleRequested)
    {
      ensureVisibleRequested = nullptr;
      ImGui::SetScrollHereY();
    }

    if (isOpen && hasChild)
    {
      fillTree(*node, double_clicked_on_item, right_clicked_on_item, clicked_on_state_icon, collapsed_item, focus_selected,
        cursor_screen_pos_max_x);
      ImGui::TreePop();
    }
  }
}

} // namespace PropPanel