// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "gamepad_classdrv.h"
#include "gamepad_device.h"
#include <drv/hid/dag_hiGlobals.h>
#include <supp/_platform.h>
#if _TARGET_PC_WIN
#include <xinput.h>
#pragma comment(lib, "xinput9_1_0.lib")
#elif _TARGET_XBOX
#include "xboxOneGamepad.h"
#endif
#include <debug/dag_debug.h>
#include <string.h>
#include <osApiWrappers/dag_threads.h>
#include <util/dag_delayedAction.h>
#include <startup/dag_inpDevClsDrv.h>
#include <osApiWrappers/dag_miscApi.h>
#include <osApiWrappers/dag_atomic.h>

#define MS_IN_SEC       1000
#define RESCAN_ATTEMPTS 10


#if _TARGET_XBOX
void refresh_xbox_gamepads_list();
#endif


const char *HumanInput::Xbox360GamepadClassDriver::gamepadName[HumanInput::Xbox360GamepadClassDriver::GAMEPAD_MAX] = {
  "Controller 1", "Controller 2", "Controller 3", "Controller 4"};

HumanInput::Xbox360GamepadClassDriver::Xbox360GamepadClassDriver(bool emulate_single_gamepad)
{
  enabled = false;
  defClient = NULL;
  memset(&device, 0, sizeof(device));
  deviceNum = 0;
  deviceStateMask = 0;
  deviceConfigChanged = false;
  defJoy = NULL;
  enableAutoDef = false;
  prevUpdateRefTime = get_time_msec();
  nextUpdatePresenseTime = 0;
  secDrv = NULL;
  rescanCount = RESCAN_ATTEMPTS;
  updaterIsRunning = 0;
  emulateSingleDevice = emulate_single_gamepad;
  virtualDevice = NULL;

  add_wnd_proc_component(this);
}

HumanInput::Xbox360GamepadClassDriver::~Xbox360GamepadClassDriver()
{
  del_wnd_proc_component(this);
  destroyDevices();
}

bool HumanInput::Xbox360GamepadClassDriver::init()
{
  stg_joy.enabled = false;

  refreshDeviceList();
  if (getDeviceCount() > 0)
    enable(true);

#if _TARGET_XBOX
  initXbox();
#endif

  return true;
}

void HumanInput::Xbox360GamepadClassDriver::destroyDevices()
{
  setDefaultJoystick(NULL);
  for (int i = 0; i < deviceNum; i++)
    delete device[i];
  deviceNum = 0;
  memset(device, 0, sizeof(device));

  if (emulateSingleDevice && virtualDevice != NULL)
  {
    delete virtualDevice;
    virtualDevice = NULL;
  }
}

void HumanInput::Xbox360GamepadClassDriver::refreshDeviceList()
{
  destroyDevices();

  bool was_enabled = enabled;
  int i;

  enable(false);

  DEBUG_CTX("Xbox360GamepadClassDriver::refreshDeviceList");

#if _TARGET_XBOX
  refresh_xbox_gamepads_list();
  updateXboxGamepads(); // Update XInputGetState shadow states.
#endif

  XINPUT_STATE st;
  deviceStateMask = 0;
  for (i = 0; i < GAMEPAD_MAX; i++)
  {
    deviceStateMask <<= 1;
    if (XInputGetState(i, &st) == ERROR_DEVICE_NOT_CONNECTED)
      continue;

    device[deviceNum] = new (inimem) Xbox360GamepadDevice(i, gamepadName[i], deviceNum);
    for (int s = 0; s < 2; s++)
      device[deviceNum]->setStickDeadZoneScale(s, stickDeadZoneScale[s]);

    deviceNum++;
    deviceStateMask |= 1;
  }
  if (emulateSingleDevice && virtualDevice == NULL)
  {
    virtualDevice = new Xbox360GamepadDevice(0, "Controller", 0, true);
    for (int s = 0; s < 2; s++)
      virtualDevice->setStickDeadZoneScale(s, stickDeadZoneScale[s]);
  }
  DEBUG_CTX("deviceNum=%d", deviceNum);

  if (deviceNum > 0)
    setDefaultJoystick(device[0]); // The most recent gamepad is enumerated first. Make it a default one.

  if (secDrv)
  {
    secDrv->refreshDeviceList();
    secDrv->setDefaultJoystick(defJoy);
  }

  // update global settings
  if (getDeviceCount() > 0)
  {
    stg_joy.present = true;
    enable(was_enabled);
  }
  else
  {
    stg_joy.present = false;
    stg_joy.enabled = false;
  }

  useDefClient(defClient);
  prevUpdateRefTime = get_time_msec();

  for (i = 0; i < deviceNum; i++)
    device[i]->updateState(0, defJoy == device[i], defJoy != NULL);
  updateVirtualDevice();

  deviceConfigChanged = false;
}

