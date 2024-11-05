// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "bhvProcessGesture.h"

#include <daRg/dag_element.h>
#include <daRg/dag_stringKeys.h>

#include "guiScene.h"
#include "scriptUtil.h"
#include "eventData.h"

#include "behaviorHelpers.h"

namespace darg
{
BhvProcessGesture bhv_process_gesture;
BhvProcessGesture::BhvProcessGesture() : Behavior(0, F_HANDLE_TOUCH) {}

int BhvProcessGesture::touchEvent(ElementTree * /*etree*/, Element *elem, InputEvent event, HumanInput::IGenPointing * /*pnt*/,
  int touch_idx, const HumanInput::PointingRawState::Touch &touch, int accum_res)
{
  return processGesture(elem, event, touch_idx, Point2(touch.x, touch.y), accum_res);
}

int BhvProcessGesture::processGesture(Element *elem, InputEvent event, int pointer_id, const Point2 &pos, int accum_res)
{
  auto gestureCb = [elem, accum_res](const GestureDetector::Info &info) {
    Sqrat::Function handler;
    const StringKeys *csk = elem->csk;

    if (info.phase == GestureDetector::Phase::BEGIN)
      handler = elem->props.scriptDesc.RawGetFunction(csk->onGestureBegin);
    else if (info.phase == GestureDetector::Phase::END)
      handler = elem->props.scriptDesc.RawGetFunction(csk->onGestureEnd);
    else if (info.phase == GestureDetector::Phase::ACTIVE)
      handler = elem->props.scriptDesc.RawGetFunction(csk->onGestureActive);

    if (handler.IsNull())
      return 0;

    HSQUIRRELVM vm = handler.GetVM();

    Sqrat::Table evt(vm);
    evt.SetValue("type", info.type);
    evt.SetValue("r0", info.r0);
    evt.SetValue("x0", info.x0);
    evt.SetValue("y0", info.y0);
    evt.SetValue("x", info.x);
    evt.SetValue("y", info.y);
    evt.SetValue("dx", info.deltaX);
    evt.SetValue("dy", info.deltaY);
    evt.SetValue("hit", elem->hitTest(Point2(info.x, info.y)));
    evt.SetValue("accumRes", accum_res);
    evt.SetValue(csk->target, elem->getRef(vm));

    switch (info.type)
    {
      case GestureDetector::Type::PINCH: evt.SetValue("scale", info.data.pinch.scale); break;
      case GestureDetector::Type::ROTATE: evt.SetValue("rotate", info.data.rotate.angle); break;
      case GestureDetector::Type::DRAG: evt.SetValue("drag", Point2(info.data.drag.x, info.data.drag.y)); break;
      default: break;
    }

    Sqrat::Object sqRes;
    if (handler.Evaluate(evt, sqRes))
      return sqRes.GetType() & SQOBJECT_NUMERIC ? sqRes.Cast<int>() : 0;
    else
      return 0;
  };

  int res = 0;
  switch (event)
  {
    case InputEvent::INP_EV_PRESS: res = gestureDetector.onTouchBegin(pointer_id, pos, gestureCb); break;
    case InputEvent::INP_EV_RELEASE: res = gestureDetector.onTouchEnd(pointer_id, pos, gestureCb); break;
    case InputEvent::INP_EV_POINTER_MOVE: res = gestureDetector.onTouchMove(pointer_id, pos, gestureCb); break;
    default: break;
  }

  return res;
}

void BhvProcessGesture::onElemSetup(Element *elem, SetupMode /* setup_mode */)
{
  gestureDetector.lim_drag_pointers_distance_max = elem->props.scriptDesc.RawGetSlotValue<float>(elem->csk->gestureDragDistanceMax,
    GestureDetector::LIM_DRAG_POINTERS_DISTANCE_MAX_DEFAULT);
  gestureDetector.lim_pinch_pointers_distance_min = elem->props.scriptDesc.RawGetSlotValue<float>(elem->csk->gesturePinchDistanceMin,
    GestureDetector::LIM_PINCH_POINTERS_DISTANCE_MIN_DEFAULT);
  gestureDetector.lim_rotate_pointers_distance_min = elem->props.scriptDesc.RawGetSlotValue<float>(elem->csk->gestureRotateDistanceMin,
    GestureDetector::LIM_ROTATE_POINTERS_DISTANCE_MIN_DEFAULT);
}

} // namespace darg