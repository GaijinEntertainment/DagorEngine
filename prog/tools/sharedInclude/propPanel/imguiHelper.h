//
// Dagor Tech 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <libTools/util/hdpiUtil.h>
#include <propPanel/constants.h>
#include <util/dag_string.h>
#include <imgui/imgui.h>

namespace PropPanel
{

class ImguiHelper
{
public:
  struct TreeNodeWithSpecialHoverBehaviorEndData
  {
    // Set to true if the tree node has been drawn. In this case you must call treeNodeWithSpecialHoverBehaviorEnd.
    bool draw = false;

    // Unlike ImGui::IsItemHovered() this is set to true even if the mouse cursor is over an action button.
    bool hovered = false;

    // These members are only valid if draw is true.

    ImGuiID id;               //-V730_NOINIT
    const char *label;        //-V730_NOINIT
    const char *labelEnd;     //-V730_NOINIT
    ImGuiTreeNodeFlags flags; //-V730_NOINIT
    ImVec2 labelSize;
    ImVec2 textPos;
    bool storeTreeNodeStackData; //-V730_NOINIT
    bool isOpen;                 //-V730_NOINIT
  };

  // Internal function for PropPanel.
  static void afterNewFrame();

  // To help with implementing overrides for PropertyControlBase::getTextValue.
  static int getTextValueForString(const String &value, char *buffer, int buflen)
  {
    if ((value.size() + 1) >= buflen)
      return 0;

    memcpy(buffer, value.c_str(), value.size());
    buffer[value.size()] = 0;
    return value.size();
  }

  static int getStringIndexInTab(const Tab<String> &strings, const char *value)
  {
    for (int i = 0; i < strings.size(); ++i)
      if (strcmp(strings[i], value) == 0)
        return i;

    return -1;
  }

  // Same as ImGui::Checkbox but supports changing multiple checkboxes at once by dragging the mouse cursor from the
  // first checkbox while holding the left mouse button.
  static bool checkboxWithDragSelection(const char *label, bool *value);

  // The default ImGui text input control reacts to Enter by making it inactive, or if ConfigInputTextEnterKeepActive
  // is set to true then it keeps active but selects the whole text.
  // We prevent that behaviour by not letting it know about Enter presses...
  // was_focused: whether the input is focused. Passing the previous frame's state is fine.
  static bool inputTextWithEnterWorkaround(const char *label, String *str, bool focused, ImGuiInputTextFlags flags = 0,
    ImGuiInputTextCallback callback = nullptr, void *user_data = nullptr);

  // Same as ImGui::CollapsingHeader but has a fixed width and no extrusion beyond the specified width.
  // Feature requests: https://github.com/ocornut/imgui/issues/6170 and https://github.com/ocornut/imgui/issues/1037.
  static bool collapsingHeaderWidth(const char *label, float width, ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_None);

  // Same as LabelText but without the text.
  // (Almost the same as TextUnformatted but this sets the baseline, so it looks correct beside other controls.)
  // use_text_width: if true then the control will be as wide as required by the text
  //   if false then it uses the width returned by ImGui::CalcItemWidth() (so either the default item width or the
  //   width set by PushItemWidth() or SetNextItemWidth())
  static void labelOnly(const char *label, const char *label_end = nullptr, bool use_text_width = false);

  // Simple helper method for the cases where the label is in a separate line than the control.
  static void separateLineLabel(const char *label, const char *label_end = nullptr)
  {
    if (label && *label)
    {
      const float verticalSpace = hdpi::_pxS(Constants::SPACE_BETWEEN_SEPARATE_LINE_LABEL_AND_CONTROL);
      ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(ImGui::GetStyle().ItemSpacing.x, verticalSpace));
      labelOnly(label, label_end, true);
      ImGui::PopStyleVar();
    }
  }

  // Changes compared to ImGui::TreeNodeBehavior:
  // - splits TreeNodeBehavior into two calls, making it easy add interactive icons before or after the label.
  // - (optional) interactive icons (e.g.: imageButtonFrameless) do not block the tree node from receiving hover
  // - too long labels get an ellipsis (with RenderTextEllipsis) and a tooltip shows their full label on mouse hover
  // - prevents unneeded horizontal scrolling when navigating with the up/down keys (see https://github.com/ocornut/imgui/issues/7932)
  // - (optional) custom open/close button drawing
  //
  // You have to call TreeNodeWithSpecialHoverBehaviorEnd if end_data.draw is set to true. That function does the label
  // drawing, so if you want to add an icon before the label you can do it.
  // If you do additional drawing you have make sure that the horizontal scrollbar iscorrectly sized (for example by
  // collecting the maximum length of the labels and using ImGui::SetCursorScreenPos and ImGui::Dummy).
  //
  // end_data: output parameter
  // allow_blocked_hover: must be true if using image buttons (real ImGui widgets), but not needed for simple state icons.
  //   False is the default behavior of ImGui::TreeNodeBehavior.
  // show_arrow: show the open/close arrow. Set it to false if you want to draw your own open/close icon.
  static bool treeNodeWithSpecialHoverBehaviorStart(ImGuiID id, ImGuiTreeNodeFlags flags, const char *label, const char *label_end,
    TreeNodeWithSpecialHoverBehaviorEndData &end_data, bool allow_blocked_hover = true, bool show_arrow = true);
  static bool treeNodeWithSpecialHoverBehaviorStart(const char *label, ImGuiTreeNodeFlags flags,
    TreeNodeWithSpecialHoverBehaviorEndData &end_data, bool allow_blocked_hover = true, bool show_arrow = true);

