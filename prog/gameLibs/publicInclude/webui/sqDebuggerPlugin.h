//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

class SqModules;

namespace webui
{
void register_quirrel_debugger(SqModules *module_mgr, const char *url_name, const char *description);
void shutdown_quirrel_debugger(SqModules *module_mgr);
} // namespace webui
