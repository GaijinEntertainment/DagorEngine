//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <EASTL/string.h>

namespace das
{
class Context;
}

void defer_to_act(das::Context *context, const eastl::string &func_name);
void clear_deferred_to_act(das::Context *context);