// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daRg/dag_behavior.h>
#include <gesture/gestureDetector.h>

/*
This is a thin interface to internal daRg touch device gesture handling.
It allows to implement arbitrary gesture processing in script.

One should be careful when using this behavior:

* Handler functions (onGestureBegin/onGestureEnd/onGestureActive)
  must be lightweight, because they are called during internal input
  processing, not in a separate script-specific step

* Consistent state should be maintained in script - that means
  properly checking 'hit' and 'accumRes' flags and returning
  correct code (R_PROCESSED or 0/null)

See dagor4\tools\dargbox\samples\_basic\behaviors\process_gesture.ui.nut for demo
*/

namespace darg
{

class BhvProcessGesture : public darg::Behavior
{
public:
  BhvProcessGesture();

  virtual int touchEvent(ElementTree *, Element *, InputEvent /*event*/, HumanInput::IGenPointing * /*pnt*/, int /*touch_idx*/,
    const HumanInput::PointingRawState::Touch & /*touch*/, int /*accum_res*/) override;

private:
  int processGesture(Element *elem, InputEvent event, int pointer_id, const Point2 &pos, int accum_res);
  void onElemSetup(Element *elem, SetupMode setup_mode) override;

  GestureDetector gestureDetector;
};

extern BhvProcessGesture bhv_process_gesture;

} // namespace darg
