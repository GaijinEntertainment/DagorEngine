#include "acc_gyro_classdrv.h"
#include "acc_gyro_device.h"
#include <humanInput/dag_hiGlobals.h>
#include <supp/_platform.h>
#include <debug/dag_debug.h>
#include <string.h>

extern void hid_iOS_init_motion();
extern void hid_iOS_term_motion();

HumanInput::AccelGyroInpClassDriver::AccelGyroInpClassDriver()
{
  enabled = false;
  defClient = NULL;
  memset(&device, 0, sizeof(device));
  deviceNum = 0;
  defJoy = NULL;
  secDrv = NULL;
  hid_iOS_init_motion();
}

HumanInput::AccelGyroInpClassDriver::~AccelGyroInpClassDriver()
{
  hid_iOS_term_motion();
  destroyDevices();
}

bool HumanInput::AccelGyroInpClassDriver::init()
{
  stg_joy.enabled = false;

  refreshDeviceList();
  if (getDeviceCount() > 0)
    enable(true);
  return true;
}

void HumanInput::AccelGyroInpClassDriver::destroyDevices()
{
  setDefaultJoystick(NULL);
  for (int i = 0; i < deviceNum; i++)
    delete device[i];
  deviceNum = 0;
  memset(device, 0, sizeof(device));
}

void HumanInput::AccelGyroInpClassDriver::refreshDeviceList()
{
  destroyDevices();
  enable(false);

  defJoy = device[deviceNum] = new (inimem) AccelGyroInpDevice;
  deviceNum++;

  if (secDrv)
  {
    secDrv->refreshDeviceList();
    secDrv->setDefaultJoystick(defJoy);
  }

  // update global settings
  if (getDeviceCount() > 0)
  {
    stg_joy.present = true;
    enable(true);
  }
  else
  {
    stg_joy.present = false;
    stg_joy.enabled = false;
  }

  useDefClient(defClient);
  for (int i = 0; i < deviceNum; i++)
    device[i]->updateState(defJoy == device[i]);
}

// generic hid class driver interface
void HumanInput::AccelGyroInpClassDriver::enable(bool en)
{
  enabled = en;
  stg_joy.enabled = en;
  if (secDrv)
    secDrv->enable(en);
}
void HumanInput::AccelGyroInpClassDriver::destroy()
{
  destroyDevices();
  stg_joy.present = false;

  destroy_it(secDrv);
  delete this;
}

void HumanInput::AccelGyroInpClassDriver::updateDevices()
{
  for (int i = 0; i < deviceNum; i++)
    device[i]->updateState(defJoy == device[i]);

  if (secDrv)
  {
    secDrv->updateDevices();
    if (!defJoy)
      setDefaultJoystick(secDrv->getDefaultJoystick());
  }
}

// generic joystick class driver interface
HumanInput::IGenJoystick *HumanInput::AccelGyroInpClassDriver::getDevice(int idx) const
{
  if (idx >= deviceNum && secDrv)
    return secDrv->getDevice(idx - deviceNum);
  if ((uint32_t)idx < (uint32_t)deviceNum)
  {
    G_ANALYSIS_ASSUME(idx < INPDEV_MAX);
    return device[idx];
  }
  return NULL;
}
void HumanInput::AccelGyroInpClassDriver::useDefClient(HumanInput::IGenJoystickClient *cli)
{
  defClient = cli;
  for (int i = 0; i < deviceNum; i++)
    device[i]->setClient(defClient);
  if (secDrv)
    secDrv->useDefClient(cli);
}

HumanInput::IGenJoystick *HumanInput::AccelGyroInpClassDriver::getDeviceByUserId(unsigned short userId) const
{
  for (int i = 0; i < deviceNum; i++)
    if (device[i]->getUserId() == userId)
      return device[i];
  return secDrv ? secDrv->getDeviceByUserId(userId) : NULL;
}

void HumanInput::AccelGyroInpClassDriver::setDefaultJoystick(IGenJoystick *ref)
{
  if (ref != defJoy)
    if (ref)
      raw_state_joy = ref->getRawState();
    else
      memset(&raw_state_joy, 0, sizeof(raw_state_joy));

  defJoy = ref;
  if (secDrv)
    secDrv->setDefaultJoystick(ref);
}
