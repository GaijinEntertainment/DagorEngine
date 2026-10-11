//
// Dagor Tech 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <libTools/util/hdpiUtil.h>
#include <propPanel/constants.h>
#include <util/dag_string.h>
#include <propPanel/control/menuStyle.h>

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

namespace PropPanel
{

enum class IconId : int;

class ImguiHelper
{
public:
  // Helper to hide key down state from controls that do not use ImGui's shortcut API. For example: ImGui::Input().
  class KeyDownHider
  {
  public:
    // If key is ImGuiKey_None then nothing will be done.
    explicit KeyDownHider(ImGuiKey key)
    {
      keyData = key == ImGuiKey_None ? nullptr : ImGui::GetKeyData(key);
      if (keyData && keyData->Down)
      {
        originalKeyData = *keyData;
        keyData->Down = false;
        keyData->DownDuration = -1.0f;
        keyData->DownDurationPrev = -1.0f;
      }
      else
      {
        originalKeyData.Down = false;
      }
    }

    ~KeyDownHider()
    {
      if (originalKeyData.Down)
        *keyData = originalKeyData;
    }

  private:
    ImGuiKeyData *keyData;
    ImGuiKeyData originalKeyData;
  };

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
    ImVec2 textOffset;

    // The row itself. Use it for whatever must line up with the row, for example TreeHierarchyLineDrawer.
    ImRect rowBB;

    // rowBB expanded with the vertical ItemSpacing, so that neighbouring rows touch. This is the row highlight and the
    // drop target. It is also the item ImGui sees (hover, click, GetItemRectMin/Max) only with a span flag; without one
    // the submitted item stops at the label width.
    ImRect itemBB;

    ImVec2 padding;
    bool storeTreeNodeStackData; //-V730_NOINIT
    bool isOpen;                 //-V730_NOINIT
    bool isMultiSelect;          //-V730_NOINIT
    bool displayFrame;           //-V730_NOINIT
    bool held;                   //-V730_NOINIT
    bool selected;               //-V730_NOINIT
    bool spanAllColumns;         //-V730_NOINIT
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

  // A checkbox drawn as a switch between two labels: "label_off [switch] label_on". The label of the current state is
  // shown with the normal text color, the other one with the disabled text color.
  // Returns true when the user changes the state. In that case "value" holds the new state.
  static bool toggleSwitch(const char *str_id, const char *label_off, const char *label_on, bool &value);

  // Similar to the ToolbarToggleButton or to ImGui::Checkbox, but looks like a regular button (text instead of icons)
  // and also supports toggling multiple buttons at once by dragging the mouse cursor from the
  // first toggle button while holding the left mouse button.
  static bool toggleButtonWithDragSelection(const char *label, bool *value, float hovered_border_size, ImU32 col_text_active);

  // The default ImGui text input control reacts to Enter by making it inactive, or if ConfigInputTextEnterKeepActive
  // is set to true then it keeps active but selects the whole text.
  // We prevent that behaviour by not letting it know about Enter presses...
  // hint: placeholder text (shown when the input is empty)
  // was_focused: whether the input is focused. Passing the previous frame's state is fine.
  static bool inputTextWithEnterWorkaround(const char *label, const char *hint, String *str, bool focused,
    ImGuiInputTextFlags flags = 0, ImGuiInputTextCallback callback = nullptr, void *user_data = nullptr);

  // Same as ImGui::CollapsingHeader but has a fixed width and no extrusion beyond the specified width.
  // Feature requests: https://github.com/ocornut/imgui/issues/6170 and https://github.com/ocornut/imgui/issues/1037.
  static bool collapsingHeaderWidth(const char *label, float width, ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_None);

  // Draw the left bottom of the frame with an optionally rounded corner.
  static void drawHalfFrame(float line_left_x, float line_top_y, float line_right_x, float line_bottom_y);

  // Same as ImGui::RenderFrame() but with draw_flags parameter, that allows specifying which side of the frame should be rounded.
  static void renderFrameWithRoundingOptions(ImVec2 p_min, ImVec2 p_max, ImU32 fill_col, bool borders, float rounding,
    ImDrawFlags draw_flags);

