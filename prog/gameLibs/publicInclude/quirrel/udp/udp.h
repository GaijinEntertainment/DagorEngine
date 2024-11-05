//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

class SqModules;

namespace bindquirrel::udp
{

void init();
void shutdown();
void bind(SqModules *module_mgr);

} // namespace bindquirrel::udp
