// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daRg/dag_behavior.h>


namespace darg
{


class BhvWheelScroll : public darg::Behavior
{
public:
  BhvWheelScroll();

  virtual void onDetach(Element *, DetachMode) override;
  virtual int pointingEvent(ElementTree *, Element *, InputDevice /*device*/, InputEvent event, int pointer_id, int data, Point2 pos,
    int /*accum_res*/) override;
};


extern BhvWheelScroll bhv_wheel_scroll;


} // namespace darg
