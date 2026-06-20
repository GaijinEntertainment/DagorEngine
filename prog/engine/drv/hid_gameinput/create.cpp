// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "gamepad_classdrv.h"
#include "gamepad_device.h"
#include "keyboard_classdrv.h"
#include "mouse_emu.h"

#include <drv/hid/dag_hiCreate.h>
#include <string.h>

using namespace HumanInput;

IGenJoystickClassDrv *HumanInput::createGameInputJoystickClassDriver(bool should_mix_input)
{
  GameInputGamepadClassDriver *cd = new (inimem) GameInputGamepadClassDriver(should_mix_input);
  debug("[HID][GINP] driver created in %d devices mode", should_mix_input ? "mixed" : "separate");
  if (!cd->init())
  {
    delete cd;
    cd = nullptr;
  }
  return cd;
}


IGenKeyboardClassDrv *HumanInput::createGameInputKeyboardClassDriver()
{
  GameInputKeyboardClassDriver *cd = new (inimem) GameInputKeyboardClassDriver;
  if (!cd->init())
  {
    delete cd;
    cd = nullptr;
  }
  return cd;
}


static MouseEmuDriver mouse_emu_drv;

IGenPointingClassDrv *HumanInput::createMouseEmuClassDriver()
{
  memset(&raw_state_pnt, 0, sizeof(raw_state_pnt));
  mouse_emu = &mouse_emu_drv;
  return &mouse_emu_drv;
}


void enable_xbox_hw_mouse(bool en)
{
  mouse_emu_drv.hwMouseEnabled = en;
  stg_pnt.mouseEnabled = (mouse_emu_drv.emuDriverEnabled && (mouse_emu_drv.emuCursorEnabled || mouse_emu_drv.emuButtonsEnabled)) ||
                         mouse_emu_drv.hwMouseEnabled;
}


bool is_xbox_hw_mouse_enabled() { return mouse_emu_drv.hwMouseEnabled; }


void enable_xbox_emulated_mouse(bool cursor, bool buttons)
{
  mouse_emu_drv.emuCursorEnabled = cursor;
  mouse_emu_drv.emuButtonsEnabled = buttons;
  stg_pnt.mouseEnabled = (mouse_emu_drv.emuDriverEnabled && (mouse_emu_drv.emuCursorEnabled || mouse_emu_drv.emuButtonsEnabled)) ||
                         mouse_emu_drv.hwMouseEnabled;
}
