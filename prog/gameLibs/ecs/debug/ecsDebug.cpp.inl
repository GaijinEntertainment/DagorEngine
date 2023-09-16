#include <daECS/core/entityManager.h>
#include <daECS/core/entitySystem.h>
#include <daECS/core/coreEvents.h>
#include <daECS/core/componentTypes.h>

#define LOGLEVEL_DEBUG _MAKE4C('ECS ')

ECS_DEF_PULL_VAR(ecs_debug);

static bool should_log_destroy = true;

ECS_TAG(ecsDebug)
ECS_BEFORE(__first_sync_point)
ECS_ON_EVENT(ecs::EventEntityManagerBeforeClear, ecs::EventEntityManagerAfterClear)
static inline void ecs_debug_entity_clear_es(const ecs::Event &e) { should_log_destroy = e.is<ecs::EventEntityManagerAfterClear>(); }

ECS_TAG(ecsDebug)
ECS_BEFORE(__first_sync_point)
ECS_REQUIRE_NOT(ecs::Tag noECSDebug)
static inline void ecs_debug_entity_created_es_event_handler(const ecs::EventEntityCreated &, ecs::EntityId eid,
  const TMatrix *transform, const Point3 *position)
{
  if (const Point3 *pos = transform ? &transform->getcol(3) : position)
    debug("%d: created <%s> at %@", ecs::entity_id_t(eid), g_entity_mgr->getEntityTemplateName(eid), *pos);
  else
    debug("%d: created <%s>", ecs::entity_id_t(eid), g_entity_mgr->getEntityTemplateName(eid));
}

ECS_TAG(ecsDebug)
ECS_BEFORE(__first_sync_point)
ECS_REQUIRE_NOT(ecs::Tag noECSDebug)
static inline void ecs_debug_entity_recreated_es_event_handler(const ecs::EventEntityRecreated &, ecs::EntityId eid)
{
  debug("%d: recreated as <%s>", ecs::entity_id_t(eid), g_entity_mgr->getEntityTemplateName(eid));
}

ECS_TAG(ecsDebug)
ECS_BEFORE(__first_sync_point)
ECS_REQUIRE_NOT(ecs::Tag noECSDebug)
static void inline ecs_debug_entity_destroyed_es_event_handler(const ecs::EventEntityDestroyed &, ecs::EntityId eid)
{
  if (should_log_destroy)
    debug("%d: destroyed <%s>", ecs::entity_id_t(eid), g_entity_mgr->getEntityTemplateName(eid));
}
