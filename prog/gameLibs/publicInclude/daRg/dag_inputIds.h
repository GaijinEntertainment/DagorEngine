//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

namespace darg
{

enum InputDevice
{
  DEVID_KEYBOARD = 1,
  DEVID_MOUSE = 2,
  DEVID_JOYSTICK = 3,
  DEVID_TOUCH = 4,
  DEVID_VR = 5,

  DEVID_MOUSE_AXIS = 2 | 0x10,
  DEVID_JOYSTICK_AXIS = 3 | 0x10,

  DEVID_NONE = 0
};


enum InputEvent
{
  INP_EV_UNDEFINED = 0,

  INP_EV_PRESS,
  INP_EV_RELEASE,
  INP_EV_POINTER_MOVE,
  INP_EV_MOUSE_WHEEL,
};
} // namespace darg
