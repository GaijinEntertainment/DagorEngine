// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "keyboard_classdrv.h"
#include "keyboard_device.h"
#include <drv/hid/dag_hiGlobals.h>
#include <memory/dag_memBase.h>
#include <cstring>

using namespace HumanInput;


bool HumanInput::keyboard_has_ime_layout() { return false; }


bool GameInputKeyboardClassDriver::init()
{
  stg_kbd.enabled = false;

  init_gameinput_keyboard_tables();
  memset(&raw_state_kbd, 0, sizeof(raw_state_kbd));

  refreshDeviceList();
  if (device)
    enable(true);
  return true;
}


void GameInputKeyboardClassDriver::destroyDevices()
{
  if (device)
    delete device;
  device = nullptr;
}


void GameInputKeyboardClassDriver::enable(bool en)
{
  enabled = en;
  stg_kbd.enabled = en;
}


void GameInputKeyboardClassDriver::destroy()
{
  destroyDevices();
  stg_kbd.present = false;

  delete this;
}


void GameInputKeyboardClassDriver::refreshDeviceList()
{
  destroyDevices();

  bool was_enabled = enabled;
  enable(false);

  device = new (inimem) GameInputKeyboardDevice;

  stg_kbd.present = true;
  enable(was_enabled);

  useDefClient(defClient);
}


void GameInputKeyboardClassDriver::updateDevices()
{
  if (device)
    device->update();
}


IGenKeyboard *GameInputKeyboardClassDriver::getDevice(int idx) const { return (idx == 0) ? device : nullptr; }


void GameInputKeyboardClassDriver::useDefClient(IGenKeyboardClient *cli)
{
  defClient = cli;
  if (device)
    device->setClient(defClient);
}
