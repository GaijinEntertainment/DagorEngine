// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <squirrel.h>
#include <sqModules/sqModules.h>
#include <json/json.h>
#include <quirrel/sqEventBus/sqEventBus.h>
#include <ecs/input/hidEventRouter.h>
#include <startup/dag_globalSettings.h>
#include <startup/dag_inpDevClsDrv.h>
#include <drv/hid/dag_hiJoystick.h>
#include <drv/hid/dag_hiXInputMappings.h>

enum
{
  DEV_USED_mouseIdx = 0,
  DEV_USED_kbdIdx = 1,
  DEV_USED_gamepadIdx = 2,
  DEV_USED_touchIdx = 3,
  DEV_USED_Count = 4,
  DEV_USED_mouse = 1 << DEV_USED_mouseIdx,
  DEV_USED_kbd = 1 << DEV_USED_kbdIdx,
  DEV_USED_gamepad = 1 << DEV_USED_gamepadIdx,
  DEV_USED_touch = 1 << DEV_USED_touchIdx
};

static constexpr char INPUT_DEVICE_USED_EVENT[] = "input_dev_used";
const unsigned FRAMES_THRESHOLD = 100;
static unsigned lastInputOnFrameNo[DEV_USED_Count] = {0, 0, 0, 0};
static unsigned last_mask = 0;

static unsigned get_last_used_device_mask() { return last_mask; }

struct InputMonitorHandler final : public ecs::IGenHidEventHandler
{
public:
  bool ehIsActive() const override { return true; }

  bool gmehMove(const Context & /*ctx*/, float /*dx*/, float /*dy*/) override { return false; }
  bool gmehWheel(const Context & /*ctx*/, int /*scroll*/) override { return false; }
  bool gkcLocksChanged(const Context &, unsigned /*locks*/) override { return false; }
  bool gkcLayoutChanged(const Context &, const char * /*layout*/) override { return false; }

  bool gmehButtonDown(const Context &, int /*btn_idx*/) override
  {
    lastInputOnFrameNo[DEV_USED_mouseIdx] = dagor_frame_no();
    return false;
  }
  bool gmehButtonUp(const Context &, int /*btn_idx*/) override
  {
    lastInputOnFrameNo[DEV_USED_mouseIdx] = dagor_frame_no();
    return false;
  }

  bool gkehButtonDown(const Context &, int btn_idx, bool repeat, wchar_t) override
  {
    if (btn_idx && !repeat)
      lastInputOnFrameNo[DEV_USED_kbdIdx] = dagor_frame_no();
    return false;
  }
  bool gkehButtonUp(const Context &, int btn_idx) override
  {
    if (btn_idx)
      lastInputOnFrameNo[DEV_USED_kbdIdx] = dagor_frame_no();
    return false;
  }
  bool gtehTouchBegan(const Context &, HumanInput::IGenPointing *, int, const HumanInput::PointingRawState::Touch &) override
  {
    lastInputOnFrameNo[DEV_USED_touchIdx] = dagor_frame_no();
    return false;
  }
  bool gtehTouchMoved(const Context &, HumanInput::IGenPointing *, int, const HumanInput::PointingRawState::Touch &) override
  {
    lastInputOnFrameNo[DEV_USED_touchIdx] = dagor_frame_no();
    return false;
  }
  bool gtehTouchEnded(const Context &, HumanInput::IGenPointing *, int, const HumanInput::PointingRawState::Touch &) override
  {
    lastInputOnFrameNo[DEV_USED_touchIdx] = dagor_frame_no();
    return false;
  }

  bool gjehStateChanged(const Context &, HumanInput::IGenJoystick *joy, int) override
  {
    const int minAxisVal = ceil(0.25 * HumanInput::JOY_XINPUT_MAX_AXIS_VAL);
    if (!::global_cls_drv_joy || !joy || ::global_cls_drv_joy->getDefaultJoystick() != joy ||
        !(joy->getRawState().buttons.getWord0() | joy->getRawState().buttonsPrev.getWord0()))
      return false;

    for (int i = 0; i < joy->getAxisCount(); i++)
      if (abs(joy->getAxisPosRaw(i)) > minAxisVal)
      {
        lastInputOnFrameNo[DEV_USED_gamepadIdx] = dagor_frame_no();
        return false;
      }

    for (int i = 0; i < joy->getButtonCount(); i++)
      if (joy->getButtonState(i))
      {
        lastInputOnFrameNo[DEV_USED_gamepadIdx] = dagor_frame_no();
        return false;
      }

    return false;
  }
} inputHandler;

static unsigned get_cur_used_device_mask()
{
  unsigned mask = 0;
  unsigned currentFrame = dagor_frame_no();
  for (int i = DEV_USED_mouseIdx; i < DEV_USED_Count; i++)
  {
    if (lastInputOnFrameNo[i] && lastInputOnFrameNo[i] + FRAMES_THRESHOLD > currentFrame)
      mask |= (1 << i);
  }
  return mask;
}


namespace inputmonitor
{
void register_input_handler() { register_hid_event_handler(&inputHandler, 1000); }

void notify_input_devices_used()
{
  // detect and publicate to SQ changes in input devices used
  unsigned mask = get_cur_used_device_mask();
  if (!mask || mask == last_mask)
    return;

  last_mask = mask;
  Json::Value params;
  params["mask"] = mask;
  sqeventbus::send_event(INPUT_DEVICE_USED_EVENT, params);
}

///@module lastInputMonitor
void register_sq_module(SqModules *module_manager)
{
  HSQUIRRELVM vm = module_manager->getVM();
  Sqrat::Table moduleTbl(vm);
  moduleTbl.SetValue("DEV_MOUSE", DEV_USED_mouse)
    .SetValue("DEV_KBD", DEV_USED_kbd)
    .SetValue("DEV_GAMEPAD", DEV_USED_gamepad)
    .SetValue("DEV_TOUCH", DEV_USED_touch)
    .SetValue("INPUT_DEVICE_USED_EVENT", INPUT_DEVICE_USED_EVENT)
    .Func("get_last_used_device_mask", get_last_used_device_mask);

  module_manager->addNativeModule("lastInputMonitor", moduleTbl);
}
} // namespace inputmonitor