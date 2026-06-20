// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "gamepad_classdrv.h"
#include "gamepad_device.h"
#include <drv/hid/dag_hiGlobals.h>
#include <supp/_platform.h>
#include <xinput.h>
#include <debug/dag_debug.h>
#include <string.h>
#include <osApiWrappers/dag_threads.h>
#include <util/dag_delayedAction.h>
#include <startup/dag_inpDevClsDrv.h>
#include <osApiWrappers/dag_miscApi.h>
#include <osApiWrappers/dag_atomic.h>
#include <EASTL/type_traits.h>

#define MS_IN_SEC       1000
#define RESCAN_ATTEMPTS 10

#if _TARGET_PC_WIN
#ifndef WIN_NOEXCEPT
#define WIN_NOEXCEPT
#endif

static FARPROC get_xinput_proc(const char *name)
{
  static HMODULE mod = LoadLibraryA("xinput9_1_0.dll");
  return mod ? GetProcAddress(mod, name) : nullptr;
}

DWORD WINAPI XInputGetState(DWORD user_index, XINPUT_STATE *state) WIN_NOEXCEPT
{
  static auto fn = (eastl::add_pointer_t<decltype(::XInputGetState)>)(void *)get_xinput_proc("XInputGetState");
  return fn ? fn(user_index, state) : ERROR_DEVICE_NOT_CONNECTED;
}

DWORD WINAPI XInputSetState(DWORD user_index, XINPUT_VIBRATION *vibration) WIN_NOEXCEPT
{
  static auto fn = (eastl::add_pointer_t<decltype(::XInputSetState)>)(void *)get_xinput_proc("XInputSetState");
  return fn ? fn(user_index, vibration) : ERROR_DEVICE_NOT_CONNECTED;
}
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
  deviceConfigChanged = true;
  defJoy = NULL;
  enableAutoDef = false;
  prevUpdateRefTime = get_time_msec();
  nextUpdatePresenseTime = 0;
  secDrv = NULL;
  rescanCount = RESCAN_ATTEMPTS;
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

  deviceConfigChanged = true;
  refreshDeviceList();
  if (getDeviceCount() > 0)
    enable(true);

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
  bool was_enabled = enabled;
  if (deviceConfigChanged)
  {
    TIME_PROFILE(HID_XDRV_refreshDeviceList);
    destroyDevices();
    deviceConfigChanged = false;

    enable(false);

    DEBUG_CTX("Xbox360GamepadClassDriver::refreshDeviceList");

    XINPUT_STATE st;
    deviceStateMask = 0;
    for (int i = 0; i < GAMEPAD_MAX; i++)
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
  }

  if (secDrv && secDrv->isDeviceConfigChanged())
    secDrv->refreshDeviceList();

  if (deviceNum > 0)
    setDefaultJoystick(device[0]); // The most recent gamepad is enumerated first. Make it a default one.

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

  for (int i = 0; i < deviceNum; i++)
    device[i]->updateState(0, defJoy == device[i], defJoy != NULL);
  updateVirtualDevice();
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
  terminateInputUpdaterThread();
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


class HumanInput::XInputUpdater final : public DaThread
{
  HumanInput::Xbox360GamepadClassDriver &driver;

public:
  XInputUpdater(HumanInput::Xbox360GamepadClassDriver &drv) :
    DaThread("XInputUpdater", DEFAULT_STACK_SZ, 0, WORKER_THREADS_AFFINITY_MASK), driver(drv)
  {}

  void execute() override
  {
    unsigned int deviceStateMask = get_device_state_mask();
    ::add_delayed_action(new DelayedDeviceMaskSetter(driver, deviceStateMask));
  }
};


void HumanInput::Xbox360GamepadClassDriver::terminateInputUpdaterThread()
{
  if (inputUpdater)
    inputUpdater->terminate(true);
}


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
  int emulatedDeviceNum = (emulateSingleDevice && virtualDevice) ? 1 : deviceNum;
  return emulatedDeviceNum + (secDrv ? secDrv->getDeviceCount() : 0);
}


void HumanInput::Xbox360GamepadClassDriver::updateDevices()
{
  TIME_PROFILE(HID_XDRV_updateDevices);
  int curTimeMs = get_time_msec();
  int dt = curTimeMs - prevUpdateRefTime;
  prevUpdateRefTime = curTimeMs;

  if (emulateSingleDevice && virtualDevice)
    for (int i = 0; i < deviceNum; i++)
      device[i]->copyStickDeadZoneScales(*virtualDevice);

  if (prevUpdateRefTime >= nextUpdatePresenseTime && rescanCount > 0 && (!inputUpdater || !inputUpdater->isThreadRunnning()))
  {
    nextUpdatePresenseTime = prevUpdateRefTime + MS_IN_SEC;

    inputUpdater.reset(new XInputUpdater(*this));
    inputUpdater->start();
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
