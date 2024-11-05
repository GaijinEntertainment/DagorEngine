// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "ms_classdrv_win.h"
#include "ms_device_win.h"
#include <drv/hid/dag_hiGlobals.h>
#include <drv/hid/dag_hiCreate.h>
#include <debug/dag_debug.h>
#include <util/dag_string.h>
using namespace HumanInput;

const unsigned JOY_XINPUT_LMB_ALIAS_BIT = 0x20000;

IGenPointingClassDrv *HumanInput::createWinMouseClassDriver()
{
  memset(&raw_state_pnt, 0, sizeof(raw_state_pnt));

  WinMouseClassDriver *cd = new (inimem) WinMouseClassDriver;
  if (!cd->init())
  {
    delete cd;
    cd = NULL;
  }
  return cd;
}


bool WinMouseClassDriver::init()
{
  stg_pnt.mouseEnabled = false;

  refreshDeviceList();
  if (device)
    enable(true);
  return true;
}

void WinMouseClassDriver::destroyDevices()
{
  unacquireDevices();
  if (device)
    delete device;
  device = NULL;
}

// generic hid class driver interface
void WinMouseClassDriver::enable(bool en)
{
  enabled = en;
  stg_pnt.mouseEnabled = en;
}
void WinMouseClassDriver::destroy()
{
  destroyDevices();
  stg_pnt.mousePresent = false;

  delete this;
}

// generic keyboard class driver interface
IGenPointing *WinMouseClassDriver::getDevice(int idx) const
{
  if (idx == 0)
    return device;
  return NULL;
}
void WinMouseClassDriver::useDefClient(IGenPointingClient *cli)
{
  defClient = cli;
  if (device)
    device->setClient(defClient);
}

void WinMouseClassDriver::refreshDeviceList()
{
  destroyDevices();

  bool was_enabled = enabled;
  enable(false);

  device = new WinMouseDevice;

  // update global settings
  stg_pnt.mousePresent = true;
  enable(was_enabled);

  useDefClient(defClient);
  acquireDevices();
}

void WinMouseClassDriver::onButtonDown(int btn_id)
{
  raw_state_pnt.mouse.buttons |= 1 << btn_id;
  if (defClient)
    defClient->gmcMouseButtonDown(device, btn_id);
}


void WinMouseClassDriver::onButtonUp(int btn_id)
{
  raw_state_pnt.mouse.buttons &= ~(1 << btn_id);
  if (defClient)
    defClient->gmcMouseButtonUp(device, btn_id);
}

void WinMouseClassDriver::updateDevices()
{
  device->update();

  if (HumanInput::stg_pnt.allowEmulatedLMB)
  {
    unsigned bw0 = raw_state_joy.buttons.getWord0();
    if (prevJoyButtons != bw0)
    {
      if ((bw0 & JOY_XINPUT_LMB_ALIAS_BIT) && !(prevJoyButtons & JOY_XINPUT_LMB_ALIAS_BIT))
        onButtonDown(0);
      else if (!(bw0 & JOY_XINPUT_LMB_ALIAS_BIT) && (prevJoyButtons & JOY_XINPUT_LMB_ALIAS_BIT))
        onButtonUp(0);
      prevJoyButtons = bw0;
    }
  }
  else
  {
    if (prevJoyButtons & JOY_XINPUT_LMB_ALIAS_BIT)
      onButtonUp(0);
    prevJoyButtons = 0;
  }
}
