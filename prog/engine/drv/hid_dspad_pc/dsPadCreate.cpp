// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "dsPadClassDrv.h"
#include <drv/hid/dag_hiCreate.h>
#include <debug/dag_debug.h>

HumanInput::IGenJoystickClassDrv *HumanInput::createDsPadJoystickClassDriver()
{
  DsPadClassDriver *cd = new (inimem) DsPadClassDriver;
  if (!cd->init())
  {
    debug("[HID][DSPAD] class driver init failed");
    delete cd;
    return nullptr;
  }
  debug("[HID][DSPAD] class driver created");
  return cd;
}
