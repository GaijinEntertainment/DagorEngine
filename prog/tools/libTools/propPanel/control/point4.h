// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <propPanel/control/propertyControlBase.h>
#include <propPanel/imguiHelper.h>
#include "spinEditStandalone.h"
#include "../scopedImguiBeginDisabled.h"

namespace PropPanel
{

class Point4PropertyControl : public PropertyControlBase
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

  virtual unsigned getTypeMaskForSet() const override { return CONTROL_DATA_TYPE_POINT4 | CONTROL_CAPTION | CONTROL_DATA_PREC; }
  virtual unsigned getTypeMaskForGet() const override { return CONTROL_DATA_TYPE_POINT4; }

  virtual Point4 getPoint4Value() const override { return controlValue; }

  virtual void setPoint4Value(Point4 value) override
  {
    spinEditX.setValue(value.x);
    spinEditY.setValue(value.y);
    spinEditZ.setValue(value.z);
    spinEditW.setValue(value.w);
  }

  virtual void setPrecValue(int prec) override
  {
    spinEditX.setPrecValue(prec);
    spinEditY.setPrecValue(prec);
    spinEditZ.setPrecValue(prec);
    spinEditW.setPrecValue(prec);
  }

  virtual void setCaptionValue(const char value[]) override { controlCaption = value; }

  virtual void reset() override
  {
    setPoint4Value(Point4::ZERO);

    PropertyControlBase::reset();
  }

  virtual void setEnabled(bool enabled) override { controlEnabled = enabled; }

  virtual void updateImgui() override
  {
    ScopedImguiBeginDisabled scopedDisabled(!controlEnabled);

    ImguiHelper::separateLineLabel(controlCaption);
    setFocusToNextImGuiControlIfRequested();

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
  }

private:
  virtual void onWcChange(WindowBase *source) override
  {
    controlValue.x = spinEditX.getValue();
    controlValue.y = spinEditY.getValue();
    controlValue.z = spinEditZ.getValue();
    controlValue.w = spinEditW.getValue();

    PropertyControlBase::onWcChange(source);
  }

  String controlCaption;
  Point4 controlValue = Point4::ZERO;
  bool controlEnabled = true;
  SpinEditControlStandalone spinEditX;
  SpinEditControlStandalone spinEditY;
  SpinEditControlStandalone spinEditZ;
  SpinEditControlStandalone spinEditW;
};

} // namespace PropPanel