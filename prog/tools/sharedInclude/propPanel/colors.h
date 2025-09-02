//
// Dagor Tech 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <imgui/imgui.h>

namespace PropPanel
{

struct ColorOverride
{
  enum
  {
    // Color of the dialogs title and tab title texts.
    DIALOG_TITLE,
    // Color of the dialog close buttons when active.
    DIALOG_CLOSE_ACTIVE,
    // Color of the dialog close buttons when hovered.
    DIALOG_CLOSE_HOVERED,

    // Color of the tab title texts on tab bars.
    TAB_BAR_TITLE,

    // The color that is used to draw the border of the collapsible groups.
    // It is the same as the collapsible header button's border color. The color comes from ImGuiCol_Header with
    // ImGuiCol_Border drawn over it.
    GROUP_BORDER,

    // Text color of the selected listbox item.
    LISTBOX_SELECTION_TEXT,
    // Background color of the selected listbox item.
    LISTBOX_SELECTION_BACKGROUND,
    // Background color of the highlighted listbox item when active.
    LISTBOX_HIGHLIGHT_BACKGROUND_ACTIVE,
    // Background color of the highlighted listbox item when hovered.
    LISTBOX_HIGHLIGHT_BACKGROUND_HOVERED,

    // Border color for the toggle image button when it is checked.
    TOGGLE_BUTTON_CHECKED_BORDER,

    // The border color used for toolbars.
    TOOLBAR_BORDER,
    // The background color used for toolbars.
    TOOLBAR_BACKGROUND,

    // Background color of the selected tree item.
    TREE_SELECTION_BACKGROUND,
    // Color of the tree hierarchy lines. (Lines connectig the open/close toggle icons.)
    TREE_HIERARCHY_LINE,
    // Color of the + and - signs within the open/close toggle icons in the tree.
    TREE_OPEN_CLOSE_ICON_INNER,

    // The color of the track-bar frame background.
    TRACKBAR_BACKGROUND,
    // The color of the track-bar frame background when active.
    TRACKBAR_BACKGROUND_ACTIVE,
    // The color of the track-bar frame background when hovered.
    TRACKBAR_BACKGROUND_HOVERED,

    // Text color of filter buttons.
    FILTER_TEXT,
    // Background color for the filter buttons.
    FILTER_BUTTON,

    // Foreground color of the log messages in the console.
    CONSOLE_LOG_TEXT,
    CONSOLE_LOG_COMMAND_TEXT,
    CONSOLE_LOG_PARAMETER_TEXT,
    CONSOLE_LOG_WARNING_TEXT,
    CONSOLE_LOG_ERROR_TEXT,
    CONSOLE_LOG_FATAL_TEXT,
    CONSOLE_LOG_REMARK_TEXT,
    // Background color of the log messages in the console.
    CONSOLE_LOG_BACKGROUND,
    // Foreground color of the command input edit box in the console.
    CONSOLE_EDIT_BOX_TEXT,
    // Background color of the command input edit box in the console.
    CONSOLE_EDIT_BOX_BACKGROUND,
    // Color of the bars of the progress bar in the console.
    CONSOLE_PROGRESS_BAR,
    // Background color of the progress bar in the console.
    CONSOLE_PROGRESS_BAR_BACKGROUND,

    AXIS_X,
    AXIS_Y,
    AXIS_Z,

    COUNT
  };

  ImVec4 color;
  const char *name;
  const int defaultIdx;

  ColorOverride() : defaultIdx(0), name("") {}

  ColorOverride(const char *_name, int _defaultIdx) : name(_name), defaultIdx(_defaultIdx) {}

  ImU32 getColorU32() const { return ImGui::GetColorU32(color); }
  void setColorU32(ImU32 _color) { color = ImGui::ColorConvertU32ToFloat4(_color); }
};

void applyClassicOverrides();
ColorOverride &getColorOverride(int index);

inline ImVec4 getOverriddenColor(int index) { return getColorOverride(index).color; }
inline ImU32 getOverriddenColorU32(int index) { return getColorOverride(index).getColorU32(); }

// Flash a given color for some + inhibit modifications of this color override via PushStyleColor() calls.
void debugFlashColorOverride(int index);
void updateDebugToolFlashColorOverride();

// Push/pop overrides constants and helper functions
enum ColorOverrideGroup : int
{
  COLORS_TOOL_BAR = 2,
  COLORS_TITLE_BAR = 3,
  COLORS_TRACK_BAR = 3,
};

inline ColorOverrideGroup pushToolBarColorOverrides()
{
  ImGui::PushStyleColor(ImGuiCol_ChildBg, getOverriddenColor(ColorOverride::TOOLBAR_BACKGROUND));
  ImGui::PushStyleColor(ImGuiCol_Border, getOverriddenColor(ColorOverride::TOOLBAR_BORDER));
  return ColorOverrideGroup::COLORS_TOOL_BAR;
}

inline void popToolBarColorOverrides() { ImGui::PopStyleColor(ColorOverrideGroup::COLORS_TOOL_BAR); }

inline ColorOverrideGroup pushDialogTitleBarColorOverrides()
{
  ImGui::PushStyleColor(ImGuiCol_Text, getOverriddenColor(ColorOverride::DIALOG_TITLE));
  ImGui::PushStyleColor(ImGuiCol_ButtonActive, getOverriddenColor(ColorOverride::DIALOG_CLOSE_ACTIVE));
  ImGui::PushStyleColor(ImGuiCol_ButtonHovered, getOverriddenColor(ColorOverride::DIALOG_CLOSE_HOVERED));
  return ColorOverrideGroup::COLORS_TITLE_BAR;
}

inline void popDialogTitleBarColorOverrides() { ImGui::PopStyleColor(ColorOverrideGroup::COLORS_TITLE_BAR); }

inline ColorOverrideGroup pushTrackBarColorOverrides()
{
  ImGui::PushStyleColor(ImGuiCol_FrameBg, getOverriddenColor(ColorOverride::TRACKBAR_BACKGROUND));
  ImGui::PushStyleColor(ImGuiCol_FrameBgActive, getOverriddenColor(ColorOverride::TRACKBAR_BACKGROUND_ACTIVE));
  ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, getOverriddenColor(ColorOverride::TRACKBAR_BACKGROUND_HOVERED));
  return ColorOverrideGroup::COLORS_TRACK_BAR;
}

inline void popTrackBarColorOverrides() { ImGui::PopStyleColor(ColorOverrideGroup::COLORS_TRACK_BAR); }

} // namespace PropPanel
