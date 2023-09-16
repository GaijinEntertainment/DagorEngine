//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <humanInput/dag_hiDeclDInput.h>

namespace HumanInput
{
extern IDirectInput8 *dinput8;

void startupDInput();

void printHResult(int hResult);
}; // namespace HumanInput
