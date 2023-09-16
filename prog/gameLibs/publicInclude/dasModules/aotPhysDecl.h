//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <daScript/daScript.h>
#include <dasModules/dasModulesCommon.h>
#include <ecs/phys/ragdoll.h>
#include <ecs/phys/physBody.h>

#include <phys/dag_physDecl.h>

MAKE_TYPE_FACTORY(PhysRagdoll, PhysRagdoll);
MAKE_TYPE_FACTORY(PhysSystemInstance, PhysSystemInstance);
MAKE_TYPE_FACTORY(PhysBody, PhysBody);
MAKE_TYPE_FACTORY(ProjectileImpulse, ProjectileImpulse);
MAKE_TYPE_FACTORY(ImpulseData, ProjectileImpulse::ImpulseData);
MAKE_TYPE_FACTORY(ProjectileImpulseDataPair, das::ProjectileImpulseDataPair);

namespace bind_dascript
{
inline PhysSystemInstance *ragdoll_getPhysSys(PhysRagdoll &ragdoll) { return ragdoll.getPhysSys(); }
inline PhysBody *phys_system_instance_getBody(PhysSystemInstance &instance, int index) { return instance.getBody(index); }

inline void projectile_impulse_get_data(const ProjectileImpulse &impulse,
  const das::TBlock<void, das::TTemporary<das::TArray<das::ProjectileImpulseDataPair>>> &block, das::Context *context,
  das::LineInfoArg *at)
{
  Tab<das::ProjectileImpulseDataPair> pairs(framemem_ptr());
  pairs.reserve(impulse.data.size());
  for (auto &pair : impulse.data)
    pairs.push_back(das::ProjectileImpulseDataPair{pair.first, &pair.second});

  das::Array arr;
  arr.data = (char *)pairs.data();
  arr.size = pairs.size();
  arr.capacity = pairs.size();
  arr.lock = 1;
  arr.flags = 0;
  vec4f arg = das::cast<das::Array *>::from(&arr);
  context->invoke(block, &arg, nullptr, at);
}

} // namespace bind_dascript
