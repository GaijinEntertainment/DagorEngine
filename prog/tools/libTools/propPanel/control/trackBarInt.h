// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <propPanel/control/propertyControlBase.h>
#include <propPanel/constants.h>
#include <propPanel/imguiHelper.h>
#include "spinEditStandalone.h"
#include "../scopedImguiBeginDisabled.h"

namespace PropPanel
{

class TrackBarIntPropertyControl : public PropertyControlBase
{
public:
  TrackBarIntPropertyControl(ControlEventHandler *event_handler, ContainerPropertyControl *parent, int id, int x, int y, hdpi::Px w,
    const char caption[], int min, int max, int step) :
    PropertyControlBase(id, event_handler, parent, x, y, w, hdpi::Px(0)), controlCaption(caption), spinEdit(min, max, step, 0)
  {}

  virtual unsigned getTypeMaskForSet() const override { return CONTROL_DATA_TYPE_INT | CONTROL_DATA_MIN_MAX_STEP | CONTROL_CAPTION; }
  virtual unsigned getTypeMaskForGet() const override { return CONTROL_DATA_TYPE_INT; }

  virtual int getIntValue() const override { return spinEdit.getValue(); }

  virtual void setIntValue(int value) override
  {
    const int step = spinEdit.getStepValue();
    const int min = spinEdit.getMinValue();
    const int max = spinEdit.getMaxValue();

    value = clamp(value, min, max);
    if (step != 0)
      value = min + (((value - min) / step) * step);

    spinEdit.setValue(value);
  }

  virtual void setMinMaxStepValue(float min, float max, float step) override
  {
    spinEdit.setMinMaxStepValue(floor(min), ceil(max), ceil(step));
  }

  virtual void setCaptionValue(const char value[]) override { controlCaption = value; }

  virtual void reset() override
  {
    setIntValue(floor(spinEdit.getMinValue()));

    PropertyControlBase::reset();
  }

  virtual void setEnabled(bool enabled) override { controlEnabled = enabled; }

  virtual void updateImgui() override
  {
    ScopedImguiBeginDisabled scopedDisabled(!controlEnabled);

    ImguiHelper::separateLineLabel(controlCaption);

    const float remainingWidthAfterTheLabel = ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemInnerSpacing.x;
    const float spinEditWidth = ImguiHelper::getDefaultRightSideEditWidth();
    const float sliderWidth = remainingWidthAfterTheLabel - spinEditWidth;
    if (sliderWidth >= hdpi::_pxS(Constants::MINIMUM_TRACK_BAR_SLIDER_WIDTH_TO_SHOW))
    {
      ImGui::SetNextItemWidth(sliderWidth);

      int value = spinEdit.getValue();
      const bool changed =
        ImGui::SliderInt("##s", &value, spinEdit.getMinValue(), spinEdit.getMaxValue(), "", ImGuiSliderFlags_NoInput);

      setPreviousImguiControlTooltip();

      if (changed)
      {
        // This is here to add stepping. (ImGui currently does not support that. See: https://github.com/ocornut/imgui/issues/1183.)
        setIntValue(value);

        onWcChange(nullptr);
      }

      ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
      ImGui::SetNextItemWidth(spinEditWidth);
    }
    else
    {
      ImGui::SetNextItemWidth(-FLT_MIN);
    }

    setFocusToNextImGuiControlIfRequested();
    spinEdit.updateImgui(*this, &controlTooltip, this);
  }

private:
  String controlCaption;
  bool controlEnabled = true;
  SpinEditControlStandalone spinEdit;
};

} // namespace PropPanel