// Copyright 2023 by Gaijin Games KFT, All rights reserved.
#ifndef _GAIJIN_DRV_HID_JOYSTICK_JOY_CLASSDRV_H
#define _GAIJIN_DRV_HID_JOYSTICK_JOY_CLASSDRV_H
#pragma once

#include <humanInput/dag_hiJoystick.h>
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

  bool tryRefreshDeviceList();
  void refreshDeviceList() override { tryRefreshDeviceList(); }

  virtual int getDeviceCount() const { return device.size() + (secDrv ? secDrv->getDeviceCount() : 0); }
  virtual void updateDevices();
  bool deviceCheckerListRunning() const;

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
  Tab<Di8JoystickDevice *> device;
  IGenJoystickClient *defClient;
  IGenJoystickClassDrv *secDrv;
  IGenJoystick *defJoy;
  int64_t prevUpdateRefTime;
  bool enabled;
  bool maybeDeviceConfigChanged;
  bool deviceConfigChanged;
  bool enableAutoDef;
  bool excludeXinputDev, remapAsX360;

  void checkDeviceList();
  volatile int deviceListCheckerIsRunning;
  eastl::unique_ptr<AsyncDeviceListChecker> deviceListChecker;
};
} // namespace HumanInput

#endif
