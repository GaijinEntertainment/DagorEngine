//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <squirrel.h>

namespace darg
{

void register_browser_rendobj_factories();
void bind_browser_behavior(HSQUIRRELVM vm);

} // namespace darg
