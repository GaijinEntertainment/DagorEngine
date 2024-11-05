// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "bhvPieMenu.h"

#include <daRg/dag_element.h>
#include <daRg/dag_properties.h>
#include <daRg/dag_stringKeys.h>
#include <daRg/dag_uiSound.h>
#include <daRg/dag_scriptHandlers.h>
#include <daRg/dag_pieMenuInput.h>

#include <quirrel/frp/dag_frp.h>

#include "guiScene.h"
#include "scriptUtil.h"

#include <drv/hid/dag_hiKeybIds.h>
#include <drv/hid/dag_hiMouseIds.h>
#include <drv/hid/dag_hiKeyboard.h>
#include <startup/dag_inpDevClsDrv.h>
#include <perfMon/dag_cpuFreq.h>
#include <gui/dag_visualLog.h>

#if DARG_WITH_SQDEBUGGER
#include <sqDebugger/sqDebugger.h>
#endif

#include "dargDebugUtils.h"
#include "behaviorHelpers.h"


using namespace sqfrp;


namespace darg
{


static PieMenuInputAdapter *input_adapter = nullptr;

void set_pie_menu_input_adapter(PieMenuInputAdapter *adapter) { input_adapter = adapter; }


BhvPieMenu bhv_pie_menu;

static const char *dataSlotName = "pie_menu:data";


struct BhvPieMenuData
{
  InputDevice clickedByDevice = DEVID_NONE;
  int pointerId = -1;
  int buttonId = -1;

  bool isPressed() { return clickedByDevice != DEVID_NONE; }

  bool isPressedBy(InputDevice device, int pointer) { return clickedByDevice == device && pointerId == pointer; }

  bool isPressedBy(InputDevice device, int pointer, int button)
  {
    return clickedByDevice == device && pointerId == pointer && buttonId == button;
  }

  void press(InputDevice device, int pointer, int button)
  {
    G_ASSERT(!isPressed());
    clickedByDevice = device;
    pointerId = pointer;
    buttonId = button;
  }

