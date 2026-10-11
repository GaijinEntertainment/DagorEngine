// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "controlSizeHelper.h"
#include "c_constants.h"
#include <propPanel/control/spinEditStandalone.h>

#include <libTools/util/hdpiUtil.h>

#include <imgui/imgui.h>

namespace PropPanel
{

float ControlSizeHelper::getDefaultRightSideEditWidth()
{
  // DEFAULT_CONTROL_WIDTH is for the 16 px default font, scale it with the actual font size.
  const float editWidth = floorf(ImGui::GetFontSize() * (((float)DEFAULT_CONTROL_WIDTH) / 16.0f));
  return editWidth + SpinEditControlStandalone::getSpaceBeforeSpinButtons() + SpinEditControlStandalone::getSpinButtonsSize().x;
}

float ControlSizeHelper::getPreferredTextInputWidth() { return floorf(ImGui::GetFontSize() * 12.0f); }

Point2 ControlSizeHelper::getSeparateLineLabelPreferredSize(const char *label, const char *label_end)
{
  if (!label || !*label)
    return Point2(0.0f, 0.0f);

  // Same as ImguiHelper::separateLineLabel().
  const ImVec2 textSize = ImGui::CalcTextSize(label, label_end);
  const float spaceBelow = hdpi::_pxS(Constants::SPACE_BETWEEN_SEPARATE_LINE_LABEL_AND_CONTROL);
  return Point2(textSize.x, textSize.y + ImGui::GetStyle().FramePadding.y * 2.0f + spaceBelow);
}

Point2 ControlSizeHelper::getSpinEditRowPreferredSize(const char *label, const char *label_end, int spin_edit_count,
  float max_width_px)
{
  const Point2 labelSize = getSeparateLineLabelPreferredSize(label, label_end);
  const float editsWidth =
    getDefaultRightSideEditWidth() * spin_edit_count + ImGui::GetStyle().ItemInnerSpacing.x * (spin_edit_count - 1);
  return Point2(min(max(labelSize.x, editsWidth), max_width_px), labelSize.y + ImGui::GetFrameHeight());
}

Point2 ControlSizeHelper::getTrackBarPreferredSize(const char *label, const char *label_end, float max_width_px)
{
  const Point2 labelSize = getSeparateLineLabelPreferredSize(label, label_end);
  const float sliderAndEditWidth =
    getPreferredTextInputWidth() + ImGui::GetStyle().ItemInnerSpacing.x + getDefaultRightSideEditWidth();
  return Point2(min(max(labelSize.x, sliderAndEditWidth), max_width_px), labelSize.y + ImGui::GetFrameHeight());
}

Point2 ControlSizeHelper::getSpinEditPreferredSize(bool width_includes_label, unsigned width, const char *label, const char *label_end,
  float max_width_px)
{
  const float height = ImGui::GetFrameHeight();

  // updateImgui() clips to the set width, so the size is a fixed width.
  if (width_includes_label && width > 0)
    return Point2(min((float)width, max_width_px), height);

  float editWidth = width > 0 ? (float)width : getDefaultRightSideEditWidth();
  if (label && *label)
    editWidth += ImGui::CalcTextSize(label, label_end).x + ImGui::GetStyle().ItemInnerSpacing.x;

  return Point2(editWidth, height);
}

float ControlSizeHelper::getLongestTextWidth(dag::ConstSpan<String> items)
{
  float longestItemWidth = 0.0f;
  for (const String &item : items)
    longestItemWidth = max(longestItemWidth, ImGui::CalcTextSize(item.begin(), item.end()).x);
  return longestItemWidth;
}

float ControlSizeHelper::getListBoxPreferredWidth(dag::ConstSpan<String> items)
{
  // The list box uses the frame padding as its window padding.
  const ImGuiStyle &style = ImGui::GetStyle();
  const float width = getLongestTextWidth(items) + style.FramePadding.x * 2.0f + style.ScrollbarSize;
  return max(width, getPreferredTextInputWidth());
}

float ControlSizeHelper::getListBoxPreferredHeight(unsigned control_height)
{
  // Same as the height in ListBoxControlStandalone::updateImgui() and ImGui::BeginListBox().
  if (control_height == (unsigned)Constants::LISTBOX_DEFAULT_HEIGHT)
    return floorf(ImGui::GetTextLineHeightWithSpacing() * 7.25f + ImGui::GetStyle().FramePadding.y * 2.0f);
  if (control_height > (unsigned)Constants::LISTBOX_FULL_HEIGHT)
    return (float)control_height;
  return 0.0f;
}

} // namespace PropPanel
