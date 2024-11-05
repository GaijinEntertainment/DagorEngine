// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <drv/hid/dag_hiKeyboard.h>
#include <drv/hid/dag_hiGlobals.h>
#include <drv/hid/dag_hiCreate.h>
#include <string.h>

HumanInput::IGenKeyboardClassDrv *HumanInput::createWinKeyboardClassDriver() { return createNullKeyboardClassDriver(); }
