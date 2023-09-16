//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <squirrel.h>

class SqModules;

void create_script_console_processor(SqModules *module_mgr, const char *cmd);
void destroy_script_console_processor(HSQUIRRELVM vm);
void clear_script_console_processor(HSQUIRRELVM vm);
