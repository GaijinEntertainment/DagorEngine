// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "treeStandalone.h"

namespace PropPanel
{

namespace
{
ITreeRenderEx defaultTreeRender;
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

static float get_tree_icon_width_with_spacing()
{
  return PropPanel::ImguiHelper::getFontSizedIconSize().x + ImGui::GetStyle().ItemInnerSpacing.x;
}

TreeNodeDrawContext::TreeNodeDrawContext(ImguiHelper::TreeNodeWithSpecialHoverBehaviorEndData &end_data, TreeNode &tree_node,
  IconId state_icon_id) :
  endData(end_data),
  node(&tree_node),
  iconOrTextStartPos(end_data.textPos),
  labelClipMaxX(ImguiHelper::treeNodeWithSpecialHoverBehaviorGetLabelClipMaxX()),
  stateIcon(state_icon_id),
  icon(tree_node.getIconId()),
  textColor(tree_node.getTextColor())
{}

ImVec2 TreeNodeDrawContext::getIconPos() const
{
  ImVec2 pos = iconOrTextStartPos;
  if (stateIcon != IconId::Invalid)
    pos.x += get_tree_icon_width_with_spacing();
  return pos;
}

bool TreeNodeDrawContext::getStateIconRect(ImRect &rect) const
{
  if (stateIcon == IconId::Invalid)
    return false;

  const ImVec2 fontSizedIconSize = PropPanel::ImguiHelper::getFontSizedIconSize();
  rect = ImRect(iconOrTextStartPos, ImVec2(iconOrTextStartPos.x + fontSizedIconSize.x, iconOrTextStartPos.y + fontSizedIconSize.y));
  return true;
}

ImVec2 TreeNodeDrawContext::getLabelPos() const
{
  ImVec2 pos = getIconPos();
  if (icon != IconId::Invalid)
    pos.x += get_tree_icon_width_with_spacing();
  return pos;
}

void TreeNodeDrawContext::setLabel(const char *text, const char *text_end)
{
  if (!text)
    text = "";
  if (!text_end)
    text_end = text + strlen(text);

  endData.label = text;
  endData.labelEnd = text_end;
  endData.labelSize = ImGui::CalcTextSize(text, text_end, false);
}

void TreeNodeDrawContext::drawBackground()
{
  if (normalBackgroundColor && !endData.hovered && !endData.selected)
  {
    const E3DCOLOR color = *normalBackgroundColor;
    ImGui::GetWindowDrawList()->AddRectFilled(endData.itemBB.Min, endData.itemBB.Max, IM_COL32(color.r, color.g, color.b, color.a));
  }

  ImguiHelper::treeNodeWithSpecialHoverBehaviorRender(endData, false);
}

void TreeNodeDrawContext::drawHierarchyLines()
{
  TreeHierarchyLineDrawer<TreeNode>::draw(*node, *ImGui::GetWindowDrawList(), endData.rowBB, iconOrTextStartPos.x, endData.isOpen,
    getOverriddenColorU32(ColorOverride::TREE_HIERARCHY_LINE), getOverriddenColorU32(ColorOverride::TREE_OPEN_CLOSE_ICON_INNER));
}

void TreeNodeDrawContext::drawIcons()
{
  ImDrawList *drawList = ImGui::GetWindowDrawList();
  const ImVec2 fontSizedIconSize = PropPanel::ImguiHelper::getFontSizedIconSize();

  ImRect stateIconRect;
  if (getStateIconRect(stateIconRect))
    drawList->AddImage(image_helper.getImTextureIdFromIconId(stateIcon), stateIconRect.Min, stateIconRect.Max);

  if (icon != IconId::Invalid)
  {
    const ImVec2 pos = getIconPos();
    drawList->AddImage(image_helper.getImTextureIdFromIconId(icon), pos,
      ImVec2(pos.x + fontSizedIconSize.x, pos.y + fontSizedIconSize.y));
  }
}

void TreeNodeDrawContext::drawLabel(const float *clip_max_x)
{
  labelClipMaxX = clip_max_x ? *clip_max_x : ImguiHelper::treeNodeWithSpecialHoverBehaviorGetLabelClipMaxX();
  endData.textPos = getLabelPos();

  const bool hasColor = textColor != E3DCOLOR(0);
  if (hasColor)
    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(textColor.r, textColor.g, textColor.b, textColor.a));

  ImguiHelper::treeNodeWithSpecialHoverBehaviorRenderLabel(endData, &labelClipMaxX);

