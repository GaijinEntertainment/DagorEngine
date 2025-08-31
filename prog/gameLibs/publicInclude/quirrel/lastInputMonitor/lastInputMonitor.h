//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <squirrel.h>

class SqModules;

namespace inputmonitor
{
void register_input_handler();
void notify_input_devices_used();
void register_sq_module(SqModules *module_manager);
} // namespace inputmonitor
