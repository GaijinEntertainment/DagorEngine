// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daRg/dag_behavior.h>


namespace darg
{


class BhvWheelScroll : public darg::Behavior
{
public:
  BhvWheelScroll();

  virtual int mouseEvent(ElementTree *, Element *, InputDevice /*device*/, InputEvent event, int pointer_id, int data, short mx,
    short my, int buttons, int /*accum_res*/);
};


extern BhvWheelScroll bhv_wheel_scroll;


} // namespace darg
