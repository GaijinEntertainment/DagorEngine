//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <daECS/core/entityComponent.h>
#include <gameRes/dag_collisionResource.h>
#include <gamePhys/collision/collisionObject.h>

ECS_DECLARE_BOXED_TYPE(CollisionResource);
ECS_DECLARE_RELOCATABLE_TYPE(CollisionObject);

namespace ecs
{
class EntityManager;
class EntityId;
class Array;
} // namespace ecs

bool apply_collres_node_flag_rules(ecs::EntityManager &mgr, ecs::EntityId eid, const ecs::Array &rules);
bool clone_collres(ecs::EntityManager &mgr, ecs::EntityId eid);
