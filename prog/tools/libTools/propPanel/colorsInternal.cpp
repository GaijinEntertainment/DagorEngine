// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "colorsInternal.h"

namespace PropPanel
{

// clang-format off
ColorOverride colors[ColorOverride::COUNT] = {
  // ImGui color overrides for the daEditorX Classic style.
  ColorOverride("DialogTitle", ImGuiCol_Text),
  ColorOverride("DialogCloseActive", ImGuiCol_ButtonActive),
  ColorOverride("DialogCloseHovered", ImGuiCol_ButtonHovered),
  ColorOverride("TabBarTitle", ImGuiCol_Text),
  ColorOverride("GroupBorder", ImGuiCol_COUNT + 1),
  ColorOverride("ListboxSelectionText", ImGuiCol_Text),
  ColorOverride("ListboxSelectionBackground", ImGuiCol_Header),
  ColorOverride("ListboxHighlightBackgroundActive", ImGuiCol_HeaderActive),
  ColorOverride("ListboxHighlightBackgroundHovered", ImGuiCol_HeaderHovered),
  ColorOverride("ToggleButtonCheckedBorder", ImGuiCol_Border),
  ColorOverride("ToolbarBorder", ImGuiCol_Border),
  ColorOverride("ToolbarBackground", ImGuiCol_ChildBg),
  ColorOverride("TreeSelectionBackground", ImGuiCol_Header),
  ColorOverride("TreeHierarchyLine", ImGuiCol_COUNT + 1),
  ColorOverride("TreeOpenCloseIconInner", ImGuiCol_COUNT + 1),
  ColorOverride("TrackbarBackground", ImGuiCol_FrameBg),
  ColorOverride("TrackbarBackgroundActive", ImGuiCol_FrameBgActive),
  ColorOverride("TrackbarBackgroundHovered", ImGuiCol_FrameBgHovered),
  ColorOverride("FilterText", ImGuiCol_Text),
  ColorOverride("FilterButton", ImGuiCol_Button),
  ColorOverride("ConsoleLogText", ImGuiCol_COUNT + 1),
  ColorOverride("ConsoleLogCommandText", ImGuiCol_COUNT + 1),
  ColorOverride("ConsoleLogParameterText", ImGuiCol_COUNT + 1),
  ColorOverride("ConsoleLogWarningText", ImGuiCol_COUNT + 1),
  ColorOverride("ConsoleLogErrorText", ImGuiCol_COUNT + 1),
  ColorOverride("ConsoleLogFatalText", ImGuiCol_COUNT + 1),
  ColorOverride("ConsoleLogRemarkText", ImGuiCol_COUNT + 1),
  ColorOverride("ConsoleLogBackground", ImGuiCol_COUNT + 1),
  ColorOverride("ConsoleEditBoxText", ImGuiCol_COUNT + 1),
  ColorOverride("ConsoleEditBoxBackground", ImGuiCol_COUNT + 1),
  ColorOverride("ConsoleProgressBar", ImGuiCol_COUNT + 1),
  ColorOverride("ConsoleProgressBarBackground", ImGuiCol_COUNT + 1),
  ColorOverride("AxisX", ImGuiCol_COUNT + 1),
  ColorOverride("AxisY", ImGuiCol_COUNT + 1),
  ColorOverride("AxisZ", ImGuiCol_COUNT + 1)};
// clang-format on
G_STATIC_ASSERT(ColorOverride::COUNT == 35);

void applyClassicOverrides()
{
  colors[ColorOverride::DIALOG_TITLE].setColorU32(IM_COL32(255, 255, 255, 255));
  colors[ColorOverride::DIALOG_CLOSE_ACTIVE].setColorU32(IM_COL32(160, 160, 160, 255));
  colors[ColorOverride::DIALOG_CLOSE_HOVERED].setColorU32(IM_COL32(160, 160, 160, 255));
  colors[ColorOverride::TAB_BAR_TITLE].setColorU32(IM_COL32(0, 0, 0, 255));
  colors[ColorOverride::GROUP_BORDER].setColorU32(IM_COL32(112, 112, 112, 255));
  colors[ColorOverride::LISTBOX_SELECTION_TEXT].setColorU32(IM_COL32(255, 255, 255, 255));
  colors[ColorOverride::LISTBOX_SELECTION_BACKGROUND].setColorU32(IM_COL32(0, 120, 215, 255));
  colors[ColorOverride::LISTBOX_HIGHLIGHT_BACKGROUND_ACTIVE].setColorU32(IM_COL32(168, 168, 168, 255));
  colors[ColorOverride::LISTBOX_HIGHLIGHT_BACKGROUND_HOVERED].setColorU32(IM_COL32(168, 168, 168, 255));
  colors[ColorOverride::TOGGLE_BUTTON_CHECKED_BORDER].setColorU32(IM_COL32(0, 0, 0, 64));
  colors[ColorOverride::TOOLBAR_BORDER].setColorU32(IM_COL32(170, 170, 170, 255));
  colors[ColorOverride::TOOLBAR_BACKGROUND].setColorU32(IM_COL32(240, 240, 240, 255));
  colors[ColorOverride::TREE_SELECTION_BACKGROUND].setColorU32(IM_COL32(217, 217, 217, 255));
  colors[ColorOverride::TREE_HIERARCHY_LINE].setColorU32(IM_COL32(160, 160, 160, 255));
  colors[ColorOverride::TREE_OPEN_CLOSE_ICON_INNER].setColorU32(IM_COL32(0, 0, 0, 255));
  colors[ColorOverride::TRACKBAR_BACKGROUND].setColorU32(IM_COL32(255, 255, 255, 255));
  colors[ColorOverride::TRACKBAR_BACKGROUND_ACTIVE].setColorU32(IM_COL32(255, 255, 255, 255));
  colors[ColorOverride::TRACKBAR_BACKGROUND_HOVERED].setColorU32(IM_COL32(255, 255, 255, 255));
  colors[ColorOverride::FILTER_TEXT].setColorU32(IM_COL32(0, 137, 255, 255));
  colors[ColorOverride::FILTER_BUTTON].setColorU32(IM_COL32(209, 209, 209, 255));
  colors[ColorOverride::CONSOLE_LOG_TEXT].setColorU32(IM_COL32(0, 0, 0, 255));
  colors[ColorOverride::CONSOLE_LOG_COMMAND_TEXT].setColorU32(IM_COL32(0, 0, 0, 255));
  colors[ColorOverride::CONSOLE_LOG_PARAMETER_TEXT].setColorU32(IM_COL32(128, 128, 0, 255));
  colors[ColorOverride::CONSOLE_LOG_WARNING_TEXT].setColorU32(IM_COL32(0, 128, 0, 255));
  colors[ColorOverride::CONSOLE_LOG_ERROR_TEXT].setColorU32(IM_COL32(255, 0, 0, 255));
  colors[ColorOverride::CONSOLE_LOG_FATAL_TEXT].setColorU32(IM_COL32(255, 0, 0, 255));
  colors[ColorOverride::CONSOLE_LOG_REMARK_TEXT].setColorU32(IM_COL32(128, 128, 128, 255));
  colors[ColorOverride::CONSOLE_LOG_BACKGROUND].setColorU32(IM_COL32(255, 255, 255, 255));
  colors[ColorOverride::CONSOLE_EDIT_BOX_TEXT].setColorU32(IM_COL32(0, 0, 0, 255));
  colors[ColorOverride::CONSOLE_EDIT_BOX_BACKGROUND].setColorU32(IM_COL32(255, 255, 255, 255));
  colors[ColorOverride::CONSOLE_PROGRESS_BAR].setColorU32(IM_COL32(89, 125, 178, 255));
  colors[ColorOverride::CONSOLE_PROGRESS_BAR_BACKGROUND].setColorU32(IM_COL32(204, 204, 204, 255));
  colors[ColorOverride::AXIS_X].setColorU32(IM_COL32(228, 55, 80, 255));
  colors[ColorOverride::AXIS_Y].setColorU32(IM_COL32(133, 209, 12, 255));
  colors[ColorOverride::AXIS_Z].setColorU32(IM_COL32(46, 131, 228, 255));
}

static float debugFlashColorOverrideTime = 0.0f;
static int debugFlashColorOverrideIdx = ColorOverride::COUNT;
static ImVec4 debugFlashColorOverrideBackup = ImVec4();

ColorOverride &getColorOverride(int index)
{
  G_ASSERT(0 <= index && index < ColorOverride::COUNT);
  return colors[index];
}

static void debugFlashColorOverrideStop()
{
  if (debugFlashColorOverrideIdx != ColorOverride::COUNT)
    colors[debugFlashColorOverrideIdx].color = debugFlashColorOverrideBackup;
  debugFlashColorOverrideIdx = ColorOverride::COUNT;
}

// Flash a given color override for some + inhibit modifications of this color via PushStyleColor() calls.
void debugFlashColorOverride(int index)
{
  G_ASSERT(0 <= index && index < ColorOverride::COUNT);
  debugFlashColorOverrideStop();
  debugFlashColorOverrideTime = 0.5f;
  debugFlashColorOverrideIdx = index;
  debugFlashColorOverrideBackup = colors[index].color;
}

void updateDebugToolFlashColorOverride()
{
  if (debugFlashColorOverrideTime <= 0.0f)
    return;

  G_ASSERT(0 <= debugFlashColorOverrideIdx && debugFlashColorOverrideIdx < ColorOverride::COUNT);
  ColorOverride &c = colors[debugFlashColorOverrideIdx];
  ImGui::ColorConvertHSVtoRGB(cosf(debugFlashColorOverrideTime * 6.0f) * 0.5f + 0.5f, 0.5f, 0.5f, c.color.x, c.color.y, c.color.z);
  c.color.w = 1.0f;
  debugFlashColorOverrideTime -= ImGui::GetIO().DeltaTime;
  if (debugFlashColorOverrideTime <= 0.0f)
    debugFlashColorOverrideStop();
}

} // namespace PropPanel