  // Display a label.
  // The label show ellipsis at the end of the visible text if the entire text cannot be displayed.
  // It sets the baseline, so it looks correct beside other controls (unlike ImGui::TextUnformatted()).
  // Thsi function is similar to ImGui::LabelText() but instead of label + text, this draws only the label.
  // use_text_width: if true then the control will be as wide as required by the text
  //   if false then it uses the width returned by ImGui::CalcItemWidth() (so either the default item width or the
  //   width set by PushItemWidth() or SetNextItemWidth())
  // returns true if the item has been added
  static bool labelOnly(const char *label, const char *label_end = nullptr, bool use_text_width = false);

  // Simple helper method for the cases where the label is in a separate line than the control.
  // returns true if the item has been added
  static bool separateLineLabel(const char *label, const char *label_end = nullptr)
  {
    if (label && *label)
    {
      const float verticalSpace = hdpi::_pxS(Constants::SPACE_BETWEEN_SEPARATE_LINE_LABEL_AND_CONTROL);
      ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(ImGui::GetStyle().ItemSpacing.x, verticalSpace));
      const bool itemAdded = labelOnly(label, label_end, true);
      ImGui::PopStyleVar();

      return itemAdded;
    }

    return false;
  }

  // Changes compared to ImGui::TreeNodeBehavior:
  // - splits TreeNodeBehavior into two calls, making it easy add interactive icons before or after the label.
  // - (optional) interactive icons (e.g.: imageButtonFrameless) do not block the tree node from receiving hover
  // - too long labels get an ellipsis (with RenderTextEllipsis) and a tooltip shows their full label on mouse hover
  // - prevents unneeded horizontal scrolling when navigating with the up/down keys (see https://github.com/ocornut/imgui/issues/7932)
  // - (optional) custom open/close button drawing
  // - the item covers the vertical spacing between the rows, so the rows touch and there is no hover gap between them.
  //
  // You have to call TreeNodeWithSpecialHoverBehaviorEnd if end_data.draw is set to true. That function does the label
  // drawing, so if you want to add an icon before the label you can do it.
  // If you do additional drawing you have to make sure that the horizontal scrollbar is correctly sized (for example by
  // collecting the maximum length of the labels and using ImGui::SetCursorScreenPos and ImGui::Dummy).
  //
  // end_data: output parameter
  // allow_blocked_hover: must be true if using image buttons (real ImGui widgets), but not needed for simple state icons.
  //   False is the default behavior of ImGui::TreeNodeBehavior.
  // show_arrow: show the open/close arrow. Set it to false if you want to draw your own open/close icon.
  //   Hierarchy lines and the opcen/close icon can be drawn with TreeHierarchyLineDrawer.
  static bool treeNodeWithSpecialHoverBehaviorStart(ImGuiID id, ImGuiTreeNodeFlags flags, const char *label, const char *label_end,
    TreeNodeWithSpecialHoverBehaviorEndData &end_data, bool allow_blocked_hover = true);
  static bool treeNodeWithSpecialHoverBehaviorStart(const char *label, ImGuiTreeNodeFlags flags,
    TreeNodeWithSpecialHoverBehaviorEndData &end_data, bool allow_blocked_hover = true);

  static void treeNodeWithSpecialHoverBehaviorRender(TreeNodeWithSpecialHoverBehaviorEndData &end_data, bool show_arrow = true);

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

  // Names the current window's scrollbars for the ImGui test runtime. ImGui submits them as items but without a label.
  // Call while the window is current.
  static void hookWindowScrollbarsForTestRuntime();

  // Based on ImGui::ImageButton but no frame is drawn (ImGui::RenderFrame), and background is drawn if the button is pressed or
  // hovered.
  static bool imageButtonFrameless(ImGuiID id, ImTextureID texture_id, const ImVec2 &image_size, const ImVec2 &uv0 = ImVec2(0, 0),
    const ImVec2 &uv1 = ImVec2(1, 1), const ImVec4 &tint_col = ImVec4(1, 1, 1, 1));
  static bool imageButtonFrameless(ImGuiID id, IconId icon_id, const ImVec2 &image_size, const ImVec2 &uv0 = ImVec2(0, 0),
    const ImVec2 &uv1 = ImVec2(1, 1), const ImVec4 &tint_col = ImVec4(1, 1, 1, 1));

