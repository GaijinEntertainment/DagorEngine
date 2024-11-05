//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <drv/hid/dag_hiDeclDInput.h>

namespace HumanInput
{
extern IDirectInput8 *dinput8;

void startupDInput();

void printHResult(const char *file, int ln, const char *prefix, int hResult);
}; // namespace HumanInput
