// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "entityManagerEvent.h"
#include <daECS/core/entitySystem.h>
#include <daECS/core/coreEvents.h>
#include <daECS/core/schemelessEvent.h>
#include "tokenize_const_string.h"
#include "check_es_optional.h"
#include "ecsPerformQueryInline.h"

namespace ecs
{

bool events_perf_markers = false;

#if DAGOR_DBGLEVEL > 0
void EntitySystemDesc::cacheProfileTokensOnceOOL() const
{
#if TIME_PROFILER_ENABLED
  interlocked_relaxed_store(dapToken, ::da_profiler::add_copy_description(getModuleName(), /*line*/ 0, /*flags*/ 0, name));
#endif
}
#endif

inline bool has_component(dag::ConstSpan<ComponentDesc> components, component_t name)
{
  for (auto &comp : components)
    if (comp.name == name)
      return true;
  return false;
}

__forceinline const EntitySystemDesc &EntityManager::getESDescForEvent(es_index_type esIndex, const Event &evt) const
{
  G_UNUSED(evt);
  const EntitySystemDesc &es = *esList[esIndex];
#if DAGOR_DBGLEVEL > 1 && DAECS_EXTENSIVE_CHECKS // it is extremely slow, to find each time
  G_ASSERTF(es.ops.onEvent && es.evSet.count(evt.getType()) > 0, "Broken ES events index while handling event '%s' in ES '%s'",
    evt.getName(), es.name);
#endif
  return es;
}

__forceinline void EntityManager::callESEvent(es_index_type esIndex, const Event &evt, QueryView &qv)
{
  const EntitySystemDesc &es = getESDescForEvent(esIndex, evt);
  qv.userData = es.userData;
  if (PROFILE_ES(es, evt))
  {
    TIME_SCOPE_ES(es);
    es.ops.onEvent(evt, qv);
  }
  else
    es.ops.onEvent(evt, qv);
}

void EntityManager::notifyESEventHandlers(EntityId eid, const Event &evt)
{
  auto eventType = evt.getType();
#if DAECS_EXTENSIVE_CHECKS
  // notifyESEventHandlers is called only on alive entities by contract
  {
    const uint32_t idx = eid.index();
    G_ASSERT(idx < entDescs.allocated_size());
    const auto &entDesc = entDescs[idx]; // intenionally create copy. entDescs can change during the loop
    const uint32_t archetype = entDesc.archetype;
    G_ASSERTF(entDesc.generation == eid.generation(), "%d: %d == %d", eid.index(), entDesc.generation, eid.generation());
    G_ASSERT(archetype < archetypes.size());
  }
  G_ASSERT(eventType != EventComponentChanged::staticType());
#endif
  auto esListIt = esEvents.find(eventType); // this is extremely slow, and should not be needed. We can register all events with flat
                                            // arrays, not search.
  // todo: optimize. need to write event idx instead of name though
  if (esListIt == esEvents.end())
    return;
  auto &list = esListIt->second;
  if (allow_create_sync_within_es && list.empty()) // to be removed when no instantiate templates is happening within ES
    return;
  const uint32_t idx = eid.index();

  QueryView qv(*this);
  QueryView::ComponentsData componentData[MAX_ONE_EID_QUERY_COMPONENTS];
  qv.componentData = componentData;
  RaiiOptionalCounter nested(!isConstrainedMTMode(), nestedQuery); // it is correcly to put it around es call, but it is faster
  es_index_type const *es_start = list.begin(), *es_end = list.end();
  do
  {
    es_index_type esIndex = *es_start;
    QueryId queryId = esListQueries[esIndex];
#if DAECS_EXTENSIVE_CHECKS
    if (!isQueryValid(queryId))
    {
      logerr("Currently supporting of 'empty' ES for unicast messages is not available."
             " If ever really neded, just remove this logerr and guards"
             " Or, likely, you somehow unintentionally made empty ES and it is a bug {%s}",
        esList[esIndex]->name);
      // const EntitySystemDesc &es = getESDescForEvent(esIndex, evt);
      // es.ops.onEvent(evt, QueryView(es.userData));
      continue;
    }
#else
    if (!queryId) // this should not be not needed, if we would never allow to ADD 'empty' ES for unicast messages
      continue;
#endif
    if (fillEidQueryView(eid, entDescs[idx], queryId, qv)) // intentionally get entDescs[idx] again. event query can add Entity, and
                                                           // more, can even re-create current entity.
      callESEvent(esIndex, evt, qv);
  } while (++es_start != es_end);
}

void EntityManager::notifyESEventHandlersInternal(EntityId eid, const Event &evt, const es_index_type *__restrict es_start,
  const es_index_type *__restrict es_end)
{
  DAECS_EXT_ASSERT(es_start != es_end);
  const uint32_t idx = eid.index();
  QueryView qv(*this);
  QueryView::ComponentsData componentData[MAX_ONE_EID_QUERY_COMPONENTS];
  qv.componentData = componentData;
  RaiiOptionalCounter nested(!isConstrainedMTMode(), nestedQuery); // it is more correcly to put it around es call, but it is faster
  do
  {
    es_index_type esIndex = *es_start;
    if (fillEidQueryView(eid, entDescs[idx], esListQueries[esIndex], qv)) // intentionally get entDescs[idx] again. event query can add
                                                                          // Entity, and more, can even sync re-create current entity.
      callESEvent(esIndex, evt, qv);
  } while (++es_start != es_end);
}

void EntityManager::dispatchEvent(EntityId eid, Event &evt) // ecs::INVALID_ENTITY_ID means broadcast
{
  const bool isMtMode = isConstrainedMTMode();
  DAECS_EXT_ASSERT(isMtMode || get_current_thread_id() == ownerThreadId);
  DAECS_EXT_ASSERTF(bool(eid) == bool(evt.getFlags() & EVCAST_UNICAST), "event %s has %s flags but sent as %s", evt.getName(),
    (evt.getFlags() & EVCAST_UNICAST) ? "unicast" : "broadcast", bool(eid) ? "unicast" : "broadcast");
  ScopedMTMutexT<decltype(deferredEventsMutex)> evtMutex(isMtMode, ownerThreadId, deferredEventsMutex);
  validateEventRegistration(evt, nullptr);

  deferredEventsCount++;
  emplaceUntypedEvent(eventsStorage, eid, evt);
}

void EntityManager::sendEventImmediate(EntityId eid, Event &evt)
{
  auto state = entDescs.getEntityState(eid);
#if DAGOR_DBGLEVEL > 0
  if (state == EntitiesDescriptors::EntityState::Dead)
    return;
  if (state == EntitiesDescriptors::EntityState::Loading) // it is likely that it is creating entity. As it is scheduled to be created,
                                                          // we can't send immediate event to it anyway.
  {
    logerr("Event %s sent with sendEventImmediate for loading entity %d (%s), and it will be skipped."
           " Consider replace with sendEvent, if you want it defer event till creation of entity.",
      evt.getName(), eid, getEntityFutureTemplateName(eid));
    return;
  }
#else
  if (state != EntitiesDescriptors::EntityState::Alive)
    return;
#endif
  DAECS_EXT_ASSERT(!(evt.getFlags() & EVFLG_CORE));
  DAECS_EXT_ASSERTF(bool(eid) == bool(evt.getFlags() & EVCAST_UNICAST), "event %s has %s flags but sent as %s", evt.getName(),
    (evt.getFlags() & EVCAST_UNICAST) ? "unicast" : "broadcast", bool(eid) ? "unicast" : "broadcast");
  notifyESEventHandlers(eid, evt);
}

void EntityManager::validateEventRegistrationInternal(const Event &evt, const char *name)
{
  if (eventDb.findEvent(evt.getType()) == eventDb.invalid_event_id)
  {
    logerr("%s event<0x%X> not registered. Flags=0x%X size=%d", __FUNCTION__, evt.getType(), evt.getFlags(), evt.getLength());
    eventDb.registerEvent(evt.getType(), 0, 0, name, nullptr, nullptr);
    eventDb.dump();
  }
}

void EntityManager::broadcastEventImmediate(Event &evt)
{
  DAECS_EXT_ASSERTF(bool(evt.getFlags() & EVCAST_BROADCAST), "event %s has %s flags but sent as broadcast", evt.getName(),
    (evt.getFlags() & EVCAST_UNICAST) ? "unicast" : "broadcast");
  auto esListIt = esEvents.find(evt.getType());
  if (esListIt != esEvents.end())
    for (es_index_type esListNo : esListIt->second)
    {
      const EntitySystemDesc &es = getESDescForEvent(esListNo, evt);
      if (PROFILE_ES(es, evt))
      {
        TIME_SCOPE_ES(es);
        performQueryEmptyAllowed(esListQueries[esListNo], (ESFuncType)es.ops.onEvent, (const ESPayLoad &)evt, es.userData, es.quant);
      }
      else
        performQueryEmptyAllowed(esListQueries[esListNo], (ESFuncType)es.ops.onEvent, (const ESPayLoad &)evt, es.userData, es.quant);
    }
}

void EntityManager::sendEvent(EntityId eid, SchemelessEvent &&evt) { dispatchEvent(eid, eastl::move(evt)); }

void EntityManager::broadcastEvent(SchemelessEvent &&evt) { dispatchEvent(INVALID_ENTITY_ID, eastl::move(evt)); }

#if DAGOR_DBGLEVEL > 0
static ska::flat_hash_set<event_type_t> subscribe_for_non_registered_errors;
#endif

void EntityManager::registerEsStage(int j)
{
  const EntitySystemDesc *es = esList[j];
  if (!es->stageMask)
    return;
  if (esForAllEntities[j])
    return;
#if TIME_PROFILER_ENABLED && DAGOR_DBGLEVEL > 0
  es->cacheProfileTokensOnce();
#endif
  for (uint32_t iu = 0, maskLeft = es->stageMask & ((1 << esUpdates.size()) - 1); maskLeft; ++iu, maskLeft >>= 1)
    if (maskLeft & 1)
      esUpdates[iu].insert(es_index_type(j));
}

void EntityManager::registerEsEvent(int j)
{
  const EntitySystemDesc *es = esList[j];
  if (!es->ops.onEvent)
    return;
  // es will perform on all entities. Valid only for unicast events
  const bool forAll = esForAllEntities[j];
  bool logggedInvalidCoreEvtSub = false;
  G_UNUSED(logggedInvalidCoreEvtSub);
#if DAGOR_DBGLEVEL > 0
  enum
  {
    APPEAR = 1,
    RECREATED = 2
  };
  uint32_t appearRecreate = 0;
#endif
  for (auto evt : es->evSet)
  {
    const auto evtId = eventDb.findEvent(evt);
#if DAGOR_DBGLEVEL > 0
    if (evtId == eventDb.invalid_event_id && es->getUserData() && // we only check for scripts, as currently c++ is subscribed for
                                                                  // scripts event!
        subscribe_for_non_registered_errors.insert(evt).second)
    {
      logerr("EntitySystem <%s> from <%s> is subscribed for event <0x%X>, which is not registered", es->name, es->getModuleName(),
        evt);
    }
#endif
    if (evtId != EventsDB::invalid_event_id && forAll && (eventDb.getEventFlags(evtId) & EVCAST_BROADCAST))
    {
#if DAGOR_DBGLEVEL > 0
      logerr("EntitySystem <%s> from module <%s> with all optional components subscribed to broadcast event <0x%X|%s>", es->name,
        es->getModuleName(), evt, eventDb.getEventName(evtId));
#endif
      continue;
    }
#if DAGOR_DBGLEVEL > 0
    if (evt == EventComponentsAppear::staticType())
      appearRecreate |= APPEAR;
    if (evt == EventEntityRecreated::staticType())
      appearRecreate |= RECREATED;
#endif
    if (evt == EventComponentChanged::staticType()) // legacy
      continue;
    esEvents[evt].insert(j);
    if (evtId != eventDb.invalid_event_id && eventDb.getEventFlags(evtId) & EVFLG_PROFILE)
      es->cacheProfileTokensOnce();
  }
#if DAGOR_DBGLEVEL > 0
  if (appearRecreate == (APPEAR | RECREATED))
  {
    logerr("EntitySystem <%s> from module <%s> signed for both ComponentsAppear (on_appear) and EntityRecreated events."
           " That doesn't make sense, as ES will be called twice in a row: first with ComponentsAppear, second with EntityRecreated.\n"
           " * EntityRecreated is sent EACH time entity suffers permutation."
           " * ComponentsAppear is sent if entity mutates in a way, that ES can be applied (and it couldn't apply before mutation)."
           " * EntityCreated is sent if entity is first created, and ES can be applied.\n"
           " Most likely you need only 'on_appear' (which is ComponentsAppear + EntityCreated)."
           " If you for whatever reason need to subscribe for ALL mutations,"
           " you should not additionally subscribe for ComponentsAppear (and you must comment about your reasoning)."
           " Typical reasoning for EntityRecreated subscribtion is debugging, inspection and replication.\n",
      es->name, es->getModuleName());
  }
#endif

  if (es->getCompSet())
  {
    const char *comps = es->getCompSet();
    if (comps[0] == '*') // compatibility, to be removed! We add all RO components and that's it
    {
      for (auto &c : es->componentsRO)
        if (c.name != ECS_HASH("eid").hash) // we can't change eid anyway, so it won't ever be changed
          esOnChangeEvents[c.name].insert(j);
    }
    else // we explicitly iterate over listed components
    {
      G_STATIC_ASSERT(sizeof(Archetype) <= 32);
      tokenize_const_string(comps, ", ", [&](eastl::string_view comp_name) {
        component_t comp = ecs_hash(comp_name);
        esOnChangeEvents[comp].insert(j);

#if DAGOR_DBGLEVEL > 0
        if (!has_component(es->componentsRO, comp) && !has_component(es->componentsRW, comp) && !has_component(es->componentsRQ, comp))
        {
          logerr("Entity System <%s> signs for tracking of changing component <%.*s> "
                 "which is not listed in any of it's list of required components.",
            es->name, comp_name.length(), comp_name.data());
        }
#endif
        return true;
      });
    }
  }
}

void EntityManager::registerEs(int esId)
{
  registerEsStage(esId);
  registerEsEvent(esId);
}

void EntityManager::registerEsEvents()
{
  G_ASSERT(ECS_HASH("name").hash == ecs_hash("name") && ECS_HASH("name").hash == ECS_HASH_SLOW("name").hash);
  esEvents.clear();
  esOnChangeEvents.clear();
  for (int j = 0, ej = esList.size(); j < ej; ++j)
  {
    QueryId h = esListQueries[j];
    if (allow_create_sync_within_es || !h || archetypeQueries[h.index()].getQueriesCount())
      registerEs(j);
    else
      queryToEsMap[h].push_back(j);
  }
}

void EntityManager::validateArchetypeES(archetype_t archetype)
{
#if DAECS_EXTENSIVE_CHECKS
  for (ArchEsList list = (ArchEsList)0; list != ARCHETYPES_ES_LIST_COUNT; list = (ArchEsList)(list + 1))
  {
    G_ASSERT(archetypesES[list].size() > archetype);
    G_UNUSED(list);
    G_UNUSED(archetype);
    auto esListIt = esEvents.find(archListEvents[list].getType());
    if (esListIt == esEvents.end())
      continue;
    G_UNUSED(esListIt);
    DAECS_EXT_ASSERT(archetypesES[list][archetype].size() <= esListIt->second.size());
#if DAGOR_DBGLEVEL > 1 // doesn't happen, so we keep it for debug build only
    for (es_index_type esIndex : esListIt->second)
    {
      // check if es is suitable for that archetype
      QueryId queryId = esListQueries[esIndex];
      if (!isQueryValid(queryId))
      {
        logerr("'empty' ES<%s> for unicast messages of (re)create, destroy doesn't make any sense.", esList[esIndex]->name);
        continue;
      }
      const bool cached = archetypesES[list][archetype].count(esIndex) > 0;
      auto &archDesc = archetypeQueries[queryId.index()];
      DAECS_EXT_ASSERT(isQueryValid(queryId));
      if (!archDesc.getQueriesCount())
      {
        G_ASSERTF(!cached, "%s:", esList[esIndex]->name);
        continue;
      }
      auto it = eastl::lower_bound(archDesc.queriesBegin(), archDesc.queriesEnd(), archetype,
        [](const auto &aq, const uint32_t arch) { return aq < arch; });
      if (it == archDesc.queriesEnd() || *it != archetype)
      {
        G_ASSERTF(!cached, "%s:", esList[esIndex]->name);
        continue;
      }
      G_ASSERTF(cached, "%s:", esList[esIndex]->name);
    }
#endif
  }
#else
  G_UNUSED(archetype);
#endif
}

bool EntityManager::doesEsApplyToArch(es_index_type esIndex, archetype_t archetype)
{
  QueryId queryId = esListQueries[esIndex];
  if (!isQueryValid(queryId))
  {
    logerr("'empty' ES<%s> for unicast messages of (re)create, destroy doesn't make any sense.", esList[esIndex]->name);
    return false;
  }
  return doesQueryIdApplyToArch(queryId, archetype);
}
template <class Container>
static inline void ensure_size(Container &c, size_t to)
{
  if (c.size() > to)
    return;
  if (c.size() == to)
    c.push_back(); // use exponential growth strategy
  else
    c.resize(to + 1);
}

void EntityManager::updateArchetypeESLists(archetype_t archetype)
{
#if DAECS_EXTENSIVE_CHECKS
  event_type_t eventTypes[ARCHETYPES_ES_LIST_COUNT] = {
    EventEntityCreated::staticType(), EventEntityRecreated::staticType(), EventEntityDestroyed::staticType()};
  for (int l = 0; l < ARCHETYPES_ES_LIST_COUNT; ++l)
    G_ASSERT(archListEvents[l].getType() == eventTypes[l]);
#endif
  ensure_size(archetypesRecreateES, archetype);
  for (int l = 0; l < ARCHETYPES_ES_LIST_COUNT; ++l)
  {
    ensure_size(archetypesES[l], archetype);
    auto esListIt = esEvents.find(archListEvents[l].getType());
    if (esListIt == esEvents.end())
      continue;
    archetypesES[l][archetype].checkUnlocked("updateArchetypeESLists archetype =", archetype);
    for (es_index_type esIndex : esListIt->second)
      if (doesEsApplyToArch(esIndex, archetype))
        archetypesES[l][archetype].insert(esIndex);
  }
}

const EntityManager::RecreateEsSet *EntityManager::updateRecreatePair(archetype_t old_archetype, archetype_t new_archetype)
{
  auto &archRecreateList = archetypesRecreateES[old_archetype];
  G_ASSERT(archRecreateList.find(new_archetype) == archRecreateList.end());
  RecreateEsSet &set = archRecreateList[new_archetype];
  set.appear.checkUnlocked("updateRecreatePair, appear, new_archetype =", new_archetype);
  set.disappear.checkUnlocked("updateRecreatePair, disappear, new_archetype =", new_archetype);

  for (int l = 0; l < RECREATE_ES_LIST_COUNT; ++l)
  {
    auto esListIt = esEvents.find(recreateEvents[l].getType());
    if (esListIt == esEvents.end() || esListIt->second.empty())
      continue;
    auto &currentSet = (l == DISAPPEAR_ES ? set.disappear : set.appear);
    const bool shouldApplyToOld = (l == DISAPPEAR_ES);
    const bool shouldApplyToNew = (l == APPEAR_ES);
    G_ASSERT(shouldApplyToOld == !shouldApplyToNew);
    for (es_index_type esIndex : esListIt->second)
    {
      const bool appliesToOld = doesEsApplyToArch(esIndex, old_archetype);
      const bool appliesToNew = doesEsApplyToArch(esIndex, new_archetype);
      if (appliesToOld == shouldApplyToOld && appliesToNew == shouldApplyToNew)
        currentSet.insert(esIndex);
    }
  }

  return &set;
}

template <class T>
uint32_t EntityManager::processEventsExhausted(uint32_t count, T &storage)
{
  DAECS_EXT_ASSERT(!storage.canProcess());
  uint32_t processed = 0;
  for (; !storage.buffers.empty();)
  {
    for (; processed != count; ++processed)
      if (!processEventInternal(*storage.buffers.begin(), // we have to re-read pointer every time, as buffers can be re-allocated
            [&](EntityId eid, Event &event) { dispatchEventImmediate(eid, event); }))
        break;
    auto bi = storage.buffers.begin();
    bool wasEmpty = bi->empty();
    if (wasEmpty)
      storage.buffers.erase(bi);
    if (processed == count)
      return processed;
    if (!wasEmpty)
    {
      G_ASSERTF(0, "buffer should be empty in the end of iteration");
      storage.buffers.erase(bi);
    }
  }
  G_ASSERT(storage.buffers.empty());
  return processed + processEventsActive(count - processed, storage);
}

template uint32_t EntityManager::processEventsExhausted<decltype(EntityManager::LoadingEntityEvents::events)>(uint32_t count,
  decltype(EntityManager::LoadingEntityEvents::events) &storage);

template uint32_t EntityManager::processEventsExhausted<decltype(EntityManager::eventsStorage)>(uint32_t count,
  decltype(EntityManager::eventsStorage) &storage);

template <class T>
void EntityManager::destroyEvents(T &storage)
{
  for (auto bi = storage.buffers.begin(); !storage.buffers.empty(); bi = storage.buffers.erase(bi))
  {
    for (;;)
      if (!processEventInternal(*bi, [](EntityId, Event &) {}))
        break;
  }
  for (;;)
    if (!processEventInternal(storage.active, [](EntityId, Event &) {}))
      break;
}

template void EntityManager::destroyEvents<decltype(EntityManager::LoadingEntityEvents::events)>(
  decltype(EntityManager::LoadingEntityEvents::events) &storage);

template void EntityManager::destroyEvents<decltype(EntityManager::eventsStorage)>(decltype(EntityManager::eventsStorage) &storage);

void EntityManager::addEventForLoadingEntity(EntityId eid, Event &evt)
{
  auto it = eventsForLoadingEntities.find_as(eid, LoadingEntityEvents::Compare());
  if (it == eventsForLoadingEntities.end())
    it = eventsForLoadingEntities.insert(it, LoadingEntityEvents{eid});

  emplaceUntypedEvent(it->events, eid, evt);
}

}; // namespace ecs
