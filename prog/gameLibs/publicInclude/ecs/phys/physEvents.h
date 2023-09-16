//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <daECS/core/entityComponent.h>

#define PHYS_ECS_EVENTS                                                                                            \
  PHYS_ECS_EVENT(EventOnProjectileHit, Point3 /*hit_pos*/, Point3 /*norm*/, int /*shell_id*/, int /*phys_mat_id*/, \
    int /*coll_node_id*/, ecs::EntityId /*projectile_eid*/)                                                        \
  PHYS_ECS_EVENT(EventOnPhysImpulse, float /*cur_time*/, int /*coll_node_id*/, Point3 /*pos*/, Point3 /*impulse*/) \
  PHYS_ECS_EVENT(CmdApplyRagdollParameters, float /*cur_time*/)                                                    \
  PHYS_ECS_EVENT(EventOnEntityTeleported, TMatrix /*newtm*/, TMatrix /*prevtm*/)

#define PHYS_ECS_EVENT ECS_UNICAST_EVENT_TYPE
PHYS_ECS_EVENTS
#undef PHYS_ECS_EVENT
