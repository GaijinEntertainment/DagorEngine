// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "gamepad_classdrv.h"
#include "gamepad_device.h"
#include "flightstick_classdrv.h"
#include "keyboard_classdrv.h"
#include "pointing_classdrv.h"
#include "gameinput.h"

#include <drv/hid/dag_hiCreate.h>
#include <drv/hid/dag_hiComposite.h>
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


IGenJoystickClassDrv *HumanInput::createGameInputFlightStickClassDriver() { return new (inimem) FlightStickClassDriver(); }


CompositeJoystickClassDriver *HumanInput::createGameInputCompositeJoystickClassDriver(bool should_mix_input, bool add_flight_stick)
{
  CompositeJoystickClassDriver *cd = CompositeJoystickClassDriver::create();
  if (cd->init())
  {
    global_cls_composite_drv_joy = cd;
    cd->addClassDrv(createGameInputJoystickClassDriver(should_mix_input), true);
    if (add_flight_stick)
      cd->addClassDrv(createGameInputFlightStickClassDriver(), true);
    return cd;
  }

  delete cd;
  return nullptr;
}


IGenJoystickClassDrv *HumanInput::createJoystickClassDriver(bool, bool, bool) { return new (inimem) FlightStickClassDriver(); }


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


static GameInputPointingClassDriver pointing_drv;

IGenPointingClassDrv *HumanInput::createGameInputPointingClassDriver(bool emu_mouse, bool hw_mouse)
{
  memset(&raw_state_pnt, 0, sizeof(raw_state_pnt));
  gameinput::init();

  pointing_drv.composite.emuEnabled = emu_mouse;
  pointing_drv.composite.hwMouseEnabled = hw_mouse;
  pointing_drv.composite.init();

  mouse_emu = &pointing_drv;
  return &pointing_drv;
}


void enable_xbox_hw_mouse(bool en)
{
  pointing_drv.composite.hwMouseEnabled = en;
  pointing_drv.composite.updateStgMouseEnabled();
}


bool is_xbox_hw_mouse_enabled() { return pointing_drv.composite.hwMouseEnabled; }


bool gameinput_is_keyboard_connected() { return gameinput::has_input_device_of_kind(GameInputKindKeyboard); }


bool gameinput_is_mouse_connected() { return gameinput::has_input_device_of_kind(GameInputKindMouse); }


void enable_xbox_emulated_mouse(bool cursor, bool buttons)
{
  pointing_drv.composite.emuMouse.cursorEnabled = cursor;
  pointing_drv.composite.emuMouse.buttonsEnabled = buttons;
  pointing_drv.composite.updateStgMouseEnabled();
}
