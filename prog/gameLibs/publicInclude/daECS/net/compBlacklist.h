//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <daECS/core/internal/typesAndLimits.h>
#include <daECS/core/entityId.h>

#define ECS_NET_COMP_BLACKLIST_ENABLED (DAGOR_DBGLEVEL > 0)

namespace net
{
#if ECS_NET_COMP_BLACKLIST_ENABLED
void replicated_component_client_modify_blacklist_reset();
void replicated_component_client_modify_blacklist_add(ecs::component_t component);
void replicated_component_on_client_deserialize(ecs::EntityManager &mgr, ecs::EntityId eid, ecs::component_index_t cidx);
void replicated_component_on_client_change(ecs::EntityManager &mgr, ecs::EntityId eid, ecs::component_index_t cidx);
void replicated_component_on_client_destroy(ecs::EntityId eid);
#else
inline void replicated_component_client_modify_blacklist_reset() {}
inline void replicated_component_client_modify_blacklist_add(ecs::component_t) {}
inline void replicated_component_on_client_deserialize(ecs::EntityManager &, ecs::EntityId, ecs::component_index_t) {}
inline void replicated_component_on_client_change(ecs::EntityManager &, ecs::EntityId, ecs::component_index_t) {}
inline void replicated_component_on_client_destroy(ecs::EntityId) {}
#endif
} // namespace net
