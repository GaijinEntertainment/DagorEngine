#include "bhvEatInput.h"

#include <daRg/dag_element.h>

namespace darg
{


BhvEatInput bhv_eat_input;

BhvEatInput::BhvEatInput() : Behavior(darg::Behavior::STAGE_ACT, F_HANDLE_MOUSE | F_HANDLE_TOUCH) {}

int BhvEatInput::touchEvent(ElementTree *, Element *elem, InputEvent event, HumanInput::IGenPointing * /*pnt*/, int /*touch_idx*/,
  const HumanInput::PointingRawState::Touch &touch, int /*accum_res*/)
{
  if (event != INP_EV_PRESS)
    return 0;

  if (elem->hitTest(touch.x, touch.y))
  {
    return R_PROCESSED;
  }
  return 0;
}


int BhvEatInput::mouseEvent(ElementTree *, Element *elem, InputDevice, InputEvent event, int /*pointer_id*/, int /*data*/, short mx,
  short my, int /*buttons*/, int /*accum_res*/)
{
  if (event != INP_EV_PRESS)
    return 0;

  if (elem->hitTest(mx, my))
  {
    return R_PROCESSED;
  }
  return 0;
}


} // namespace darg