  void release()
  {
    clickedByDevice = DEVID_NONE;
    pointerId = -1;
    buttonId = -1;
  }
};


static ScriptValueObservable *get_observable(Element *elem, const Sqrat::Object &key)
{
  Sqrat::Object obj = elem->props.getObject(key);
  if (obj.GetType() != OT_INSTANCE)
    return nullptr;

  return obj.Cast<ScriptValueObservable *>();
}


static void reset_sector(Element *elem)
{
  ScriptValueObservable *obsCurSector = get_observable(elem, elem->csk->curSector);
  ScriptValueObservable *obsCurAngle = get_observable(elem, elem->csk->curAngle);
  String errMsg;

  Sqrat::Object nullObj;
  if (obsCurSector)
    obsCurSector->setValue(nullObj, errMsg);
  if (obsCurAngle)
    obsCurAngle->setValue(nullObj, errMsg);
}


static void update_sector_rel(Element *elem, float x, float y)
{
  ScriptValueObservable *obsCurSector = get_observable(elem, elem->csk->curSector);
  ScriptValueObservable *obsCurAngle = get_observable(elem, elem->csk->curAngle);
  String errMsg;

  if (x * x + y * y > 0.1f)
  {
    float angle = atan2f(x, y);
    if (obsCurSector)
    {
      int sectorsCount = elem->props.getInt(elem->csk->sectorsCount, 0);
      if (sectorsCount > 0)
      {
        float sectorRange = TWOPI / sectorsCount;
        int curSector = floorf(fmodf((angle + TWOPI + sectorRange * 0.5), TWOPI) / sectorRange);
        obsCurSector->setValue(Sqrat::Object(curSector, elem->getVM()), errMsg);
      }
    }
    if (obsCurAngle)
    {
      float angleStep = 3 * DEG_TO_RAD;
      float angleAligned = floorf(angle / angleStep + 0.5f) * angleStep;
      obsCurAngle->setValue(Sqrat::Object(angleAligned, elem->getVM()), errMsg);
    }
  }
  else
  {
    Sqrat::Object nullObj;
    if (obsCurSector)
      obsCurSector->setValue(nullObj, errMsg);
    if (obsCurAngle)
      obsCurAngle->setValue(nullObj, errMsg);
  }
}

static void update_sector_abs(Element *elem, const Point2 &pos)
{
  Point2 halfSize = elem->screenCoord.size * 0.5f;
  Point2 center = elem->calcTransformedBbox().center();
  Point2 mpos = div(pos - center, halfSize);
  update_sector_rel(elem, mpos.x, -mpos.y);
}


BhvPieMenu::BhvPieMenu() : Behavior(STAGE_ACT, F_HANDLE_MOUSE | F_HANDLE_TOUCH | F_OVERRIDE_GAMEPAD_STICK) {}


void BhvPieMenu::onAttach(Element *elem)
{
  BhvPieMenuData *bhvData = new BhvPieMenuData();
  elem->props.storage.SetValue(dataSlotName, bhvData);
}


void BhvPieMenu::onDetach(Element *elem, DetachMode)
{
  BhvPieMenuData *bhvData = elem->props.storage.RawGetSlotValue<BhvPieMenuData *>(dataSlotName, nullptr);
  if (bhvData)
  {
    elem->props.storage.DeleteSlot(dataSlotName);
    delete bhvData;
  }
}

int BhvPieMenu::mouseEvent(ElementTree *etree, Element *elem, InputDevice device, InputEvent event, int pointer_id, int data, short mx,
  short my, int /*buttons*/, int accum_res)
{
  return pointingEvent(etree, elem, device, event, pointer_id, data, Point2(mx, my), accum_res);
}


int BhvPieMenu::pointingEvent(ElementTree *etree, Element *elem, InputDevice device, InputEvent event, int pointer_id, int button_id,
  const Point2 &pos, int accum_res)
{
  int pointingDevice = elem->props.getInt(elem->csk->devId, -1);
  if (pointingDevice != DEVID_MOUSE)
    return 0;

  BhvPieMenuData *bhvData = elem->props.storage.RawGetSlotValue<BhvPieMenuData *>(dataSlotName, nullptr);
  G_ASSERT_RETURN(bhvData, 0);

  int result = 0;

  int activeStateFlags = active_state_flags_for_device(device);

  if (event == INP_EV_PRESS && elem->hitTest(pos) && !(accum_res & R_PROCESSED))
  {
    if (!bhvData->isPressed())
    {
      elem->setGroupStateFlags(activeStateFlags);
      bhvData->press(device, pointer_id, button_id);
      update_sector_abs(elem, pos);
    }

    if (!elem->props.getBool(elem->csk->eventPassThrough, false))
      result |= R_PROCESSED;
  }
  else if (event == INP_EV_RELEASE)
  {
    if (bhvData->isPressedBy(device, pointer_id, button_id))
    {
      Sqrat::Function onClick = elem->props.scriptDesc.GetFunction(elem->csk->onClick);
      elem->playSound(elem->csk->click);

      if (!onClick.IsNull())
      {
        SQInteger nparams = 0, nfreevars = 0;
        G_VERIFY(get_closure_info(onClick, &nparams, &nfreevars));

        if (nparams < 1 || nparams > 2)
        {
          darg_assert_trace_var("Expected 1 param or no params for Click Handler", elem->props.scriptDesc, elem->csk->onClick);
        }
        else
        {
#if DARG_WITH_SQDEBUGGER
          if (sq_type(onClick.GetFunc()) == OT_CLOSURE)
          {
            DagSqDebugger &debugger = dag_sq_debuggers.get(onClick.GetVM());
            if (debugger.debugger_enabled && debugger.breakOnEvent)
              debugger.pause();
          }
#endif

          if (nparams == 1)
            etree->guiScene->queueScriptHandler(new ScriptHandlerSqFunc<>(onClick));
          else
          {
            ScriptValueObservable *obsCurSector = get_observable(elem, elem->csk->curSector);
            Sqrat::Object val = obsCurSector ? obsCurSector->getValue() : Sqrat::Object();
            auto handler = new ScriptHandlerSqFunc<Sqrat::Object>(onClick, val);
            etree->guiScene->queueScriptHandler(handler);
          }
        }
      }

      bhvData->release();
      elem->clearGroupStateFlags(activeStateFlags);
      if (!elem->props.getBool(elem->csk->eventPassThrough, false))
        result |= R_PROCESSED;
    }
  }
  else if (event == INP_EV_POINTER_MOVE)
  {
    bool wasProcessedAbove = (accum_res & R_PROCESSED) != 0;
    if (wasProcessedAbove)
      reset_sector(elem);
    // mouse don't have to be pressed for tracking
    else if (device != DEVID_TOUCH || bhvData->isPressedBy(device, pointer_id, button_id))
      update_sector_abs(elem, pos);
  }

  return result;
}


int BhvPieMenu::touchEvent(ElementTree *etree, Element *elem, InputEvent event, HumanInput::IGenPointing * /*pnt*/, int touch_idx,
  const HumanInput::PointingRawState::Touch &touch, int accum_res)
{
  return pointingEvent(etree, elem, DEVID_TOUCH, event, touch_idx, 0, Point2(touch.x, touch.y), accum_res);
}


int BhvPieMenu::update(UpdateStage /*stage*/, Element *elem, float /*dt*/)
{
  if (input_adapter)
  {
    Point2 axis = input_adapter->getAxisValue(elem->props.scriptDesc);
    update_sector_rel(elem, axis.x, axis.y);
    return 0;
  }

  using namespace HumanInput;

  int pointingDevice = elem->props.getInt(elem->csk->devId, -1);
  if (pointingDevice == DEVID_JOYSTICK)
  {
    IGenJoystick *joy = GuiScene::getJoystick();
    if (joy)
    {
      const int defStick = 0;
      int stickNo = elem->props.getInt(elem->csk->stickNo, defStick);
      if (stickNo != 0 && stickNo != 1)
        stickNo = defStick;

      float xVal =
        joy->getAxisPos(stickNo ? JOY_XINPUT_REAL_AXIS_R_THUMB_H : JOY_XINPUT_REAL_AXIS_L_THUMB_H) / JOY_XINPUT_MAX_AXIS_VAL;
      float yVal =
        joy->getAxisPos(stickNo ? JOY_XINPUT_REAL_AXIS_R_THUMB_V : JOY_XINPUT_REAL_AXIS_L_THUMB_V) / JOY_XINPUT_MAX_AXIS_VAL;
      update_sector_rel(elem, xVal, yVal);
    }
  }
  else if (pointingDevice == DEVID_VR)
  {
    const int defStick = 0;
    int stickNo = elem->props.getInt(elem->csk->stickNo, defStick);
    if (stickNo != 0 && stickNo != 1)
      stickNo = defStick;

    GuiScene *scene = GuiScene::get_from_elem(elem);
    Point2 stickPos = scene->getVrStickState(stickNo);
    update_sector_rel(elem, stickPos.x, stickPos.y);
  }

  return 0;
}


void BhvPieMenu::collectConsumableButtons(Element *elem, Tab<HotkeyButton> &input)
{
  using namespace HumanInput;

  int stickNo = elem->props.getInt(elem->csk->stickNo, 0);
  if (stickNo != 0 && stickNo != 1)
    stickNo = 0;

  input.push_back(HotkeyButton(DEVID_JOYSTICK_AXIS, (stickNo ? JOY_XINPUT_REAL_AXIS_R_THUMB_H : JOY_XINPUT_REAL_AXIS_L_THUMB_H)));
  input.push_back(HotkeyButton(DEVID_JOYSTICK_AXIS, (stickNo ? JOY_XINPUT_REAL_AXIS_R_THUMB_V : JOY_XINPUT_REAL_AXIS_L_THUMB_V)));
  input.push_back(HotkeyButton(DEVID_MOUSE_AXIS, 0));
  input.push_back(HotkeyButton(DEVID_MOUSE_AXIS, 1));
}


int BhvPieMenu::onDeactivateInput(Element *elem, InputDevice device, int pointer_id)
{
  BhvPieMenuData *bhvData = elem->props.storage.RawGetSlotValue<BhvPieMenuData *>(dataSlotName, nullptr);
  G_ASSERT_RETURN(bhvData, 0);

  if (bhvData->isPressedBy(device, pointer_id))
  {
    bhvData->release();
    elem->clearGroupStateFlags(active_state_flags_for_device(device));
  }
  return 0;
}


} // namespace darg
