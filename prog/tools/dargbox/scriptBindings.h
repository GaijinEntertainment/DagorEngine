// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <squirrel.h>
class SqModules;

void bind_dargbox_script_api(SqModules *module_mgr);
void unbind_dargbox_script_api(HSQUIRRELVM vm);
