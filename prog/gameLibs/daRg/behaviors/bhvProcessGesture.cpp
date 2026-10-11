// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "bhvProcessGesture.h"

#include <daRg/dag_element.h>
#include <daRg/dag_stringKeys.h>
#include <gesture/gestureDetector.h>

#include "guiScene.h"
#include "scriptUtil.h"
#include "eventData.h"

#include "behaviorHelpers.h"

namespace darg
{
BhvProcessGesture bhv_process_gesture;
BhvProcessGesture::BhvProcessGesture() : Behavior(0, F_HANDLE_TOUCH | F_HANDLE_TOUCH_PAD_GESTURE) {}

static int call_gesture_handler(Element *elem, const Sqrat::Function &handler, InputDevice device, const TouchPadGesture &gesture,
  float r0, int accum_res)
{
  HSQUIRRELVM vm = handler.GetVM();
  const StringKeys *csk = elem->csk;

  EventDataRect targetRect;
  calc_elem_rect_for_event_handler(targetRect, elem);

  Sqrat::Table evt(vm);
  evt.SetValue("type", gesture.type);
  evt.SetValue(csk->devId, device);
  evt.SetValue("r0", r0);
  evt.SetValue("x0", gesture.pos0.x);
  evt.SetValue("y0", gesture.pos0.y);
  evt.SetValue(csk->x, gesture.pos.x);
  evt.SetValue(csk->y, gesture.pos.y);
  evt.SetValue("dx", gesture.delta.x);
  evt.SetValue("dy", gesture.delta.y);
  evt.SetValue(csk->hit, elem->hitTest(gesture.pos));
  evt.SetValue(csk->accumRes, accum_res);
  evt.SetValue(csk->target, elem->getRef(vm));
  evt.SetValue("targetRect", targetRect);

  switch (gesture.type)
  {
    case GestureDetector::Type::PINCH:
      evt.SetValue(csk->scale, gesture.scale);
      evt.SetValue("dscale", gesture.deltaScale);
      break;
    case GestureDetector::Type::ROTATE:
      evt.SetValue(csk->rotate, gesture.angle);
      evt.SetValue("drotate", gesture.deltaAngle);
      break;
    case GestureDetector::Type::DRAG: evt.SetValue(csk->drag, gesture.drag); break;
    default: break;
  }

  auto sqRes = handler.Eval<Sqrat::Object>(evt);
  return (sqRes && sqRes.value().GetType() & SQOBJECT_NUMERIC) ? sqRes.value().Cast<int>() : 0;
}

int BhvProcessGesture::pointingEvent(ElementTree * /*etree*/, Element *elem, InputDevice, InputEvent event, int touch_idx, int,
  Point2 pos, int accum_res)
{
  return processGesture(elem, event, touch_idx, pos, accum_res);
}

int BhvProcessGesture::processGesture(Element *elem, InputEvent event, int pointer_id, const Point2 &pos, int accum_res)
{
  GestureDetector *gestureDetector = elem->props.storage.RawGetSlotValue<GestureDetector *>(elem->csk->gestureData, nullptr);
  G_ASSERT_RETURN(gestureDetector, 0);

  auto gestureCb = [elem, accum_res](const GestureDetector::Info &info) {
    const StringKeys *csk = elem->csk;
    Sqrat::Function handler;

    if (info.phase == GestureDetector::Phase::BEGIN)
      handler = elem->props.scriptDesc.RawGetFunction(csk->onGestureBegin);
    else if (info.phase == GestureDetector::Phase::END)
      handler = elem->props.scriptDesc.RawGetFunction(csk->onGestureEnd);
    else if (info.phase == GestureDetector::Phase::ACTIVE)
      handler = elem->props.scriptDesc.RawGetFunction(csk->onGestureActive);

    if (handler.IsNull())
      return 0;

    TouchPadGesture gesture;
    gesture.type = info.type;
    gesture.pos.set(info.x, info.y);
    gesture.pos0.set(info.x0, info.y0);
    gesture.delta.set(info.deltaX, info.deltaY);
    switch (info.type)
    {
      case GestureDetector::Type::PINCH:
        gesture.scale = info.data.pinch.scale;
        gesture.deltaScale = info.data.pinch.deltaScale;
        break;
      case GestureDetector::Type::ROTATE:
        gesture.angle = info.data.rotate.angle;
        gesture.deltaAngle = info.data.rotate.deltaAngle;
        break;
      case GestureDetector::Type::DRAG: gesture.drag.set(info.data.drag.x, info.data.drag.y); break;
      default: break;
    }
    return call_gesture_handler(elem, handler, DEVID_TOUCH, gesture, info.r0, accum_res);
  };

  int res = 0;
  switch (event)
  {
    case InputEvent::INP_EV_PRESS: res = gestureDetector->onTouchBegin(pointer_id, pos, gestureCb); break;
    case InputEvent::INP_EV_RELEASE: res = gestureDetector->onTouchEnd(pointer_id, pos, gestureCb); break;
    case InputEvent::INP_EV_POINTER_MOVE: res = gestureDetector->onTouchMove(pointer_id, pos, gestureCb); break;
    default: break;
  }

  return res;
}

int BhvProcessGesture::touchPadGestureEvent(ElementTree *, Element *elem, int state, const TouchPadGesture &gesture, int accum_res)
{
  const bool isTap = gesture.type & (GestureDetector::Type::TAP | GestureDetector::Type::DOUBLE_TAP);
  const bool isHold = gesture.type == GestureDetector::Type::HOLD;
  const StringKeys *csk = elem->csk;

  const Sqrat::Object *handlerKey = nullptr;
  if (state == HumanInput::Gesture::BEGIN && !isTap)
    handlerKey = &csk->onGestureBegin;
  else if (state == HumanInput::Gesture::ACTIVE && !isTap && !isHold)
    handlerKey = &csk->onGestureActive;
  else if (state == HumanInput::Gesture::END || (state == HumanInput::Gesture::CANCELLED && !isTap))
    handlerKey = &csk->onGestureEnd;

  Sqrat::Function handler = handlerKey ? elem->props.scriptDesc.RawGetFunction(*handlerKey) : Sqrat::Function();
  if (!handler.IsNull())
    return call_gesture_handler(elem, handler, DEVID_JOYSTICK, gesture, 0, accum_res);
  return (state == HumanInput::Gesture::BEGIN && !(accum_res & R_PROCESSED)) ? R_PROCESSED : 0;
}

int BhvProcessGesture::getTouchPadGestures(const Element *elem) { return elem->props.getInt<int>(elem->csk->touchPadGestures, 0); }

void BhvProcessGesture::onElemSetup(Element *elem, SetupMode /* setup_mode */)
{
  // new elements run onElemSetup before onAttach
  GestureDetector *gestureDetector = elem->props.storage.RawGetSlotValue<GestureDetector *>(elem->csk->gestureData, nullptr);
  if (!gestureDetector)
  {
    gestureDetector = new GestureDetector();
    elem->props.storage.SetValue(elem->csk->gestureData, gestureDetector);
  }

  gestureDetector->lim_drag_pointers_distance_max = elem->props.scriptDesc.RawGetSlotValue<float>(elem->csk->gestureDragDistanceMax,
    GestureDetector::LIM_DRAG_POINTERS_DISTANCE_MAX_DEFAULT);
  gestureDetector->lim_pinch_pointers_distance_min = elem->props.scriptDesc.RawGetSlotValue<float>(elem->csk->gesturePinchDistanceMin,
    GestureDetector::LIM_PINCH_POINTERS_DISTANCE_MIN_DEFAULT);
  gestureDetector->lim_rotate_pointers_distance_min = elem->props.scriptDesc.RawGetSlotValue<float>(
    elem->csk->gestureRotateDistanceMin, GestureDetector::LIM_ROTATE_POINTERS_DISTANCE_MIN_DEFAULT);
}

void BhvProcessGesture::onDetach(Element *elem, DetachMode)
{
  GestureDetector *gestureDetector = elem->props.storage.RawGetSlotValue<GestureDetector *>(elem->csk->gestureData, nullptr);
  if (gestureDetector)
  {
    elem->props.storage.DeleteSlot(elem->csk->gestureData);
    delete gestureDetector;
  }
}

} // namespace darg
