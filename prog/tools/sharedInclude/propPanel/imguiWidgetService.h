//
// Dagor Tech 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <propPanel/imguiHelper.h>

namespace PropPanel
{

class IImguiWidgetService
{
public:
  virtual ~IImguiWidgetService() = default;

  // ImguiHelper::searchInput
  virtual bool searchInput(const void *focus_id, const char *label, const char *hint, String &text_to_search, ImTextureID search_icon,
    ImTextureID clear_icon, bool *input_focused = nullptr, ImGuiID *input_id = nullptr, ImRect *input_rect = nullptr,
    bool *deactivated_after_edit = nullptr) = 0;
  virtual bool searchInput(const void *focus_id, const char *label, const char *hint, String &text_to_search, IconId search_icon_id,
    IconId clear_icon_id, bool *input_focused = nullptr, ImGuiID *input_id = nullptr, ImRect *input_rect = nullptr,
    bool *deactivated_after_edit = nullptr) = 0;

  // ImguiHelper::imageButtonWithArrow
  virtual bool imageButtonWithArrow(const char *str_id, ImTextureID texture_id, const ImVec2 &image_size, bool checked = false,
    ImGuiButtonFlags flags = ImGuiButtonFlags_None) = 0;
  virtual bool imageButtonWithArrow(const char *str_id, IconId icon_id, const ImVec2 &image_size, bool checked = false,
    ImGuiButtonFlags flags = ImGuiButtonFlags_None) = 0;

  // ImguiHelper::imageButtonFrameless
  virtual bool imageButtonFrameless(const char *str_id, ImTextureID texture_id, const ImVec2 &image_size, const char *tooltip) = 0;

  // ImguiHelper::treeNodeWithSpecialHoverBehavior*, in the order of the calls.
  // treeNodeEnd draws the label, so a custom renderer inserts its own icons between render and end.
  virtual bool treeNodeStart(ImGuiID id, ImGuiTreeNodeFlags flags, const char *label, const char *label_end,
    ImguiHelper::TreeNodeWithSpecialHoverBehaviorEndData &end_data, bool allow_blocked_hover = true) = 0;
  virtual void treeNodeRender(ImguiHelper::TreeNodeWithSpecialHoverBehaviorEndData &end_data, bool show_arrow = true) = 0;
  virtual void treeNodeEnd(const ImguiHelper::TreeNodeWithSpecialHoverBehaviorEndData &end_data,
    const float *label_clip_max_x = nullptr) = 0;
  virtual float treeNodeGetLabelClipMaxX() = 0;
};

} // namespace PropPanel
