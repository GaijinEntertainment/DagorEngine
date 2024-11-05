// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "joy_classdrv.h"
#include "joy_device.h"
#include <generic/dag_sort.h>
#include <drv/hid/dag_hiGlobals.h>
#include <osApiWrappers/dag_progGlobals.h>
#include <debug/dag_hwExcept.h>
#include <debug/dag_except.h>
#include <debug/dag_debug.h>
#include <osApiWrappers/dag_progGlobals.h>

using namespace HumanInput;

HidJoystickClassDriver::HidJoystickClassDriver() : device(inimem), enabled(false)
{
  defJoy = NULL;
  enableAutoDef = true;
  defClient = NULL;
  deviceConfigChanged = false;
  prevUpdateRefTime = ::ref_time_ticks();
  secDrv = NULL;

  OSStatus result = -1;

  // create the manager
  gIOHIDManagerRef = IOHIDManagerCreate(kCFAllocatorDefault, kIOHIDOptionsTypeNone);
  if (gIOHIDManagerRef)
  {
    // open it
    IOReturn tIOReturn = IOHIDManagerOpen(gIOHIDManagerRef, kIOHIDOptionsTypeNone);
    if (kIOReturnSuccess == tIOReturn)
      debug("IOHIDManager (%p) created and opened!\n", (void *)gIOHIDManagerRef);
    else
      debug("Couldn't open IOHIDManager.");
  }
  else
    debug("Couldn't create a IOHIDManager.");
  if (gIOHIDManagerRef)
  {
    // schedule with runloop
    IOHIDManagerScheduleWithRunLoop(gIOHIDManagerRef, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
    // register callbacks
    IOHIDManagerRegisterDeviceMatchingCallback(gIOHIDManagerRef, deviceMatchingCallback, this);
    IOHIDManagerRegisterDeviceRemovalCallback(gIOHIDManagerRef, deviceRemovalCallback, this);
  }

  HIDBuildMultiDeviceList();
  refreshDeviceList();
}

HidJoystickClassDriver::~HidJoystickClassDriver()
{
  destroyDevices();

  if (gIOHIDManagerRef)
  {
    IOHIDManagerRegisterDeviceMatchingCallback(gIOHIDManagerRef, NULL, NULL);
    IOHIDManagerRegisterDeviceRemovalCallback(gIOHIDManagerRef, NULL, NULL);
    IOHIDManagerUnscheduleFromRunLoop(gIOHIDManagerRef, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
  }
  if (gElementCFArrayRef)
  {
    CFRelease(gElementCFArrayRef);
    gElementCFArrayRef = NULL;
  }
  if (gDeviceCFArrayRef)
  {
    CFRelease(gDeviceCFArrayRef);
    gDeviceCFArrayRef = NULL;
  }
  if (gIOHIDManagerRef)
  {
    IOHIDManagerClose(gIOHIDManagerRef, 0);
    gIOHIDManagerRef = NULL;
  }
}

bool HidJoystickClassDriver::init()
{
  stg_joy.enabled = false;

  refreshDeviceList();
  if (getDeviceCount() > 0)
    enable(true);
  return true;
}

void HidJoystickClassDriver::destroyDevices()
{
  setDefaultJoystick(NULL);
  unacquireDevices();
  for (int i = 0; i < device.size(); i++)
    delete device[i];
  clear_and_shrink(device);
}

// generic hid class driver interface
void HidJoystickClassDriver::enable(bool en)
{
  enabled = en;
  stg_joy.enabled = en;
  prevUpdateRefTime = ::ref_time_ticks();
  if (secDrv)
    secDrv->enable(en);
}
void HidJoystickClassDriver::acquireDevices()
{
  for (int i = 0; i < device.size(); i++)
    device[i]->acquire();
  if (secDrv)
    secDrv->acquireDevices();
}
void HidJoystickClassDriver::unacquireDevices()
{
  for (int i = 0; i < device.size(); i++)
    device[i]->unacquire();
  if (secDrv)
    secDrv->unacquireDevices();
}
void HidJoystickClassDriver::destroy()
{
  destroyDevices();
  stg_joy.present = false;

  destroy_it(secDrv);
  delete this;
}

void HidJoystickClassDriver::updateDevices()
{
  if (!enabled)
    return;

  int dt = get_time_usec(prevUpdateRefTime) / 1000;
  prevUpdateRefTime = ::ref_time_ticks();

  if (!defJoy && enableAutoDef)
    for (int i = 0; i < device.size(); i++)
    {
      if (device[i]->updateState(dt, false) && !defJoy)
      {
        setDefaultJoystick(device[i]);

        if (defJoy->getClient())
          defJoy->getClient()->stateChanged(defJoy, i);
      }
    }
  else
    for (int i = 0; i < device.size(); i++)
      device[i]->updateState(dt, device[i] == defJoy);

  if (secDrv)
  {
    secDrv->updateDevices();
    if (!defJoy)
      setDefaultJoystick(secDrv->getDefaultJoystick());
  }
}

// generic joystick class driver interface
IGenJoystick *HidJoystickClassDriver::getDevice(int idx) const
{
  if (idx >= device.size() && secDrv)
    return secDrv->getDevice(idx - device.size());
  if (idx >= 0 && idx < device.size())
    return device[idx];
  return NULL;
}
void HidJoystickClassDriver::useDefClient(IGenJoystickClient *cli)
{
  defClient = cli;
  for (int i = 0; i < device.size(); i++)
    device[i]->setClient(defClient);
  if (secDrv)
    secDrv->useDefClient(cli);
}

HumanInput::IGenJoystick *HumanInput::HidJoystickClassDriver::getDeviceByUserId(unsigned short userId) const
{
  for (int i = 0; i < device.size(); i++)
    if (device[i]->getUserId() == userId)
      return device[i];
  return secDrv ? secDrv->getDeviceByUserId(userId) : NULL;
}

void HumanInput::HidJoystickClassDriver::setDefaultJoystick(IGenJoystick *ref)
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

static int cmp_dev_name(HumanInput::HidJoystickDevice *const *a, HumanInput::HidJoystickDevice *const *b)
{
  if (int d = strcmp(a[0]->getDeviceID(), b[0]->getDeviceID()))
    return d;
  return (a[0] > b[0]) ? 1 : ((a[0] < b[0]) ? -1 : 0);
}

void HidJoystickClassDriver::refreshDeviceList()
{
  deviceConfigChanged = false;
  HIDRebuildDevices();

  for (CFIndex devIndex = 0, devCount = CFArrayGetCount(gDeviceCFArrayRef); devIndex < devCount; devIndex++)
  {
    IOHIDDeviceRef dev = (IOHIDDeviceRef)CFArrayGetValueAtIndex(gDeviceCFArrayRef, devIndex);
    if (!dev)
      continue;
    if (!IOHIDDeviceConformsTo(dev, kHIDPage_GenericDesktop, kHIDUsage_GD_Joystick) &&
        !IOHIDDeviceConformsTo(dev, kHIDPage_GenericDesktop, kHIDUsage_GD_GamePad) &&
        !IOHIDDeviceConformsTo(dev, kHIDPage_GenericDesktop, kHIDUsage_GD_MultiAxisController))
      continue;

    int found = -1;
    for (int i = 0; i < device.size(); i++)
      if (device[i]->getDev() == dev)
      {
        device[i]->setConnected(true);
        found = i;
        break;
      }
    if (found < 0)
    {
      found = device.size();
      device.push_back(new HidJoystickDevice(dev));
    }
  }

  for (int i = device.size() - 1; i >= 0; i--)
  {
    int found = -1;
    for (CFIndex devIndex = 0, devCount = CFArrayGetCount(gDeviceCFArrayRef); devIndex < devCount; devIndex++)
      if (IOHIDDeviceRef dev = (IOHIDDeviceRef)CFArrayGetValueAtIndex(gDeviceCFArrayRef, devIndex))
        if (device[i]->getDev() == dev)
        {
          found = devIndex;
          break;
        }
    if (found < 0)
    {
      delete device[i];
      erase_items(device, i, 1);
    }
  }
  if (stg_joy.sortDevices)
    sort(device, &cmp_dev_name);
}

void HidJoystickClassDriver::deviceMatchingCallback(void *inContext, IOReturn, void *, IOHIDDeviceRef dev)
{
  if (IOHIDDeviceConformsTo(dev, kHIDPage_GenericDesktop, kHIDUsage_GD_Joystick) ||
      IOHIDDeviceConformsTo(dev, kHIDPage_GenericDesktop, kHIDUsage_GD_GamePad) ||
      IOHIDDeviceConformsTo(dev, kHIDPage_GenericDesktop, kHIDUsage_GD_MultiAxisController))
    reinterpret_cast<HidJoystickClassDriver *>(inContext)->deviceConfigChanged = true;
}

void HidJoystickClassDriver::deviceRemovalCallback(void *inContext, IOReturn, void *, IOHIDDeviceRef dev)
{
  HidJoystickClassDriver *drv = reinterpret_cast<HidJoystickClassDriver *>(inContext);
  for (int i = 0; i < drv->device.size(); i++)
    if (drv->device[i]->getDev() == dev)
    {
      drv->device[i]->setConnected(false);
      drv->deviceConfigChanged = true;
      return;
    }
}
