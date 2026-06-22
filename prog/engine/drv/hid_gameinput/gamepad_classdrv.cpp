// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "gamepad_classdrv.h"
#include "gamepad_device.h"
#include <drv/hid/dag_hiGlobals.h>
#include <supp/_platform.h>
#include <debug/dag_debug.h>
#include <string.h>
#include <startup/dag_inpDevClsDrv.h>
#include <osApiWrappers/dag_miscApi.h>
#include <osApiWrappers/gdk/gameinput.h>

static constexpr int MS_IN_SEC = 1000;


const char *HumanInput::GameInputGamepadClassDriver::gamepadName[HumanInput::GameInputGamepadClassDriver::GAMEPAD_MAX] = {
  "Controller 1", "Controller 2", "Controller 3", "Controller 4", "Controller 5", "Controller 6", "Controller 7", "Controller 8"};

HumanInput::GameInputGamepadClassDriver::GameInputGamepadClassDriver(bool emulate_single_gamepad) :
  prevUpdateRefTime(get_time_msec()), emulateSingleDevice(emulate_single_gamepad)
{}


HumanInput::GameInputGamepadClassDriver::~GameInputGamepadClassDriver() { destroyDevices(); }


bool HumanInput::GameInputGamepadClassDriver::init()
{
  stg_joy.enabled = false;

  deviceConfigChanged = true;
  refreshDeviceList();
  if (getDeviceCount() > 0)
    enable(true);

  return true;
}


void HumanInput::GameInputGamepadClassDriver::destroyDevices()
{
  setDefaultJoystick(nullptr);
  for (int i = 0; i < deviceNum; i++)
  {
    delete device[i];
    device[i] = nullptr;
  }
  deviceNum = 0;

  if (emulateSingleDevice && virtualDevice != nullptr)
  {
    delete virtualDevice;
    virtualDevice = nullptr;
  }
}


void HumanInput::GameInputGamepadClassDriver::refreshDeviceList()
{
  bool was_enabled = enabled;
  if (deviceConfigChanged)
  {
    TIME_PROFILE(HID_GINP_refreshDeviceList);
    destroyDevices();
    deviceConfigChanged = false;

    enable(false);

    DEBUG_CTX("GameInputGamepadClassDriver::refreshDeviceList");

    refresh_gamepads();

    deviceStateMask = 0;
    for (int i = 0; i < GAMEPAD_MAX; i++)
    {
      deviceStateMask <<= 1;
      if (!is_gamepad_connected(i))
        continue;

      device[deviceNum] = new (inimem) GameInputGamepadDevice(i, gamepadName[i], deviceNum);
      for (int s = 0; s < 2; s++)
        device[deviceNum]->setStickDeadZoneScale(s, stickDeadZoneScale[s]);

      deviceNum++;
      deviceStateMask |= 1;
    }
    if (emulateSingleDevice && virtualDevice == nullptr)
    {
      virtualDevice = new GameInputGamepadDevice(0, "Controller", 0, true);
      for (int s = 0; s < 2; s++)
        virtualDevice->setStickDeadZoneScale(s, stickDeadZoneScale[s]);
    }
    DEBUG_CTX("deviceNum=%d", deviceNum);
  }

  if (secDrv && secDrv->isDeviceConfigChanged())
    secDrv->refreshDeviceList();

  if (deviceNum > 0)
    setDefaultJoystick(device[0]);

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

  for (GameInputGamepadDevice *dev : device)
    if (dev)
      dev->updateState(0, defJoy == dev, defJoy != nullptr);
  updateVirtualDevice();
}

// generic hid class driver interface
void HumanInput::GameInputGamepadClassDriver::enable(bool en)
{
  enabled = en;
  stg_joy.enabled = en;
  prevUpdateRefTime = get_time_msec();
  if (secDrv)
    secDrv->enable(en);
}


void HumanInput::GameInputGamepadClassDriver::destroy()
{
  destroyDevices();
  stg_joy.present = false;

  destroy_it(secDrv);
  delete this;
}


static unsigned int get_device_state_mask()
{
  unsigned int deviceStateMask = 0;
  for (int i = 0; i < HumanInput::GameInputGamepadClassDriver::GAMEPAD_MAX; i++)
    deviceStateMask = (deviceStateMask << 1) | (is_gamepad_connected(i) ? 1 : 0);
  return deviceStateMask;
}


void HumanInput::GameInputGamepadClassDriver::setDeviceMask(const unsigned int new_mask)
{
  if (new_mask != deviceStateMask)
  {
    deviceConfigChanged = true;
    deviceStateMask = new_mask;
  }
}


