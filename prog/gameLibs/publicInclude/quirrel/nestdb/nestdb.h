//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

class SqModules;

namespace nestdb
{

void init();
void shutdown();
void bind_api(SqModules *module_mgr);

} // namespace nestdb
