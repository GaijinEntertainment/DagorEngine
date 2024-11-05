// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <util/dag_simpleString.h>
#include <linux/input.h>

struct udev;
struct udev_monitor;
struct udev_device;

namespace HumanInput
{

class UDev
{
public:
  enum DeviceClass
  {
    KEYBOARD,
    MOUSE,
    JOYSTICK,
    UNKNOWN
  };

  enum Action
  {
    ADDED,
    REMOVED
  };

  struct Device
  {
    union
    {
      input_id id;
      uint64_t guid;
    };

    SimpleString name;
    SimpleString devnode;
    SimpleString model;

    DeviceClass devClass;
  };

  typedef void (*DeviceCb)(const Device &device, Action action, void *user_data);

  UDev(DeviceCb cb, void *user_data);
  ~UDev();

  bool init();

  void enumerateDevices();
  void poll();

private:
  void onDeviceAction(udev_device *dev, Action act);
  bool loadLibrary();

private:
  udev *udevCtx;
  udev_monitor *udevMon;

  void *udevLibHandle;

  DeviceCb callback;
  void *userData;
};
} // namespace HumanInput
