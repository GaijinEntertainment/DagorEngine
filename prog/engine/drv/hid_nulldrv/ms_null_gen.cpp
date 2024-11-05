// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <drv/hid/dag_hiPointing.h>
#include <drv/hid/dag_hiGlobals.h>
#include <drv/hid/dag_hiCreate.h>
#include <string.h>

HumanInput::IGenPointingClassDrv *HumanInput::createWinMouseClassDriver() { return createNullMouseClassDriver(); }
