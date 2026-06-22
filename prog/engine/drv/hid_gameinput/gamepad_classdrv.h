// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <drv/hid/dag_hiJoystick.h>
#include <perfMon/dag_cpuFreq.h>

namespace HumanInput
{
class GameInputGamepadDevice;


class GameInputGamepadClassDriver final : public IGenJoystickClassDrv
{
public:
  GameInputGamepadClassDriver(bool emulate_single_gamepad = false);
  ~GameInputGamepadClassDriver();

  bool init();
  void destroyDevices();

  // generic hid class driver interface
  void enable(bool en) override;
  void acquireDevices() override { secDrv ? secDrv->acquireDevices() : (void)0; }
  void unacquireDevices() override { secDrv ? secDrv->unacquireDevices() : (void)0; }
  void destroy() override;

  void refreshDeviceList() override;

  int getDeviceCount() const override;
  void updateDevices() override;

  // generic joystick class driver interface
  IGenJoystick *getDevice(int idx) const override;
  IGenJoystick *getDeviceByUserId(unsigned short userId) const override;
  bool isDeviceConfigChanged() const override
  {
    if (deviceConfigChanged)
      return deviceConfigChanged;
    return secDrv ? secDrv->isDeviceConfigChanged() : false;
  }
  void useDefClient(IGenJoystickClient *cli) override;

  IGenJoystick *getDefaultJoystick() override { return defJoy; }
  void setDefaultJoystick(IGenJoystick *ref) override;
  void enableAutoDefaultJoystick(bool enable) override
  {
    enableAutoDef = enable;
    if (secDrv)
      secDrv->enableAutoDefaultJoystick(enable);
  }

  IGenJoystickClassDrv *setSecondaryClassDrv(IGenJoystickClassDrv *drv) override
  {
    IGenJoystickClassDrv *prev = secDrv;
    secDrv = drv;
    if (secDrv)
      secDrv->setDefaultJoystick(defJoy);
    return prev;
  }

  bool isStickDeadZoneSetupSupported() const override { return true; }
  float getStickDeadZoneScale(int stick_idx, bool) const override
  {
    G_ASSERT_RETURN(stick_idx >= 0 && stick_idx < 2, 0);
    return stickDeadZoneScale[stick_idx];
  }
  void setStickDeadZoneScale(int stick_idx, bool, float scale) override;
  float getStickDeadZoneAbs(int stick_idx, bool, IGenJoystick *for_joy) const override;

  static constexpr int GAMEPAD_MAX = 8;

  void setDeviceMask(unsigned int new_mask);

  void updateVirtualDevice();

protected:
  GameInputGamepadDevice *virtualDevice = nullptr;
  GameInputGamepadDevice *device[GAMEPAD_MAX] = {};
  unsigned int deviceStateMask = 0;
  int deviceNum = 0;
  IGenJoystickClient *defClient = nullptr;
  IGenJoystickClassDrv *secDrv = nullptr;
  IGenJoystick *defJoy = nullptr;
  int prevUpdateRefTime = 0;
  int nextUpdatePresenseTime = 0;
  bool enabled = false;
  bool deviceConfigChanged = true;
  bool enableAutoDef = false;
  bool emulateSingleDevice = false;
  float stickDeadZoneScale[2] = {1, 1};

  static const char *gamepadName[GAMEPAD_MAX];
};
} // namespace HumanInput
