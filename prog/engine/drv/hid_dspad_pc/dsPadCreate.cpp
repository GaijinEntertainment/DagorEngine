// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "dsPadClassDrv.h"
#include <drv/hid/dag_hiCreate.h>
#include <debug/dag_debug.h>

HumanInput::IGenJoystickClassDrv *HumanInput::createSonyPadJoystickClassDriver()
{
  SonyPadClassDriver *cd = new (inimem) SonyPadClassDriver;
  if (!cd->init())
  {
    debug("[HID][SONY] class driver init failed");
    delete cd;
    return nullptr;
  }
  debug("[HID][SONY] class driver created");
  return cd;
}
