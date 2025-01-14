// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "bhvButton.h"

#include <daRg/dag_element.h>
#include <daRg/dag_properties.h>
#include <daRg/dag_stringKeys.h>
#include <daRg/dag_uiSound.h>
#include <daRg/dag_scriptHandlers.h>

#include "guiScene.h"
#include "scriptUtil.h"
#include "eventData.h"

#include <drv/hid/dag_hiKeybIds.h>
#include <drv/hid/dag_hiMouseIds.h>
#include <drv/hid/dag_hiKeyboard.h>
#include <startup/dag_inpDevClsDrv.h>
#include <perfMon/dag_cpuFreq.h>
#include <util/dag_convar.h>

#if DARG_WITH_SQDEBUGGER
#include <sqDebugger/sqDebugger.h>
#endif

#include "dargDebugUtils.h"
#include "behaviorHelpers.h"
#include "guiGlobals.h"


namespace darg
{

CONSOLE_FLOAT_VAL("darg", touch_hold_time, 1.0f);
CONSOLE_BOOL_VAL("darg", button_margin_debug, false);


struct BhvButtonData
{
  InputDevice pressedByDevice = DEVID_NONE;
  int pressedByPointerId = -1;
  int pressedByBtnId = -1;
  Point2 pointerStartPos = Point2(-9999, -9999);
  BBox2 clickStartClippedBBox;
  BBox2 clickStartFullBBox;

  float touchHoldCallTime = 0;

  // double click handling
  bool wasClicked = false;
  int lastClickTime = 0;
  Point2 lastClickPos = Point2(0, 0);

  void press(InputDevice device, int pointer_id, int btn_id, const Point2 &pos, const BBox2 &clipped_elem_box,
    const BBox2 &full_elem_box)
  {
    pressedByDevice = device;
    pressedByPointerId = pointer_id;
    pressedByBtnId = btn_id;
    pointerStartPos = pos;
    clickStartClippedBBox = clipped_elem_box;
    clickStartFullBBox = full_elem_box;
  }

  void release()
  {
    pressedByDevice = DEVID_NONE;
    pressedByPointerId = -1;
    pressedByBtnId = -1;
    pointerStartPos.set(-9999, -9999);
    clickStartClippedBBox.setempty();
    clickStartFullBBox.setempty();
    touchHoldCallTime = 0;
  }

  bool isPressed() { return pressedByDevice != DEVID_NONE; }

  bool isPressedBy(InputDevice device, int pointer_id) { return pressedByDevice == device && pressedByPointerId == pointer_id; }

  bool isPressedBy(InputDevice device, int pointer_id, int btn_id)
  {
    return pressedByDevice == device && pressedByPointerId == pointer_id && pressedByBtnId == btn_id;
  }

  void saveClick(int cur_time, const Point2 &pos)
  {
    lastClickTime = cur_time;
    lastClickPos = pos;
    wasClicked = true;
  }

