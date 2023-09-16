// Copyright 2023 by Gaijin Games KFT, All rights reserved.
#ifndef _GAIJIN_DRV_HID_GAMEPAD_XBOX_GAMEPAD_CLASSDRV_H
#define _GAIJIN_DRV_HID_GAMEPAD_XBOX_GAMEPAD_CLASSDRV_H
#pragma once

#include <humanInput/dag_hiJoystick.h>
#include <perfMon/dag_cpuFreq.h>
#include <osApiWrappers/dag_wndProcComponent.h>
#include <EASTL/unique_ptr.h>

namespace HumanInput
{
class Xbox360GamepadDevice;
class XInputUpdater;


class Xbox360GamepadClassDriver : public IGenJoystickClassDrv, public IWndProcComponent
{
  friend XInputUpdater;

public:
  Xbox360GamepadClassDriver(bool emulate_single_gamepad = false);
  ~Xbox360GamepadClassDriver();

  bool init();
  void destroyDevices();

  // generic hid class driver interface
  virtual void enable(bool en);
  virtual void acquireDevices() { secDrv ? secDrv->acquireDevices() : (void)0; }
  virtual void unacquireDevices() { secDrv ? secDrv->unacquireDevices() : (void)0; }
  virtual void destroy();

  virtual void refreshDeviceList();

  virtual int getDeviceCount() const;
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

  virtual bool isStickDeadZoneSetupSupported() const { return true; }
  virtual float getStickDeadZoneScale(int stick_idx, bool) const override
  {
    G_ASSERT_RETURN(stick_idx >= 0 && stick_idx < 2, 0);
    return stickDeadZoneScale[stick_idx];
  }
  virtual void setStickDeadZoneScale(int stick_idx, bool, float scale) override;
  virtual float getStickDeadZoneAbs(int stick_idx, bool, IGenJoystick *for_joy) const override;

#if _TARGET_PC
  static constexpr int GAMEPAD_MAX = 4;
#elif _TARGET_XBOX
  static constexpr int GAMEPAD_MAX = 8;
#endif

  virtual RetCode process(void *hwnd, unsigned msg, uintptr_t wParam, intptr_t lParam, intptr_t &result);

  void setDeviceMask(unsigned int new_mask);

  void updateVirtualDevice();

protected:
  Xbox360GamepadDevice *virtualDevice;
  Xbox360GamepadDevice *device[GAMEPAD_MAX];
  unsigned int deviceStateMask;
  int deviceNum;
  IGenJoystickClient *defClient;
  IGenJoystickClassDrv *secDrv;
  IGenJoystick *defJoy;
  int prevUpdateRefTime;
  int nextUpdatePresenseTime;
  bool enabled;
  bool deviceConfigChanged;
  bool enableAutoDef;
  bool emulateSingleDevice;
  int rescanCount;
  volatile int updaterIsRunning;
  eastl::unique_ptr<XInputUpdater> inputUpdater;
  float stickDeadZoneScale[2] = {1, 1};

  static const char *gamepadName[GAMEPAD_MAX];

  void initXbox();
  void updateXboxGamepads();
};
} // namespace HumanInput

#endif
