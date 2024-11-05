// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <drv/hid/dag_hiJoystick.h>
#include <perfMon/dag_cpuFreq.h>

namespace HumanInput
{
class SteamGamepadDevice;

class SteamGamepadClassDriver : public IGenJoystickClassDrv
{
public:
  SteamGamepadClassDriver();
  ~SteamGamepadClassDriver();

  bool init(const char *absolute_path_to_controller_config);
  void destroyDevices();

  // generic hid class driver interface
  virtual void enable(bool en);
  virtual void acquireDevices() { secDrv ? secDrv->acquireDevices() : (void)0; }
  virtual void unacquireDevices() { secDrv ? secDrv->unacquireDevices() : (void)0; }
  virtual void destroy();

  virtual void refreshDeviceList();

  virtual int getDeviceCount() const { return deviceNum + (secDrv ? secDrv->getDeviceCount() : 0); }
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
  static constexpr int GAMEPAD_MAX = 4;

  SteamGamepadDevice *device[GAMEPAD_MAX];
  unsigned int deviceStateMask;
  int deviceNum;
  IGenJoystickClient *defClient;
  IGenJoystickClassDrv *secDrv;
  IGenJoystick *defJoy;
  int64_t prevUpdateRefTime;
  bool enabled;
  bool deviceConfigChanged;
  bool enableAutoDef;

  static const char *gamepadName[GAMEPAD_MAX];
};
} // namespace HumanInput