// generic hid class driver interface
void HumanInput::Xbox360GamepadClassDriver::enable(bool en)
{
  enabled = en;
  stg_joy.enabled = en;
  prevUpdateRefTime = get_time_msec();
  if (secDrv)
    secDrv->enable(en);
}
void HumanInput::Xbox360GamepadClassDriver::destroy()
{
  destroyDevices();
  stg_joy.present = false;

  destroy_it(secDrv);
  delete this;
}


IWndProcComponent::RetCode HumanInput::Xbox360GamepadClassDriver::process(void *hwnd, unsigned msg, uintptr_t wParam, intptr_t lParam,
  intptr_t &result)
{
#if _TARGET_PC_WIN
  if (msg == WM_DEVICECHANGE)
  {
    static int wmDeviceChangeCount = 0;
    wmDeviceChangeCount++;
    if (wmDeviceChangeCount == MAX_WM_DEVICECHANGE_COUNT)
      debug("Disable further WM_DEVICECHANGE");
    if (wmDeviceChangeCount < MAX_WM_DEVICECHANGE_COUNT)
    {
      debug("Xbox360GamepadClassDriver: WM_DEVICECHANGE");
      rescanCount = RESCAN_ATTEMPTS;
    }
  }
#endif
  return PROCEED_OTHER_COMPONENTS;
}


static unsigned int get_device_state_mask()
{
  unsigned int deviceStateMask = 0;
  XINPUT_STATE st;
  for (int i = 0; i < HumanInput::Xbox360GamepadClassDriver::GAMEPAD_MAX; i++)
    if (XInputGetState(i, &st) != ERROR_DEVICE_NOT_CONNECTED)
      deviceStateMask = (deviceStateMask << 1) | 1;
    else
      deviceStateMask <<= 1;
  return deviceStateMask;
}


class DelayedDeviceMaskSetter : public DelayedAction
{
  HumanInput::Xbox360GamepadClassDriver &driver;
  unsigned int deviceStateMask;

public:
  DelayedDeviceMaskSetter(HumanInput::Xbox360GamepadClassDriver &drv, const unsigned int device_mask) :
    driver(drv), deviceStateMask(device_mask)
  {}

  void performAction() override
  {
    if (::global_cls_drv_joy)
      driver.setDeviceMask(deviceStateMask);
  }
};


class HumanInput::XInputUpdater : public DaThread
{
  HumanInput::Xbox360GamepadClassDriver &driver;

public:
  XInputUpdater(HumanInput::Xbox360GamepadClassDriver &drv) :
    DaThread("XInputUpdater", DEFAULT_STACK_SZ, 0, WORKER_THREADS_AFFINITY_MASK), driver(drv)
  {}

  virtual void execute()
  {
    unsigned int deviceStateMask = get_device_state_mask();
    ::add_delayed_action(new DelayedDeviceMaskSetter(driver, deviceStateMask));
    interlocked_release_store(driver.updaterIsRunning, 0);
  }
};


void HumanInput::Xbox360GamepadClassDriver::setDeviceMask(const unsigned int new_mask)
{
  if (new_mask != deviceStateMask)
  {
    deviceConfigChanged = true;
    deviceStateMask = new_mask;
  }
  rescanCount--;
}

void HumanInput::Xbox360GamepadClassDriver::updateVirtualDevice()
{
  if (!emulateSingleDevice || virtualDevice == NULL)
    return;

  HumanInput::JoystickRawState rs = {0};
  for (int i = 0; i < deviceNum; i++)
  {
    HumanInput::JoystickRawState jrs = device[i]->getRawState();
    rs.buttons.bitOr(jrs.buttons);

#define SUM_FIELD(field_name) rs.field_name += jrs.field_name;
    SUM_FIELD(x);
    SUM_FIELD(y);
    SUM_FIELD(z);
    SUM_FIELD(rx);
    SUM_FIELD(ry);
    SUM_FIELD(slider[0]);
    SUM_FIELD(slider[1]);
#undef SUM_FIELD
  }

  virtualDevice->setRawState(rs);
}


