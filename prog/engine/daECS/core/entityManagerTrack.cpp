// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daECS/core/componentTypes.h>
#include <vecmath/dag_vecMath.h>
#include "specialized_memcpy.h"
#include "entityManagerEvent.h"
#include <daECS/core/entitySystem.h>
#include <daECS/core/coreEvents.h>
#include <util/dag_stlqsort.h>
#include <dag/dag_vectorMap.h>
#include <generic/dag_relocatableFixedVector.h>
#include <daECS/core/tokenize_const_string.h>
#include "ecsQueryManager.h"

namespace ecs
{
#if DAECS_EXTENSIVE_CHECKS
bool log_unchaged_track_components_marked_as_changed = false;
dag::VectorMap<int, int> component_index_to_unchanged_track_count;
#endif

__forceinline void EntityManager::onChangeEvents(const TrackedChangesTemp &process)
{
  SmallTab<uint64_t, framemem_allocator> processList(process.size());
  uint32_t i = 0;
  for (auto p : process)
    processList[i++] = p;
  stlsort::sort(processList.begin(), processList.end());

  QueryView qv(*this);
  QueryView::ComponentsData componentData[MAX_ONE_EID_QUERY_COMPONENTS];
  qv.componentData = componentData;
  EventComponentChanged evt;
  for (auto p : processList)
  {
    const EntityId eid = get_eid_from_scheduled(p);
    const es_index_type esIndex = es_index_type(get_second_from_scheduled(p));
    const uint32_t idx = eid.index();
    DAECS_EXT_ASSERT(idx < entDescs.allocated_size());
    auto entDesc = entDescs[idx]; // intenionally create copy. entDescs can change during the loop
    if (entDesc.generation != eid.generation())
      continue;
    DAECS_EXT_ASSERT(entDesc.archetype < archetypes.size());
    if (fillEidQueryView(eid, entDesc, esListQueries[esIndex], qv))
    {
      const EntitySystemDesc &es = *esList[esIndex];
      qv.userData = es.userData;
      if (PROFILE_ES(es, evt))
      {
        TIME_SCOPE_ES(es);
        es.ops.onEvent(evt, qv);
      }
      else
        es.ops.onEvent(evt, qv);
    }
  }
}

bool Templates::isReplicatedComponent(template_t t, component_index_t cidx) const
{
  const uint32_t at = templateReplData[t];
  const uint32_t cnt = templateReplData[t + 1] - at;
  if (cnt == 0)
    return false;
  const auto replAt = &replicatedComponents[at], replEnd = replAt + cnt;
  if (cnt < 16) // linear search is faster on small lists. for 16 expected comparisons are 4 + 4 branches, so similar to 8 linear. But
                // instruction cache locality and we prefer to have one branch
    return eastl::find(replAt, replEnd, cidx) != replEnd;
  return eastl::binary_search(replAt, replEnd, cidx);
}

// Loop-invariant part of one (archetype, tracked component) scan: the change-ES list
// filtered to this archetype and the replication gate. Resolved lazily on the first
// changed entity, so scans that find nothing (the common case) pay nothing.
struct EntityManager::TrackedChangeContext
{
  dag::RelocatableFixedVector<es_index_type, 8, true, framemem_allocator> esList; // change-ESes applying to this archetype
  archetype_t archetype;
  component_index_t cidx;
  template_t lastTemplate = INVALID_TEMPLATE_INDEX; // memo: chunks are near-homogeneous in template
  bool lastReplicated = false;
  bool resolved = false;
  bool replicationPossible = false;
  TrackedChangeContext(archetype_t arch, component_index_t c) : archetype(arch), cidx(c) {}
};

void EntityManager::resolveTrackedChangeContext(TrackedChangeContext &ctx)
{
  ctx.resolved = true;
  ctx.replicationPossible = replicationCb && canBeReplicated.test(ctx.cidx, false);
  auto esListIt = esOnChangeEvents.find(dataComponents.getComponentTpById(ctx.cidx));
  if (esListIt != esOnChangeEvents.end())
    for (es_index_type esIndex : esListIt->second)
      if (doesEsApplyToArch(esIndex, ctx.archetype))
        ctx.esList.push_back(esIndex);
}

__forceinline void EntityManager::applyTrackedChange(EntityId eid, TrackedChangeContext &ctx, TrackedChangesTemp &process)
{
  if (DAGOR_UNLIKELY(!ctx.resolved))
    resolveTrackedChangeContext(ctx);
  const uint32_t idx = eid.index();
  if (ctx.replicationPossible && (idx <= MAX_RESERVED_EID_IDX_CONST || exhaustedReservedIndices))
  {
    const template_t templ = entDescs[idx].template_id;
    if (DAGOR_UNLIKELY(templ != ctx.lastTemplate))
    {
      ctx.lastTemplate = templ;
      ctx.lastReplicated = templates.isReplicatedComponent(templ, ctx.cidx);
    }
    if (ctx.lastReplicated)
      replicationCb(*this, eid, ctx.cidx);
  }
  for (const es_index_type esIndex : ctx.esList)
    process.insert(scheduled_from_eid_second(eid, esIndex));
}

__forceinline void EntityManager::preprocessTrackedChange(EntityId eid, archetype_t archetype, component_index_t cidx,
  TrackedChangesTemp &process)
{
  const uint32_t idx = eid.index();
  if (replicationCb && (idx <= MAX_RESERVED_EID_IDX_CONST || exhaustedReservedIndices))
  {
    // todo: remove me, replace with callback in repliction. no query needed, just pass eid + cidx
    // todo: check if we have replication callback. our clients do not have one in release build
    //  todo: make me out of line
    //  check if component is potentially replicated, at least in any template.
    //  That is part of template, but we can first check if component is POTENTIALLY replicated first.
    //  Not all tracked components are replicated, less than 2/3
    if (canBeReplicated.test(cidx, false) && templates.isReplicatedComponent(entDescs[idx].template_id, cidx))
      replicationCb(*this, eid, cidx);
  }

  auto esListIt = esOnChangeEvents.find(dataComponents.getComponentTpById(cidx)); // todo: use direct addressing, by cidx, remove
                                                                                  // lookups
  if (esListIt != esOnChangeEvents.end())
  {
    for (es_index_type esIndex : esListIt->second)
    {
      if (doesEsApplyToArch(esIndex, archetype))
        process.insert(scheduled_from_eid_second(eid, esIndex));
    }
  }
}


// index of the first differing byte between a and b, or sz if streams are equal
static __forceinline size_t first_difference(const uint8_t *__restrict a, const uint8_t *__restrict b, size_t sz)
{
  size_t at = 0;
  for (; at + 64 <= sz; at += 64) // one cache line of each stream per iteration; the common case is full equality
  {
    vec4i eq01 = v_andi(v_cmp_eqi(v_ldui((const int *)(a + at)), v_ldui((const int *)(b + at))),
      v_cmp_eqi(v_ldui((const int *)(a + at + 16)), v_ldui((const int *)(b + at + 16))));
    vec4i eq23 = v_andi(v_cmp_eqi(v_ldui((const int *)(a + at + 32)), v_ldui((const int *)(b + at + 32))),
      v_cmp_eqi(v_ldui((const int *)(a + at + 48)), v_ldui((const int *)(b + at + 48))));
    if (!v_check_xyzw_all_true(v_cast_vec4f(v_andi(eq01, eq23))))
      break;
  }
  for (; at + 16 <= sz; at += 16)
    if (!v_check_xyzw_all_true(v_cast_vec4f(v_cmp_eqi(v_ldui((const int *)(a + at)), v_ldui((const int *)(b + at))))))
      break;
  for (; at < sz; ++at)
    if (a[at] != b[at])
      return at;
  return sz;
}

static __forceinline void copy_pod_component(uint8_t *__restrict to, const uint8_t *__restrict from, size_t sz)
{
  if (sz >= 16)
  {
    size_t at = 0;
    for (; at + 16 <= sz; at += 16)
      v_stui(to + at, v_ldui((const int *)(from + at)));
    if (at != sz) // overlapping tail store; to and from streams never alias
      v_stui(to + sz - 16, v_ldui((const int *)(from + sz - 16)));
  }
  else
    SPECIALIZE_MEMCPY(sz, to, from);
}

template <bool use_ctm, typename T, int csz>
struct ReplicateComparator;

template <typename T, int csz>
struct ReplicateComparator<true, T, csz>
{
  static __forceinline bool replicateCompare(T *__restrict old, const T *__restrict from, size_t, const ComponentTypeManager *ctm)
  {
    return ctm->replicateCompare(old, from);
  }
};
// practicality beats putiry section. specialize for most common non pods, string, array, object
template <>
struct ReplicateComparator<false, ecs::string, 1>
{
  typedef ecs::string type;
  static __forceinline bool replicateCompare(type *__restrict old, const type *__restrict from, size_t, const ComponentTypeManager *)
  {
    if ((*old) == (*from))
      return false;
    *old = *from;
    return true;
  }
};

template <typename T>
struct FinalReplicateComparator // Specialization to bypass ctm virt call to 'replicateCompare'
{
  static __forceinline bool replicateCompare(T *__restrict old, const T *__restrict from, size_t, const ComponentTypeManager *)
  {
    return old->replicateCompare(*from);
  }
};
template <>
struct ReplicateComparator<false, ecs::Object, 1> : public FinalReplicateComparator<ecs::Object>
{};
template <>
struct ReplicateComparator<false, ecs::Array, 1> : public FinalReplicateComparator<ecs::Array>
{};
template <>
struct ReplicateComparator<false, ecs::IntList, 1> : public FinalReplicateComparator<ecs::IntList>
{};

template <class T, int const_csz, bool use_ctm>
void EntityManager::compare_data(TrackedChangesTemp &to_process, uint32_t arch, const uint32_t compOffset,
  const uint32_t oldCompOffset, TrackedChangeContext &ctx, size_t csz, const ComponentTypeManager *ctm)
{
  if (const_csz)
    csz = const_csz;
  const Archetype *archetype = &archetypes.getArchetype(arch);
  for (uint32_t chunkI = 0, chunkE = archetype->manager.getChunksCount(); chunkI < chunkE; ++chunkI)
  {
    const auto &chunk = archetype->manager.getChunk(chunkI);
    uint32_t chunkUsed = chunk.getUsed();
    if (!chunkUsed)
      continue;
    const T *__restrict compStream = (const T *__restrict)chunk.getCompDataUnsafe(compOffset);
    T *__restrict oldCompStream = (T *__restrict)chunk.getCompDataUnsafe(oldCompOffset);

    if (const_csz)
      csz = const_csz;

    const EntityId *__restrict eidStart = ((const EntityId *)chunk.getCompDataUnsafe(0));
    for (const EntityId *__restrict eid = eidStart, *__restrict eidEnd = eidStart + chunkUsed; eid != eidEnd;
         compStream += csz, oldCompStream += csz, ++eid)
      if (ReplicateComparator<use_ctm, T, const_csz>::replicateCompare(oldCompStream, compStream, csz, ctm))
      {
        applyTrackedChange(*eid, ctx, to_process);
      }
#if DAECS_EXTENSIVE_CHECKS
      else if (log_unchaged_track_components_marked_as_changed)
      {
        if (component_index_to_unchanged_track_count.find(ctx.cidx) == component_index_to_unchanged_track_count.end())
          component_index_to_unchanged_track_count[ctx.cidx] = 0;
        component_index_to_unchanged_track_count[ctx.cidx] += 1;
      }
#endif
  }
}

// POD components are compared bitwise (as raw bytes, floats included): scan both streams a cache line at a time
// and locate exact entities only when a difference is found; full equality is the common case.
void EntityManager::compare_data_pod(TrackedChangesTemp &to_process, uint32_t arch, const uint32_t compOffset,
  const uint32_t oldCompOffset, TrackedChangeContext &ctx, const size_t csz)
{
  const Archetype *archetype = &archetypes.getArchetype(arch);
  for (uint32_t chunkI = 0, chunkE = archetype->manager.getChunksCount(); chunkI < chunkE; ++chunkI)
  {
    const auto &chunk = archetype->manager.getChunk(chunkI);
    const uint32_t chunkUsed = chunk.getUsed();
    if (!chunkUsed)
      continue;
    const uint8_t *__restrict compStream = (const uint8_t *__restrict)chunk.getCompDataUnsafe(compOffset);
    uint8_t *__restrict oldCompStream = (uint8_t *__restrict)chunk.getCompDataUnsafe(oldCompOffset);
    const EntityId *__restrict eidStart = ((const EntityId *)chunk.getCompDataUnsafe(0));
    const size_t streamSize = csz * chunkUsed;
#if DAECS_EXTENSIVE_CHECKS
    uint32_t changedInChunk = 0;
#endif
    size_t at = 0;
    while (at < streamSize)
    {
      const size_t diffAt = at + first_difference(oldCompStream + at, compStream + at, streamSize - at);
      if (diffAt == streamSize)
        break;
      const uint32_t inChunkId = uint32_t(diffAt / csz);
      const size_t entityAt = size_t(inChunkId) * csz;
      copy_pod_component(oldCompStream + entityAt, compStream + entityAt, csz);
      applyTrackedChange(eidStart[inChunkId], ctx, to_process);
#if DAECS_EXTENSIVE_CHECKS
      changedInChunk++;
#endif
      at = entityAt + csz;
    }
#if DAECS_EXTENSIVE_CHECKS
    if (log_unchaged_track_components_marked_as_changed && chunkUsed != changedInChunk)
    {
      if (component_index_to_unchanged_track_count.find(ctx.cidx) == component_index_to_unchanged_track_count.end())
        component_index_to_unchanged_track_count[ctx.cidx] = 0;
      component_index_to_unchanged_track_count[ctx.cidx] += chunkUsed - changedInChunk;
    }
#endif
  }
}

bool EntityManager::trackChangedArchetype(uint32_t archetypeId, component_index_t cidx, component_index_t old_cidx,
  TrackedChangesTemp &to_process)
{
#if DAECS_EXTENSIVE_CHECKS
  G_ASSERT_RETURN(archetypeId < archetypes.size(), false);
#endif
  auto componentInArchetypeIndex = archetypes.getArchetypeComponentIdUnsafe(archetypeId, cidx);
  auto oldComponentInArchetypeIndex = archetypes.getArchetypeComponentIdUnsafe(archetypeId, old_cidx);
  G_ASSERT_RETURN(
    componentInArchetypeIndex != INVALID_ARCHETYPE_COMPONENT_ID && oldComponentInArchetypeIndex != INVALID_ARCHETYPE_COMPONENT_ID,
    false);

  const DataComponent dataComponent = dataComponents.getComponentById(cidx);
  const type_index_t componentType = dataComponent.componentType;
  ComponentType componentTypeInfo = componentTypes.getTypeInfo(componentType);
  DAECS_EXT_ASSERT(componentType == dataComponents.getComponentById(old_cidx).componentType);
  const uint32_t componentSize = componentTypeInfo.size;
  DAECS_EXT_ASSERT(componentSize == archetypes.getComponentSize(archetypeId, componentInArchetypeIndex));
  if (!componentSize)
    return false;
  // compare data streams
  const bool podComparable = is_pod(componentTypeInfo.flags);
  const uint32_t compOffset = archetypes.getComponentOfs(archetypeId, componentInArchetypeIndex);
  const uint32_t oldCompOffset = archetypes.getComponentOfs(archetypeId, oldComponentInArchetypeIndex);
  TrackedChangeContext ctx(archetype_t(archetypeId), cidx);
  if (podComparable)
    compare_data_pod(to_process, archetypeId, compOffset, oldCompOffset, ctx, componentSize);
  else
  {
    switch (dataComponent.componentTypeName) // practicality beats purity. Specialize for most common slow types
    {
      case ecs::ComponentTypeInfo<ecs::string>::type:
        G_STATIC_ASSERT(!ecs::ComponentTypeInfo<ecs::string>::is_boxed);
        compare_data<ecs::string>(to_process, archetypeId, compOffset, oldCompOffset, ctx);
        break;
      case ecs::ComponentTypeInfo<ecs::Object>::type:
        G_STATIC_ASSERT(!ecs::ComponentTypeInfo<ecs::Object>::is_boxed);
        compare_data<ecs::Object>(to_process, archetypeId, compOffset, oldCompOffset, ctx);
        break;
      case ecs::ComponentTypeInfo<ecs::Array>::type:
        G_STATIC_ASSERT(!ecs::ComponentTypeInfo<ecs::Array>::is_boxed);
        compare_data<ecs::Array>(to_process, archetypeId, compOffset, oldCompOffset, ctx);
        break;
      case ecs::ComponentTypeInfo<ecs::EidList>::type:
      case ecs::ComponentTypeInfo<ecs::IntList>::type:
      case ecs::ComponentTypeInfo<ecs::FloatList>::type:
        G_STATIC_ASSERT(!ecs::ComponentTypeInfo<ecs::EidList>::is_boxed && !ecs::ComponentTypeInfo<ecs::IntList>::is_boxed &&
                        !ecs::ComponentTypeInfo<ecs::FloatList>::is_boxed);
        G_STATIC_ASSERT(
          sizeof(ecs::EidList::value_type) == sizeof(ecs::IntList::value_type) && sizeof(ecs::EidList) == sizeof(ecs::IntList));
        compare_data<ecs::IntList>(to_process, archetypeId, compOffset, oldCompOffset, ctx);
        break;
      default:
      {
        const ComponentTypeManager *ctm = componentTypes.getTypeManager(componentType);
        if (!ctm)
          return true;
        compare_data<uint8_t, 0, true>(to_process, archetypeId, compOffset, oldCompOffset, ctx, componentSize, ctm);
      }
    }
  }
  return true;
}

void EntityManager::scheduleTrackChanged(EntityId eid, component_index_t cidx)
{
  // if constrained mode, lock mutex | use lock-free queue
  ScopedMTMutexT<OSSpinlock> lock(isConstrainedMTMode(), ownerThreadId, eidTrackingMutex);
  scheduleTrackChangedNoMutex(eid, cidx);
}

inline void EntityManager::trackChanged(EntityId eid, component_index_t cidx, uint64_t scanned_cidx_mask,
  TrackedChangesTemp &to_process)
{
  const unsigned idx = eid.index();
  if (idx >= entDescs.allocated_size())
    return;
  auto entDesc = entDescs[idx];
  uint32_t archetypeId = entDesc.archetype;
  if (entDesc.generation != eid.generation() || archetypeId == INVALID_ARCHETYPE)
    return;
  DAECS_VALIDATE_ARCHETYPE(archetypeId);

  // covered by an archetype-wide scan earlier this tick: shadow is already in sync.
  // the cidx bloom mask keeps the hash probe off entries whose component was not scanned at all
  if (DAGOR_UNLIKELY((scanned_cidx_mask >> (cidx & 63)) & 1) &&
      archetypeTrackingQueue.has_key(uint32_t(archetypeId) | (uint32_t(cidx) << (8 * sizeof(archetype_t)))))
    return;

  const component_index_t old_cidx = dataComponents.getTrackedPair(cidx);
  if (old_cidx == INVALID_COMPONENT_INDEX)
    return;
  archetype_component_id oldComponentInArchetypeIndex = archetypes.getArchetypeComponentIdUnsafe(archetypeId, old_cidx);
  if (oldComponentInArchetypeIndex == INVALID_ARCHETYPE_COMPONENT_ID)
    return;
  const archetype_component_id componentInArchetypeIndex = archetypes.getArchetypeComponentIdUnsafe(archetypeId, cidx);

  G_ASSERT_RETURN(componentInArchetypeIndex < INVALID_ARCHETYPE_COMPONENT_ID, );

  const DataComponent dataComponent = dataComponents.getComponentById(cidx);
  const type_index_t componentType = dataComponent.componentType;
  ComponentType componentTypeInfo = componentTypes.getTypeInfo(componentType);
  const uint32_t componentSize = componentTypeInfo.size;
  const void *compData =
    archetypes.getComponentDataUnsafe(archetypeId, componentInArchetypeIndex, componentSize, entDesc.chunkId, entDesc.idInChunk);
  void *oldCompData =
    archetypes.getComponentDataUnsafe(archetypeId, oldComponentInArchetypeIndex, componentSize, entDesc.chunkId, entDesc.idInChunk);
  const bool podComparable = is_pod(componentTypeInfo.flags);
  if (podComparable)
  {
    if (memcmp(oldCompData, compData, componentSize) == 0)
      return;
    memcpy(oldCompData, compData, componentSize);
  }
  else
  {
    ComponentTypeManager *ctm = componentTypes.getTypeManager(componentType);
    if (!ctm || !ctm->replicateCompare(oldCompData, compData))
      return;
  }
  preprocessTrackedChange(eid, archetypeId, cidx, to_process);
}

void EntityManager::convertArchetypeScheduledChanges()
{
  for (auto i : trackQueryIndices)
  {
    queryScheduled.set(i, false);
    auto &archDesc = archetypeQueries[i];
    auto trackedI = archDesc.trackedBegin(), trackedE = archDesc.trackedEnd();
    do
    {
      archetypeTrackingQueue.insert(trackedI->combined);
    } while (++trackedI != trackedE);
  }
  trackQueryIndices.clear();
}

inline unsigned EntityManager::trackScheduledChanges()
{
  TIME_PROFILE_DEV(ecs_track_scheduled_changes);
  // debug("archetypeTrackingQueue.size() = %d", archetypeTrackingQueue.size());
  convertArchetypeScheduledChanges();

  const uint32_t aei = archetypeTrackingQueue.size();
  const uint32_t eei = eidTrackingQueue.size();
  const uint32_t tei = aei + eei;
  if (!tei)
    return 0;
  FRAMEMEM_REGION;
  TrackedChangesTemp toProcess = {eastl::max((uint32_t)8, (uint32_t)lastTrackedCount)}; // not less than two cache-lines, and use
                                                                                        // lastTrackedCount as guesstimate
  uint64_t scannedCidxMask = 0; // bloom over scanned pairs' cidx, gates the eid-loop archetypeTrackingQueue probe
  {
    TIME_PROFILE_DEV(ecs_track_archetypes);
    SmallTab<uint32_t, framemem_allocator> sortedPairs(aei);
    uint32_t pairCount = 0;
    for (auto scheduled : archetypeTrackingQueue)
    {
      sortedPairs[pairCount++] = scheduled;
      scannedCidxMask |= uint64_t(1) << ((scheduled >> (8 * sizeof(archetype_t))) & 63);
    }
    // archetype-major order: scans of several tracked components of one archetype walk the same chunks back-to-back
    stlsort::sort(sortedPairs.begin(), sortedPairs.end(), [](uint32_t a, uint32_t b) {
      constexpr uint32_t rot = 8 * sizeof(archetype_t);
      return ((a << rot) | (a >> rot)) < ((b << rot) | (b >> rot));
    });
    for (const uint32_t scheduled : sortedPairs)
    {
      G_STATIC_ASSERT(sizeof(archetype_t) + sizeof(component_index_t) <= sizeof(scheduled));
      const archetype_t archetype = (scheduled & eastl::numeric_limits<archetype_t>::max());
      const component_index_t cidx = (scheduled >> (8 * sizeof(archetype_t)));
      DAECS_EXT_ASSERT(archetypes.getArchetypeComponentId(archetype, cidx) != INVALID_ARCHETYPE_COMPONENT_ID);
      DAECS_EXT_ASSERT(dataComponents.isTracked(cidx));
      trackChangedArchetype(archetype, cidx, dataComponents.getTrackedPairUnsafe(cidx), toProcess);
    }
  }
  {
    TIME_PROFILE_DEV(ecs_track_eids);
    for (auto scheduled : eidTrackingQueue)
    {
      G_STATIC_ASSERT(sizeof(EntityId) + sizeof(component_index_t) <= sizeof(scheduled));
      trackChanged(get_eid_from_scheduled(scheduled), get_second_from_scheduled(scheduled), scannedCidxMask, toProcess);
    }
  }
  // cleared only after the eid loop: trackChanged skips eid entries whose pair was already scanned
  archetypeTrackingQueue.clear(archetypeTrackingQueue.size());
  if (max<size_t>(32, eidTrackingQueue.get_capacity(eidTrackingQueue.size()) * 2) < eidTrackingQueue.bucket_count()) // allow
                                                                                                                     // overallocation
                                                                                                                     // of size of 2,
                                                                                                                     // do not reduce
                                                                                                                     // to less than
                                                                                                                     // 128 bytes size
                                                                                                                     // (two
                                                                                                                     // cachelines)
    eidTrackingQueue.clear(eidTrackingQueue.size());
  else
    eidTrackingQueue.clear();
  if (!toProcess.empty())
  {
    TIME_PROFILE_DEV(ecs_process_change_events);
    RaiiCounter nested(nestedQuery); // do it once per all tracking
    onChangeEvents(toProcess);
    lastTrackedCount = lerp((float)lastTrackedCount, (float)toProcess.size(), lastTrackedCount > toProcess.size() ? 0.2f : 0.8f);
  }
#if DAGOR_DBGLEVEL > 0
  DA_PROFILE_TAG(tracked_changes_amount, "archetypes %d, eids %d, to_process %d", aei, eei, toProcess.size());
#endif
  return tei;
}

void EntityManager::performTrackChanges(bool flush_all)
{
  G_ASSERT_RETURN(!isConstrainedMTMode(), );
  const uint32_t iterations = 32; // we limit maximum amount of iterations, to avoid inifinite loop (halting problem)
  for (uint32_t i = flush_all ? iterations : 1; i; --i)
    if (!trackScheduledChanges())
      break;
}

void EntityManager::sheduleArchetypeTracking(const ArchetypesQuery &archDesc)
{
  // if constrained mode, lock mutex/use lockless
  ScopedMTMutexT<OSSpinlock> lock(isConstrainedMTMode(), ownerThreadId, archetypeTrackingMutex);
  const size_t index = &archDesc - archetypeQueries.begin();
  if (queryScheduled.test(index, false))
    return;
  queryScheduled.set(index, true);
  trackQueryIndices.push_back(index);
}

}; // namespace ecs
