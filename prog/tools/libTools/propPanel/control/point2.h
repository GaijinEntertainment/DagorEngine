// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <propPanel/control/propertyControlBase.h>
#include <propPanel/imguiHelper.h>
#include "spinEditStandalone.h"
#include "../scopedImguiBeginDisabled.h"

namespace PropPanel
{

class Point2PropertyControl : public PropertyControlBase
{
public:
  Point2PropertyControl(ControlEventHandler *event_handler, ContainerPropertyControl *parent, int id, int x, int y, hdpi::Px w,
    const char caption[], int prec) :
    PropertyControlBase(id, event_handler, parent, x, y, w, hdpi::Px(0)),
    controlCaption(caption),
    spinEditX(0.0f, 0.0f, 0.0f, prec),
    spinEditY(0.0f, 0.0f, 0.0f, prec)
  {
    Point2PropertyControl::setPoint2Value(Point2::ZERO);
  }

  virtual unsigned getTypeMaskForSet() const override { return CONTROL_DATA_TYPE_POINT2 | CONTROL_CAPTION | CONTROL_DATA_PREC; }
  virtual unsigned getTypeMaskForGet() const override { return CONTROL_DATA_TYPE_POINT2; }

  virtual Point2 getPoint2Value() const override { return controlValue; }

  virtual void setPoint2Value(Point2 value) override
  {
    controlValue = value;
    spinEditX.setValue(value.x);
    spinEditY.setValue(value.y);
  }

  virtual void setPrecValue(int prec) override
  {
    spinEditX.setPrecValue(prec);
    spinEditY.setPrecValue(prec);
  }

  virtual void setCaptionValue(const char value[]) override { controlCaption = value; }

  virtual void reset() override
  {
    setPoint2Value(Point2::ZERO);

    PropertyControlBase::reset();
  }

  virtual void setEnabled(bool enabled) override { controlEnabled = enabled; }

  virtual void updateImgui() override
  {
    ScopedImguiBeginDisabled scopedDisabled(!controlEnabled);

    ImguiHelper::separateLineLabel(controlCaption);
    setFocusToNextImGuiControlIfRequested();

    ImGui::PushMultiItemsWidths(2, ImGui::GetContentRegionAvail().x);

    spinEditX.updateImgui(*this, &controlTooltip, this);
    ImGui::PopItemWidth();

    ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x); // NOTE: PushMultiItemsWidths calculated with ItemInnerSpacing.
    spinEditY.updateImgui(*this, &controlTooltip, this);
    ImGui::PopItemWidth();
  }

private:
  virtual void onWcChange(WindowBase *source) override
  {
    controlValue.x = spinEditX.getValue();
    controlValue.y = spinEditY.getValue();

    PropertyControlBase::onWcChange(source);
  }

  String controlCaption;
  Point2 controlValue = Point2::ZERO;
  bool controlEnabled = true;
  SpinEditControlStandalone spinEditX;
  SpinEditControlStandalone spinEditY;
};

} // namespace PropPanel