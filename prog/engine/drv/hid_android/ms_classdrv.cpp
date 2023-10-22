#include "ms_classdrv.h"
#include "ms_device.h"
#include <humanInput/dag_hiGlobals.h>
#include <humanInput/dag_hiCreate.h>
#include <debug/dag_debug.h>
#include <util/dag_string.h>
using namespace HumanInput;

extern float dagor_android_scr_xdpi;
extern float dagor_android_scr_ydpi;
extern int dagor_android_scr_xres;
extern int dagor_android_scr_yres;

IGenPointingClassDrv *HumanInput::createWinMouseClassDriver()
{
  memset(&raw_state_pnt, 0, sizeof(raw_state_pnt));
  stg_pnt.touchScreen = true;
  stg_pnt.tapStillCircleRad = dagor_android_scr_xdpi * 1.0f / 25.4f; // +-1 mm
  if (stg_pnt.tapStillCircleRad > 16)
    stg_pnt.tapStillCircleRad = 16;
  debug("tapStillCircleRad=%d (%.1fpix/mm)", stg_pnt.tapStillCircleRad, dagor_android_scr_xdpi / 25.4f);

  TapMouseClassDriver *cd = new (inimem) TapMouseClassDriver;
  if (!cd->init())
  {
    delete cd;
    cd = NULL;
  }
  return cd;
}


bool TapMouseClassDriver::init()
{
  stg_pnt.mouseEnabled = false;

  refreshDeviceList();
  if (device)
    enable(true);
  return true;
}

void TapMouseClassDriver::destroyDevices()
{
  unacquireDevices();
  if (device)
    delete device;
  device = NULL;
}

// generic hid class driver interface
void TapMouseClassDriver::enable(bool en)
{
  enabled = en;
  stg_pnt.mouseEnabled = en;
}
void TapMouseClassDriver::destroy()
{
  destroyDevices();
  stg_pnt.mousePresent = false;

  delete this;
}

// generic keyboard class driver interface
IGenPointing *TapMouseClassDriver::getDevice(int idx) const
{
  if (idx == 0)
    return device;
  return NULL;
}
void TapMouseClassDriver::useDefClient(IGenPointingClient *cli)
{
  defClient = cli;
  if (device)
    device->setClient(defClient);
}

void TapMouseClassDriver::refreshDeviceList()
{
  destroyDevices();

  bool was_enabled = enabled;
  enable(false);

  device = new TapMouseDevice;

  // update global settings
  if (device)
  {
    stg_pnt.mousePresent = true;
    enable(was_enabled);
  }
  else
  {
    stg_pnt.mousePresent = false;
    stg_pnt.mouseEnabled = false;
  }

  useDefClient(defClient);
  acquireDevices();
}
