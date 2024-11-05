// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <propPanel/control/propertyControlBase.h>
#include <propPanel/imguiHelper.h>
#include "spinEditStandalone.h"
#include "../scopedImguiBeginDisabled.h"

namespace PropPanel
{

class SpinEditFloatPropertyControl : public PropertyControlBase
{
public:
  // width_includes_label: if true then the width set to control includes both the label and the spin edit, and the
  // label is aligned to left and the spin edit to the right. If false then label is rendered as wide as required by its
  // contents, and the spin edit follows it directly.
  SpinEditFloatPropertyControl(ControlEventHandler *event_handler, ContainerPropertyControl *parent, int id, const char caption[],
    int precision = 3, bool width_includes_label = true) :
    PropertyControlBase(id, event_handler, parent),
    controlCaption(caption),
    spinEdit(0.0f, 0.0f, 0.0f, precision),
    widthIncludesLabel(width_includes_label)
  {}

  virtual unsigned getTypeMaskForSet() const override
  {
    return CONTROL_DATA_TYPE_FLOAT | CONTROL_DATA_MIN_MAX_STEP | CONTROL_CAPTION | CONTROL_DATA_PREC;
  }

  virtual unsigned getTypeMaskForGet() const override { return CONTROL_DATA_TYPE_FLOAT; }

  virtual float getFloatValue() const override { return spinEdit.getValue(); }
  virtual void setFloatValue(float value) override { spinEdit.setValue(value); }
  virtual void setMinMaxStepValue(float min, float max, float step) override { spinEdit.setMinMaxStepValue(min, max, step); }
  virtual void setPrecValue(int prec) override { spinEdit.setPrecValue(prec); }
  virtual void setCaptionValue(const char value[]) override { controlCaption = value; }

  virtual void reset() override
  {
    setFloatValue(spinEdit.getMinValue());

    PropertyControlBase::reset();
  }

  virtual void setEnabled(bool enabled) override { controlEnabled = enabled; }

  virtual void updateImgui() override
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
        ImguiHelper::labelOnly(controlCaption);

        ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
      }
      else if (spaceBeforeSpinEdit > 0.0f)
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + spaceBeforeSpinEdit);
      else
        spinEditWidth = availableSpace;
    }
    else
    {
      ImguiHelper::labelOnly(controlCaption.begin(), controlCaption.end(), true);
      ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);

      const float availableSpace = mW > 0 ? min((float)mW, ImGui::GetContentRegionAvail().x) : ImGui::GetContentRegionAvail().x;
      spinEditWidth = availableSpace;
    }

    ImGui::SetNextItemWidth(spinEditWidth);
    setFocusToNextImGuiControlIfRequested();
    spinEdit.updateImgui(*this, &controlTooltip, this);
  }

private:
  String controlCaption;
  bool controlEnabled = true;
  const bool widthIncludesLabel;
  SpinEditControlStandalone spinEdit;
};

} // namespace PropPanel