  // This is just a convenience overload with an option to set the tooltip.
  static bool imageButtonFrameless(const char *str_id, ImTextureID texture_id, const ImVec2 &image_size, const char *tooltip);
  static bool imageButtonFrameless(const char *str_id, IconId icon_id, const ImVec2 &image_size, const char *tooltip);

  // Helper function to either place an imageButtonFrameless control or just reserve the space for it.
  static bool imageButtonFramelessOrPlaceholder(const char *str_id, ImTextureID texture_id, const ImVec2 &image_size,
    const char *tooltip, bool use_button);
  static bool imageButtonFramelessOrPlaceholder(const char *str_id, IconId icon_id, const ImVec2 &image_size, const char *tooltip,
    bool use_button);

  static ImVec2 getImageButtonFramelessFullSize(const ImVec2 &image_size) { return image_size; }

  static ImVec2 getImageButtonSize(const ImVec2 &image_size)
  {
    const ImVec2 &framePadding = GImGui->Style.FramePadding;
    return ImVec2(image_size.x + framePadding.x * 2.0f, image_size.y + framePadding.y * 2.0f);
  }

  // A toggle image button.
  static bool imageCheckButtonWithBackground(const char *str_id, ImTextureID texture_id, const ImVec2 &image_size, bool checked,
    const char *tooltip);
  static bool imageCheckButtonWithBackground(const char *str_id, IconId icon_id, const ImVec2 &image_size, bool checked,
    const char *tooltip);

  static ImVec2 getImageButtonWithDownArrowSize(const ImVec2 &image_size)
  {
    float defaultHeight;
    ImVec2 arrowHalfSize;
    return getImageButtonWithDownArrowSizeInternal(image_size, defaultHeight, arrowHalfSize);
  }

  // A toggle image button with a down arrow displayed on the right side of the image.
  static bool imageButtonWithArrow(const char *str_id, ImTextureID texture_id, const ImVec2 &image_size, bool checked = false,
    ImGuiButtonFlags flags = ImGuiButtonFlags_None);
  static bool imageButtonWithArrow(const char *str_id, IconId icon_id, const ImVec2 &image_size, bool checked = false,
    ImGuiButtonFlags flags = ImGuiButtonFlags_None);

  // Same as ImGui::ImageButtonEx() but with frame_draw_flags parameter, that allows specifying which side of the button should be
  // rounded.
  static bool imageButtonWithRoundingOptions(ImGuiID id, ImTextureRef tex_ref, const ImVec2 &image_size, const ImVec2 &uv0,
    const ImVec2 &uv1, const ImVec4 &bg_col, const ImVec4 &tint_col, ImGuiButtonFlags flags, ImDrawFlags frame_draw_flags);

  // This is ImGui::Button() with an image before the text. Both the image and the text are aligned to the left.
  // (The same can almost be achieved by using ImGui::Image() with ImGui::Selectable(..., ImGuiSelectableFlags_SpanAllColumns) but
  // unfortunately the highlight on that lacks padding.)
  static bool imageButtonWithText(const char *label, ImTextureID texture_id, const ImVec2 &image_size,
    const ImVec2 &size = ImVec2(0, 0), ImGuiButtonFlags flags = ImGuiButtonFlags_None);
  static bool imageButtonWithText(const char *label, IconId icon_id, const ImVec2 &image_size, const ImVec2 &size = ImVec2(0, 0),
    ImGuiButtonFlags flags = ImGuiButtonFlags_None);

  // Get the size of ImGui::Button.
  static ImVec2 getButtonSize(const char *label, bool hide_text_after_double_hash = false, const ImVec2 &size_arg = ImVec2(0, 0))
  {
    const ImVec2 labelSize = ImGui::CalcTextSize(label, nullptr, hide_text_after_double_hash);
    return ImGui::CalcItemSize(size_arg, labelSize.x + ImGui::GetStyle().FramePadding.x * 2.0f,
      labelSize.y + ImGui::GetStyle().FramePadding.y * 2.0f);
  }

