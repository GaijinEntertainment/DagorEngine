//
// Dagor Tech 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <libTools/util/hdpiUtil.h>
#include <imgui/imgui.h>

namespace PropPanel
{

class Constants
{
public:
  // The default vertical space between controls in the container.
  static constexpr int DEFAULT_CONTROLS_INTERVAL = 7;

  // If the track bar's slider width would be smaller than this then only the spin edit control is shown.
  static constexpr int MINIMUM_TRACK_BAR_SLIDER_WIDTH_TO_SHOW = 20;

  // The space between edit box and the "..." button in the FileEditBoxPropertyControl.
  static constexpr int SPACE_BETWEEN_EDIT_BOX_AND_BUTTON_IN_COMBINED_CONTROL = 0.0f;

  // The space between the picker button and the clear button in the FileButtonPropertyControl.
  static constexpr int SPACE_BETWEEN_BUTTONS_IN_COMBINED_CONTROL = 2.0f;

  // Vertical space between a label and a control when they are on a separate line.
  static constexpr int SPACE_BETWEEN_SEPARATE_LINE_LABEL_AND_CONTROL = 0.0f;

  // The default height can fit ~7 items.
  static constexpr hdpi::Px LISTBOX_DEFAULT_HEIGHT = (hdpi::Px)0;

  // Use all the available height.
  static constexpr hdpi::Px LISTBOX_FULL_HEIGHT = (hdpi::Px)1;

  // Colors for the daEditorX Classic style.

  // Color of the dialogs title and tab title texts.
  static constexpr ImVec4 DIALOG_TITLE_COLOR = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);

  // The color that is used to draw the border of the collapsible groups.
  // It is the same as the collapsible header button's border color. The color comes from ImGuiCol_Header with
  // ImGuiCol_Border drawn over it.
  static constexpr ImU32 GROUP_BORDER_COLOR = IM_COL32(112, 112, 112, 255);

  // Text color of the selected listbox item.
  static constexpr ImU32 LISTBOX_SELECTION_TEXT_COLOR = IM_COL32(255, 255, 255, 255);

  // Background color of the selected listbox item.
  static constexpr ImU32 LISTBOX_SELECTION_BACKGROUND_COLOR = IM_COL32(0, 120, 215, 255);

  // Background color of the highlighted listbox item.
  static constexpr ImU32 LISTBOX_HIGHLIGHT_BACKGROUND_COLOR = IM_COL32(168, 168, 168, 255);

  // Border color for the toggle image button when it is checked.
  static constexpr ImU32 TOGGLE_BUTTON_CHECKED_BORDER_COLOR = IM_COL32(0, 0, 0, 64);

  // The border color used for toolbars.
  static constexpr ImU32 TOOLBAR_BORDER_COLOR = IM_COL32(170, 170, 170, 255);

  // The inner window padding of toolbars.
  static constexpr ImVec2 TOOLBAR_WINDOW_PADDING = ImVec2(0.0f, 5.0f);

  // Background color of the selected tree item.
  static constexpr ImU32 TREE_SELECTION_BACKGROUND_COLOR = IM_COL32(217, 217, 217, 255);

  // Color of the tree hierarchy lines. (Lines connectig the open/close toggle icons.)
  static constexpr ImU32 TREE_HIERARCHY_LINE_COLOR = IM_COL32(160, 160, 160, 255);

  // Color of the + and - signs within the open/close toggle icons in the tree.
  static constexpr ImU32 TREE_OPEN_CLOSE_ICON_INNER_COLOR = IM_COL32(0, 0, 0, 255);
};

} // namespace PropPanel