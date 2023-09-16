//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

class SqModules;

namespace nestdb
{

void init();
void shutdown();
void bind_api(SqModules *module_mgr);

} // namespace nestdb
