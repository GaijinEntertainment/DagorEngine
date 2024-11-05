//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <squirrel.h>
class SqModules;

namespace bindquirrel
{
void register_yupfile_module(SqModules *module_mgr);

void register_yupfile_api(HSQUIRRELVM vm); // depreacted: used only for launchers, cause they do not have modulemgrs yet
} // namespace bindquirrel