  if (hasColor)
    ImGui::PopStyleColor();
}

void TreeNodeDrawContext::drawDefault()
{
  drawBackground();
  drawHierarchyLines();
  drawIcons();
  drawLabel();
}

bool TreeNodeDrawContext::isLabelClipped() const { return endData.labelSize.x > labelClipMaxX - getLabelPos().x; }

void TreeControlStandalone::drawNode(TreeNode *node, bool &double_clicked_on_item, bool &right_clicked_on_item,
  bool &clicked_on_state_icon, TreeNode *&collapsed_item, bool focus_selected, float &cursor_screen_pos_max_x)
{
  G_ASSERT(node);
  G_ASSERT(treeRenderEx);

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

  const bool hasRowWidgets = treeRenderEx->hasRowWidgets();
  if (hasRowWidgets)
    flags |= ImGuiTreeNodeFlags_AllowOverlap;

  ImguiHelper::TreeNodeWithSpecialHoverBehaviorEndData endData;
  const ImGuiID nodeId = ImGui::GetCurrentWindow()->GetID(node);

  const bool isOpen =
    ImguiHelper::treeNodeWithSpecialHoverBehaviorStart(nodeId, flags, node->title.begin(), node->title.end(), endData, hasRowWidgets);

  // The arrow keys move from the nav cursor, so a selection set by code has to move it too.
  if (node == moveNavCursorRequested && node == lastSelected)
  {
    moveNavCursorRequested = nullptr;

    ImGuiWindow *window = ImGui::GetCurrentWindow();
    if (g.NavWindow == window)
    {
      ImGui::SetFocusID(nodeId, window);
    }
    else
    {
      // Used when the tree gets the focus again. Same as the end of ImGui::SetFocusID(), but that would take the focus.
      const ImGuiNavLayer navLayer = window->DC.NavLayerCurrent;
      window->NavLastIds[navLayer] = nodeId;
      if (g.LastItemData.ID == nodeId)
        window->NavRectRel[navLayer] = ImGui::WindowRectAbsToRel(window, g.LastItemData.NavRect);
    }

    // ImGui keeps the range source and its selection state as a pair, and Ctrl+Shift ranges copy that state.
    ImGuiMultiSelectState *storage = g.CurrentMultiSelect->Storage;
    storage->RangeSrcItem = (ImGuiSelectionUserData)node;
    storage->RangeSelected = 1;
  }

  bool isItemHovered = false;
  bool clickedOnThisStateIcon = false;

  if (endData.draw) // -V547
  {
    handleDragAndDrop(node, endData);

    IconId stateIcon = node->stateIcon;
    if (hasCheckboxes && node->getFlagValue(TreeNode::CHECKBOX_ENABLED))
      stateIcon = node->getFlagValue(TreeNode::CHECKBOX_VALUE) ? checkedIcon : uncheckedIcon;

    TreeNodeDrawContext ctx(endData, *node, stateIcon);

    // These use LastItemData, and drawTreeNode() can add widgets.
    ImguiHelper::treeNodeWithSpecialHoverBehaviorTreePush(endData);
    isItemHovered = ImGui::IsItemHovered();
    const bool clickedOnItem = endData.hovered && ImGui::IsItemClicked(ImGuiMouseButton_Left);
    const ImVec2 mousePos = ImGui::GetMousePos();

    // The next row's Y coordinate comes from the cursor, so make sure that no override moves it.
    const ImVec2 cursorPos = ImGui::GetCursorScreenPos();
    treeRenderEx->drawTreeNode(ctx);
    ImGui::SetCursorScreenPos(cursorPos);

    ImRect stateIconRect;
    if (clickedOnItem && ctx.getStateIconRect(stateIconRect))
      clickedOnThisStateIcon = stateIconRect.Contains(mousePos);

    const float currentMaxX = ctx.getLabelPos().x + endData.labelSize.x;
    if (currentMaxX > cursor_screen_pos_max_x)
      cursor_screen_pos_max_x = currentMaxX;
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

    // Scroll a partially visible row (for example the one that the user just clicked) only as much as needed, and
    // center only a row that is not visible.
    ImGuiWindow *window = ImGui::GetCurrentWindow();
    const ImRect &visibleRect = window->InnerClipRect;
    if (endData.draw // -V560 endData.draw is not always false
        && endData.rowBB.Max.y > visibleRect.Min.y && endData.rowBB.Min.y < visibleRect.Max.y)
    {
      // Use the visible X range to prevent horizontal scrolling.
      const ImRect rowRect(visibleRect.Min.x, endData.rowBB.Min.y, visibleRect.Max.x, endData.rowBB.Max.y);
      ImGui::ScrollToRectEx(window, rowRect, ImGuiScrollFlags_KeepVisibleEdgeY | ImGuiScrollFlags_NoScrollParent);
    }
    else
    {
      ImGui::SetScrollHereY();
    }
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
      const ImRect &itemRect = endData.itemBB;
      const ImRect &rowRect = endData.rowBB;

      if (ImGui::BeginDragDropTargetCustom(itemRect, nodeId))
      {
        auto mousePos = ImGui::GetMousePos();
        float betweenDropArea = rowRect.GetHeight() * 0.15f;
        const TLeafHandle leaf = nodeAsLeafHandle(node);

        const auto drawBetweenDropLine = [&itemRect](bool top) {
          ImDrawList *drawList = ImGui::GetWindowDrawList();
          const ImVec2 lineStart = top ? itemRect.GetTL() : itemRect.GetBL();
          const ImVec2 lineEnd = top ? itemRect.GetTR() : itemRect.GetBR();

          const ImVec2 oldCursorPos = ImGui::GetCursorScreenPos();
          ImGui::SetCursorScreenPos(lineStart);
          drawList->AddLine(lineStart, lineEnd, ImGui::GetColorU32(ImGuiCol_DragDropTarget), ImGui::GetStyle().ItemSpacing.y);
          ImGui::SetCursorScreenPos(oldCursorPos);
        };

        if (mousePos.y <= rowRect.Min.y + betweenDropArea)
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
        else if (mousePos.y >= rowRect.Max.y - betweenDropArea)
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