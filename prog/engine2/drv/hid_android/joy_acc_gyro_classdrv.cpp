#include "joy_android.h"
#include "and_sensor.h"
#include "joy_acc_gyro_classdrv.h"
#include "joy_acc_gyro_device.h"
#include <humanInput/dag_hiGlobals.h>
#include <supp/_platform.h>
#include <debug/dag_debug.h>
#include "joy_helper_android.h"
#include <string.h>
#include <osApiWrappers/dag_miscApi.h>
#include <osApiWrappers/dag_atomic.h>


HumanInput::JoyAccelGyroInpClassDriver::JoyAccelGyroInpClassDriver()
{
  enabled = false;
  defClient = nullptr;
  device = nullptr;
  defJoy = nullptr;
  vendorId = GAMEPAD_VENDOR_UNKNOWN;
  pendingAndroidCleanupOp = 0;
  processedAndroidCleanupOp = 0;
}

HumanInput::JoyAccelGyroInpClassDriver::~JoyAccelGyroInpClassDriver() { destroyDevices(); }

bool HumanInput::JoyAccelGyroInpClassDriver::init()
{
  stg_joy.enabled = false;
  refreshDeviceList();
  if (getDeviceCount() > 0)
    enable(true);
  return true;
}

void HumanInput::JoyAccelGyroInpClassDriver::destroyDevices()
{
  setDefaultJoystick(NULL);
  del_it(device);
}

void HumanInput::JoyAccelGyroInpClassDriver::refreshDeviceList()
{
  destroyDevices();
  enable(false);
  vendorId = GAMEPAD_VENDOR_UNKNOWN;

  if (JoyDeviceHelper::isGamepadConnected())
  {
    vendorId = JoyDeviceHelper::getConnectedGamepadVendorId();
    defJoy = device = new (inimem) JoyAccelGyroInpDevice;

    interlocked_increment(pendingAndroidCleanupOp);
  }
  else
  {
    vendorId = GAMEPAD_VENDOR_UNKNOWN;
    defJoy = device = new (inimem) AndroidSensor;
  }

  stg_joy.present = true;
  enable(true);
  useDefClient(defClient);
}

// generic hid class driver interface
void HumanInput::JoyAccelGyroInpClassDriver::enable(bool en)
{
  enabled = en;
  stg_joy.enabled = en;
}
void HumanInput::JoyAccelGyroInpClassDriver::destroy()
{
  destroyDevices();
  stg_joy.present = false;

  JoyDeviceHelper::detach();

  delete this;
}

void HumanInput::JoyAccelGyroInpClassDriver::updateDevices()
{
  if (!is_main_thread())
  {
    const int pendingOp = interlocked_relaxed_load(pendingAndroidCleanupOp);
    if (DAGOR_UNLIKELY(pendingOp > processedAndroidCleanupOp))
    {
      debug("android: pending sensor cleanup [pending op:%d processed op:%d]", pendingOp, processedAndroidCleanupOp);
      AndroidSensor::cleanupAndroidResources();
      processedAndroidCleanupOp = pendingOp;
    }
  }

  if (device && device->isConnected())
    device->updateState();
}

// generic joystick class driver interface
HumanInput::IGenJoystick *HumanInput::JoyAccelGyroInpClassDriver::getDevice(int idx) const { return idx == 0 ? device : nullptr; }

void HumanInput::JoyAccelGyroInpClassDriver::useDefClient(HumanInput::IGenJoystickClient *cli)
{
  defClient = cli;
  if (device)
    device->setClient(defClient);
}

HumanInput::IGenJoystick *HumanInput::JoyAccelGyroInpClassDriver::getDeviceByUserId(unsigned short userId) const
{
  return device->getUserId() == userId ? device : nullptr;
}

bool HumanInput::JoyAccelGyroInpClassDriver::isDeviceConfigChanged() const
{
  static int nextTimeToCheckChanges = 0;
  if (DAGOR_UNLIKELY(get_time_msec() >= nextTimeToCheckChanges))
  {
    nextTimeToCheckChanges = get_time_msec() + 1000;
    return JoyDeviceHelper::acquireDeviceChanges();
  }

  return false;
}

void HumanInput::JoyAccelGyroInpClassDriver::setDefaultJoystick(IGenJoystick *ref)
{
  if (ref != defJoy)
  {
    if (ref)
      raw_state_joy = ref->getRawState();
    else
      memset(&raw_state_joy, 0, sizeof(raw_state_joy));
  }

  defJoy = ref;
}

void HumanInput::JoyAccelGyroInpClassDriver::enableGyroscope(bool enable, bool)
{
  if (device)
    device->enableGyroscope(enable);
}

int HumanInput::JoyAccelGyroInpClassDriver::getVendorId() const { return vendorId; }
