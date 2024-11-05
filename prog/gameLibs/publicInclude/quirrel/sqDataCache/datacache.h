//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

class SqModules;

namespace bindquirrel
{
void bind_datacache(SqModules *module_mgr);
void shutdown_datacache();
} // namespace bindquirrel
