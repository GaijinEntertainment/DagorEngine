#pragma once

#include <daRg/dag_behavior.h>

/*
This is a thin interface to internal daRg pointing device input handling.
It allows to implement arbitrary input processing in script.

One should be careful when using this behavior:

* Handler functions (onPointerPress/onPointerRelease/onPointerMove)
  must be lightweight, because they are called during internal input
  processing, not in a separate script-specific step

* Consistent state should be maintained in script - that means
  properly checking 'hit' and 'accumRes' flags, paying attention
  to devId/pointerId/btnId in different events and returning
  correct code (R_PROCESSED or 0/null)

See dagor4\tools\dargbox\samples\_basic\behaviors\process_pointing_input.ui.nut
for demo
*/

namespace darg
{


class BhvProcessPointingInput : public darg::Behavior
{
public:
  BhvProcessPointingInput();

  virtual int mouseEvent(ElementTree *, Element *, InputDevice /*device*/, InputEvent event, int pointer_id, int data, short mx,
    short my, int buttons, int /*accum_res*/) override;

  virtual int touchEvent(ElementTree *, Element *, InputEvent /*event*/, HumanInput::IGenPointing * /*pnt*/, int /*touch_idx*/,
    const HumanInput::PointingRawState::Touch & /*touch*/, int /*accum_res*/) override;

private:
  int pointerEvent(ElementTree *etree, Element *elem, InputDevice device, InputEvent event, int pointer_id, int btn_id,
    const Point2 &pos, int accum_res);
};


extern BhvProcessPointingInput bhv_process_pointing_input;


} // namespace darg
