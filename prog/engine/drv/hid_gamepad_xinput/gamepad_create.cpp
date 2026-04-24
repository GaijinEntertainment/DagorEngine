// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "gamepad_classdrv.h"

#include <drv/hid/dag_hiCreate.h>

using namespace HumanInput;

IGenJoystickClassDrv *HumanInput::createXinputJoystickClassDriver(bool should_mix_input)
{
  Xbox360GamepadClassDriver *cd = new (inimem) Xbox360GamepadClassDriver(should_mix_input);
  debug("[HID][XINP] driver created in %d devices mode", should_mix_input ? "mixed" : "separate");
  if (!cd->init())
  {
    delete cd;
    cd = NULL;
  }
  return cd;
}
