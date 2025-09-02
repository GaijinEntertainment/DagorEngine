// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <propPanel/control/customControl.h>
#include <propPanel/control/propertyControlBase.h>

namespace PropPanel
{

class CustomControlHolderPropertyControl : public PropertyControlBase
{
public:
  CustomControlHolderPropertyControl(ControlEventHandler *event_handler, ContainerPropertyControl *parent, int id, int x, int y,
    hdpi::Px w, ICustomControl *in_control) :
    PropertyControlBase(id, event_handler, parent, x, y, w, hdpi::Px(0)), control(in_control)
  {}

  unsigned getTypeMaskForSet() const override { return 0; }
  unsigned getTypeMaskForGet() const override { return 0; }

  void updateImgui() override
  {
    if (control)
      control->customControlUpdate(mId);
  }

private:
  ICustomControl *control;
};

} // namespace PropPanel