// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "bhvEatInput.h"

#include <daRg/dag_element.h>

namespace darg
{


BhvEatInput bhv_eat_input;

BhvEatInput::BhvEatInput() : Behavior(darg::Behavior::STAGE_ACT, F_HANDLE_MOUSE | F_HANDLE_TOUCH) {}

int BhvEatInput::pointingEvent(ElementTree *, Element *elem, InputDevice, InputEvent event, int /*pointer_id*/, int /*data*/,
  Point2 pos, int /*accum_res*/)
{
  if (event != INP_EV_PRESS)
    return 0;

  if (elem->hitTest(pos))
  {
    return R_PROCESSED | R_STOPPED;
  }
  return 0;
}


} // namespace darg
