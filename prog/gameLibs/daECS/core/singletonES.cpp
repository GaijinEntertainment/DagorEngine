// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daECS/core/entityManager.h>
#include <daECS/core/entitySystem.h>
#include <daECS/core/coreEvents.h>
#include "ecsQueryInternal.h"
#include "entityManagerEvent.h"
#include "singletonName.h"

namespace ecs
{

static inline hash_str_t get_base_template_hash(const char *name)
{
  for (const char *plus = name;; ++plus)
    if (!*plus || *plus == '+')
      return mem_hash_fnv1(name, (plus - name));
}

EntityId EntityManager::hasSingletoneEntity(hash_str_t hash) const
{
  auto it = singletonEntities.find(hash);
  return it != singletonEntities.end() ? it->second : EntityId();
}

EntityId EntityManager::hasSingletoneEntity(const char *n) const { return hasSingletoneEntity(get_base_template_hash(n)); }

static constexpr ComponentDesc singleton_creation_comps[] = {
  {ECS_HASH("eid"), ComponentTypeInfo<EntityId>()},    // ro
  {ECS_HASH(SINGLETON_NAME), ComponentTypeInfo<Tag>()} // rq
};

inline void singleton_creation_event(const Event &evt, const QueryView &components)
{
  auto *__restrict eid = ECS_QUERY_COMP_RO_PTR(EntityId, singleton_creation_comps, "eid");
  const char *eidTemplName = components.manager().getEntityTemplateName(*eid);
  if (!eidTemplName)
  {
    logerr("entity %d has no template!", entity_id_t(*eid));
    return;
  }
  if (evt.is<EventComponentsDisappear>() || evt.is<EventComponentsAppear>())
    logerr("Entity:%d(%s) is adding singleton to entity. That's illegal.", entity_id_t(*eid), eidTemplName);
  const auto hash = get_base_template_hash(eidTemplName);
  auto it = components.manager().singletonEntities.find(hash);
  const bool hasEntity = it != components.manager().singletonEntities.end();
  if (evt.is<EventComponentsDisappear>() || evt.is<EventEntityDestroyed>()) // disappearing
  {
    if (!hasEntity)
    {
      logerr("singleton template %s on destroy entity %d wasn't registered", eidTemplName, entity_id_t(*eid));
    }
    else
      components.manager().singletonEntities.erase(it);
    return;
  }
  if (hasEntity) // appearing
  {
    logerr("singleton template %s already created in entity %d, while creating %d, %s", eidTemplName, entity_id_t(it->second),
      entity_id_t(*eid), evt.getName());
  }
  components.manager().singletonEntities[hash] = *eid;
}

static EntitySystemDesc singleton_creation_events_es_desc("singleton_creation_events_es",
  EntitySystemOps(nullptr, singleton_creation_event), dag::ConstSpan<ComponentDesc>(), make_span(singleton_creation_comps + 0, 1),
  make_span(singleton_creation_comps + 1, 1), dag::ConstSpan<ComponentDesc>(),
  EventSetBuilder<EventEntityCreated, EventComponentsAppear, EventEntityDestroyed,
    EventComponentsDisappear>::build(), //
  0, nullptr, nullptr, "__first_sync_point");


EntityId EntityManager::getSingletonEntity(const HashedConstString name)
{
  auto oldIt = singletonEntities.find(name.hash);
  return oldIt != singletonEntities.end() ? oldIt->second : INVALID_ENTITY_ID;
}

// this all can be done completely independent from entityManager. Only validation is not possible.
EntityId EntityManager::getOrCreateSingletonEntity(const HashedConstString name)
{
  auto oldIt = singletonEntities.find(name.hash);
  if (oldIt != singletonEntities.end())
    return oldIt->second;

  const EntityId eid = createEntitySync(name.str);
  auto newIt = singletonEntities.find(name.hash);
  if (newIt == singletonEntities.end())
  {
    logerr("%s is not singletone!", name.str);
    destroyEntity(eid);
  }
  else if (newIt->second != eid)
  {
    logerr("%s %d singleton was possesed by %d!", name.str, entity_id_t(eid), entity_id_t(newIt->second));
    destroyEntity(eid);
  }
  return eid;
}


} // namespace ecs
