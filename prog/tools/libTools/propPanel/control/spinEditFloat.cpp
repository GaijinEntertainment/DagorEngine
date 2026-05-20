// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <propPanel/control/spinEditFloat.h>

#include "../scopedImguiBeginDisabled.h"

namespace PropPanel
{

void SpinEditFloatPropertyControl::updateImgui()
{
  ScopedImguiBeginDisabled scopedDisabled(!controlEnabled);

  float spinEditWidth = ImguiHelper::getDefaultRightSideEditWidth();

  if (widthIncludesLabel)
  {
    const float availableSpace = mW > 0 ? min((float)mW, ImGui::GetContentRegionAvail().x) : ImGui::GetContentRegionAvail().x;
    const float spaceBeforeSpinEdit = availableSpace - spinEditWidth;

    const float availableSpaceForLabel =
      controlCaption.size() > 0 ? (spaceBeforeSpinEdit - ImGui::GetStyle().ItemInnerSpacing.x) : 0.0f;
    if (availableSpaceForLabel > 0.0f)
    {
      ImGui::SetNextItemWidth(availableSpaceForLabel);
      labelWithTooltip(controlCaption.begin(), controlCaption.end());

      ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
    }
    else if (spaceBeforeSpinEdit > 0.0f)
      ImGui::SetCursorPosX(ImGui::GetCursorPosX() + spaceBeforeSpinEdit);
    else
      spinEditWidth = availableSpace;
  }
  else
  {
    labelWithTooltip(controlCaption.begin(), controlCaption.end(), true);
    ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);

    const float availableSpace = mW > 0 ? min((float)mW, ImGui::GetContentRegionAvail().x) : ImGui::GetContentRegionAvail().x;
    spinEditWidth = availableSpace;
  }

  ImGui::SetNextItemWidth(spinEditWidth);
  setFocusToNextImGuiControlIfRequested();

  // Override the background color of the edit box.
  const bool valueHighlightColorSet = valueHighlightColor != ColorOverride::NONE;
  if (valueHighlightColorSet)
    ImGui::PushStyleColor(ImGuiCol_FrameBg, getOverriddenColor(valueHighlightColor));

  spinEdit.updateImgui(*this, &controlTooltip, this);

  if (valueHighlightColorSet)
    ImGui::PopStyleColor();

  if (spinEdit.isTextInputFocused())
    set_focused_immediate_focus_loss_handler(this);
  else if (get_focused_immediate_focus_loss_handler() == this)
    set_focused_immediate_focus_loss_handler(nullptr);
}

} // namespace PropPanel