void HumanInput::GameInputGamepadClassDriver::updateVirtualDevice()
{
  if (!emulateSingleDevice || virtualDevice == nullptr)
    return;

  HumanInput::JoystickRawState rs = {};
  for (GameInputGamepadDevice *dev : device)
  {
    if (!dev)
      continue;
    HumanInput::JoystickRawState jrs = dev->getRawState();
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


int HumanInput::GameInputGamepadClassDriver::getDeviceCount() const
{
  int emulatedDeviceNum = (emulateSingleDevice && virtualDevice) ? 1 : deviceNum;
  return emulatedDeviceNum + (secDrv ? secDrv->getDeviceCount() : 0);
}


void HumanInput::GameInputGamepadClassDriver::updateDevices()
{
  TIME_PROFILE(HID_GINP_updateDevices);
  int curTimeMs = get_time_msec();
  int dt = curTimeMs - prevUpdateRefTime;
  prevUpdateRefTime = curTimeMs;

  if (emulateSingleDevice && virtualDevice)
    for (GameInputGamepadDevice *dev : device)
      if (dev)
        dev->copyStickDeadZoneScales(*virtualDevice);

  refresh_gamepads();

  if (prevUpdateRefTime >= nextUpdatePresenseTime)
  {
    nextUpdatePresenseTime = prevUpdateRefTime + MS_IN_SEC;
    setDeviceMask(get_device_state_mask());
  }

  if (defJoy || !enableAutoDef)
    for (int i = 0; i < deviceNum; i++)
      device[i]->updateState(dt, defJoy == device[i], defJoy != nullptr);
  else
    for (int i = 0; i < deviceNum; i++)
      if (device[i]->updateState(dt, false, defJoy != nullptr) && !defJoy)
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
HumanInput::IGenJoystick *HumanInput::GameInputGamepadClassDriver::getDevice(int idx) const
{
  if (emulateSingleDevice)
  {
    if (idx == 0)
      return virtualDevice;
    if (secDrv != nullptr)
      return secDrv->getDevice(idx - 1);
    return nullptr;
  }
  else
  {
    if ((uint32_t)idx < (uint32_t)deviceNum)
    {
      G_ANALYSIS_ASSUME(idx < GAMEPAD_MAX);
      return device[idx];
    }
    if (secDrv != nullptr)
      return secDrv->getDevice(idx - deviceNum);
    return nullptr;
  }
}


void HumanInput::GameInputGamepadClassDriver::useDefClient(HumanInput::IGenJoystickClient *cli)
{
  defClient = cli;
  for (GameInputGamepadDevice *dev : device)
    if (dev)
      dev->setClient(defClient);
  if (secDrv)
    secDrv->useDefClient(cli);
}


HumanInput::IGenJoystick *HumanInput::GameInputGamepadClassDriver::getDeviceByUserId(unsigned short userId) const
{
  for (GameInputGamepadDevice *dev : device)
    if (dev && dev->getUserId() == userId)
      return dev;
  return secDrv ? secDrv->getDeviceByUserId(userId) : nullptr;
}


void HumanInput::GameInputGamepadClassDriver::setDefaultJoystick(IGenJoystick *ref)
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


void HumanInput::GameInputGamepadClassDriver::setStickDeadZoneScale(int stick_idx, bool main_dev, float scale)
{
  if (!main_dev)
    return;
  G_ASSERT_RETURN(stick_idx >= 0 && stick_idx < 2, );
  stickDeadZoneScale[stick_idx] = scale;
  for (GameInputGamepadDevice *dev : device)
    if (dev)
      dev->setStickDeadZoneScale(stick_idx, scale);
}


float HumanInput::GameInputGamepadClassDriver::getStickDeadZoneAbs(int stick_idx, bool main_dev, IGenJoystick *for_joy) const
{
  if (!main_dev)
    return 0;
  G_ASSERT_RETURN(stick_idx >= 0 && stick_idx < 2, 0);
  for (const GameInputGamepadDevice *dev : device)
  {
    if (!dev)
      continue;
    if (for_joy && dev != for_joy)
      continue;
    return dev->getStickDeadZoneAbs(stick_idx);
  }
  return stickDeadZoneScale[stick_idx] * axis_dead_thres[0][stick_idx] / JOY_XINPUT_MAX_AXIS_VAL;
}


static gdk::gameinput::DevicesList devices;

void refresh_gamepads() { gdk::gameinput::get_devices(GameInputKindGamepad, devices); }

bool is_gamepad_connected(int slot) { return slot >= 0 && slot < (int)devices.size() && devices[slot]; }

bool read_gamepad(int slot, GamepadReading &out)
{
  TIME_PROFILE(HID_GINP_readGamepad);
  if (!is_gamepad_connected(slot))
    return false;

  gdk::gameinput::Reading reading = gdk::gameinput::get_current_reading(GameInputKindGamepad, devices[slot]);
  if (!reading)
    return false;

  GameInputGamepadState state;
  if (!reading->GetGamepadState(&state))
    return false;

  out = {};
#define REMAP_BTN(X, B)                    \
  if (state.buttons & GameInputGamepad##X) \
  out.buttons |= HumanInput::JOY_XINPUT_REAL_MASK_##B
#define REMAP_AXIS(X, A, S) out.A = (int)floorf(state.X * (S))

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

  REMAP_AXIS(leftTrigger, leftTrigger, 255);
  REMAP_AXIS(rightTrigger, rightTrigger, 255);
  REMAP_AXIS(leftThumbstickX, leftThumbX, HumanInput::JOY_XINPUT_MAX_AXIS_VAL);
  REMAP_AXIS(leftThumbstickY, leftThumbY, HumanInput::JOY_XINPUT_MAX_AXIS_VAL);
  REMAP_AXIS(rightThumbstickX, rightThumbX, HumanInput::JOY_XINPUT_MAX_AXIS_VAL);
  REMAP_AXIS(rightThumbstickY, rightThumbY, HumanInput::JOY_XINPUT_MAX_AXIS_VAL);

#undef REMAP_AXIS
#undef REMAP_BTN
  return true;
}


void set_gamepad_rumble(int slot, float low_freq, float high_freq)
{
  if (!is_gamepad_connected(slot))
    return;
  const GameInputRumbleParams params = {low_freq, high_freq, 0, 0};
  devices[slot]->SetRumbleState(&params);
}