  // A search input with an search icon (typically a magnifying glass) and clear icon (typically an X). The latter is
  // only displayed when text_to_search is not empty. Clicking on it clears text_to_search.
  // When Enter is pressed it keeps the focus. (See the comment at inputTextWithEnterWorkaround().)
  // input_focused: optional input and output parameter. As an input it should contain whether the input is focused. Passing the
  //   previous frame's state is fine. As an output parameter it contains whether the input is focused.
  // input_id: optional output parameter. Contains the ID of the input control. Use this instead of ImGui::GetItemID() because of the
  //   clear button.
  // input_rect: optional output parameter. Contains the full rectangle of the input control. Use this instead of ImGui::GetItemRect*()
  //   because of the clear button.
  // deactivated_after_edit: optional output parameter. ImGui::IsItemDeactivatedAfterEdit() for the text input, captured before the
  //   clear button is submitted (a plain IsItemDeactivatedAfterEdit() after this call would read the clear button instead).
  static bool searchInput(const void *focus_id, const char *label, const char *hint, String &text_to_search, ImTextureID search_icon,
    ImTextureID clear_icon, bool *input_focused = nullptr, ImGuiID *input_id = nullptr, ImRect *input_rect = nullptr,
    bool *deactivated_after_edit = nullptr);
  static bool searchInput(const void *focus_id, const char *label, const char *hint, String &text_to_search, IconId search_icon_id,
    IconId clear_icon_id, bool *input_focused = nullptr, ImGuiID *input_id = nullptr, ImRect *input_rect = nullptr,
    bool *deactivated_after_edit = nullptr);

  // Same as ImGui::BeginMenuEx but meant to be used along with menuItemExWithLeftSideCheckmark. It aligns sub-menu
  // items the same way as menuItemExWithLeftSideCheckmark, shortcut column included.
  static bool beginMenuExWithLeftSideCheckmark(const char *label, const char *icon, const char *shortcut = nullptr,
    bool enabled = true, const MenuStyle &menu_style = DEFAULT_MENU_STYLE);

  // Same as ImGui::MenuItemEx but the checkmark is rendered before the label just like in Windows.
  // This also can draw bullets instead of checkmarks (for radio button-like menu items).
  // menu_item_selected: if true then draw a highlight the same way as if the menu item had a sub-menu opened. This can
  //   used when opening a context menu for a menu item.
  static bool menuItemExWithLeftSideCheckmark(const char *label, const char *icon, const char *shortcut = nullptr,
    bool selected = false, bool enabled = true, bool bullet = false, bool menu_item_selected = false,
    const MenuStyle &menu_style = DEFAULT_MENU_STYLE, bool secondary_is_comment = false);

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

  // This is a more convenient version of pre-registering all owned keys and then checking them with ImGui::IsKeyChordPressed.
  static bool isKeyChordPressedOwned(ImGuiKeyChord key_chord, ImGuiID canvas_id);

  // Makes the item with str_id inactive if it is currently active and disabled, and returns true if it did so.
  // It does not change ImGui's navigation focus, so an item that handles keys must do that itself.
  // It is recommended to call it before submitting the item, to prevent the disabled item from making any further changes.
  static bool deactivateItemIfActiveAndDisabled(const char *str_id);

  // Reset all window settings (position, size, dock state, ImGuiCond_FirstUseEver, etc.) stored by ImGui.
  static void resetWindowLayout();

private:
  static ImVec2 getImageButtonWithDownArrowSizeInternal(const ImVec2 &image_size, float &default_height, ImVec2 &arrow_half_size)
  {
    const ImGuiContext &g = *GImGui;
    const float arrowScale = 0.5f;

    default_height = ImGui::GetFrameHeight();
    arrow_half_size = ImVec2(g.FontSize * 0.5f * arrowScale, g.FontSize * 0.25f * arrowScale);

    return ImVec2(
      g.Style.FramePadding.x + image_size.x + g.Style.ItemInnerSpacing.x + (arrow_half_size.x * 2.0f) + g.Style.FramePadding.x,
      ImMax(image_size.y, default_height));
  }

  static bool checkboxDragSelectionInProgress;
  static bool checkboxDragSelectionValue;

  static bool buttonDragSelectionInProgress;
  static bool buttonDragSelectionValue;
};

} // namespace PropPanel