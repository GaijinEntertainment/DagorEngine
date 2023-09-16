#include "gamepad_classdrv.h"
#include "gamepad_device.h"
#include <humanInput/dag_hiGlobals.h>
#include <supp/_platform.h>
#include <debug/dag_debug.h>
#include <util/dag_watchdog.h>
#include <string.h>
#include <steam/steam.h>

const char *HumanInput::SteamGamepadClassDriver::gamepadName[HumanInput::SteamGamepadClassDriver::GAMEPAD_MAX] = {
  "Controller 1", "Controller 2", "Controller 3", "Controller 4"};

HumanInput::SteamGamepadClassDriver::SteamGamepadClassDriver()
{
  enabled = false;
  defClient = NULL;
  memset(&device, 0, sizeof(device));
  deviceNum = 0;
  deviceStateMask = 0;
  deviceConfigChanged = false;
  defJoy = NULL;
  enableAutoDef = false;
  prevUpdateRefTime = ::ref_time_ticks();
  secDrv = NULL;
}

HumanInput::SteamGamepadClassDriver::~SteamGamepadClassDriver() { destroyDevices(); }

bool HumanInput::SteamGamepadClassDriver::init(const char *absolute_path_to_controller_config)
{
  stg_joy.enabled = false;

  if (!steamapi::is_running())
    return false;

  if (!steamapi::init_controller(absolute_path_to_controller_config))
    return false;

  refreshDeviceList();
  if (getDeviceCount() > 0)
    enable(true);

  return true;
}

void HumanInput::SteamGamepadClassDriver::destroyDevices()
{
  setDefaultJoystick(NULL);
  for (int i = 0; i < deviceNum; i++)
    delete device[i];
  deviceNum = 0;
  memset(device, 0, sizeof(device));

  if (stg_joy.enabled)
    steamapi::shutdown_controller();
}

void HumanInput::SteamGamepadClassDriver::refreshDeviceList()
{
  destroyDevices();

  bool was_enabled = enabled;
  int i;

  enable(false);

  debug_ctx("SteamGamepadClassDriver::refreshDeviceList");

  deviceStateMask = 0;
  for (i = 0; i < GAMEPAD_MAX; i++)
  {
    deviceStateMask <<= 1;
    if (!steamapi::get_controller_state(i, NULL, 0))
      continue;

    device[deviceNum] = new (inimem) SteamGamepadDevice(i, gamepadName[i], deviceNum);
    deviceNum++;
    deviceStateMask |= 1;
  }

  debug_ctx("deviceNum=%d", deviceNum);

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
  prevUpdateRefTime = ::ref_time_ticks();
  for (i = 0; i < deviceNum; i++)
    device[i]->updateState(0, defJoy == device[i], defJoy != NULL);

  deviceConfigChanged = false;
}

// generic hid class driver interface
void HumanInput::SteamGamepadClassDriver::enable(bool en)
{
  enabled = en;
  stg_joy.enabled = en;
  prevUpdateRefTime = ::ref_time_ticks();
  if (secDrv)
    secDrv->enable(en);
}
void HumanInput::SteamGamepadClassDriver::destroy()
{
  destroyDevices();
  stg_joy.present = false;

  destroy_it(secDrv);
  delete this;
}

void HumanInput::SteamGamepadClassDriver::updateDevices()
{
  // temporally disable watchdog because of XINPUT have habit to setup drivers on API calls
  // with dialog windows and everything (and it takes quite a long time obviously)
  int oldTrigThresh = watchdog_set_option(WATCHDOG_OPTION_TRIG_THRESHOLD, WATCHDOG_DISABLE, 1);

  unsigned int newDeviceStateMask = 0;

  int dt = get_time_usec(prevUpdateRefTime) / 1000;
  prevUpdateRefTime = ::ref_time_ticks();

  if (1)
  {
    for (int i = 0; i < GAMEPAD_MAX; i++)
      if (steamapi::get_controller_state(i, NULL, 0))
        newDeviceStateMask = (newDeviceStateMask << 1) | 1;
      else
        newDeviceStateMask <<= 1;

    if (newDeviceStateMask != deviceStateMask)
    {
      deviceConfigChanged = true;
      // if (deviceStateMask == 0)
      //   refreshDeviceList();
      deviceStateMask = newDeviceStateMask;
    }
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

  if (secDrv)
  {
    secDrv->updateDevices();
    if (!defJoy)
      setDefaultJoystick(secDrv->getDefaultJoystick());
  }

  watchdog_set_option(WATCHDOG_OPTION_TRIG_THRESHOLD, oldTrigThresh, 1); // restore watchdog
}

// generic joystick class driver interface
HumanInput::IGenJoystick *HumanInput::SteamGamepadClassDriver::getDevice(int idx) const
{
  if (idx >= deviceNum && secDrv)
    return secDrv->getDevice(idx - deviceNum);
  if ((uint32_t)idx < (uint32_t)deviceNum)
  {
    G_ANALYSIS_ASSUME(idx < GAMEPAD_MAX);
    return device[idx];
  }
  return NULL;
}
void HumanInput::SteamGamepadClassDriver::useDefClient(HumanInput::IGenJoystickClient *cli)
{
  defClient = cli;
  for (int i = 0; i < deviceNum; i++)
    device[i]->setClient(defClient);
  if (secDrv)
    secDrv->useDefClient(cli);
}

HumanInput::IGenJoystick *HumanInput::SteamGamepadClassDriver::getDeviceByUserId(unsigned short userId) const
{
  for (int i = 0; i < deviceNum; i++)
    if (device[i]->getUserId() == userId)
      return device[i];
  return secDrv ? secDrv->getDeviceByUserId(userId) : NULL;
}

void HumanInput::SteamGamepadClassDriver::setDefaultJoystick(IGenJoystick *ref)
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
