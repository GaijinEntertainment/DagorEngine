#pragma once

#include <humanInput/dag_hiJoystick.h>
#include <generic/dag_tab.h>
#include <osApiWrappers/dag_wndProcComponent.h>
#include <perfMon/dag_cpuFreq.h>
#include "osx_hid_util.h"

namespace HumanInput
{
class HidJoystickDevice;

class HidJoystickClassDriver : public IGenJoystickClassDrv
{
public:
  HidJoystickClassDriver();
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
  Tab<HidJoystickDevice *> device;
  IGenJoystickClient *defClient;
  IGenJoystickClassDrv *secDrv;
  IGenJoystick *defJoy;
  int64_t prevUpdateRefTime;
  bool enabled;
  bool deviceConfigChanged;
  bool enableAutoDef;

  static void deviceMatchingCallback(void *inContext, IOReturn inResult, void *inSender, IOHIDDeviceRef inIOHIDDeviceRef);
  static void deviceRemovalCallback(void *inContext, IOReturn inResult, void *inSender, IOHIDDeviceRef inIOHIDDeviceRef);
};
} // namespace HumanInput
