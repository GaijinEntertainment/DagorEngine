//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <daECS/core/entityId.h>
#include <debug/dag_assert.h>
#include <sqrat.h>
#include <EASTL/type_traits.h>

namespace Sqrat
{
template <>
struct Var<ecs::EntityId>
{
  ecs::EntityId value;
  Var(HSQUIRRELVM vm, SQInteger idx)
  {
    SQInteger intVal = ecs::ECS_INVALID_ENTITY_ID_VAL;
    G_VERIFY(SQ_SUCCEEDED(sq_getinteger(vm, idx, &intVal)));
    value = ecs::EntityId(ecs::entity_id_t(intVal));
  }

  static void push(HSQUIRRELVM vm, ecs::EntityId value) { sq_pushinteger(vm, static_cast<SQInteger>((ecs::entity_id_t)value)); }
  static const char *getVarTypeName() { return "EntityId"; }
  static bool check_type(HSQUIRRELVM vm, SQInteger idx) { return sq_gettype(vm, idx) == OT_INTEGER; }
};


template <>
struct Var<const ecs::EntityId &>
{
  ecs::EntityId value;
  Var(HSQUIRRELVM vm, SQInteger idx)
  {
    SQInteger intVal = ecs::ECS_INVALID_ENTITY_ID_VAL;
    G_VERIFY(SQ_SUCCEEDED(sq_getinteger(vm, idx, &intVal)));
    value = ecs::EntityId(ecs::entity_id_t(intVal));
  }

  static void push(HSQUIRRELVM vm, const ecs::EntityId &value) { sq_pushinteger(vm, static_cast<SQInteger>((ecs::entity_id_t)value)); }
  static const char *getVarTypeName() { return "EntityId const ref"; }
  static bool check_type(HSQUIRRELVM vm, SQInteger idx) { return sq_gettype(vm, idx) == OT_INTEGER; }
};

SQRAT_MAKE_NONREFERENCABLE(ecs::EntityId)
} // namespace Sqrat
static_assert(!Sqrat::is_referencable<ecs::EntityId>::value, "err");
