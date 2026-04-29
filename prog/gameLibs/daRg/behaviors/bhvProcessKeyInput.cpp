// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "bhvProcessKeyInput.h"

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


BhvProcessKeyInput bhv_process_key_input;


BhvProcessKeyInput::BhvProcessKeyInput() : Behavior(0, F_HANDLE_JOYSTICK | F_HANDLE_KEYBOARD_GLOBAL) {}


int BhvProcessKeyInput::sendKeyEvent(Element *elem, InputEvent event, InputDevice dev_id, int btn_idx, int accum_res)
{
  if (!elem->isInMainTree())
    return 0;

  const StringKeys *csk = elem->csk;
  Sqrat::Function handler;

  if (event == INP_EV_PRESS)
    handler = elem->props.scriptDesc.RawGetFunction(csk->onKeyPress);
  else if (event == INP_EV_RELEASE)
    handler = elem->props.scriptDesc.RawGetFunction(csk->onKeyRelease);
  if (handler.IsNull())
    return 0;

  HSQUIRRELVM vm = handler.GetVM();
  Sqrat::Table evt(vm);
  evt.SetValue(csk->devId, dev_id);
  evt.SetValue(csk->eventId, event);
  evt.SetValue("btnId", btn_idx);
  evt.SetValue("accumRes", accum_res);
  evt.SetValue(csk->target, elem->getRef(vm));

  auto sqRes = handler.Eval<Sqrat::Object>(evt);
  return (sqRes && sqRes.value().GetType() & SQOBJECT_NUMERIC) ? sqRes.value().Cast<int>() : 0;
}


int BhvProcessKeyInput::joystickBtnEvent(ElementTree * /*etree*/, Element *elem, const HumanInput::IGenJoystick *, InputEvent event,
  int btn_idx, int /*device_number*/, const HumanInput::ButtonBits & /*buttons*/, int accum_res)
{
  return sendKeyEvent(elem, event, DEVID_JOYSTICK, btn_idx, accum_res);
}


int BhvProcessKeyInput::kbdEvent(ElementTree * /*etree*/, Element *elem, InputEvent event, int key_idx, bool repeat, wchar_t /*wc*/,
  int accum_res)
{
  if (repeat)
    return 0;
  return sendKeyEvent(elem, event, DEVID_KEYBOARD, key_idx, accum_res);
}

} // namespace darg