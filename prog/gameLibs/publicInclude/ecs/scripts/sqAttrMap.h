//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <squirrel.h>
#include <sqrat.h>

namespace ecs
{
SQInteger sq_obj_to_comp_map(HSQUIRRELVM vm, const Sqrat::Var<Sqrat::Object> &attr_map_obj, ComponentsMap &comp_map);
SQInteger comp_map_to_sq_obj(HSQUIRRELVM vm, const ComponentsMap &comp_map);
}; // namespace ecs