int HumanInput::Xbox360GamepadClassDriver::getDeviceCount() const
{
  int emulatedDeviceNum = emulateSingleDevice ? 1 : deviceNum;
  return emulatedDeviceNum + (secDrv ? secDrv->getDeviceCount() : 0);
}


void HumanInput::Xbox360GamepadClassDriver::updateDevices()
{
  int curTimeMs = get_time_msec();
  int dt = curTimeMs - prevUpdateRefTime;
  prevUpdateRefTime = curTimeMs;

  if (emulateSingleDevice && virtualDevice)
    for (int i = 0; i < deviceNum; i++)
      device[i]->copyStickDeadZoneScales(*virtualDevice);

#if _TARGET_XBOX
  updateXboxGamepads();
  rescanCount = 1; // Always rescan on xbox instead of relying on WM_DEVICECHANGE.
#endif

  if (prevUpdateRefTime >= nextUpdatePresenseTime && rescanCount > 0 && !interlocked_acquire_load(updaterIsRunning))
  {
    nextUpdatePresenseTime = prevUpdateRefTime + MS_IN_SEC;

#if _TARGET_XBOX
    setDeviceMask(get_device_state_mask());
#else
    interlocked_release_store(updaterIsRunning, 1);
    inputUpdater.reset(new XInputUpdater(*this));
    inputUpdater->start();
#endif //_TARGET_XBOX
  }

  if (defJoy || !enableAutoDef)
    for (int i = 0; i < deviceNum; i++)
      device[i]->updateState(dt, defJoy == device[i], defJoy != NULL);
  else
    for (int i = 0; i < deviceNum; i++)
      if (device[i]->updateState(dt, false, defJoy != NULL) && !defJoy)
      {
        setDefaultJoystick(device[i]);

        if (defJoy->getClient())
          defJoy->getClient()->stateChanged(defJoy, i);
      }

  updateVirtualDevice();

  if (secDrv)
  {
    secDrv->updateDevices();
    if (!defJoy)
      setDefaultJoystick(secDrv->getDefaultJoystick());
  }
}

// generic joystick class driver interface
HumanInput::IGenJoystick *HumanInput::Xbox360GamepadClassDriver::getDevice(int idx) const
{
  if (emulateSingleDevice)
  {
    if (idx == 0)
      return virtualDevice;
    if (secDrv != NULL)
      return secDrv->getDevice(idx - 1);
    return NULL;
  }
  else
  {
    if ((uint32_t)idx < (uint32_t)deviceNum)
    {
      G_ANALYSIS_ASSUME(idx < GAMEPAD_MAX);
      return device[idx];
    }
    if (secDrv != NULL)
      return secDrv->getDevice(idx - deviceNum);
    return NULL;
  }
}
void HumanInput::Xbox360GamepadClassDriver::useDefClient(HumanInput::IGenJoystickClient *cli)
{
  defClient = cli;
  for (int i = 0; i < deviceNum; i++)
    device[i]->setClient(defClient);
  if (secDrv)
    secDrv->useDefClient(cli);
}

HumanInput::IGenJoystick *HumanInput::Xbox360GamepadClassDriver::getDeviceByUserId(unsigned short userId) const
{
  for (int i = 0; i < deviceNum; i++)
    if (device[i]->getUserId() == userId)
      return device[i];
  return secDrv ? secDrv->getDeviceByUserId(userId) : NULL;
}

void HumanInput::Xbox360GamepadClassDriver::setDefaultJoystick(IGenJoystick *ref)
{
  if (ref != defJoy)
  {
    if (ref)
      raw_state_joy = ref->getRawState();
    else
      memset(&raw_state_joy, 0, sizeof(raw_state_joy));
  }

  defJoy = ref;
  if (secDrv)
    secDrv->setDefaultJoystick(ref);
}

void HumanInput::Xbox360GamepadClassDriver::setStickDeadZoneScale(int stick_idx, bool main_dev, float scale)
{
  if (!main_dev)
    return;
  G_ASSERT_RETURN(stick_idx >= 0 && stick_idx < 2, );
  stickDeadZoneScale[stick_idx] = scale;
  for (Xbox360GamepadDevice *dev : device)
    if (dev)
      dev->setStickDeadZoneScale(stick_idx, scale);
}
float HumanInput::Xbox360GamepadClassDriver::getStickDeadZoneAbs(int stick_idx, bool main_dev, IGenJoystick *for_joy) const
{
  if (!main_dev)
    return 0;
  G_ASSERT_RETURN(stick_idx >= 0 && stick_idx < 2, 0);
  for (const Xbox360GamepadDevice *dev : device)
  {
    if (!dev)
      continue;
    if (for_joy && dev != for_joy)
      continue;
    return dev->getStickDeadZoneAbs(stick_idx);
  }
  return stickDeadZoneScale[stick_idx] * axis_dead_thres[0][stick_idx] / JOY_XINPUT_MAX_AXIS_VAL;
}


