// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <drv/hid/dag_hiJoystick.h>
#include <generic/dag_tab.h>
#include <osApiWrappers/dag_wndProcComponent.h>
#include <perfMon/dag_cpuFreq.h>
#include <EASTL/unique_ptr.h>
#include <dag/dag_vector.h>

#include "hid_udev.h"

namespace HumanInput
{
class UDevJoystick;

class HidJoystickClassDriver : public IGenJoystickClassDrv
{
public:
  HidJoystickClassDriver(bool exclude_xinput, bool remap_360);
  ~HidJoystickClassDriver();

  bool init();
  void destroyDevices();

  // generic hid class driver interface
  virtual void enable(bool en);
  virtual void acquireDevices();
  virtual void unacquireDevices();
  virtual void destroy();

  virtual void refreshDeviceList();

  virtual int getDeviceCount() const { return device.size() + (secDrv ? secDrv->getDeviceCount() : 0); }
  virtual void updateDevices();

  // generic joystick class driver interface
  virtual IGenJoystick *getDevice(int idx) const;
  virtual IGenJoystick *getDeviceByUserId(unsigned short userId) const;
  virtual bool isDeviceConfigChanged() const
  {
    if (deviceConfigChanged)
      return deviceConfigChanged;
    return secDrv ? secDrv->isDeviceConfigChanged() : false;
  }
  virtual void useDefClient(IGenJoystickClient *cli);

  virtual IGenJoystick *getDefaultJoystick() { return defJoy; }
  virtual void setDefaultJoystick(IGenJoystick *ref);
  virtual void enableAutoDefaultJoystick(bool enable)
  {
    enableAutoDef = enable;
    if (secDrv)
      secDrv->enableAutoDefaultJoystick(enable);
  }

  virtual IGenJoystickClassDrv *setSecondaryClassDrv(IGenJoystickClassDrv *drv)
  {
    IGenJoystickClassDrv *prev = secDrv;
    secDrv = drv;
    if (secDrv)
      secDrv->setDefaultJoystick(defJoy);
    return prev;
  }

protected:
  dag::Vector<eastl::unique_ptr<UDevJoystick>> device;
  IGenJoystickClient *defClient;
  IGenJoystickClassDrv *secDrv;
  IGenJoystick *defJoy;
  int64_t prevUpdateRefTime;
  bool enabled;
  bool deviceConfigChanged;
  bool enableAutoDef;
  bool remapAsX360;
  bool excludeXInput;

private:
  static UDev udev;
  static Tab<HidJoystickClassDriver *> udev_users;

  void onDeviceAction(const UDev::Device &dev, UDev::Action action);
  static void on_device_action(const UDev::Device &dev, UDev::Action action, void *user_data);

  struct UDevAction
  {
    UDev::Device dev;
    UDev::Action action;
  };

  Tab<UDevAction> delayedActions;
};
} // namespace HumanInput
