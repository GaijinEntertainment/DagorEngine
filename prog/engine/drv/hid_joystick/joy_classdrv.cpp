// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "include_dinput.h"
#include "joy_classdrv.h"
#include "joy_device.h"
#include <drv/hid/dag_hiDInput.h>
#include <drv/hid/dag_hiGlobals.h>
#include <osApiWrappers/dag_progGlobals.h>
#include <debug/dag_hwExcept.h>
#include <debug/dag_except.h>
#include <debug/dag_debug.h>
#include <dbt.h>
#include <osApiWrappers/dag_progGlobals.h>
#include <osApiWrappers/dag_atomic.h>
#include <osApiWrappers/dag_threads.h>
#include <perfMon/dag_statDrv.h>
using namespace HumanInput;


static const int ENUMERATION_TIMEOUT_MS = 30000;

Di8JoystickClassDriver::Di8JoystickClassDriver(bool exclude_xinput, bool remap_360) :
  device(inimem),
  enabled(false),
  excludeXinputDev(exclude_xinput),
  remapAsX360(remap_360),
  deviceEnumerator{ENUMERATION_TIMEOUT_MS, exclude_xinput, remap_360}
{
  defJoy = NULL;
  enableAutoDef = true;
  add_wnd_proc_component(this);
  defClient = NULL;
  maybeDeviceConfigChanged = false;
  deviceConfigChanged = false;
  prevUpdateRefTime = ::ref_time_ticks();
  secDrv = NULL;
}


Di8JoystickClassDriver::~Di8JoystickClassDriver()
{
  deviceEnumerator.terminate(true, 1000);
  del_wnd_proc_component(this);
  destroyDevices();
}


bool Di8JoystickClassDriver::init()
{
  stg_joy.enabled = false;

  maybeDeviceConfigChanged = true;
  enable(true);
  return enabled;
}


IWndProcComponent::RetCode Di8JoystickClassDriver::process(void *hwnd, unsigned msg, uintptr_t wParam, intptr_t lParam,
  intptr_t &result)
{
  if (msg == WM_DEVICECHANGE && wParam == DBT_DEVNODES_CHANGED)
  {
    debug("[HID][DI8] %s() got devnodes changed", __FUNCTION__);
    maybeDeviceConfigChanged = true;
  }
  return PROCEED_OTHER_COMPONENTS;
}


void Di8JoystickClassDriver::destroyDevices()
{
  TIME_PROFILE(HID_DI8_destroyDevices);
  setDefaultJoystick(NULL);
  unacquireDevices();
  for (int i = 0; i < device.size(); i++)
    delete device[i];
  clear_and_shrink(device);
}


void Di8JoystickClassDriver::enable(bool en)
{
  enabled = en;
  stg_joy.enabled = en;
  prevUpdateRefTime = ::ref_time_ticks();
  if (secDrv)
    secDrv->enable(en);
}


void Di8JoystickClassDriver::acquireDevices()
{
  for (int i = 0; i < device.size(); i++)
    device[i]->acquire();
  if (secDrv)
    secDrv->acquireDevices();
}


void Di8JoystickClassDriver::unacquireDevices()
{
  TIME_PROFILE(HID_DI8_unacquireDevices);
  for (int i = 0; i < device.size(); i++)
    device[i]->unacquire();
  if (secDrv)
    secDrv->unacquireDevices();
}


void Di8JoystickClassDriver::destroy()
{
  deviceEnumerator.terminate(true, 1000);
  destroyDevices();
  stg_joy.present = false;

  destroy_it(secDrv);
  delete this;
}


void Di8JoystickClassDriver::updateDevices()
{
  TIME_PROFILE(HID_DI8_updateDevices);

  // TODO: some better debounce might be necessary
  if (maybeDeviceConfigChanged && !deviceEnumerator.isThreadRunnning())
  {
    deviceEnumerator.start();
    maybeDeviceConfigChanged = false;
  }

  if (!deviceEnumerator.isThreadRunnning())
    deviceEnumerator.terminate(false);
  if (deviceEnumerator.foundChanges())
  {
    debug("[HID][DI8] found device changes");
    maybeDeviceConfigChanged = false;
    deviceConfigChanged = true;
  }

  if (!enabled)
    return;

  int dt = get_time_usec(prevUpdateRefTime) / 1000;
  prevUpdateRefTime = ::ref_time_ticks();

  if (!defJoy && enableAutoDef)
    for (int i = 0; i < device.size(); i++)
    {
      device[i]->poll();
      if (device[i]->updateState(dt, false) && !defJoy)
      {
        setDefaultJoystick(device[i]);

        if (defJoy->getClient())
          defJoy->getClient()->stateChanged(defJoy, i);
      }
    }
  else
    for (int i = 0; i < device.size(); i++)
    {
      device[i]->poll();
      device[i]->updateState(dt, device[i] == defJoy);
    }

  if (secDrv)
  {
    secDrv->updateDevices();
    if (!defJoy)
      setDefaultJoystick(secDrv->getDefaultJoystick());
  }
}


// generic joystick class driver interface
IGenJoystick *Di8JoystickClassDriver::getDevice(int idx) const
{
  if (idx >= device.size() && secDrv)
    return secDrv->getDevice(idx - device.size());
  if (idx >= 0 && idx < device.size())
    return device[idx];
  return NULL;
}


void Di8JoystickClassDriver::useDefClient(IGenJoystickClient *cli)
{
  defClient = cli;
  for (int i = 0; i < device.size(); i++)
    device[i]->setClient(defClient);
  if (secDrv)
    secDrv->useDefClient(cli);
}


HumanInput::IGenJoystick *HumanInput::Di8JoystickClassDriver::getDeviceByUserId(unsigned short userId) const
{
  for (int i = 0; i < device.size(); i++)
    if (device[i]->getUserId() == userId)
      return device[i];
  return secDrv ? secDrv->getDeviceByUserId(userId) : NULL;
}


void HumanInput::Di8JoystickClassDriver::setDefaultJoystick(IGenJoystick *ref)
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
