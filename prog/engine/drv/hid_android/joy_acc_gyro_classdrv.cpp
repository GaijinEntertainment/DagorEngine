// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "joy_android.h"
#include "and_sensor.h"
#include "joy_acc_gyro_classdrv.h"
#include "joy_acc_gyro_device.h"
#include <drv/hid/dag_hiGlobals.h>
#include <ioSys/dag_dataBlock.h>
#include <supp/_platform.h>
#include <debug/dag_debug.h>
#include "joy_helper_android.h"
#include <string.h>
#include <startup/dag_globalSettings.h>
#include <osApiWrappers/dag_miscApi.h>
#include <osApiWrappers/dag_atomic.h>

static bool gamepad_exist_in_block(const DataBlock *blk, int vid, int pid, const char *blockName)
{
  if (const DataBlock *layoutBlk = blk->getBlockByName(blockName))
  {
    for (int blockNo = 0; blockNo < layoutBlk->blockCount(); blockNo++)
    {
      const DataBlock *configBlk = layoutBlk->getBlock(blockNo);
      int v = configBlk->getInt("vendor", -1);
      int p = configBlk->getInt("product", -1);
      if (v == vid && (p == pid || p == -1))
        return true;
    }
  }
  return false;
}

static int get_gamepad_layout()
{
  int vendor = HumanInput::JoyDeviceHelper::getConnectedGamepadVendorId();
  if (vendor == HumanInput::GAMEPAD_VENDOR_NINTENDO || vendor == HumanInput::GAMEPAD_VENDOR_SONY ||
      vendor == HumanInput::GAMEPAD_VENDOR_MICROSOFT)
    return vendor;

  const DataBlock *modelsBlk = dgs_get_settings()->getBlockByNameEx("gamepad_layout");
  if (modelsBlk->isEmpty())
  {
    logdbg("gamepad vendors list are not found");
    return HumanInput::GAMEPAD_VENDOR_UNKNOWN;
  }
  int product = HumanInput::JoyDeviceHelper::getConnectedGamepadProductId();
  logdbg("gamepad: searching gamepad layouts for device vendor %d product %d", vendor, product);

  const int res = gamepad_exist_in_block(modelsBlk, vendor, product, "nintendo") ? HumanInput::GAMEPAD_VENDOR_NINTENDO
                  : gamepad_exist_in_block(modelsBlk, vendor, product, "sony")   ? HumanInput::GAMEPAD_VENDOR_SONY
                  : gamepad_exist_in_block(modelsBlk, vendor, product, "xbox")   ? HumanInput::GAMEPAD_VENDOR_MICROSOFT
                                                                                 : HumanInput::GAMEPAD_VENDOR_UNKNOWN;

  if (res != HumanInput::GAMEPAD_VENDOR_UNKNOWN)
  {
    logdbg("gamepad: found layout - %d", res);
  }
  return res;
}


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
    vendorId = get_gamepad_layout();
    defJoy = device = new (inimem) JoyAccelGyroInpDevice;
    ((JoyAccelGyroInpDevice *)device)->setVendorId(vendorId);

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
