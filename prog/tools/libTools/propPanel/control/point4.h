// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/optional.h>
#include <propPanel/control/propertyControlBase.h>
#include <propPanel/colors.h>
#include <propPanel/imguiHelper.h>
#include <propPanel/immediateFocusLossHandler.h>
#include "spinEditStandalone.h"
#include "../scopedImguiBeginDisabled.h"

namespace PropPanel
{

class Point4PropertyControl : public PropertyControlBase, public IImmediateFocusLossHandler
{
public:
  Point4PropertyControl(ControlEventHandler *event_handler, ContainerPropertyControl *parent, int id, int x, int y, hdpi::Px w,
    const char caption[], int prec) :
    PropertyControlBase(id, event_handler, parent, x, y, w, hdpi::Px(0)),
    controlCaption(caption),
    spinEditX(0.0f, 0.0f, 0.0f, prec),
    spinEditY(0.0f, 0.0f, 0.0f, prec),
    spinEditZ(0.0f, 0.0f, 0.0f, prec),
    spinEditW(0.0f, 0.0f, 0.0f, prec)
  {
    Point4PropertyControl::setPoint4Value(Point4::ZERO);
  }

  ~Point4PropertyControl() override
  {
    if (get_focused_immediate_focus_loss_handler() == this)
      set_focused_immediate_focus_loss_handler(nullptr);
  }

  unsigned getTypeMaskForSet() const override
  {
    return CONTROL_DATA_TYPE_POINT4 | CONTROL_DATA_MIN_MAX_STEP | CONTROL_CAPTION | CONTROL_DATA_PREC;
  }

  unsigned getTypeMaskForGet() const override { return CONTROL_DATA_TYPE_POINT4; }

  Point4 getPoint4Value() const override { return controlValue; }

  void setPoint4Value(Point4 value) override
  {
    controlValue = value;
    spinEditX.setValue(value.x);
    spinEditY.setValue(value.y);
    spinEditZ.setValue(value.z);
    spinEditW.setValue(value.w);
  }

  void setMinMaxStepValue(float min, float max, float step) override
  {
    spinEditX.setMinMaxStepValue(min, max, step);
    spinEditY.setMinMaxStepValue(min, max, step);
    spinEditZ.setMinMaxStepValue(min, max, step);
    spinEditW.setMinMaxStepValue(min, max, step);
  }

  void setPrecValue(int prec) override
  {
    spinEditX.setPrecValue(prec);
    spinEditY.setPrecValue(prec);
    spinEditZ.setPrecValue(prec);
    spinEditW.setPrecValue(prec);
  }

  void setCaptionValue(const char value[]) override { controlCaption = value; }

  void setValueHighlight(ColorOverride::ColorIndex color) override { valueHighlightColor = color; }

  void reset() override
  {
    const float minValue = spinEditX.getMinValue();
    setPoint4Value(Point4(minValue, minValue, minValue, minValue));

    PropertyControlBase::reset();
  }

  void setEnabled(bool enabled) override { controlEnabled = enabled; }

  bool isDefaultValueSet() const override { return defaultValue ? *defaultValue == getPoint4Value() : true; }

  void applyDefaultValue() override
  {
    if (isDefaultValueSet())
    {
      return;
    }

    if (defaultValue)
    {
      setPoint4Value(*defaultValue);
      onWcChange(nullptr);
    }
  }

  void setDefaultValue(Variant var) override { defaultValue = var.convert<Point4>(); }

  void updateImgui() override
  {
    ScopedImguiBeginDisabled scopedDisabled(!controlEnabled);

    separateLineLabelWithTooltip(controlCaption.begin(), controlCaption.end());
    setFocusToNextImGuiControlIfRequested();

    // Override the background color of the edit box.
    const bool valueHighlightColorSet = valueHighlightColor != ColorOverride::NONE;
    if (valueHighlightColorSet)
      ImGui::PushStyleColor(ImGuiCol_FrameBg, getOverriddenColor(valueHighlightColor));

    ImGui::PushMultiItemsWidths(4, ImGui::GetContentRegionAvail().x);

    spinEditX.updateImgui(*this, &controlTooltip, this);
    ImGui::PopItemWidth();

    ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x); // NOTE: PushMultiItemsWidths calculated with ItemInnerSpacing.
    spinEditY.updateImgui(*this, &controlTooltip, this);
    ImGui::PopItemWidth();

    ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x); // NOTE: PushMultiItemsWidths calculated with ItemInnerSpacing.
    spinEditZ.updateImgui(*this, &controlTooltip, this);
    ImGui::PopItemWidth();

    ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x); // NOTE: PushMultiItemsWidths calculated with ItemInnerSpacing.
    spinEditW.updateImgui(*this, &controlTooltip, this);
    ImGui::PopItemWidth();

    if (valueHighlightColorSet)
      ImGui::PopStyleColor();

    if (spinEditX.isTextInputFocused() || spinEditY.isTextInputFocused() || spinEditZ.isTextInputFocused() ||
        spinEditW.isTextInputFocused())
      set_focused_immediate_focus_loss_handler(this);
    else if (get_focused_immediate_focus_loss_handler() == this)
      set_focused_immediate_focus_loss_handler(nullptr);
  }

private:
  void onWcChange(WindowBase *source) override
  {
    controlValue.x = spinEditX.getValue();
    controlValue.y = spinEditY.getValue();
    controlValue.z = spinEditZ.getValue();
    controlValue.w = spinEditW.getValue();

    PropertyControlBase::onWcChange(source);
  }

  void onImmediateFocusLoss() override
  {
    spinEditX.sendWcChangeIfVarChanged(*this);
    spinEditY.sendWcChangeIfVarChanged(*this);
    spinEditZ.sendWcChangeIfVarChanged(*this);
    spinEditW.sendWcChangeIfVarChanged(*this);
  }

  String controlCaption;
  Point4 controlValue = Point4::ZERO;
  bool controlEnabled = true;
  SpinEditControlStandalone spinEditX;
  SpinEditControlStandalone spinEditY;
  SpinEditControlStandalone spinEditZ;
  SpinEditControlStandalone spinEditW;
  eastl::optional<Point4> defaultValue;
  ColorOverride::ColorIndex valueHighlightColor = ColorOverride::NONE;
};

} // namespace PropPanel