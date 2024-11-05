// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <math/dag_Point2.h>
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

// This is a heavily modified version of ImGui::SplitterBehavior that handles two splitters that are placed like a crosshair.
inline bool crosshair_splitter_behavior(const ImRect &bbHorizontal, const ImRect &bbVertical, ImGuiID id, float *newLeftWidth,
  float *newRightWidth, float *newTopHeight, float *newBottomHeight, float min_size, float hover_extend, float hover_visibility_delay,
  ImU32 bg_col)
{
  static bool draggingSplitter = false;
  static ImVec2 previousMousePos = ImVec2(0.0f, 0.0f);

  ImGuiContext &g = *GImGui;
  ImGuiWindow *window = g.CurrentWindow;

  if (!ImGui::ItemAdd(bbHorizontal, id, NULL, ImGuiItemFlags_NoNav))
    return false;

  bool horizontalHovered, horizontalHeld;
  ImRect bbHorizontalInteract = bbHorizontal;
  bbHorizontalInteract.Expand(ImVec2(0.0f, hover_extend));
  ImGui::ButtonBehavior(bbHorizontalInteract, id, &horizontalHovered, &horizontalHeld,
    ImGuiButtonFlags_FlattenChildren | ImGuiButtonFlags_AllowOverlap);

  bool verticalHovered, verticalHeld;
  ImRect bbVerticalInteract = bbVertical;
  bbVerticalInteract.Expand(ImVec2(0.0f, hover_extend));
  ImGui::ButtonBehavior(bbVerticalInteract, id, &verticalHovered, &verticalHeld,
    ImGuiButtonFlags_FlattenChildren | ImGuiButtonFlags_AllowOverlap);

  bool hovered = horizontalHovered || verticalHovered;
  bool held = horizontalHeld || verticalHeld;

  if (hovered)
    g.LastItemData.StatusFlags |= ImGuiItemStatusFlags_HoveredRect; // for IsItemHovered(), because bb_interact is larger than bb

  if (held || (hovered && g.HoveredIdPreviousFrame == id && g.HoveredIdTimer >= hover_visibility_delay))
    ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);

  if (draggingSplitter != held)
  {
    draggingSplitter = held;
    previousMousePos = g.IO.MousePos;
  }

  ImRect bbHorizontalRender = bbHorizontal;
  ImRect bbVerticalRender = bbVertical;
  if (held)
  {
    { // Horizontal
      float mouse_delta = (g.IO.MousePos - previousMousePos)[ImGuiAxis_Y];

      // Minimum pane size
      float size_1_maximum_delta = ImMax(0.0f, *newTopHeight - min_size);
      float size_2_maximum_delta = ImMax(0.0f, *newBottomHeight - min_size);
      if (mouse_delta < -size_1_maximum_delta)
        mouse_delta = -size_1_maximum_delta;
      if (mouse_delta > size_2_maximum_delta)
        mouse_delta = size_2_maximum_delta;

      // Apply resize
      if (mouse_delta != 0.0f)
      {
        *newTopHeight = ImMax(*newTopHeight + mouse_delta, min_size);
        *newBottomHeight = ImMax(*newBottomHeight - mouse_delta, min_size);
        ImGui::MarkItemEdited(id);
      }
    }

    { // Vertical
      float mouse_delta = (g.IO.MousePos - previousMousePos)[ImGuiAxis_X];

      // Minimum pane size
      float size_1_maximum_delta = ImMax(0.0f, *newLeftWidth - min_size);
      float size_2_maximum_delta = ImMax(0.0f, *newRightWidth - min_size);
      if (mouse_delta < -size_1_maximum_delta)
        mouse_delta = -size_1_maximum_delta;
      if (mouse_delta > size_2_maximum_delta)
        mouse_delta = size_2_maximum_delta;

      // Apply resize
      if (mouse_delta != 0.0f)
      {
        *newLeftWidth = ImMax(*newLeftWidth + mouse_delta, min_size);
        *newRightWidth = ImMax(*newRightWidth - mouse_delta, min_size);
        ImGui::MarkItemEdited(id);
      }
    }

    previousMousePos = g.IO.MousePos;
  }

  // Render at new position
  if (bg_col & IM_COL32_A_MASK)
  {
    window->DrawList->AddRectFilled(bbHorizontalRender.Min, bbHorizontalRender.Max, bg_col, 0.0f);
    window->DrawList->AddRectFilled(bbVerticalRender.Min, bbVerticalRender.Max, bg_col, 0.0f);
  }
  const ImU32 col = ImGui::GetColorU32(held                                                      ? ImGuiCol_SeparatorActive
                                       : (hovered && g.HoveredIdTimer >= hover_visibility_delay) ? ImGuiCol_SeparatorHovered
                                                                                                 : ImGuiCol_Separator);
  window->DrawList->AddRectFilled(bbHorizontalRender.Min, bbHorizontalRender.Max, col, 0.0f);
  window->DrawList->AddRectFilled(bbVerticalRender.Min, bbVerticalRender.Max, col, 0.0f);

  return held;
}

// viewport_split_ratio is both an input and output parameter.
inline void render_imgui_viewport_splitter(Point2 &viewport_split_ratio, const Point2 &region_available, float left_width,
  float right_width, float top_height, float bottom_height, float item_spacing)
{
  if (region_available.x <= 0.0f || region_available.y <= 0.0f)
    return;

  const ImVec2 origin = ImGui::GetCurrentWindow()->ContentRegionRect.Min;
  const float minimumSize = 100.0f;
  const ImU32 backgroundColor = ImGui::GetColorU32(ImGuiCol_WindowBg);

  // Use a short delay before highlighting the splitter (and changing the mouse cursor) in order for regular mouse movement to not
  // highlight many splitters
  const float WINDOWS_HOVER_PADDING = 4.0f; // Extend outside window for hovering/resizing (maxxed with TouchPadding) and inside
                                            // windows for borders. Affect FindHoveredWindow().
  const float WINDOWS_RESIZE_FROM_EDGES_FEEDBACK_TIMER = 0.04f; // Reduce visual noise by only highlighting the border after a certain
                                                                // time.

  const ImVec2 horizontalStart(origin.x, origin.y + top_height);
  const ImRect horizontalRect(horizontalStart, horizontalStart + ImVec2(region_available.x, item_spacing));
  const ImVec2 verticalStart(origin.x + left_width, origin.y);
  const ImRect verticalRect(verticalStart, verticalStart + ImVec2(item_spacing, region_available.y));
  float newLeftWidth = left_width;
  float newRightWidth = right_width;
  float newTopHeight = top_height;
  float newBottomHeight = bottom_height;

  if (
    crosshair_splitter_behavior(horizontalRect, verticalRect, ImGui::GetID("ViewportSplitter"), &newLeftWidth, &newRightWidth,
      &newTopHeight, &newBottomHeight, minimumSize, WINDOWS_HOVER_PADDING, WINDOWS_RESIZE_FROM_EDGES_FEEDBACK_TIMER, backgroundColor))
  {
    if (newLeftWidth != left_width)
      viewport_split_ratio.x = newLeftWidth / region_available.x;

    if (newTopHeight != top_height)
      viewport_split_ratio.y = newTopHeight / region_available.y;
  }
}
