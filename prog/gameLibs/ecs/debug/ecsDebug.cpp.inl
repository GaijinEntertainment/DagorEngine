// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daECS/core/entityManager.h>
#include <daECS/core/entitySystem.h>
#include <daECS/core/coreEvents.h>
#include <daECS/core/componentTypes.h>
#include <startup/dag_globalSettings.h>
#include <ioSys/dag_dataBlock.h>

#define debug(...) logmessage(_MAKE4C('ECS '), __VA_ARGS__)

ECS_DEF_PULL_VAR(ecs_debug);

static bool should_log_destroy = true;
static int8_t force_ecs_debug_state = -1;
static inline bool force_ecs_debug()
{
  if (DAGOR_UNLIKELY(force_ecs_debug_state < 0))
    force_ecs_debug_state = dgs_get_settings()->getBool("forceEcsDebug", false);
  return force_ecs_debug_state > 0;
}

ECS_TAG(ecsDebug)
ECS_BEFORE(__first_sync_point)
ECS_ON_EVENT(ecs::EventEntityManagerBeforeClear, ecs::EventEntityManagerAfterClear)
static inline void ecs_debug_entity_clear_es(const ecs::Event &evt)
{
  // Do not log destroy on Emgr's clear
  should_log_destroy = evt.is<ecs::EventEntityManagerAfterClear>();
}

ECS_TAG(ecsDebug)
ECS_BEFORE(__first_sync_point)
static inline void ecs_debug_entity_created_es(const ecs::EventEntityCreated &, ecs::EntityManager &manager, ecs::EntityId eid,
  const TMatrix *transform, const Point3 *position, const ecs::Tag *noECSDebug)
{
  if (noECSDebug && !force_ecs_debug())
    ; // no-op
  else if (const Point3 *pos = transform ? &transform->getcol(3) : position)
    debug("%d: created <%s> at %@", ecs::entity_id_t(eid), manager.getEntityTemplateName(eid), *pos);
  else
    debug("%d: created <%s>", ecs::entity_id_t(eid), manager.getEntityTemplateName(eid));
}

ECS_TAG(ecsDebug)
ECS_BEFORE(__first_sync_point)
static inline void ecs_debug_entity_recreated_es(const ecs::EventEntityRecreated &, ecs::EntityManager &manager, ecs::EntityId eid,
  const ecs::Tag *noECSDebug)
{
  if (noECSDebug && !force_ecs_debug())
    ; // no-op
  else
    debug("%d: recreated as <%s>", ecs::entity_id_t(eid), manager.getEntityTemplateName(eid));
}

ECS_TAG(ecsDebug)
ECS_BEFORE(__first_sync_point)
static inline void ecs_debug_entity_destroyed_es(const ecs::EventEntityDestroyed &, ecs::EntityManager &manager, ecs::EntityId eid,
  const ecs::Tag *noECSDebug)
{
  if (noECSDebug && !force_ecs_debug())
    ; // no-op
  else if (should_log_destroy)
    debug("%d: destroyed <%s>", ecs::entity_id_t(eid), manager.getEntityTemplateName(eid));
}