  // This uses LastItemData, so do not add new widgets between calling TreeNodeWithSpecialHoverBehaviorStart and
  // TreeNodeWithSpecialHoverBehaviorEnd.
  static void treeNodeWithSpecialHoverBehaviorEnd(const TreeNodeWithSpecialHoverBehaviorEndData &end_data,
    const float *label_clip_max_x = nullptr);

  static float treeNodeWithSpecialHoverBehaviorGetLabelClipMaxX();

  // Based on ImGui::RenderArrow but uses explicit size instead of (font size * scale).
  static void renderArrowWithSize(ImDrawList *draw_list, ImVec2 pos, ImU32 col, ImGuiDir dir, ImVec2 size);

  // Same as ImGui::ArrowButtonEx but uses RenderArrowWithSize instead of RenderArrow, so it can draw the arrow with
  // the specified size.
  static bool arrowButtonExWithSize(const char *str_id, ImGuiDir dir, ImVec2 size, ImVec2 arrow_size, ImGuiButtonFlags flags = 0);

  // Same as ImGui::ListBox but with the window flags parameters.
  // This allows specifying ImGuiWindowFlags_HorizontalScrollbar.
  static bool beginListBoxWithWindowFlags(const char *label, const ImVec2 &size_arg = ImVec2(0.0f, 0.0f),
    ImGuiChildFlags child_flags = ImGuiChildFlags_FrameStyle, ImGuiWindowFlags window_flags = ImGuiWindowFlags_None);

  // Based on ImGui::ImageButton but no frame is drawn (ImGui::RenderFrame), and background is drawn if the button is pressed or
  // hovered.
  static bool imageButtonFrameless(ImGuiID id, ImTextureID texture_id, const ImVec2 &image_size, const ImVec2 &uv0 = ImVec2(0, 0),
    const ImVec2 &uv1 = ImVec2(1, 1), const ImVec4 &tint_col = ImVec4(1, 1, 1, 1));

  // This is just a convenience overload with an option to set the tooltip.
  static bool imageButtonFrameless(const char *str_id, ImTextureID texture_id, const ImVec2 &image_size, const char *tooltip);

  // Helper function to either place an imageButtonFrameless control or just reserve the space for it.
  static bool imageButtonFramelessOrPlaceholder(const char *str_id, ImTextureID texture_id, const ImVec2 &image_size,
    const char *tooltip, bool use_button);

  static ImVec2 getImageButtonFramelessFullSize(const ImVec2 &image_size) { return image_size; }

  // A toggle image button.
  static bool imageCheckButtonWithBackground(const char *str_id, ImTextureID texture_id, const ImVec2 &image_size, bool checked,
    const char *tooltip);

  static ImVec2 getImageButtonWithDownArrowSize(const ImVec2 &image_size);

  // A toggle image button with a down arrow displayed on the right side of the image.
  static bool imageButtonWithArrow(const char *str_id, ImTextureID texture_id, const ImVec2 &image_size, bool checked = false,
    ImGuiButtonFlags flags = ImGuiButtonFlags_None);

  // Get the size of ImGui::Button.
  static ImVec2 getButtonSize(const char *label, bool hide_text_after_double_hash = false, const ImVec2 &size_arg = ImVec2(0, 0));

  // A search input with an search icon (typically a magnifying glass) and clear icon (typically an X). The latter is
  // only displayed when text_to_search is not empty. Clicking on it clears text_to_search.
  static bool searchInput(const void *focus_id, const char *label, const char *hint, String &text_to_search, ImTextureID search_icon,
    ImTextureID clear_icon, bool *input_focused = nullptr, ImGuiID *input_id = nullptr);

  // Same as ImGui::BeginMenuEx but meant to be used along with menuItemExWithLeftSideCheckmark. It aligns sub-menu
  // items the same way as menuItemExWithLeftSideCheckmark.
  static bool beginMenuExWithLeftSideCheckmark(const char *label, const char *icon, bool enabled = true);

  // Same as ImGui::MenuItemEx but the checkmark is rendered before the label just like in Windows.
  // This also can draw bullets instead of checkmarks (for radio button-like menu items).
  static bool menuItemExWithLeftSideCheckmark(const char *label, const char *icon, const char *shortcut = nullptr,
    bool selected = false, bool enabled = true, bool bullet = false);

  static float getDefaultRightSideEditWidth();

  // This functions returns with ImGui::GetCursorPosY() minus the item spacing added by the last control.
  // Simply using ImGui::GetCursorPosY() and subtracting the current GetStlye().ItemSpacing.y could be incorrect for
  // example if the style has been changed before the last control and PopStyleVar already ran.
  static float getCursorPosYWithoutLastItemSpacing();

  // This function can be used instead of a fixed icon size.
  // For example when using for ImGui::ImageButton's image size parameter the control will match other control's -- like the text input
  // -- height.
  static ImVec2 getFontSizedIconSize()
  {
    const float textLineHeight = ImGui::GetTextLineHeight();
    return ImVec2(textLineHeight, textLineHeight);
  }

  static void setPointSampler();
  static void setDefaultSampler();

private:
  static ImVec2 getImageButtonWithDownArrowSizeInternal(const ImVec2 &image_size, float &default_height, ImVec2 &arrow_half_size);

  static bool checkboxDragSelectionInProgress;
  static bool checkboxDragSelectionValue;
};

} // namespace PropPanel