// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <debug/dag_debug.h>
#include "hid_udev.h"
#include <math/dag_mathUtils.h>
#include <util/dag_finally.h>

#include <string.h>
#include <sys/select.h>
#include <libudev.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <dlfcn.h>
#include <errno.h>

#include "evdev_helpers.h"

namespace HumanInput
{

static const char *UDEV_LIBS[] = {"libudev.so.1", "libudev.so.0"};

const char *(*udev_device_get_action)(struct udev_device *);
const char *(*udev_device_get_devnode)(struct udev_device *);
const char *(*udev_device_get_property_value)(struct udev_device *, const char *);
struct udev_device *(*udev_device_new_from_syspath)(struct udev *, const char *);
void (*udev_device_unref)(struct udev_device *);
int (*udev_enumerate_add_match_subsystem)(struct udev_enumerate *, const char *);
struct udev_list_entry *(*udev_enumerate_get_list_entry)(struct udev_enumerate *);
struct udev_enumerate *(*udev_enumerate_new)(struct udev *);
int (*udev_enumerate_scan_devices)(struct udev_enumerate *);
void (*udev_enumerate_unref)(struct udev_enumerate *);
const char *(*udev_list_entry_get_name)(struct udev_list_entry *);
struct udev_list_entry *(*udev_list_entry_get_next)(struct udev_list_entry *);
int (*udev_monitor_enable_receiving)(struct udev_monitor *);
int (*udev_monitor_filter_add_match_subsystem_devtype)(struct udev_monitor *, const char *, const char *);
int (*udev_monitor_get_fd)(struct udev_monitor *);
struct udev_monitor *(*udev_monitor_new_from_netlink)(struct udev *, const char *);
struct udev_device *(*udev_monitor_receive_device)(struct udev_monitor *);
void (*udev_monitor_unref)(struct udev_monitor *);
struct udev *(*udev_new)(void);
void (*udev_unref)(struct udev *);


#define LOAD_UDEV_FUN(f)                              \
  if ((*((void **)&f) = dlsym(udev_lib, #f)) == NULL) \
  return false
bool load_udev_api(void *udev_lib)
{
  LOAD_UDEV_FUN(udev_new);
  LOAD_UDEV_FUN(udev_unref);

  LOAD_UDEV_FUN(udev_device_new_from_syspath);
  LOAD_UDEV_FUN(udev_device_get_action);
  LOAD_UDEV_FUN(udev_device_get_devnode);
  LOAD_UDEV_FUN(udev_device_get_property_value);
  LOAD_UDEV_FUN(udev_device_unref);

  LOAD_UDEV_FUN(udev_monitor_new_from_netlink);
  LOAD_UDEV_FUN(udev_monitor_unref);
  LOAD_UDEV_FUN(udev_monitor_filter_add_match_subsystem_devtype);
  LOAD_UDEV_FUN(udev_monitor_enable_receiving);
  LOAD_UDEV_FUN(udev_monitor_get_fd);
  LOAD_UDEV_FUN(udev_monitor_receive_device);

  LOAD_UDEV_FUN(udev_enumerate_new);
  LOAD_UDEV_FUN(udev_enumerate_unref);
  LOAD_UDEV_FUN(udev_enumerate_add_match_subsystem);
  LOAD_UDEV_FUN(udev_enumerate_scan_devices);
  LOAD_UDEV_FUN(udev_enumerate_get_list_entry);
  LOAD_UDEV_FUN(udev_list_entry_get_name);
  LOAD_UDEV_FUN(udev_list_entry_get_next);
  return true;
}


// if device has some axes assume that it joystick
bool is_device_joystick(const char *path)
{
  int fd = open(path, O_RDONLY, 0);
  if (fd < 0)
    return false;
  FINALLY([fd] { ::close(fd); });

  InputBitArray<EV_CNT> evbits;
  if (!evbits.evdevRead(fd, 0))
    return false;

  // joystick must have atleast one axis
  if (!evbits.test(EV_ABS))
    return false;
  InputBitArray<ABS_CNT> absbits;
  if (!absbits.evdevRead(fd, EV_ABS))
    return false;

  // ABS_VOLUME usually used by usb-speakers
  // ABS_MISC may present in gamepad but as addition to common axes
  for (int axis = 0; axis < ABS_CNT; ++axis)
  {
    if (axis == ABS_MISC || axis == ABS_VOLUME)
      continue;
    if (absbits.test(axis))
      return true;
  }
  return false;
}

UDev::UDev(UDev::DeviceCb cb, void *user_data) : callback(cb), userData(user_data)
{
  udevCtx = NULL;
  udevMon = NULL;
  udevLibHandle = NULL;
}

UDev::~UDev()
{
  if (udevMon)
    udev_monitor_unref(udevMon);
  if (udevCtx)
    udev_unref(udevCtx);

  if (udevLibHandle)
    dlclose(udevLibHandle);
}

bool UDev::loadLibrary()
{
  if (udevLibHandle)
    return false;

  for (int i = 0; i < 2; ++i)
  {
    udevLibHandle = dlopen(UDEV_LIBS[i], RTLD_LAZY);
    if (udevLibHandle)
    {
      if (load_udev_api(udevLibHandle))
        return true;
      dlclose(udevLibHandle);
      udevLibHandle = NULL;
    }
  }

  debug("UDEV library not-found in system. joystick disabled");
  return false;
}

bool UDev::init()
{
  if (udevCtx)
    return true;

  if (!loadLibrary())
    return false;

  udevCtx = udev_new();
  if (!udevCtx)
    return false;
  udevMon = udev_monitor_new_from_netlink(udevCtx, "udev");
  if (!udevMon)
    return false;

  if (udev_monitor_filter_add_match_subsystem_devtype(udevMon, "input", NULL) < 0)
    return false;
  if (udev_monitor_enable_receiving(udevMon) < 0)
    return false;

  return true;
}

static bool is_dev_property_set(udev_device *dev, const char *propname)
{
  const char *val = udev_device_get_property_value(dev, propname);
  return val != NULL && *val == '1';
}

static const char *devclass_name(UDev::DeviceClass dclass)
{
  switch (dclass)
  {
    case UDev::KEYBOARD: return "Keyboard";
    case UDev::MOUSE: return "Mouse";
    case UDev::JOYSTICK: return "Joystick";
    default: return "Unknown";
  }
}

void UDev::onDeviceAction(udev_device *dev, Action action)
{
  const char *path = udev_device_get_devnode(dev);
  if (!path)
    return;

  DeviceClass dclass = UNKNOWN;

  if (is_dev_property_set(dev, "ID_INPUT_JOYSTICK"))
  {
    dclass = JOYSTICK;
  }
  else if (is_dev_property_set(dev, "ID_INPUT_MOUSE") || is_dev_property_set(dev, "ID_INPUT_TOUCHPAD"))
  {
    dclass = MOUSE;
  }
  else if (is_dev_property_set(dev, "ID_INPUT_KEYBOARD"))
  {
    dclass = KEYBOARD;
  }
  else
  {
    if (is_device_joystick(path))
      dclass = JOYSTICK;
    else
      return;
  }

  Device device;
  device.guid = 0;
  device.devnode = path;
  device.devClass = dclass;
  device.model = udev_device_get_property_value(dev, "ID_MODEL");

  if (action == ADDED)
  {
    char namebuf[256] = "Unknown";
    struct stat sb;
    if (stat(path, &sb) == -1)
      return;

    int fd = open(path, O_RDONLY, 0);
    if (fd < 0)
    {
      char error_buf[128];
      debug("failed to open device node '%s': %s", path, strerror_r(errno, error_buf, 128));
      return;
    }
    FINALLY([fd] { ::close(fd); });

    if (ioctl(fd, EVIOCGID, &device.id) < 0)
    {
      // device is not evdev interface
      return;
    }

    if (ioctl(fd, EVIOCGNAME(sizeof(namebuf)), namebuf) < 0)
    {
      const char *modelEnc = udev_device_get_property_value(dev, "ID_MODEL_ENC");
      if (modelEnc)
        device.name = modelEnc;
      else
      {
        debug("[UDEV] failed to get device name %s", device.model);
        return;
      }
    }
    else
    {
      device.name = namebuf;
    }
  }

  if (action == ADDED)
    debug("[UDEV] device added %s (%s) as %s", device.name, path, devclass_name(device.devClass));
  else
    debug("[UDEV] device removed %s (%s)", device.name, path);

  callback(device, action, userData);
}

void UDev::enumerateDevices()
{
  udev_enumerate *enumerate = NULL;
  enumerate = udev_enumerate_new(udevCtx);
  udev_enumerate_add_match_subsystem(enumerate, "input");
  udev_enumerate_scan_devices(enumerate);
  udev_list_entry *devs = udev_enumerate_get_list_entry(enumerate);

  for (udev_list_entry *item = devs; item; item = udev_list_entry_get_next(item))
  {
    const char *path = udev_list_entry_get_name(item);
    udev_device *dev = udev_device_new_from_syspath(udevCtx, path);
    onDeviceAction(dev, ADDED);
    udev_device_unref(dev);
  }

  udev_enumerate_unref(enumerate);
}

void UDev::poll()
{
  udev_device *dev;

  const int fd = udev_monitor_get_fd(udevMon);
  fd_set fds;
  struct timeval tv;
  FD_ZERO(&fds);
  FD_SET(fd, &fds);
  tv.tv_sec = 0;
  tv.tv_usec = 0;

  while (select(fd + 1, &fds, NULL, NULL, &tv) > 0)
  {
    if (!FD_ISSET(fd, &fds))
      break;
    udev_device *dev = udev_monitor_receive_device(udevMon);
    if (dev)
    {
      const char *action = udev_device_get_action(dev);
      if (action)
      {
        if (!strcmp(action, "add"))
          onDeviceAction(dev, ADDED);
        else if (!strcmp(action, "remove"))
          onDeviceAction(dev, REMOVED);
      }
      udev_device_unref(dev);
    }
  }
}

} // namespace HumanInput
