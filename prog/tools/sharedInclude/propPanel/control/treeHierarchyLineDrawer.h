//
// Dagor Tech 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <libTools/util/hdpiUtil.h>
#include <imgui/imgui_internal.h>

namespace PropPanel
{

template <typename TreeNodeType>
class TreeHierarchyLineDrawer
{
public:
  // Draw Windows-style hierarchy lines and open/close icons for tree items.
  // It can be used with ImguiHelper::treeNodeWithSpecialHoverBehaviorStart.
  //
  // See the functions at the bottom of this class that need to be specialized for TreeNodeType.
  static void draw(const TreeNodeType &node, ImDrawList &draw_list, const ImRect &item_rect, float icon_or_text_start_x, bool open,
    ImU32 tree_line_color, ImU32 open_close_icon_inner_color)
  {
    const bool hasPreviousSibling = nodeHasPreviousSibling(node);
    const bool hasNextSibling = nodeHasNextSibling(node);
    const float treeNodeToLabelSpacing = ImGui::GetTreeNodeToLabelSpacing();
    const float indentSpacing = ImGui::GetStyle().IndentSpacing;
    const float centerX = floorf(item_rect.Min.x + (ImGui::GetStyle().FramePadding.x * 3.0f));
    const float centerY = floorf((item_rect.Min.y + item_rect.Max.y) / 2.0f);
    const float rectangleRadius = floorf(draw_list._Data->FontSize * 0.30f);
    const float itemSpacingY = ImGui::GetStyle().ItemSpacing.y;
    const float itemTopMinusSpacing = item_rect.Min.y - itemSpacingY;
    const float itemBottomPlusSpacing = item_rect.Max.y + itemSpacingY;
    const float horizontalLineRight = icon_or_text_start_x - hdpi::_pxS(3);

    if (nodeHasChildren(node))
    {
      if (hasPreviousSibling || nodeGetParent(node))
      {
        const float verticalLineTop = hasPreviousSibling ? item_rect.Min.y : itemTopMinusSpacing;
        draw_list.AddLine(ImVec2(centerX, verticalLineTop), ImVec2(centerX, centerY - rectangleRadius), tree_line_color);
      }

      drawTreeOpenCloseIcon(draw_list, ImVec2(centerX, centerY), rectangleRadius, open, tree_line_color, open_close_icon_inner_color);

      draw_list.AddLine(ImVec2(centerX + rectangleRadius, centerY), ImVec2(horizontalLineRight, centerY), tree_line_color);

      if (hasNextSibling)
        draw_list.AddLine(ImVec2(centerX, centerY + rectangleRadius), ImVec2(centerX, itemBottomPlusSpacing), tree_line_color);
    }
    else
    {
      draw_list.AddLine(ImVec2(centerX, centerY), ImVec2(horizontalLineRight, centerY), tree_line_color);

      const float verticalLineTop = hasPreviousSibling ? item_rect.Min.y : itemTopMinusSpacing;
      const float verticalLineBottom = hasNextSibling ? itemBottomPlusSpacing : centerY;
      draw_list.AddLine(ImVec2(centerX, verticalLineTop), ImVec2(centerX, verticalLineBottom), tree_line_color);
    }

    float currentTreeLineCenterX = centerX;
    for (const TreeNodeType *currentNode = nodeGetParent(node); currentNode; currentNode = nodeGetParent(*currentNode))
    {
      currentTreeLineCenterX -= indentSpacing; // Do it before the first draw because we started at the parent.

      if (nodeHasNextSibling(*currentNode))
      {
        const float verticalLineBottom = itemBottomPlusSpacing;
        draw_list.AddLine(ImVec2(currentTreeLineCenterX, item_rect.Min.y), ImVec2(currentTreeLineCenterX, verticalLineBottom),
          tree_line_color);
      }
    }
  }

private:
  static void drawTreeOpenCloseIcon(ImDrawList &draw_list, const ImVec2 &center, float rectangle_radius, bool open,
    ImU32 tree_line_color, ImU32 open_close_icon_inner_color)
  {
    const float r = rectangle_radius;
    draw_list.AddRect(ImVec2(center.x - r, center.y - r), ImVec2(center.x + r + 1.0f, center.y + r + 1.0f), tree_line_color);

    const float r2 = r - (r > 4.0f ? hdpi::_pxS(2) : 1); // hdpi::_pxS(1) did not work well here
    draw_list.AddLine(ImVec2(center.x - r2 + 1.0f, center.y), ImVec2(center.x + r2, center.y), open_close_icon_inner_color);
    if (!open)
      draw_list.AddLine(ImVec2(center.x, center.y - r2 + 1.0f), ImVec2(center.x, center.y + r2), open_close_icon_inner_color);
  }

  // These need to be specialized.
  static inline const TreeNodeType *nodeGetParent(const TreeNodeType &node);
  static inline bool nodeHasChildren(const TreeNodeType &node);
  static inline bool nodeHasPreviousSibling(const TreeNodeType &node);
  static inline bool nodeHasNextSibling(const TreeNodeType &node);
};

} // namespace PropPanel