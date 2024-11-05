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
using namespace HumanInput;

Di8JoystickClassDriver::Di8JoystickClassDriver(bool exclude_xinput, bool remap_360) :
  device(inimem), enabled(false), excludeXinputDev(exclude_xinput), remapAsX360(remap_360)
{
  defJoy = NULL;
  enableAutoDef = true;
  add_wnd_proc_component(this);
  defClient = NULL;
  maybeDeviceConfigChanged = false;
  deviceConfigChanged = false;
  prevUpdateRefTime = ::ref_time_ticks();
  secDrv = NULL;
  deviceListCheckerIsRunning = 0;
}

Di8JoystickClassDriver::~Di8JoystickClassDriver()
{
  del_wnd_proc_component(this);
  destroyDevices();
}

bool Di8JoystickClassDriver::init()
{
  stg_joy.enabled = false;

  if (!tryRefreshDeviceList())
    return false;
  if (getDeviceCount() > 0)
    enable(true);
  return true;
}

IWndProcComponent::RetCode Di8JoystickClassDriver::process(void *hwnd, unsigned msg, uintptr_t wParam, intptr_t lParam,
  intptr_t &result)
{
  if (msg == WM_DEVICECHANGE && wParam == DBT_DEVNODES_CHANGED)
  {
    static int wmDeviceChangeCount = 0;
    wmDeviceChangeCount++;
    if (wmDeviceChangeCount < MAX_WM_DEVICECHANGE_COUNT)
      maybeDeviceConfigChanged = true;
  }
  return PROCEED_OTHER_COMPONENTS;
}

void Di8JoystickClassDriver::destroyDevices()
{
  setDefaultJoystick(NULL);
  unacquireDevices();
  for (int i = 0; i < device.size(); i++)
    delete device[i];
  clear_and_shrink(device);
}

// generic hid class driver interface
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
  for (int i = 0; i < device.size(); i++)
    device[i]->unacquire();
  if (secDrv)
    secDrv->unacquireDevices();
}
void Di8JoystickClassDriver::destroy()
{
  destroyDevices();
  stg_joy.present = false;

  destroy_it(secDrv);
  delete this;
}

class Di8JoystickClassDriver::AsyncDeviceListChecker : public DaThread
{
  Di8JoystickClassDriver &driver;

public:
  AsyncDeviceListChecker(HumanInput::Di8JoystickClassDriver &drv) :
    DaThread("AsyncDeviceListChecker", DEFAULT_STACK_SZ, 0, WORKER_THREADS_AFFINITY_MASK), driver(drv)
  {}

  virtual void execute()
  {
    driver.checkDeviceList();
    ::interlocked_release_store(driver.deviceListCheckerIsRunning, 0);
  }
};

void Di8JoystickClassDriver::updateDevices()
{
  if (maybeDeviceConfigChanged && !::interlocked_acquire_load(deviceListCheckerIsRunning))
  {
    ::interlocked_release_store(deviceListCheckerIsRunning, 1);
    deviceListChecker = eastl::make_unique<AsyncDeviceListChecker>(*this);
    deviceListChecker->start();
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

bool Di8JoystickClassDriver::deviceCheckerListRunning() const { return ::interlocked_acquire_load(deviceListCheckerIsRunning); }

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
