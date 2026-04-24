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

class Point3PropertyControl : public PropertyControlBase, public IImmediateFocusLossHandler
{
public:
  Point3PropertyControl(ControlEventHandler *event_handler, ContainerPropertyControl *parent, int id, int x, int y, hdpi::Px w,
    const char caption[], int prec) :
    PropertyControlBase(id, event_handler, parent, x, y, w, hdpi::Px(0)),
    controlCaption(caption),
    spinEditX(0.0f, 0.0f, 0.0f, prec),
    spinEditY(0.0f, 0.0f, 0.0f, prec),
    spinEditZ(0.0f, 0.0f, 0.0f, prec)
  {
    Point3PropertyControl::setPoint3Value(Point3::ZERO);
  }

  ~Point3PropertyControl() override
  {
    if (get_focused_immediate_focus_loss_handler() == this)
      set_focused_immediate_focus_loss_handler(nullptr);
  }

  unsigned getTypeMaskForSet() const override
  {
    return CONTROL_DATA_TYPE_POINT3 | CONTROL_DATA_MIN_MAX_STEP | CONTROL_CAPTION | CONTROL_DATA_PREC;
  }

  unsigned getTypeMaskForGet() const override { return CONTROL_DATA_TYPE_POINT3; }

  Point3 getPoint3Value() const override { return controlValue; }

  void setPoint3Value(Point3 value) override
  {
    controlValue = value;
    spinEditX.setValue(value.x);
    spinEditY.setValue(value.y);
    spinEditZ.setValue(value.z);
  }

  void setMinMaxStepValue(float min, float max, float step) override
  {
    spinEditX.setMinMaxStepValue(min, max, step);
    spinEditY.setMinMaxStepValue(min, max, step);
    spinEditZ.setMinMaxStepValue(min, max, step);
  }

  void setPrecValue(int prec) override
  {
    spinEditX.setPrecValue(prec);
    spinEditY.setPrecValue(prec);
    spinEditZ.setPrecValue(prec);
  }

  void setCaptionValue(const char value[]) override { controlCaption = value; }

  void setValueHighlight(ColorOverride::ColorIndex color) override { valueHighlightColor = color; }

  void reset() override
  {
    const float minValue = spinEditX.getMinValue();
    setPoint3Value(Point3(minValue, minValue, minValue));

    PropertyControlBase::reset();
  }

  void setEnabled(bool enabled) override { controlEnabled = enabled; }

  bool isDefaultValueSet() const override { return defaultValue ? *defaultValue == getPoint3Value() : true; }

  void applyDefaultValue() override
  {
    if (isDefaultValueSet())
    {
      return;
    }

    if (defaultValue)
    {
      setPoint3Value(*defaultValue);
      onWcChange(nullptr);
    }
  }

  void setDefaultValue(Variant var) override { defaultValue = var.convert<Point3>(); }

  void updateImgui() override
  {
    ScopedImguiBeginDisabled scopedDisabled(!controlEnabled);

    separateLineLabelWithTooltip(controlCaption.begin(), controlCaption.end());
    setFocusToNextImGuiControlIfRequested();

    // Override the background color of the edit box.
    const bool valueHighlightColorSet = valueHighlightColor != ColorOverride::NONE;
    if (valueHighlightColorSet)
      ImGui::PushStyleColor(ImGuiCol_FrameBg, getOverriddenColor(valueHighlightColor));

    ImGui::PushMultiItemsWidths(3, ImGui::GetContentRegionAvail().x);

    spinEditX.updateImgui(*this, &controlTooltip, this);
    ImGui::PopItemWidth();

    ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x); // NOTE: PushMultiItemsWidths calculated with ItemInnerSpacing.
    spinEditY.updateImgui(*this, &controlTooltip, this);
    ImGui::PopItemWidth();

    ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x); // NOTE: PushMultiItemsWidths calculated with ItemInnerSpacing.
    spinEditZ.updateImgui(*this, &controlTooltip, this);
    ImGui::PopItemWidth();

    if (valueHighlightColorSet)
      ImGui::PopStyleColor();

    if (spinEditX.isTextInputFocused() || spinEditY.isTextInputFocused() || spinEditZ.isTextInputFocused())
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

    PropertyControlBase::onWcChange(source);
  }

  void onImmediateFocusLoss() override
  {
    spinEditX.sendWcChangeIfVarChanged(*this);
    spinEditY.sendWcChangeIfVarChanged(*this);
    spinEditZ.sendWcChangeIfVarChanged(*this);
  }

  String controlCaption;
  Point3 controlValue = Point3::ZERO;
  bool controlEnabled = true;
  SpinEditControlStandalone spinEditX;
  SpinEditControlStandalone spinEditY;
  SpinEditControlStandalone spinEditZ;
  eastl::optional<Point3> defaultValue;
  ColorOverride::ColorIndex valueHighlightColor = ColorOverride::NONE;
};

} // namespace PropPanel