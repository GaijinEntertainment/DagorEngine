#pragma once
#include "ecsQueryInternal.h"
#include <daECS/core/entityManager.h>

#define DEBUG_VERBOSE_QUERY(...)
// #define DEBUG_VERBOSE_QUERY debug

#define DEBUG_VERY_VERBOSE_QUERY(...)
// #define DEBUG_VERY_VERBOSE_QUERY debug

namespace ecs
{
static constexpr bool allow_create_sync_within_es = true; // decreases optimizations
struct RaiiCounter
{
  int &counter;
  RaiiCounter(int &counter_) : counter(counter_) { ++counter; }
  ~RaiiCounter() { --counter; }
  EA_NON_COPYABLE(RaiiCounter)
};
struct RaiiOptionalCounter
{
  int *__restrict counter;
  static void inc(int *__restrict c)
  {
    if (c)
      ++c;
  }
  static void dec(int *__restrict c)
  {
    if (c)
      --c;
  }
  RaiiOptionalCounter(bool can, int &c) : counter(can ? &c : nullptr) { inc(counter); }
  ~RaiiOptionalCounter() { dec(counter); }
  EA_NON_COPYABLE(RaiiOptionalCounter)
};

__forceinline bool EntityManager::isResolved(uint32_t index) const { return isResolved(getQueryStatus(index)); }

__forceinline bool EntityManager::isCompletelyResolved(uint32_t index) const { return isFullyResolved(getQueryStatus(index)); }


__forceinline bool EntityManager::updatePersistentQueryInternal(archetype_t last_arch_count, uint32_t index, bool should_re_resolve)
{
  DEBUG_VERY_VERBOSE_QUERY("updatePersistent = %s, index = %d", queryDescs[index].name.c_str(), index);
  DAECS_EXT_ASSERT(!isConstrainedMTMode());
  DEBUG_VERBOSE_QUERY("update = %s, current resolved= %d", queryDescs[index].name.c_str(), getQueryStatus(index));
  const bool ret = makeArchetypesQueryInternal(last_arch_count, index, should_re_resolve);
#if DAGOR_DBGLEVEL > 0
  if (!ret)
  {
    DEBUG_VERBOSE_QUERY("no archs %s", queryDescs[index].name.c_str());
  }
#endif
  return ret;
}

__forceinline bool EntityManager::updatePersistentQuery(archetype_t last_arch_count, QueryId h, uint32_t &index,
  bool should_re_resolve)
{
  DAECS_EXT_ASSERT(isQueryValid(h));
  index = h.index();
  return updatePersistentQueryInternal(last_arch_count, index, should_re_resolve);
}

// checks if es is suitable for that archetype
__forceinline bool EntityManager::doesQueryIdApplyToArch(QueryId queryId, archetype_t archetype)
{
  DAECS_EXT_ASSERT(isQueryValid(queryId) && queriesReferences[queryId.index()]);
  auto &archDesc = archetypeQueries[queryId.index()];
  if (!archDesc.archSubQueriesCount)
    return false;
  if (archetype == archDesc.firstArch) // 18 % of all queries!
    return true;
  const uint32_t archSubQueryId = uint32_t(archetype) - uint32_t(archDesc.firstArch);
  if (archSubQueryId >= archDesc.archSubQueriesCount)
    return false;
  if (archDesc.isInplaceQueries())
  {
    // linear search inside inplace queries. that is likely be to quiet often case, and this data is already inside cache line
    auto e(archDesc.queriesInplace() + archDesc.getQueriesCount());
    return eastl::find(archDesc.queriesInplace() + 1, e, archetype) != e; // first one is already compared, as it is firstArch
  }

  return archSubQueriesContainer[archetypeEidQueries[queryId.index()].archSubQueriesAt + archSubQueryId] != INVALID_ARCHETYPE;
}

__forceinline void EntityManager::notifyESEventHandlers(EntityId eid, archetype_t archetype, ArchEsList list)
{
  DAECS_EXT_ASSERT(archetypesES[list].size() > archetype);
  // we use shallow copy instead of just const reference, because unfortunately we still have createSync/instantiateTemplate inside ES
  // calls as long as it is removed - should not be needed anymore.
  auto &refList = archetypesES[list][archetype];
  if (!refList.empty())
  {
    auto es_list = refList.getShallowCopy();
#if DAECS_EXTENSIVE_CHECKS
    refList.lock(eid);
#endif
    notifyESEventHandlersInternal(eid, archListEvents[list], es_list.cbegin(), es_list.cend());
#if DAECS_EXTENSIVE_CHECKS
    archetypesES[list][archetype].unlock(); // archetypesES[list] could get reallocated inside
#endif
  }
}

__forceinline uint64_t scheduled_from_eid_second(EntityId eid, uint32_t cidx)
{
  return uint64_t(entity_id_t(eid)) | (uint64_t(cidx) << 32ULL);
}
__forceinline EntityId get_eid_from_scheduled(uint64_t scheduled) { return EntityId(entity_id_t(scheduled & 0xFFFFFFFF)); }
__forceinline uint32_t get_second_from_scheduled(uint64_t scheduled) { return (scheduled >> 32ULL); }

__forceinline void EntityManager::scheduleTrackChangedNoMutex(EntityId eid, component_index_t cidx)
{
  eidTrackingQueue.insert(scheduled_from_eid_second(eid, cidx));
}


}; // namespace ecs
