//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once


#include <daECS/core/entityManager.h>

namespace ecs
{
struct BaseQueryDesc;
}

namespace ecsdebug
{
enum TrackOp
{
  TRACK_READ,
  TRACK_WRITE
};

#if DAECS_EXTENSIVE_CHECKS
void start_track_ecs_component(ecs::component_t comp);
void stop_dump_track_ecs_components();

void track_ecs_component(const ecs::BaseQueryDesc &desc, const char *details, ecs::EntityId eid = ecs::INVALID_ENTITY_ID,
  bool need_stack = false);

void track_ecs_component(ecs::component_t comp, TrackOp op, const char *details, ecs::EntityId eid = ecs::INVALID_ENTITY_ID,
  bool need_stack = false);
inline void track_ecs_component_by_index(ecs::component_index_t cidx, TrackOp op, const char *details,
  ecs::EntityId eid = ecs::INVALID_ENTITY_ID, bool need_stack = false)
{
  const ecs::DataComponents &dataComps = g_entity_mgr->getDataComponents();
  ecsdebug::track_ecs_component(dataComps.getComponentTpById(cidx), op, details, eid, need_stack);
}
inline void track_ecs_component_by_index_with_stack(ecs::component_index_t cidx, TrackOp op, const char *details,
  ecs::EntityId eid = ecs::INVALID_ENTITY_ID)
{
  ecsdebug::track_ecs_component_by_index(cidx, op, details, eid, true);
}
#else
inline void start_track_ecs_component(ecs::component_t) {}
inline void stop_dump_track_ecs_components() {}
inline void track_ecs_component(const ecs::BaseQueryDesc &, const char *, ecs::EntityId = ecs::INVALID_ENTITY_ID, bool = false) {}
inline void track_ecs_component(ecs::component_t, TrackOp, const char *, ecs::EntityId = ecs::INVALID_ENTITY_ID, bool = false) {}
inline void track_ecs_component_by_index(ecs::component_index_t, TrackOp, const char *, ecs::EntityId = ecs::INVALID_ENTITY_ID,
  bool = false)
{}
inline void track_ecs_component_by_index_with_stack(ecs::component_index_t, TrackOp, const char *,
  ecs::EntityId = ecs::INVALID_ENTITY_ID)
{}
#endif
} // namespace ecsdebug