#if _TARGET_XBOX

#include <osApiWrappers/gdk/gameinput.h>

static constexpr size_t GAMEPADS_MAX = gdk::gameinput::MAX_DEVICES_PER_TYPE;
static DagorXboxOneGamepadState g_st[GAMEPADS_MAX] = {0};
static gdk::gameinput::DevicesList devices;

void refresh_xbox_gamepads_list() {}

void HumanInput::Xbox360GamepadClassDriver::initXbox() {}

int XInputGetState(int idx, DagorXboxOneGamepadState *st)
{
  if (idx < 0 || idx >= GAMEPADS_MAX || !devices[idx])
    return ERROR_DEVICE_NOT_CONNECTED;
  if (st)
    *st = g_st[idx];
  return 0;
}
int XInputSetState(int idx, DagorXboxOneGamepadVibro *xiv)
{
  if (idx < 0 || idx >= GAMEPADS_MAX || !devices[idx])
    return ERROR_DEVICE_NOT_CONNECTED;
  const GameInputRumbleParams params = {xiv->wLeftMotorSpeed / 65535.0f, xiv->wRightMotorSpeed / 65535.0f, 0, 0};
  if (devices[idx])
    devices[idx]->SetRumbleState(&params);
  return 0;
}

void HumanInput::Xbox360GamepadClassDriver::updateXboxGamepads()
{
  gdk::gameinput::get_devices(GameInputKindGamepad, devices);

#define REMAP_BTN(X, B)                    \
  if (state.buttons & GameInputGamepad##X) \
  st.wButtons |= HumanInput::JOY_XINPUT_REAL_MASK_##B
#define REMAP_AXIS(X, A, S) st.A = (int)floorf(state.X * (S))

  for (size_t i = 0; i < GAMEPADS_MAX; ++i)
  {
    if (devices[i])
    {
      gdk::gameinput::Reading reading = gdk::gameinput::get_current_reading(GameInputKindGamepad, devices[i]);
      if (reading)
      {
        GameInputGamepadState state;
        if (SUCCEEDED(reading->GetGamepadState(&state)))
        {
          DagorXboxOneGamepadState::Data st = {0};

          REMAP_BTN(A, A);
          REMAP_BTN(B, B);
          REMAP_BTN(X, X);
          REMAP_BTN(Y, Y);
          REMAP_BTN(DPadDown, D_DOWN);
          REMAP_BTN(DPadLeft, D_LEFT);
          REMAP_BTN(DPadRight, D_RIGHT);
          REMAP_BTN(DPadUp, D_UP);
          REMAP_BTN(LeftShoulder, L_SHOULDER);
          REMAP_BTN(LeftThumbstick, L_THUMB);
          REMAP_BTN(Menu, START);
          REMAP_BTN(RightShoulder, R_SHOULDER);
          REMAP_BTN(RightThumbstick, R_THUMB);
          REMAP_BTN(View, BACK);

          REMAP_AXIS(leftTrigger, bLeftTrigger, 255);
          REMAP_AXIS(rightTrigger, bRightTrigger, 255);
          REMAP_AXIS(leftThumbstickX, sThumbLX, JOY_XINPUT_MAX_AXIS_VAL);
          REMAP_AXIS(leftThumbstickY, sThumbLY, JOY_XINPUT_MAX_AXIS_VAL);
          REMAP_AXIS(rightThumbstickX, sThumbRX, JOY_XINPUT_MAX_AXIS_VAL);
          REMAP_AXIS(rightThumbstickY, sThumbRY, JOY_XINPUT_MAX_AXIS_VAL);

          if (memcmp(&st, &g_st[i].Gamepad, sizeof(st)) == 0)
            continue;

          g_st[i].Gamepad = st;
          g_st[i].dwPacketNumber++;
        }
      }
    }
  }

#undef REMAP_AXIS
#undef REMAP_BTN
}

#endif
