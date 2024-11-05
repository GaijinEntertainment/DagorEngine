// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daScript/daScript.h>
#include <dasModules/dasModulesCommon.h>
#include <ecs/scripts/dasEcsEntity.h>
#include <gamePhys/phys/physActor.h>

#include "phys/physUtils.h"
#include "phys/netPhys.h"

MAKE_TYPE_FACTORY(BasePhysActor, BasePhysActor);

DAS_BIND_ENUM_CAST_98_IN_NAMESPACE(IPhysActor::RoleFlags, RoleFlags);
DAS_BIND_ENUM_CAST_98_IN_NAMESPACE(IPhysActor::NetRole, NetRole);
DAS_BIND_ENUM_CAST(PhysTickRateType);

DAS_BASE_BIND_ENUM_98(IPhysActor::RoleFlags, RoleFlags, URF_INITIALIZED, URF_LOCAL_CONTROL, URF_AUTHORITY)
DAS_BASE_BIND_ENUM(PhysTickRateType, PhysTickRateType, Normal, LowFreq)
DAS_BASE_BIND_ENUM_98(IPhysActor::NetRole,
  NetRole,
  ROLE_UNINITIALIZED,
  ROLE_REMOTELY_CONTROLLED_SHADOW,
  ROLE_LOCALLY_CONTROLLED_SHADOW,
  ROLE_REMOTELY_CONTROLLED_AUTHORITY,
  ROLE_LOCALLY_CONTROLLED_AUTHORITY)

namespace bind_dascript
{
inline void base_phys_actor_set_role_and_tickrate_type(BasePhysActor &physActor, IPhysActor::NetRole nr, PhysTickRateType trtype)
{
  physActor.setRoleAndTickrateType(nr, trtype);
}
inline void base_phys_actor_init_role(BasePhysActor &physActor) { physActor.initRole(); }
inline void base_phys_actor_reset_aas(BasePhysActor &physActor) { physActor.resetAAS(); }
inline void base_phys_actor_reset(BasePhysActor &physActor) { physActor.reset(); }

inline float get_actor_min_time_step() { return BasePhysActor::minTimeStep; }
inline float get_actor_max_time_step() { return BasePhysActor::maxTimeStep; }
} // namespace bind_dascript
