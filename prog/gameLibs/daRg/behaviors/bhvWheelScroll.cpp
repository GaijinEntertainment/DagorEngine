#include "bhvWheelScroll.h"

#include <daRg/dag_element.h>
#include <daRg/dag_inputIds.h>
#include <daRg/dag_stringKeys.h>
#include <daRg/dag_properties.h>


namespace darg
{


BhvWheelScroll bhv_wheel_scroll;


BhvWheelScroll::BhvWheelScroll() : Behavior(0, F_HANDLE_MOUSE | F_ALLOW_AUTO_SCROLL) {}


int BhvWheelScroll::mouseEvent(ElementTree * /*etree*/, Element *elem, InputDevice /*device*/, InputEvent event, int /*pointer_id*/,
  int data, short mx, short my, int /*buttons*/, int accum_res)
{
  int result = 0;

  if (event == INP_EV_MOUSE_WHEEL && !(accum_res & R_PROCESSED) && elem->hitTest(mx, my))
  {
    const float wheelStep = elem->props.scriptDesc.RawGetSlotValue("wheelStep", 0.2f);
    if (elem->props.scriptDesc.RawGetSlotValue(elem->csk->orientation, O_VERTICAL) == O_VERTICAL)
      elem->scroll(Point2(0, -data * wheelStep));
    else
      elem->scroll(Point2(-data * wheelStep, 0));

    result = R_PROCESSED;
  }

  return result;
}


} // namespace darg