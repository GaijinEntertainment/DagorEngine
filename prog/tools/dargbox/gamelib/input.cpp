#include "input.h"

#include <quirrel/sqEventBus/sqEventBus.h>
#include <quirrel/sqModules/sqModules.h>
#include <sqrat.h>

#include <humanInput/dag_hiKeybIds.h>
#include <humanInput/dag_hiKeyboard.h>
#include <humanInput/dag_hiPointing.h>
#include <humanInput/dag_hiMouseIds.h>
#include <startup/dag_inpDevClsDrv.h>
#include <startup/dag_globalSettings.h>
#include <humanInput/dag_hiComposite.h>
#include <osApiWrappers/dag_critSec.h>
#include <osApiWrappers/dag_miscApi.h>

#include <EASTL/unordered_map.h>

#include <hotkeys.h> // daRg internals

#include <gui/dag_visualLog.h>


using namespace HumanInput;


namespace gamelib
{

namespace input
{

static const char *eventbus_src = "gameinput";

static FastStrMapT<darg::Hotkeys::EncodedKey> btn_name_map;
static eastl::unordered_map<int, eastl::string> btn_name_by_id;


static Tab<HumanInput::ButtonBits> btnStack;
static HumanInput::IGenJoystick *curJoy = nullptr;
static int curJoyOrdId = -1;


void initialize()
{
  darg::Hotkeys::collectButtonNames(btn_name_map);

  dag::ConstSpan<FastStrMapT<darg::Hotkeys::EncodedKey>::Entry> buttonsRawSlice = btn_name_map.getMapRaw();
  for (auto &e : buttonsRawSlice)
    btn_name_by_id[e.id] = e.name;
}

void finalize()
{
  btn_name_map.reset();
  btn_name_by_id.clear();
  clear_and_shrink(btnStack);
  curJoy = nullptr;
  curJoyOrdId = -1;
}


void on_keyboard_event(HSQUIRRELVM vm, HumanInput::IGenKeyboard *kbd, int btn_idx, bool pressed, bool repeat, wchar_t wc,
  bool processed_by_ui)
{
  G_UNUSED(kbd);
  using namespace darg;

  Hotkeys::EncodedKey btnEncoded = (DEVID_KEYBOARD << Hotkeys::DEV_ID_SHIFT) | btn_idx;

  Sqrat::Table data(vm);
  data.SetValue("btn", btnEncoded);
  auto nameIt = btn_name_by_id.find(btnEncoded);
  if (nameIt != btn_name_by_id.end())
    data.SetValue("btnName", nameIt->second.c_str());
  else
    data.SetValue("btnName", Sqrat::Object(vm));
  data.SetValue("pressed", pressed);
  data.SetValue("repeat", repeat);
  data.SetValue("char", SQInteger(wc));
  data.SetValue("processedByUi", processed_by_ui);
  sqeventbus::send_event("kb-input", data, eventbus_src);
}

void on_mouse_button_event(HSQUIRRELVM vm, HumanInput::IGenPointing *pnt, int btn_idx, bool pressed, const Point2 &pos,
  bool processed_by_ui)
{
  G_UNUSED(pnt);
  using namespace darg;

  Hotkeys::EncodedKey btnEncoded = (DEVID_MOUSE << Hotkeys::DEV_ID_SHIFT) | btn_idx;
  Sqrat::Table data(vm);

  data.SetValue("btn", btnEncoded);
  auto nameIt = btn_name_by_id.find(btnEncoded);
  if (nameIt != btn_name_by_id.end())
    data.SetValue("btnName", nameIt->second.c_str());
  else
    data.SetValue("btnName", Sqrat::Object(vm));
  data.SetValue("pressed", pressed);
  data.SetValue("x", pos.x);
  data.SetValue("y", pos.y);
  data.SetValue("processedByUi", processed_by_ui);

  sqeventbus::send_event("mouse-button", data, eventbus_src);
}


void on_mouse_wheel_event(HSQUIRRELVM vm, HumanInput::IGenPointing *pnt, int delta, const Point2 &pos, bool processed_by_ui)
{
  G_UNUSED(pnt);
  using namespace darg;

  Sqrat::Table data(vm);

  data.SetValue("delta", delta);
  data.SetValue("x", pos.x);
  data.SetValue("y", pos.y);
  data.SetValue("processedByUi", processed_by_ui);

  sqeventbus::send_event("mouse-wheel", data, eventbus_src);
}

void on_mouse_move_event(HSQUIRRELVM vm, HumanInput::IGenPointing *pnt, const Point2 &pos, bool processed_by_ui)
{
  G_UNUSED(pnt);
  using namespace darg;

  Sqrat::Table data(vm);

  data.SetValue("x", pos.x);
  data.SetValue("y", pos.y);
  data.SetValue("processedByUi", processed_by_ui);

  sqeventbus::send_event("mouse-move", data, eventbus_src);
}


static bool dispatch_joystick_state_changed(HSQUIRRELVM vm, const HumanInput::ButtonBits &buttons,
  const HumanInput::ButtonBits &buttonsPrev)
{
  using namespace darg;

  HumanInput::ButtonBits btnXor;
  buttons.bitXor(btnXor, buttonsPrev);

  Sqrat::Table data(vm);

  bool processed = false;
  for (int btn = 0, inc = 0; btn < btnXor.FULL_SZ; btn += inc)
  {
    if (btnXor.getIter(btn, inc))
    {
      Hotkeys::EncodedKey btnEncoded = (DEVID_JOYSTICK << Hotkeys::DEV_ID_SHIFT) | btn;
      data.SetValue("btn", btnEncoded);
      auto nameIt = btn_name_by_id.find(btnEncoded);
      if (nameIt != btn_name_by_id.end())
        data.SetValue("btnName", nameIt->second.c_str());
      else
        data.SetValue("btnName", Sqrat::Object(vm));
      data.SetValue("pressed", buttons.get(btn) ? true : false);

      sqeventbus::send_event("joystick-button", data, eventbus_src);
    }
  }

  return processed;
}


void process_pending_joystick_btn_stack(HSQUIRRELVM vm)
{
  global_cls_drv_update_cs.lock();
  for (int i = 1; i < btnStack.size(); i++)
    dispatch_joystick_state_changed(vm, btnStack[i], btnStack[i - 1]);
  btnStack.clear();
  global_cls_drv_update_cs.unlock();
}


void on_joystick_state_changed(HSQUIRRELVM vm, HumanInput::IGenJoystick *joy, int joy_ord_id)
{
  const HumanInput::JoystickRawState &rs = joy->getRawState();

  if (is_main_thread())
  {
    process_pending_joystick_btn_stack(vm);
    curJoy = joy;
    curJoyOrdId = joy_ord_id;
    if (!rs.buttons.cmpEq(rs.buttonsPrev))
      dispatch_joystick_state_changed(vm, rs.buttons, rs.buttonsPrev);
  }
  else
  {
    if (joy == curJoy && joy_ord_id == curJoyOrdId)
    {
      if (!rs.buttons.cmpEq(rs.buttonsPrev))
      {
        if (!btnStack.size())
          btnStack.push_back() = rs.buttonsPrev;
        btnStack.push_back() = rs.buttons;
      }
    }
  }
}


static SQInteger get_button_state(HSQUIRRELVM vm)
{
  using namespace darg;

  SQInteger btnEncoded = 0;

  if (sq_gettype(vm, 2) & SQOBJECT_NUMERIC)
    G_VERIFY(SQ_SUCCEEDED(sq_getinteger(vm, 2, &btnEncoded)));
  else
  {
    const char *btnName = nullptr;
    G_VERIFY(SQ_SUCCEEDED(sq_getstring(vm, 2, &btnName)));
    btnEncoded = btn_name_map.getStrId(btnName);
    if (btnEncoded < 0)
      return sq_throwerror(vm, "Invalid button name");
  }


  InputDevice devId = InputDevice(btnEncoded >> Hotkeys::DEV_ID_SHIFT);
  int btnId = btnEncoded & Hotkeys::BTN_ID_MASK;

  switch (devId)
  {
    case DEVID_KEYBOARD:
    {
      IGenKeyboard *kbd = ::global_cls_drv_kbd->getDevice(0);
      if (!kbd)
        return 0;
      const KeyboardRawState &rs = kbd->getRawState();
      sq_pushbool(vm, rs.isKeyDown(btnId));
      return 1;
    }

    case DEVID_MOUSE:
    {
      IGenPointing *pnt = ::global_cls_drv_pnt->getDevice(0);
      if (!pnt)
        return 0;
      const PointingRawState &rs = pnt->getRawState();
      sq_pushbool(vm, rs.mouse.isBtnDown(btnId));
      return 1;
    }

    case DEVID_JOYSTICK:
    {
      IGenJoystick *joy = ::global_cls_drv_joy->getDevice(0);
      if (!joy)
        return 0;
      sq_pushbool(vm, joy->getButtonState(btnId));
      return 1;
    }

    default: break;
  }

  return sq_throwerror(vm, "Unsupported device");
}


static SQInteger get_mouse_pos(HSQUIRRELVM vm)
{
  IGenPointing *pnt = ::global_cls_drv_pnt->getDevice(0);
  if (!pnt)
    return 0;

  const PointingRawState &rs = pnt->getRawState();
  Sqrat::Var<Point2>::push(vm, Point2(rs.mouse.x, rs.mouse.y));
  return 1;
}


static SQInteger get_joystick_axis(HSQUIRRELVM vm)
{
  IGenJoystick *joy = ::global_cls_drv_joy->getDevice(0);
  if (!joy)
    return 0;

  SQInteger axisId = 0;
  G_VERIFY(SQ_SUCCEEDED(sq_getinteger(vm, 2, &axisId)));

  float pos = joy->getAxisPos(axisId) / JOY_XINPUT_MAX_AXIS_VAL;
  sq_pushfloat(vm, pos);
  return 1;
}


void bind_script(SqModules *module_mgr)
{
  HSQUIRRELVM vm = module_mgr->getVM();

  Sqrat::Table exports(vm);
  exports.SquirrelFunc("get_button_state", get_button_state, 2, ". n|s")
    .SquirrelFunc("get_mouse_pos", get_mouse_pos, 1)
    .SquirrelFunc("get_joystick_axis", get_joystick_axis, 2, ". n");

  dag::ConstSpan<FastStrMapT<darg::Hotkeys::EncodedKey>::Entry> buttonsRawSlice = btn_name_map.getMapRaw();
  Sqrat::Table buttonNames(vm);
  Sqrat::Table buttonIds(vm);
  for (auto &e : buttonsRawSlice)
  {
    buttonIds.SetValue(e.name, e.id);
    buttonNames.SetValue(e.id, e.name);
  }
  buttonIds.FreezeSelf();
  buttonNames.FreezeSelf();
  exports.SetValue("buttonIds", buttonIds);
  exports.SetValue("buttonNames", buttonNames);

  module_mgr->addNativeModule("gamelib.input", exports);
}

} // namespace input

} // namespace gamelib