  bool isDoubleClick(int cur_time, const Point2 &pos, float pos_thresh)
  {
    if (wasClicked && (cur_time - lastClickTime <= double_click_interval_ms))
    {
      if (fabsf(lastClickPos.x - pos.x) <= pos_thresh && fabsf(lastClickPos.y - pos.y) <= pos_thresh)
        return true;
    }
    return false;
  }
};


BhvButton bhv_button;

static const char *dataSlotName = "btn:data";


BhvButton::BhvButton() :
  Behavior(STAGE_ACT,
    F_HANDLE_KEYBOARD_GLOBAL | F_HANDLE_MOUSE | F_HANDLE_TOUCH | F_CAN_HANDLE_CLICKS | F_HANDLE_JOYSTICK | F_USES_TOUCH_MARGIN)
{}


void BhvButton::onAttach(Element *elem)
{
  BhvButtonData *btnData = new BhvButtonData();
  elem->props.storage.SetValue(dataSlotName, btnData);
}


void BhvButton::onDetach(Element *elem, DetachMode)
{
  BhvButtonData *btnData = elem->props.storage.RawGetSlotValue<BhvButtonData *>(dataSlotName, nullptr);
  if (btnData)
  {
    elem->props.storage.DeleteSlot(dataSlotName);
    delete btnData;
  }
}


bool BhvButton::willHandleClick(Element *elem)
{
  if (!elem->props.scriptDesc.RawGetSlot(elem->csk->onClick).IsNull())
    return true;
  if (!elem->props.scriptDesc.RawGetSlot(elem->csk->onDoubleClick).IsNull())
    return true;

  return false;
}


int BhvButton::mouseEvent(ElementTree *etree, Element *elem, InputDevice device, InputEvent event, int pointer_id, int data, short mx,
  short my, int /*buttons*/, int accum_res)
{
  return pointerEvent(etree, elem, device, event, pointer_id, data, Point2(mx, my), accum_res);
}

int BhvButton::pointerEvent(ElementTree *etree, Element *elem, InputDevice device, InputEvent event, int pointer_id, int button_id,
  const Point2 &pointer_pos, int accum_res)
{
  BhvButtonData *btnData = elem->props.storage.RawGetSlotValue<BhvButtonData *>(dataSlotName, nullptr);
  if (!btnData)
    return 0;

  int result = 0;

  bool eventPassThrough = elem->props.getBool(elem->csk->eventPassThrough, false);
  int activeStateFlag = active_state_flags_for_device(device);

  if (event == INP_EV_PRESS)
  {
    if (!(accum_res & R_PROCESSED) && !btnData->isPressed() && elem->hitTestWithTouchMargin(pointer_pos, device))
    {
      if (button_margin_debug)
      {
        BBox2 elemBox = elem->clippedScreenRect;
        etree->guiScene->spawnDebugRenderBox(elemBox, E3DCOLOR(0, 50, 0, 50), E3DCOLOR(200, 200, 0), 0.8f);

        BBox2 clickBox;
        clickBox += pointer_pos;
        clickBox.inflate(10);
        etree->guiScene->spawnDebugRenderBox(clickBox, E3DCOLOR(50, 0, 0, 50), E3DCOLOR(200, 0, 0), 0.8f);
      }

      if (!eventPassThrough)
        result = R_PROCESSED;
      elem->setGroupStateFlags(activeStateFlag);
      btnData->press(device, pointer_id, button_id, pointer_pos, elem->clippedScreenRect, elem->transformedBbox);

      if (device == DEVID_TOUCH)
      {
        Sqrat::Object onTouchHold = elem->props.scriptDesc.RawGetSlot(elem->csk->onTouchHold);
        if (!onTouchHold.IsNull())
        {
          float touchHoldTime = elem->props.scriptDesc.RawGetSlotValue<float>(elem->csk->touchHoldTime, touch_hold_time);
          btnData->touchHoldCallTime = get_time_msec() * 0.001f + touchHoldTime;
        }
        else
          btnData->touchHoldCallTime = 0;
      }
    }
  }
  else if (event == INP_EV_RELEASE)
  {
    if (btnData->isPressedBy(device, pointer_id, button_id))
    {
      if (!eventPassThrough)
        result = R_PROCESSED;

      elem->clearGroupStateFlags(activeStateFlag);

      if (!(accum_res & R_PROCESSED) && elem->hitTestWithTouchMargin(pointer_pos, device))
      {
        BBox2 extendedBbox = btnData->clickStartClippedBBox;
        if (should_use_outside_area(device)) // Drag-friendly clicks (see commit 94979869ffe78763394d3068eb24569204f4bcef)
          extendedBbox.inflate(etree->guiScene->config.buttonTouchMargin);
        if (extendedBbox & pointer_pos)
        {
          int curTime = ::get_time_msec();
          float dblClickRange = (DEVID_TOUCH == device) ? double_touch_range(etree->guiScene) : double_click_range(etree->guiScene);
          bool isDouble = btnData->isDoubleClick(curTime, pointer_pos, dblClickRange);
          call_click_handler(etree->guiScene, elem, device, isDouble, button_id, pointer_pos.x, pointer_pos.y);
          btnData->saveClick(curTime, pointer_pos);

          if (elem->props.getBool(elem->csk->focusOnClick, false) && !etree->hasCapturedKbFocus())
            etree->setKbFocus(elem);
        }
      }
      btnData->release();
    }
  }
  else if (event == INP_EV_POINTER_MOVE)
  {
    if (btnData->isPressedBy(device, pointer_id))
    {
      float moveThreshold = move_click_threshold(etree->guiScene);
      bool mouseOutOfThresholdRange = (fabsf(pointer_pos.x - btnData->pointerStartPos.x) > moveThreshold ||
                                       fabsf(pointer_pos.y - btnData->pointerStartPos.y) > moveThreshold);

      // if the pointer was moved far enough, start passing the event to other behaviors
      if (!eventPassThrough && !mouseOutOfThresholdRange)
        result = R_PROCESSED;

      const BBox2 &curBbox = elem->transformedBbox;
      if (fabsf(curBbox.lim[0].x - btnData->clickStartFullBBox.lim[0].x) > moveThreshold ||
          fabsf(curBbox.lim[0].y - btnData->clickStartFullBBox.lim[0].y) > moveThreshold ||
          fabsf(curBbox.lim[1].x - btnData->clickStartFullBBox.lim[1].x) > moveThreshold ||
          fabsf(curBbox.lim[1].y - btnData->clickStartFullBBox.lim[1].y) > moveThreshold)
      {
        // release if the button was moved by panning/scrolling
        elem->clearGroupStateFlags(activeStateFlag);
        btnData->release();
      }
      else
      {
        if (elem->hitTestWithTouchMargin(pointer_pos, device))
          elem->setGroupStateFlags(activeStateFlag);
        else
          elem->clearGroupStateFlags(activeStateFlag);
      }
    }
  }

  return result;
}


int BhvButton::touchEvent(ElementTree *etree, Element *elem, InputEvent event, HumanInput::IGenPointing *pnt, int touch_idx,
  const HumanInput::PointingRawState::Touch &touch, int accum_res)
{
  G_UNUSED(pnt);
  return pointerEvent(etree, elem, DEVID_TOUCH, event, touch_idx, 0, Point2(touch.x, touch.y), accum_res);
}


int BhvButton::update(UpdateStage /*stage*/, Element *elem, float /*dt*/)
{
  BhvButtonData *btnData = elem->props.storage.RawGetSlotValue<BhvButtonData *>(dataSlotName, nullptr);
  if (!btnData)
    return 0;

  float curTime = get_time_msec() * 0.001f;

  // enable for mouse also?
  if (btnData->pressedByDevice == DEVID_TOUCH && (btnData->touchHoldCallTime != 0) && (curTime >= btnData->touchHoldCallTime))
  {
    btnData->touchHoldCallTime = 0;

    Sqrat::Function onTouchHold = elem->props.scriptDesc.RawGetFunction(elem->csk->onTouchHold);
    if (!onTouchHold.IsNull())
    {
      SQInteger nparams = 0, nfreevars = 0;
      G_VERIFY(get_closure_info(onTouchHold, &nparams, &nfreevars));

      if (nparams < 1 || nparams > 2)
        darg_assert_trace_var("Expected 1 param or no params for onTouchHold", elem->props.scriptDesc, elem->csk->onTouchHold);
      else
      {
#if DARG_WITH_SQDEBUGGER
        if (sq_type(onTouchHold.GetFunc()) == OT_CLOSURE)
        {
          DagSqDebugger &debugger = dag_sq_debuggers.get(onTouchHold.GetVM());
          if (debugger.debugger_enabled && debugger.breakOnEvent)
            debugger.pause();
        }
#endif

        elem->playSound(elem->csk->hold);

        GuiScene *scene = GuiScene::get_from_elem(elem);
        G_ASSERT_RETURN(scene, 0);

        if (nparams == 1)
          scene->queueScriptHandler(new ScriptHandlerSqFunc<>(onTouchHold));
        else
        {
          MouseClickEventData evt;
          evt.button = -1;
          evt.screenX = btnData->pointerStartPos.x;
          evt.screenY = btnData->pointerStartPos.y;
          evt.target = elem->getRef(onTouchHold.GetVM());
          evt.devId = DEVID_TOUCH;
          calc_elem_rect_for_event_handler(evt.targetRect, elem);

          const HumanInput::IGenKeyboard *kbd = global_cls_drv_kbd ? global_cls_drv_kbd->getDevice(0) : NULL;
          if (kbd)
          {
            int modifiers = kbd->getRawState().shifts;
            evt.shiftKey = (modifiers & HumanInput::KeyboardRawState::SHIFT_BITS) != 0;
            evt.ctrlKey = (modifiers & HumanInput::KeyboardRawState::CTRL_BITS) != 0;
            evt.altKey = (modifiers & HumanInput::KeyboardRawState::ALT_BITS) != 0;
          }
          scene->queueScriptHandler(new ScriptHandlerSqFunc<MouseClickEventData>(onTouchHold, evt));
        }
      }
    }
  }

  return 0;
}


int BhvButton::onDeactivateInput(Element *elem, InputDevice device, int pointer_id)
{
  BhvButtonData *btnData = elem->props.storage.RawGetSlotValue<BhvButtonData *>(dataSlotName, nullptr);
  G_ASSERT_RETURN(btnData, 0);
  if (btnData->isPressedBy(device, pointer_id))
  {
    elem->clearGroupStateFlags(active_state_flags_for_device(btnData->pressedByDevice));
    btnData->release();
  }
  return 0;
}


int BhvButton::simulateClick(ElementTree *etree, Element *elem, InputEvent event, InputDevice dev_id, int btn_idx, int accum_res)
{
  if (!elem->isInMainTree())
    return 0;

  Point2 mpos = etree->guiScene->getMousePos();
  if (event == INP_EV_PRESS && elem != etree->guiScene->getInputStack().hitTest(mpos, Behavior::F_HANDLE_MOUSE, Element::F_STOP_MOUSE))
    return 0;

  return pointerEvent(etree, elem, dev_id, event, 0, btn_idx, mpos, accum_res);
}


int BhvButton::joystickBtnEvent(ElementTree *etree, Element *elem, const HumanInput::IGenJoystick *, InputEvent event, int btn_idx,
  int /*device_number*/, const HumanInput::ButtonBits & /*buttons*/, int accum_res)
{
  const SceneConfig &config = etree->guiScene->config;
  if (config.actionClickByBehavior && config.isClickButton(DEVID_JOYSTICK, btn_idx))
    return simulateClick(etree, elem, event, DEVID_JOYSTICK, btn_idx, accum_res);

  return 0;
}


int BhvButton::kbdEvent(ElementTree *etree, Element *elem, InputEvent event, int key_idx, bool repeat, wchar_t /*wc*/, int accum_res)
{
  if (repeat)
    return 0;

  if (elem == etree->kbFocus)
  {
    // Keep this behavior just in case we need to force the element to process keyboard events for some reason
    // Should we remove this?
    if (key_idx == HumanInput::DKEY_RETURN || key_idx == HumanInput::DKEY_NUMPADENTER || key_idx == HumanInput::DKEY_SPACE)
    {
      if (event == INP_EV_PRESS && !(accum_res & R_PROCESSED))
      {
        elem->setGroupStateFlags(Element::S_KBD_ACTIVE);
      }
      else
      {
        if (elem->getStateFlags() & Element::S_KBD_ACTIVE)
        {
          elem->clearGroupStateFlags(Element::S_KBD_ACTIVE);
          Point2 mpos = etree->guiScene->getMousePos();
          call_click_handler(etree->guiScene, elem, DEVID_KEYBOARD, false, -1, mpos.x, mpos.y);
        }
      }
      return R_PROCESSED;
    }
  }
  else
  {
    SceneConfig &config = etree->guiScene->config;
    if (config.actionClickByBehavior && config.kbCursorControl && config.isClickButton(DEVID_KEYBOARD, key_idx))
      return simulateClick(etree, elem, event, DEVID_KEYBOARD, key_idx, accum_res);
  }

  return 0;
}

} // namespace darg