//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <squirrel.h>

class String;


SQInteger sq_cross_call(HSQUIRRELVM from, HSQUIRRELVM to, HSQOBJECT namespace_obj);

// Make deep copy of squirrel data object in another VM.
// Has the following assumptions/limitations:
// * not all squirrel types are supported (i.e. only data types, not functions, weakrefs, etc...)
// * unsafe, i.e. does not perform graph loop checks
// * classes and instances are converted to tables
bool sq_cross_push(HSQUIRRELVM vm_from, SQInteger idx, HSQUIRRELVM vm_to, String *err_msg);
