//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <ecs/anim/animCharEffectors.h>
#include <dasModules/dasModulesCommon.h>
#include <dasModules/aotEcs.h>
#include <ecs/anim/animCharEffectors.h>

MAKE_TYPE_FACTORY(EffectorData, EffectorData)

namespace bind_dascript
{
inline EffectorData *getNullableRW_EffectorData(ecs::Object &animchar_effectors__effectorsState, const char *name)
{
  return animchar_effectors__effectorsState.getNullableRW<EffectorData>(ECS_HASH_SLOW(name ? name : ""));
}

inline const EffectorData *getNullable_EffectorData(const ecs::Object &animchar_effectors__effectorsState, const char *name)
{
  return animchar_effectors__effectorsState.getNullable<EffectorData>(ECS_HASH_SLOW(name ? name : ""));
}
} // namespace bind_dascript
