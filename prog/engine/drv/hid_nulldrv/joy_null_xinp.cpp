// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <drv/hid/dag_hiJoystick.h>
#include <drv/hid/dag_hiGlobals.h>
#include <drv/hid/dag_hiCreate.h>

HumanInput::IGenJoystickClassDrv *HumanInput::createXinputJoystickClassDriver(bool)
{
  return HumanInput::createNullJoystickClassDriver();
}
