// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "joy_enumdev.h"

#include <drv/hid/dag_hiJoystick.h>
#include <generic/dag_tab.h>
#include <osApiWrappers/dag_wndProcComponent.h>
#include <perfMon/dag_cpuFreq.h>
#include <EASTL/unique_ptr.h>

namespace HumanInput
{
class Di8JoystickDevice;

class Di8JoystickClassDriver final : public IGenJoystickClassDrv, public IWndProcComponent
{
  class AsyncDeviceListChecker;
  friend AsyncDeviceListChecker;

public:
  using Di8DeviceList = Tab<Di8JoystickDevice *>;

  Di8JoystickClassDriver(bool exclude_xinput, bool remap_360);
  ~Di8JoystickClassDriver();

  bool init();
  void destroyDevices();
  IWndProcComponent::RetCode process(void *hwnd, unsigned msg, uintptr_t wParam, intptr_t lParam, intptr_t &result);

  // generic hid class driver interface
  virtual void enable(bool en);
  virtual void acquireDevices();
  virtual void unacquireDevices();
  virtual void destroy();

  void refreshDeviceList() override;

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
  Di8DeviceList device;
  IGenJoystickClient *defClient;
  IGenJoystickClassDrv *secDrv;
  IGenJoystick *defJoy;
  int64_t prevUpdateRefTime;
  bool enabled;
  bool maybeDeviceConfigChanged;
  bool deviceConfigChanged;
  bool enableAutoDef;
  bool excludeXinputDev, remapAsX360;

  DeviceEnumerator deviceEnumerator;
};
} // namespace HumanInput
