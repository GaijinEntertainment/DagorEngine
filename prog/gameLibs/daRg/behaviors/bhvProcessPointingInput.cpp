// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "bhvProcessPointingInput.h"

#include <daRg/dag_element.h>
#include <daRg/dag_stringKeys.h>

#include "guiScene.h"
#include "scriptUtil.h"
#include "eventData.h"

#include "behaviorHelpers.h"
#include <startup/dag_inpDevClsDrv.h>
#include <drv/hid/dag_hiKeyboard.h>

namespace darg
{


BhvProcessPointingInput bhv_process_pointing_input;


BhvProcessPointingInput::BhvProcessPointingInput() : Behavior(0, F_HANDLE_KEYBOARD_GLOBAL | F_HANDLE_MOUSE | F_HANDLE_TOUCH) {}


int BhvProcessPointingInput::pointerEvent(ElementTree * /*etree*/, Element *elem, InputDevice device, InputEvent event, int pointer_id,
  int btn_id, const Point2 &pos, int accum_res)
{
  Sqrat::Function handler;

  const StringKeys *csk = elem->csk;

  if (event == INP_EV_PRESS)
    handler = elem->props.scriptDesc.RawGetFunction(csk->onPointerPress);
  else if (event == INP_EV_RELEASE)
    handler = elem->props.scriptDesc.RawGetFunction(csk->onPointerRelease);
  else if (event == INP_EV_POINTER_MOVE)
    handler = elem->props.scriptDesc.RawGetFunction(csk->onPointerMove);

  if (handler.IsNull())
    return 0;

  HSQUIRRELVM vm = handler.GetVM();

  Sqrat::Table evt(vm);
  evt.SetValue(csk->devId, device);
  evt.SetValue(csk->eventId, event);
  evt.SetValue("pointerId", pointer_id);
  evt.SetValue(csk->btnId, btn_id);
  evt.SetValue("x", pos.x);
  evt.SetValue("y", pos.y);
  evt.SetValue("hit", elem->hitTest(pos));
  evt.SetValue("accumRes", accum_res);
  evt.SetValue(csk->target, elem->getRef(vm));

  const HumanInput::IGenKeyboard *kbd = global_cls_drv_kbd ? global_cls_drv_kbd->getDevice(0) : NULL;
  if (kbd)
  {
    int modifiers = kbd->getRawState().shifts;
    evt.SetValue("shiftKey", (modifiers & HumanInput::KeyboardRawState::SHIFT_BITS) != 0);
    evt.SetValue("ctrlKey", (modifiers & HumanInput::KeyboardRawState::CTRL_BITS) != 0);
    evt.SetValue("altKey", (modifiers & HumanInput::KeyboardRawState::ALT_BITS) != 0);
  }
  else
  {
    evt.SetValue("shiftKey", false);
    evt.SetValue("ctrlKey", false);
    evt.SetValue("altKey", false);
  }

  Sqrat::Object sqRes;
  if (handler.Evaluate(evt, sqRes))
    return sqRes.GetType() & SQOBJECT_NUMERIC ? sqRes.Cast<int>() : 0;
  else
    return 0;
}


int BhvProcessPointingInput::mouseEvent(ElementTree *etree, Element *elem, InputDevice device, InputEvent event, int pointer_id,
  int data, short mx, short my, int /*buttons*/, int accum_res)
{
  if (event == INP_EV_PRESS || event == INP_EV_RELEASE || event == INP_EV_POINTER_MOVE)
    return pointerEvent(etree, elem, device, event, pointer_id, data, Point2(mx, my), accum_res);
  else
    return 0;
}


int BhvProcessPointingInput::touchEvent(ElementTree *etree, Element *elem, InputEvent event, HumanInput::IGenPointing * /*pnt*/,
  int touch_idx, const HumanInput::PointingRawState::Touch &touch, int accum_res)
{
  return pointerEvent(etree, elem, DEVID_TOUCH, event, touch_idx, 0, Point2(touch.x, touch.y), accum_res);
}


} // namespace darg