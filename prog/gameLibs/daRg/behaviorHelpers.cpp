// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "behaviorHelpers.h"
#include "sceneConfig.h"
#include "guiScene.h"

#include <daRg/dag_element.h>
#include <daRg/dag_stringKeys.h>
#include <daRg/dag_scriptHandlers.h>

#include <startup/dag_globalSettings.h>
#include <sqrat.h>

#include "dargDebugUtils.h"
#include "scriptUtil.h"
#include "eventData.h"

#include <drv/hid/dag_hiKeybIds.h>
#include <drv/hid/dag_hiKeyboard.h>
#include <startup/dag_inpDevClsDrv.h>

#include <forceFeedback/forceFeedback.h>

#if DARG_WITH_SQDEBUGGER
#include <sqDebugger/sqDebugger.h>
#endif


namespace darg
{

void calc_elem_rect_for_event_handler(EventDataRect &rect, Element *elem)
{
  BBox2 bbox = elem->calcTransformedBbox();
  if (!elem->transformedBbox.isempty())
  {
    rect.l = bbox.left();
    rect.t = bbox.top();
    rect.r = bbox.right();
    rect.b = bbox.bottom();
  }
  else
    rect.l = rect.t = rect.r = rect.b = -1;
}


static void do_rumble(const SceneConfig &config)
{
  force_feedback::rumble::EventParams evt;
  evt.durationMsec = config.clickRumbleDuration * 1000;
  if (config.clickRumbleLoFreq > 0.0f)
  {
    evt.band = evt.LO;
    evt.freq = config.clickRumbleLoFreq;
    force_feedback::rumble::add_event(evt);
  }
  if (config.clickRumbleHiFreq > 0.0f)
  {
    evt.band = evt.HI;
    evt.freq = config.clickRumbleHiFreq;
    force_feedback::rumble::add_event(evt);
  }
}

void call_click_handler(IGuiScene *scene, Element *elem, InputDevice dev_id, bool dbl, int mouse_btn, short mx, short my)
{
  if (!::dgs_app_active) // quick fix for buttons pressed by joystick 'click' key when window is in background
    return;

  const Sqrat::Object *slot = dbl ? &elem->csk->onDoubleClick : &elem->csk->onClick;
  Sqrat::Function f = elem->props.scriptDesc.RawGetFunction(*slot);

  if (dbl && f.IsNull())
  {
    f = elem->props.scriptDesc.RawGetFunction(elem->csk->onClick);
    slot = &elem->csk->onClick;
    dbl = false;
  }

  if (!dbl)
    elem->playSound(elem->csk->click);

#if _TARGET_PC
  const bool needVibration = (dev_id == DEVID_JOYSTICK);
#else
  (void)dev_id;
  const bool needVibration = true;
#endif

  const SceneConfig &config = static_cast<GuiScene *>(scene)->config;
  if (needVibration && config.clickRumbleEnabled && elem->props.scriptDesc.RawGetSlotValue(elem->csk->rumble, true))
    do_rumble(config);

  if (!f.IsNull())
  {
    SQInteger nparams = 0, nfreevars = 0;
    G_VERIFY(get_closure_info(f, &nparams, &nfreevars));

    if (nparams < 1 || nparams > 2)
    {
      darg_assert_trace_var("Expected 1 param or no params for Click Handler", elem->props.scriptDesc, *slot);
      return;
    }

#if DARG_WITH_SQDEBUGGER
    if (sq_type(f.GetFunc()) == OT_CLOSURE)
    {
      DagSqDebugger &debugger = dag_sq_debuggers.get(f.GetVM());
      if (debugger.debugger_enabled && debugger.breakOnEvent)
        debugger.pause();
    }
#endif

    if (nparams == 1)
      scene->queueScriptHandler(new ScriptHandlerSqFunc<>(f));
    else
    {
      MouseClickEventData evt;
      evt.button = mouse_btn;
      evt.screenX = mx;
      evt.screenY = my;
      evt.target = elem->getRef(f.GetVM());
      evt.devId = dev_id;
      calc_elem_rect_for_event_handler(evt.targetRect, elem);

      const HumanInput::IGenKeyboard *kbd = global_cls_drv_kbd ? global_cls_drv_kbd->getDevice(0) : NULL;
      if (kbd)
      {
        int modifiers = kbd->getRawState().shifts;
        evt.shiftKey = (modifiers & HumanInput::KeyboardRawState::SHIFT_BITS) != 0;
        evt.ctrlKey = (modifiers & HumanInput::KeyboardRawState::CTRL_BITS) != 0;
        evt.altKey = (modifiers & HumanInput::KeyboardRawState::ALT_BITS) != 0;
      }
      scene->queueScriptHandler(new ScriptHandlerSqFunc<MouseClickEventData>(f, evt));
    }
  }
}

static bool call_mouse_handler(IGuiScene *scene, Element *elem, const Sqrat::Object &func_slot, int btn_id, short mx, short my)
{
  Sqrat::Function f = elem->props.scriptDesc.RawGetFunction(func_slot);

  if (!f.IsNull())
  {
    SQInteger nparams = 0, nfreevars = 0;
    G_VERIFY(get_closure_info(f, &nparams, &nfreevars));

    if (nparams < 1 || nparams > 2)
    {
      darg_assert_trace_var("Expected 1 param or no params for mouse handler", elem->props.scriptDesc, func_slot);
      return false;
    }

#if DARG_WITH_SQDEBUGGER
    if (sq_type(f.GetFunc()) == OT_CLOSURE)
    {
      DagSqDebugger &debugger = dag_sq_debuggers.get(f.GetVM());
      if (debugger.debugger_enabled && debugger.breakOnEvent)
        debugger.pause();
    }
#endif

    if (nparams == 1)
      scene->queueScriptHandler(new ScriptHandlerSqFunc<>(f));
    else
    {
      MouseClickEventData evt;
      evt.button = btn_id;
      evt.screenX = mx;
      evt.screenY = my;
      evt.target = elem->getRef(f.GetVM());
      calc_elem_rect_for_event_handler(evt.targetRect, elem);

      const HumanInput::IGenKeyboard *kbd = global_cls_drv_kbd ? global_cls_drv_kbd->getDevice(0) : NULL;
      if (kbd)
      {
        int modifiers = kbd->getRawState().shifts;
        evt.shiftKey = (modifiers & HumanInput::KeyboardRawState::SHIFT_BITS) != 0;
        evt.ctrlKey = (modifiers & HumanInput::KeyboardRawState::CTRL_BITS) != 0;
        evt.altKey = (modifiers & HumanInput::KeyboardRawState::ALT_BITS) != 0;
      }
      scene->queueScriptHandler(new ScriptHandlerSqFunc<MouseClickEventData>(f, evt));
    }
    return true;
  }
  return false;
}

bool call_mouse_move_handler(IGuiScene *scene, Element *elem, short mx, short my)
{
  return call_mouse_handler(scene, elem, elem->csk->onMouseMove, 0, mx, my);
}

bool call_mouse_wheel_handler(IGuiScene *scene, Element *elem, int delta, short mx, short my)
{
  G_ASSERT_RETURN(delta != 0, false);
  return call_mouse_handler(scene, elem, elem->csk->onMouseWheel, delta > 0 ? 1 : -1, mx, my);
}

void call_hotkey_handler(IGuiScene *scene, Element *elem, Sqrat::Function &handler, bool from_joystick)
{
  if (handler.IsNull())
    return;

  SQInteger nparams = 0, nfreevars = 0;
  G_VERIFY(get_closure_info(handler, &nparams, &nfreevars));

#if DARG_WITH_SQDEBUGGER
  if (sq_type(handler.GetFunc()) == OT_CLOSURE)
  {
    DagSqDebugger &debugger = dag_sq_debuggers.get(handler.GetVM());
    if (debugger.debugger_enabled && debugger.breakOnEvent)
      debugger.pause();
  }
#endif

#if _TARGET_PC
  const bool needVibration = from_joystick;
#else
  (void)from_joystick;
  const bool needVibration = true;
#endif

  const SceneConfig &config = static_cast<GuiScene *>(scene)->config;
  if (needVibration && config.clickRumbleEnabled && elem->props.scriptDesc.RawGetSlotValue(elem->csk->rumble, true))
    do_rumble(config);

  if (nparams == 1)
    scene->queueScriptHandler(new ScriptHandlerSqFunc<>(handler));
  else
  {
    HotkeyEventData evt;
    calc_elem_rect_for_event_handler(evt.targetRect, elem);
    scene->queueScriptHandler(new ScriptHandlerSqFunc<HotkeyEventData>(handler, evt));
  }
}

float move_click_threshold(GuiScene *scene)
{
  if (scene->config.moveClickThreshold > 0)
    return scene->config.moveClickThreshold;
  IPoint2 screenSize = scene->getDeviceScreenSize();
  return min(screenSize.x, screenSize.y) * 0.03f;
}

float double_click_range(GuiScene *scene)
{
  IPoint2 screenSize = scene->getDeviceScreenSize();
  return 20.0f * min(screenSize.x, screenSize.y) / 1080;
}

float double_touch_range(GuiScene *scene)
{
  // TODO: this should be changed to use Centimeters -> Pixels convertion
  // using DPI to calculate range independent from device physical size
  IPoint2 screenSize = scene->getDeviceScreenSize();
  return 100.0f * min(screenSize.x, screenSize.y) / 1080;
}

int active_state_flags_for_device(InputDevice device)
{
  if (device == DEVID_MOUSE)
    return Element::S_MOUSE_ACTIVE;
  if (device == DEVID_TOUCH)
    return Element::S_TOUCH_ACTIVE;
  if (device == DEVID_JOYSTICK)
    return Element::S_JOYSTICK_ACTIVE;
  if (device == DEVID_KEYBOARD)
    return Element::S_KBD_ACTIVE;
  if (device == DEVID_VR)
    return Element::S_VR_ACTIVE;
  return 0;
}


} // namespace darg
