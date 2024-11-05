// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <drv/hid/dag_hiJoystick.h>
#include <perfMon/dag_cpuFreq.h>

namespace HumanInput
{
class IAndroidJoystick;


class JoyAccelGyroInpClassDriver : public IGenJoystickClassDrv
{
public:
  JoyAccelGyroInpClassDriver();
  ~JoyAccelGyroInpClassDriver();

  bool init();
  void destroyDevices();

  // generic hid class driver interface
  virtual void enable(bool en);
  virtual void acquireDevices() {}
  virtual void unacquireDevices() {}
  virtual void destroy();

  virtual void refreshDeviceList();

  virtual int getDeviceCount() const { return device != nullptr ? 1 : 0; }
  virtual void updateDevices();

  // generic joystick class driver interface
  virtual IGenJoystick *getDevice(int idx) const;
  virtual IGenJoystick *getDeviceByUserId(unsigned short userId) const;
  virtual bool isDeviceConfigChanged() const;
  virtual void useDefClient(IGenJoystickClient *cli);

  virtual IGenJoystick *getDefaultJoystick() { return defJoy; }
  virtual void setDefaultJoystick(IGenJoystick *ref);
  virtual void enableAutoDefaultJoystick(bool enable) {}

  virtual bool isStickDeadZoneSetupSupported() const { return true; }
  virtual float getStickDeadZoneScale(int stick_idx, bool) const override
  {
    G_ASSERT_RETURN(stick_idx >= 0 && stick_idx < 2, 0);
    return defJoy ? defJoy->getStickDeadZoneScale(stick_idx) : 0.f;
  }
  virtual void setStickDeadZoneScale(int stick_idx, bool main_dev, float scale) override
  {
    if (!main_dev)
      return;
    G_ASSERT_RETURN(stick_idx >= 0 && stick_idx < 2, );
    if (defJoy)
      defJoy->setStickDeadZoneScale(stick_idx, scale);
  }
  virtual float getStickDeadZoneAbs(int stick_idx, bool main_dev, IGenJoystick *for_joy) const override
  {
    if (!main_dev)
      return 0.f;
    G_ASSERT_RETURN(stick_idx >= 0 && stick_idx < 2, 0);
    return defJoy ? defJoy->getStickDeadZoneAbs(stick_idx) : 0.f;
  }

  virtual IGenJoystickClassDrv *setSecondaryClassDrv(IGenJoystickClassDrv *drv) { return nullptr; }

  virtual void enableGyroscope(bool enable, bool main_dev) override;

  virtual int getVendorId() const;

protected:
  int vendorId;
  IAndroidJoystick *device;
  IGenJoystickClient *defClient;
  IGenJoystick *defJoy;
  bool enabled;

  int pendingAndroidCleanupOp;
  int processedAndroidCleanupOp;
};
} // namespace HumanInput
