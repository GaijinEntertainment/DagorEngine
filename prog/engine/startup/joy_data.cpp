// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <drv/hid/dag_hiGlobals.h>

HumanInput::JoystickSettings HumanInput::stg_joy = {false, false, 200, 500, false, false};

HumanInput::JoystickRawState HumanInput::raw_state_joy = {0, 0, 0, 0, 0, 0, {0, 0}, HumanInput::ButtonBits(), HumanInput::ButtonBits(),
  {-1, -1, -1, -1}, {-1, -1, -1, -1}, 0, 0, 0, 0, 0, 0, 0, 0, false,
  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}};

HumanInput::ButtonBits HumanInput::JoystickRawState::btmp[2];
const HumanInput::ButtonBits HumanInput::ButtonBits::ZERO;
