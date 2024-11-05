//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <squirrel.h>
class SqModules;

namespace darg
{

void register_browser_rendobj_factories();
void bind_browser_behavior(SqModules *moduleMgr);

} // namespace darg
