// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "joy_classdrv.h"
#include "joy_device.h"
#include "joy_device_xinput.h"
#include <generic/dag_sort.h>
#include <drv/hid/dag_hiGlobals.h>
#include <osApiWrappers/dag_progGlobals.h>
#include <debug/dag_hwExcept.h>
#include <debug/dag_except.h>
#include <debug/dag_debug.h>
#include "../hid_commonCode/dsPadIds.h"
#include <dirent.h>
#include <unistd.h>
#include <stdio.h>

#include <string.h>

using namespace HumanInput;

UDev HidJoystickClassDriver::udev(HidJoystickClassDriver::on_device_action, NULL);
Tab<HidJoystickClassDriver *> HidJoystickClassDriver::udev_users;


HidJoystickClassDriver::HidJoystickClassDriver(bool exclude_xinput, bool remap_360, bool exclude_sony_pads) :
  enabled(false), excludeXInput(exclude_xinput), remapAsX360(remap_360), excludeSonyPads(exclude_sony_pads)
{
  defJoy = NULL;
  enableAutoDef = true;
  defClient = NULL;
  deviceConfigChanged = false;
  prevUpdateRefTime = ::ref_time_ticks();
  secDrv = NULL;
  udev_users.push_back(this);
}

HidJoystickClassDriver::~HidJoystickClassDriver()
{
  erase_item_by_value(udev_users, this);
  destroyDevices();
}

bool HidJoystickClassDriver::init()
{
  if (!udev.init())
    return false;
  stg_joy.enabled = false;

  udev.enumerateDevices();
  for (int i = 0; i < device.size(); i++)
  {
    if (i > 0 && strcasestr(device[i]->getName(), "stick") != 0)
      eastl::swap(device[0], device[i]);
  }

  if (getDeviceCount() > 0)
    enable(true);
  return true;
}

void HidJoystickClassDriver::destroyDevices()
{
  setDefaultJoystick(NULL);
  unacquireDevices();
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
  if (secDrv)
    secDrv->acquireDevices();
}
void HidJoystickClassDriver::unacquireDevices()
{
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
  udev.poll();
  if (!enabled)
    return;

  int dt = get_time_usec(prevUpdateRefTime) / 1000;
  prevUpdateRefTime = ::ref_time_ticks();

  if (!defJoy && enableAutoDef)
    for (int i = 0; i < device.size(); i++)
    {
      if (device[i]->updateState(dt, false) && !defJoy)
      {
        setDefaultJoystick(device[i].get());

        if (defJoy->getClient())
          defJoy->getClient()->stateChanged(defJoy, i);
      }
    }
  else
    for (int i = 0; i < device.size(); i++)
      device[i]->updateState(dt, device[i].get() == defJoy);

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
    return device[idx].get();
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

IGenJoystick *HidJoystickClassDriver::getDeviceByUserId(unsigned short userId) const
{
  for (int i = 0; i < device.size(); i++)
    if (device[i]->getUserId() == userId)
      return device[i].get();
  return secDrv ? secDrv->getDeviceByUserId(userId) : NULL;
}

void HidJoystickClassDriver::setDefaultJoystick(IGenJoystick *ref)
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


static int cmp_dev_name(eastl::unique_ptr<HumanInput::UDevJoystick> const *a, eastl::unique_ptr<HumanInput::UDevJoystick> const *b)
{
  if (int d = strcmp((*a)->getDeviceID(), (*b)->getDeviceID()))
    return d;
  return (a->get() > b->get()) ? 1 : ((a->get() < b->get()) ? -1 : 0);
}

void HidJoystickClassDriver::refreshDeviceList()
{
  deviceConfigChanged = false;

  for (size_t i = 0; i < delayedActions.size(); ++i)
    onDeviceAction(delayedActions[i].dev, delayedActions[i].action);

  clear_and_shrink(delayedActions);
  if (stg_joy.sortDevices)
    sort(device, &cmp_dev_name);
}

// constraint: a system without the udev rules denies the hidraw access hid_dspad_pc needs, so the pad stays an evdev joystick
static bool hidraw_node_accessible(const char *evdev_node)
{
  const char *name = strrchr(evdev_node, '/');
  char path[320];
  snprintf(path, sizeof(path), "/sys/class/input/%s/device/device/hidraw", name ? name + 1 : evdev_node);
  DIR *dir = opendir(path);
  if (!dir)
    return false;
  bool ok = false;
  while (dirent *e = readdir(dir))
    if (strncmp(e->d_name, "hidraw", 6) == 0)
    {
      snprintf(path, sizeof(path), "/dev/%s", e->d_name);
      ok |= access(path, R_OK | W_OK) == 0;
    }
  closedir(dir);
  return ok;
}

void HidJoystickClassDriver::onDeviceAction(const UDev::Device &dev, UDev::Action action)
{
  if (dev.devClass != UDev::JOYSTICK)
    return;
  if (action == UDev::ADDED && excludeSonyPads && sonypad::is_sony_pad(dev.id.vendor, dev.id.product) &&
      hidraw_node_accessible(dev.devnode))
  {
    JOY_LOG("skip %04X:%04X '%s': hid_dspad_pc reads it over hidraw", dev.id.vendor, dev.id.product, dev.name);
    return;
  }

  JOY_LOG("DEVICE EVENT node: %s model: '%s' name: '%s' action: %s this: %p devices count, %lu, excludeXInput %d", dev.devnode,
    dev.model, dev.name, action == UDev::Action::ADDED ? "added" : "removed", this, device.size(), excludeXInput);
  if (action == UDev::ADDED)
  {
    int found = -1;
    for (int i = 0; i < device.size(); i++)
    {
      if (dev.devnode == device[i]->getNode() && dev.model == device[i]->getModel())
      {
        device[i]->updateDevice(dev);
        device[i]->setConnected(true);
        found = i;
        break;
      }
    }

    if (found < 0)
    {
      eastl::unique_ptr<HidJoystickDevice> hidDev(new HidJoystickDevice(dev));

      bool isXinputCompat = HidJoystickDeviceXInput::device_xinput_compatible(hidDev.get());
      eastl::unique_ptr<UDevJoystick> hidDevXinput;
      if (remapAsX360 && isXinputCompat)
        hidDevXinput.reset(new HidJoystickDeviceXInput(eastl::move(hidDev)));

      if (hidDevXinput)
        device.push_back(eastl::move(hidDevXinput));
      else if ((!isXinputCompat || !excludeXInput) && !remapAsX360)
        device.push_back(eastl::move(hidDev));
      else
        JOY_LOG("discard device '%s'. excludeXInput %d, xinput compatable %d", dev.name, (int)isXinputCompat, (int)excludeXInput);
    }
  }
  else
  {
    for (int i = 0; i < device.size(); i++)
    {
      if (dev.devnode == device[i]->getNode() && dev.model == device[i]->getModel())
      {
        JOY_LOG("device %s removed", device[i]->getName());
        device[i]->setConnected(false);
        erase_items(device, i, 1);
        break;
      }
    }
  }
}

void HidJoystickClassDriver::on_device_action(const UDev::Device &dev, UDev::Action action, void *user_data)
{
  G_UNREFERENCED(user_data);
  for (int userNo = 0; userNo < udev_users.size(); ++userNo)
  {
    HidJoystickClassDriver *pthis = udev_users[userNo];

    UDevAction act;
    act.dev = dev;
    act.action = action;
    pthis->delayedActions.push_back(act);
    pthis->deviceConfigChanged = true;
  }
}
