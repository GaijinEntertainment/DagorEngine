// Copyright 2023 by Gaijin Games KFT, All rights reserved.
#pragma once

#include <humanInput/dag_hiJoystick.h>
#include <perfMon/dag_cpuFreq.h>

namespace HumanInput
{
class AccelGyroInpDevice;


class AccelGyroInpClassDriver : public IGenJoystickClassDrv
{
public:
  AccelGyroInpClassDriver();
  ~AccelGyroInpClassDriver();

  bool init();
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
  virtual bool isDeviceConfigChanged() const { return false; }
  virtual void useDefClient(IGenJoystickClient *cli);

  virtual IGenJoystick *getDefaultJoystick() { return defJoy; }
  virtual void setDefaultJoystick(IGenJoystick *ref);
  virtual void enableAutoDefaultJoystick(bool enable)
  {
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
  static constexpr int INPDEV_MAX = 1;

  AccelGyroInpDevice *device[INPDEV_MAX];
  int deviceNum;
  IGenJoystickClient *defClient;
  IGenJoystickClassDrv *secDrv;
  IGenJoystick *defJoy;
  bool enabled;
};
} // namespace HumanInput
