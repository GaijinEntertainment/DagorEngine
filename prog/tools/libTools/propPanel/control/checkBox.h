// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <propPanel/control/propertyControlBase.h>
#include "../scopedImguiBeginDisabled.h"

namespace PropPanel
{

class CheckBoxPropertyControl : public PropertyControlBase
{
public:
  CheckBoxPropertyControl(ControlEventHandler *event_handler, ContainerPropertyControl *parent, int id, int x, int y, hdpi::Px w,
    const char caption[]) :
    PropertyControlBase(id, event_handler, parent, x, y, w, hdpi::Px(0)), controlCaption(caption)
  {}

  virtual unsigned getTypeMaskForSet() const override { return CONTROL_DATA_TYPE_BOOL | CONTROL_CAPTION; }
  virtual unsigned getTypeMaskForGet() const override { return CONTROL_DATA_TYPE_BOOL; }

  virtual void setCaptionValue(const char value[]) override { controlCaption = value; }

  virtual void setBoolValue(bool value) override { controlValue = value; }
  virtual bool getBoolValue() const override { return controlValue; }

  virtual void reset() override
  {
    setBoolValue(false);

    PropertyControlBase::reset();
  }

  virtual void setEnabled(bool enabled) override { controlEnabled = enabled; }

  virtual void updateImgui() override
  {
    ScopedImguiBeginDisabled scopedDisabled(!controlEnabled);

    setFocusToNextImGuiControlIfRequested();

    const bool clicked = ImGui::Checkbox(controlCaption, &controlValue);

    setPreviousImguiControlTooltip();

    if (clicked)
    {
      onWcClick(nullptr);
      onWcChange(nullptr);
    }
  }

private:
  String controlCaption;
  bool controlValue = false;
  bool controlEnabled = true;
};

} // namespace PropPanel