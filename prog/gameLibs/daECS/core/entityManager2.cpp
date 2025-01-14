// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daECS/core/entityManager.h>
#include <memory/dag_framemem.h>
#include <EASTL/fixed_set.h>
#include <EASTL/vector_set.h>
#include <daECS/core/coreEvents.h>
#include <daECS/core/baseIo.h>
#include <util/dag_fixedBitArray.h>
#include <util/dag_stlqsort.h>
#include <perfMon/dag_cpuFreq.h>
#include "ecsQueryInternal.h"
#include "ecsQueryManager.h"
#include "entityManagerEvent.h"
#include "daECS/core/componentTypes.h" // TMatrix
#include "entityCreationProfiler.h"
#include "ecsInternal.h"


#define ASYNC_RESOURCES                         "render"
#define REQUEST_RESOURCES_CAN_RECREATE_ENTITIES 1

static constexpr int MINIMUM_FREE_INDICES = 256;

#if DAECS_EXTENSIVE_CHECKS
#define VALIDATE_DESTROYING_GET(eid_)                           \
  struct ScopedDestrValidate                                    \
  {                                                             \
    EntityId &eid;                                              \
    ScopedDestrValidate(EntityId &e, EntityId to) : eid(e = to) \
    {}                                                          \
    ~ScopedDestrValidate()                                      \
    {                                                           \
      eid = INVALID_ENTITY_ID;                                  \
    }                                                           \
  } scopedDestr(destroyingEntity, eid_)

#else
#define VALIDATE_DESTROYING_GET(eid_)
#endif

ECS_REGISTER_TYPE_BASE(ecs::Tag, ECS_TAG_NAME, nullptr, nullptr, nullptr, 0); // same as ECS_REGISTER_TYPE (ecs::Tag, nullptr), just to
                                                                              // ensure tag name

InitOnDemandValidateThread<ecs::EntityManager, false> g_entity_mgr;

namespace ecs
{

EntityManager &get_entity_mgr() { return *g_entity_mgr; }

bool EntityComponentRef::operator==(const EntityComponentRef &a) const
{
  const component_index_t typeId = getTypeId();
  if (typeId != a.getTypeId())
    return false;
  ComponentType typeInfo = g_entity_mgr->getComponentTypes().getTypeInfo(typeId);
  if (is_pod(typeInfo.flags))
    return memcmp(getRawData(), a.getRawData(), typeInfo.size) == 0;
  ComponentTypeManager *ctm = g_entity_mgr->getComponentTypes().getTypeManager(typeId);
  return ctm ? ctm->is_equal(getRawData(), a.getRawData()) : true;
}

typedef size_t createdWordSize;
static constexpr size_t createdWordSizeBits = sizeof(createdWordSize) * CHAR_BIT;

__forceinline void createdSet(createdWordSize *__restrict created, size_t bit)
{
  created[bit / createdWordSizeBits] |= createdWordSize(1) << (bit % createdWordSizeBits);
}
__forceinline bool createdGetExists(const createdWordSize *__restrict created, size_t bit)
{
  return (created[bit / createdWordSizeBits] & (createdWordSize(1) << (bit % createdWordSizeBits))) != 0;
}

__forceinline bool createdSetIfNotSet(createdWordSize *__restrict created, size_t bit) // returns true if set
{
  const createdWordSize bitMask = createdWordSize(1) << (bit % createdWordSizeBits);
  createdWordSize &word = created[bit / createdWordSizeBits];
  if (word & bitMask)
    return false;
  word |= bitMask;
  return true;
}

__forceinline bool createdGet(const createdWordSize *__restrict created, size_t bit)
{
  return created && createdGetExists(created, bit);
}

extern size_t pull_components_type;
size_t pulled_data = pull_components_type + 1; // otherwise we don't pull base types

// out of line, rare case
void EntityManager::recreateOnSameArchetype(EntityId eid, template_t templId, ComponentsInitializer &&initializer,
  const create_entity_async_cb_t &creation_cb)
{
  // we still move
  // not often, so better move it to separate function
  const unsigned idx = eid.index();
  entDescs[idx].template_id = templId;
  if (initializer.empty())
    return;
  const uint32_t archetype = entDescs[idx].archetype;
  DAECS_EXT_ASSERT(archetype != INVALID_ARCHETYPE);
  const chunk_type_t chunkId = entDescs[idx].chunkId;
  const uint32_t idInChunk = entDescs[idx].idInChunk;
  auto *chunk = &archetypes.getArchetype(archetype).manager.getChunk(chunkId);
  uint8_t *__restrict chunkData = chunk->getData();
  const uint16_t *__restrict newOffsets = archetypes.componentDataOffsets(archetype);
  const uint32_t chunkCapacityBits = chunk->getCapacityBits();
  // for (auto initIt = initializer.begin(), initEnd = initializer.end(); initIt != initEnd; ++initIt)
  for (InitializerNode &init : initializer)
  {
    const component_index_t initializerIndex = init.cIndex;
    DAECS_EXT_ASSERT(initializerIndex == dataComponents.findComponentId(init.name));
    const auto i = archetypes.getArchetypeComponentIdUnsafe(archetype, initializerIndex);
    if (i == INVALID_ARCHETYPE_COMPONENT_ID)
      continue;
    const uint32_t componentSize = init.second.getSize();
    void *__restrict oldData = chunkData + componentSize * idInChunk + (newOffsets[i] << chunkCapacityBits);
    type_index_t typeId = init.second.getTypeId();
    DAECS_EXT_ASSERT(oldData == archetypes.getComponentDataUnsafeNoCheck(archetype, i, componentSize, chunkId, idInChunk));
    if (componentTypes.getTypeInfo(typeId).flags & COMPONENT_TYPE_NON_TRIVIAL_CREATE)
    {
      auto typeManager = componentTypes.getTypeManager(typeId);
      G_ASSERT_CONTINUE(typeManager);
      typeManager->replicateCompare(oldData, init.second.getRawData());
    }
    else
    {
      SPECIALIZE_MEMCPY(componentSize, oldData, init.second.getRawData());
      init.second.resetBoxedMem(); // was moved
    }
    scheduleTrackChangedCheck(eid, archetype, initializerIndex);
  }

  if (creation_cb)
    creation_cb(eid);
}

// we can make template <bool recreate>, to skip some branches.
void EntityManager::createEntityInternal(EntityId eid, template_t templId, ComponentsInitializer &&initializer, ComponentsMap &&map,
  create_entity_async_cb_t &&creation_cb)
{
  TIME_PROFILE_DEV(createEntityInternal)
  G_FAST_ASSERT(templId != INVALID_TEMPLATE_INDEX); // couldn't happen
  EntityCreationProfiler entCreatProf(templId, *this);
  DAECS_EXT_ASSERTF(doesEntityExist(eid), "Attempt to use destroyed/not allocated eid handle %d %d", (ecs::entity_id_t)eid,
    entDescs.size());
  const InstantiatedTemplate *doNotUseTempl = &templates.getTemplate(templId); // never use this after copying data from it! it can
                                                                               // change
  const uint32_t newArchetype = doNotUseTempl->archetype;
  const uint8_t *__restrict initialData = doNotUseTempl->initialData.get();
  doNotUseTempl = nullptr; // dead store elimination will get rid of it anyway, though it is useless assignment
  const unsigned idx = eid.index();
  DAECS_EXT_ASSERT(idx != 0);
  const uint32_t oldArchetype = entDescs[idx].archetype;
  const chunk_type_t oldChunkId = entDescs[idx].chunkId;
  const uint32_t oldIdInChunk = entDescs[idx].idInChunk;
  const bool isRecreating = oldArchetype != INVALID_ARCHETYPE;

  if (DAGOR_UNLIKELY(oldArchetype == newArchetype)) // effectively doing nothing, all data is already created
  {
    recreateOnSameArchetype(eid, templId, eastl::move(initializer), eastl::move(creation_cb));
    return;
  }

  if (oldArchetype != INVALID_ARCHETYPE)
  {
    PROFILE_CREATE(disappear_events);
    notifyESEventHandlersDisappear(eid, oldArchetype, newArchetype);
    G_ASSERTF(oldArchetype == entDescs[eid.index()].archetype,
      "we don't allow sync recreate entity from within ES on EventComponentsDisappear ES for eid %d", (ecs::entity_id_t)eid);
  }

  DAECS_EXT_ASSERT(eid.generation() == entDescs[idx].generation); // is alive
  if (isRecreating && isQueryingArchetype(oldArchetype))
  {
    // also, it is only error if we have queried 'creating archetype'.
    // as for now, keep it logwarn
    logerr("We don't support sync re-creating of entities within queries. Create <%s> was <%s>", getTemplateName(templId),
      getEntityTemplateName(eid));
  }
  // auto it = find_loading_entity(eid);
  // G_ASSERTF(it==loadingEntities.end() || it->eid != eid,
  //          "you can't recreate entity until it is loaded");
  DAECS_EXT_ASSERTF(creatingEntityTop.eid != eid, "you can't recreate entity until it is created");

  chunk_type_t chunkId;
  uint32_t idInChunk;
  auto *__restrict nArch = &archetypes.getArchetype(newArchetype);
  if (isRecreating)
  {
    nArch->manager.allocateEmpty(chunkId, idInChunk, nArch->entitySize);
    if (DAGOR_UNLIKELY(nArch->manager.lock() == 0))
    {
      logerr("Attempt to recreate Entity on same archetype too often <%s>", getTemplateName(templId));
      nArch->manager.unlock();
      return;
    }
  }
  else
    nArch->manager.allocate(chunkId, idInChunk, nArch->entitySize, initialData, archetypes.componentDataSizes(newArchetype),
      archetypes.initialComponentDataOffset(newArchetype));

  auto *chunk = &nArch->manager.getChunk(chunkId);
  uint8_t *__restrict chunkData = chunk->getData();
  const uint32_t chunkCapacityBits = chunk->getCapacityBits();
  chunk = NULL;
  *(EntityId *__restrict)(chunkData + idInChunk * sizeof(EntityId)) = eid;
  const uint16_t componentsCount = nArch->getComponentsCount();


  const uint32_t createdWordsCount = (componentsCount + createdWordSizeBits - 1) & ~(createdWordSizeBits - 1);
  uint32_t allocatedSize = createdWordsCount * sizeof(createdWordSize);
  const bool initializerEmpty = initializer.empty();
  createdWordSize *createdBits =
    !isRecreating && initializerEmpty ? nullptr : (createdWordSize *)creationAllocator.allocate(allocatedSize);
  if (createdBits)
    memset(createdBits, 0, createdWordsCount * sizeof(createdWordSize));

  const uint16_t *__restrict newOffsets = archetypes.componentDataOffsets(newArchetype);
  // Iterate in reverse order, so later values overwrite earlier ones
  for (auto initIt = initializer.end() - 1, initE = initializer.begin() - 1; initIt != initE; --initIt)
  {
    InitializerNode &init = *initIt;
    const component_index_t initializerIndex = init.cIndex;
    DAECS_EXT_ASSERT(initializerIndex == INVALID_COMPONENT_INDEX || initializerIndex == dataComponents.findComponentId(initIt->name));
    const auto i = archetypes.getArchetypeComponentIdUnsafe(newArchetype, initializerIndex);
    if (i == INVALID_ARCHETYPE_COMPONENT_ID || !createdSetIfNotSet(createdBits, i)) //-V1004
      continue;
    const uint32_t componentSize = init.second.getSize();
    // optimize me
    void *__restrict cData = chunkData + componentSize * idInChunk + (newOffsets[i] << chunkCapacityBits);
    DAECS_EXT_ASSERT(cData == archetypes.getComponentDataUnsafeNoCheck(newArchetype, i, componentSize, chunkId, idInChunk));
    SPECIALIZE_MEMCPY(componentSize, cData, init.second.getRawData());
    init.second.resetBoxedMem(); // was moved
  }

  if (isRecreating) // migrate from old template
  {
    PROFILE_CREATE(destructors);
    auto *__restrict oArch = &archetypes.getArchetype(oldArchetype);
    auto &oldChunk = oArch->manager.getChunk(oldChunkId);
    uint8_t *__restrict oldChunkData = oldChunk.getData();
    const uint32_t oldChunkCapacityBits = oldChunk.getCapacityBits();
    auto *__restrict oArchInfo = &archetypes.getArchetypeInfoUnsafe(oldArchetype);

    const uint16_t oldComponentsCount = oArch->getComponentsCount();
    const uint8_t *__restrict compStreamBegin = chunkData;
    const uint8_t *__restrict oldCompStreamBegin = oldChunkData;
    const uint16_t *__restrict initialOffsets = archetypes.initialComponentDataOffset(newArchetype);
    const uint16_t *__restrict componentSizesStart = archetypes.componentDataSizes(newArchetype);
    const uint16_t *__restrict componentSizes = componentSizesStart + componentsCount - 1;
    const uint16_t *__restrict oldComponentSizes = archetypes.componentDataSizes(oldArchetype) + oldComponentsCount - 1;
    uint8_t *__restrict compStream = chunkData + ((nArch->entitySize - *componentSizes) << chunkCapacityBits);
    uint8_t *__restrict oldCompStream = oldChunkData + ((oArch->entitySize - *oldComponentSizes) << oldChunkCapacityBits);
    const component_index_t *__restrict componentIndices = archetypes.componentIndices(newArchetype) + componentsCount - 1;
    const component_index_t *__restrict componentIndicesOld = archetypes.componentIndices(oldArchetype) + oldComponentsCount - 1;
    const bool hadCreatableOrTracked = archetypes.getArchetypeCombinedTypeFlags(oldArchetype) & COMPONENT_TYPE_NON_TRIVIAL_CREATE;

    if (initializerEmpty)
    {
      for (; compStream > compStreamBegin;)
      {
        DAECS_EXT_ASSERTF(compStream >= compStreamBegin && oldCompStream >= oldCompStreamBegin, "compStream = %d oldCompStream = %d",
          compStream - compStreamBegin, oldCompStream - oldCompStreamBegin);
        if (*componentIndices == *componentIndicesOld)
        {
          DAECS_EXT_ASSERT(*componentIndices); // how we reached eid?
          const uint16_t componentSize = *componentSizes;
          SPECIALIZE_MEMCPY_IN_SOA2(componentSize, compStream, oldCompStream, idInChunk, oldIdInChunk);
          compStream -= componentSizes[-1] << chunkCapacityBits;
          oldCompStream -= oldComponentSizes[-1] << oldChunkCapacityBits;
          createdSet(createdBits, componentSizes - componentSizesStart); //-V595
          --componentIndices;
          --componentIndicesOld;
          --componentSizes;
          --oldComponentSizes;
          continue;
        }
        for (auto initialOfsPtr = initialOffsets + (componentSizes - componentSizesStart);
             compStream > compStreamBegin && *componentIndices > *componentIndicesOld;
             --componentIndices, --componentSizes, --initialOfsPtr) // these components are missing in old archetype!
        {
          const uint16_t componentSize = *componentSizes;
          const uint8_t *__restrict from = initialData + *initialOfsPtr;
          SPECIALIZE_MEMCPY_TO_SOA(componentSize, compStream, idInChunk, from);
          compStream -= componentSizes[-1] << chunkCapacityBits;
        }
        for (; oldCompStream > oldCompStreamBegin && *componentIndicesOld > *componentIndices;
             --componentIndicesOld, --oldComponentSizes) // these components are missing in new archetype!
        {
          // call destructor here
          if (hadCreatableOrTracked) // better have some faster way, like one bit allocated in archetype.
          {
            const DataComponent dc = dataComponents.getComponentById(*componentIndicesOld);
            const type_index_t tIndex = dc.componentType;
            if (dc.flags & DataComponent::TYPE_HAS_CONSTRUCTOR)
            {
              auto typeManager = componentTypes.getTypeManager(tIndex);
              DAECS_EXT_ASSERT(typeManager);
              auto oldData = oldCompStream + oldIdInChunk * *oldComponentSizes;
              typeManager->destroy(oldData);
              oArchInfo = &archetypes.getArchetypeInfoUnsafe(oldArchetype); // could changed in destructor
            }
          }
          oldCompStream -= (oldComponentSizes[-1]) << oldChunkCapacityBits;
        }
      }
    }
    else
    {
      int ci = componentsCount - 1;
      for (; compStream > compStreamBegin;)
      {
        DAECS_EXT_ASSERTF(compStream >= compStreamBegin && oldCompStream >= oldCompStreamBegin, "compStream = %d oldCompStream = %d",
          compStream - compStreamBegin, oldCompStream - oldCompStreamBegin);
        if (*componentIndices == *componentIndicesOld)
        {
          DAECS_EXT_ASSERT(*componentIndices); // how we reached eid?
          const uint16_t componentSize = *componentSizes;
          if (!createdGetExists(createdBits, ci)) //-V595
          {
            SPECIALIZE_MEMCPY_IN_SOA2(componentSize, compStream, oldCompStream, idInChunk, oldIdInChunk);
          }
          else if (hadCreatableOrTracked)
          {
            const DataComponent dc = dataComponents.getComponentById(*componentIndicesOld);
            type_index_t tIndex = dc.componentType;
            if (dc.flags & DataComponent::TYPE_HAS_CONSTRUCTOR)
            {
              auto typeManager = componentTypes.getTypeManager(tIndex);
              DAECS_EXT_ASSERT(typeManager);
              auto oldData = oldCompStream + oldIdInChunk * componentSize;
              typeManager->destroy(oldData);
              oArchInfo = &archetypes.getArchetypeInfoUnsafe(oldArchetype); // could changed in destructor
            }
          }
          createdSet(createdBits, ci); //-V595
          compStream -= componentSizes[-1] << chunkCapacityBits;
          oldCompStream -= oldComponentSizes[-1] << oldChunkCapacityBits;
          --componentIndices;
          --componentIndicesOld;
          --componentSizes;
          --oldComponentSizes;
          --ci;
          continue;
        }
        for (auto initialOfsPtr = initialOffsets + ci; compStream > compStreamBegin && *componentIndices > *componentIndicesOld;
             --componentIndices, --componentSizes, --initialOfsPtr, --ci) // these components are missing in old archetype!
        {
          if (!createdGetExists(createdBits, ci))
          {
            const uint16_t componentSize = *componentSizes;
            const uint8_t *__restrict from = initialData + *initialOfsPtr;
            SPECIALIZE_MEMCPY_TO_SOA(componentSize, compStream, idInChunk, from);
          }
          compStream -= componentSizes[-1] << chunkCapacityBits;
        }
        for (; oldCompStream > oldCompStreamBegin && *componentIndicesOld > *componentIndices;
             --componentIndicesOld, --oldComponentSizes) // these components are missing in new archetype!
        {
          // call destructor here, if we had one
          if (hadCreatableOrTracked) // better have some faster way, like one bit allocated in archetype.
          {
            const DataComponent dc = dataComponents.getComponentById(*componentIndicesOld);
            const type_index_t tIndex = dc.componentType;
            if (dc.flags & DataComponent::TYPE_HAS_CONSTRUCTOR)
            {
              auto typeManager = componentTypes.getTypeManager(tIndex);
              DAECS_EXT_ASSERT(typeManager);
              auto oldData = oldCompStream + oldIdInChunk * *oldComponentSizes;
              typeManager->destroy(oldData);
              oArchInfo = &archetypes.getArchetypeInfoUnsafe(oldArchetype); // could changed in destructor
            }
          }
          oldCompStream -= (oldComponentSizes[-1]) << oldChunkCapacityBits;
        }
      }
    }
    if (hadCreatableOrTracked) // call rest of destructors
    {
      for (; oldCompStream > oldCompStreamBegin; --componentIndicesOld, --oldComponentSizes) // these components are missing in new
                                                                                             // archetype!
      {
        // call destructor here, if we had one
        const DataComponent dc = dataComponents.getComponentById(*componentIndicesOld);
        const type_index_t tIndex = dc.componentType;
        if (dc.flags & DataComponent::TYPE_HAS_CONSTRUCTOR) // better have some faster way, like one bit allocated in archetype.
        {
          auto typeManager = componentTypes.getTypeManager(tIndex);
          DAECS_EXT_ASSERT(typeManager);
          auto oldData = oldCompStream + oldIdInChunk * *oldComponentSizes;
          typeManager->destroy(oldData);
          oArchInfo = &archetypes.getArchetypeInfoUnsafe(oldArchetype); // could changed in destructor
        }
        oldCompStream -= (oldComponentSizes[-1]) << oldChunkCapacityBits;
      }
    }
    VALIDATE_DESTROYING_GET(eid);
    removeDataFromArchetype(oldArchetype, oldChunkId, oldIdInChunk);
    // removeFromArchetype(oldArchetype, oldChunkId, oldIdInChunk, [&, oldArchetype](int, component_index_t cidx)->bool
    //   {return archetypes.getArchetypeComponentIdUnsafe(newArchetype, cidx) != INVALID_ARCHETYPE_COMPONENT_ID;});
    // todo: optimization: if total entities count in oldArchetype is 0 - disable all potentially affected es and queries (which now
    // has nothing to do)
    archetypes.entityAllocated(newArchetype, chunkId); // fixme: make it RAII way
  }

#if DAECS_EXTENSIVE_CHECKS
  CreatingEntity oldCreatingTop = creatingEntityTop;
  creatingEntityTop = CreatingEntity{eid, 0};
#endif

  {
    entDescs[idx].template_id = templId;
    entDescs[idx].archetype = newArchetype;
    entDescs[idx].chunkId = chunkId;
    entDescs[idx].idInChunk = idInChunk;
  }

  // epilogue
  const auto hasCreatableOrTracked = archetypes.getArchetypeCombinedTypeFlags(newArchetype);
  // pods can not cause any creation of anything
  if ((hasCreatableOrTracked & Archetypes::HAS_TRACKED_COMPONENT) && createdBits) // if it is just from template, nothing to check
  {
    TIME_PROFILE_DEV(copy_tracked)
    const component_index_t *__restrict podCidx = archetypes.getTrackedPodCidxUnsafe(newArchetype);
    // since there can't be new archetype creation within memcpy
    for (auto &trackedPod : archetypes.getTrackedPodPairs(newArchetype)) // tracked pod
    {
      const bool created = createdGet(createdBits, trackedPod.shadowArchComponentId);
      void *__restrict dst;
      const void *__restrict src;
      if (isRecreating || !created)
      {
        dst = archetypes.getComponentDataUnsafeOfsNoCheck(newArchetype, trackedPod.dataOffset, trackedPod.size, chunkId, idInChunk);
        src =
          archetypes.getComponentDataUnsafeOfsNoCheck(newArchetype, trackedPod.trackedFromOfs, trackedPod.size, chunkId, idInChunk);
        if (!created)
        {
          SPECIALIZE_MEMCPY(trackedPod.size, dst, src); //-V575
        }
        else
        {
          // check if changed
          if (specialized_mem_nequal(dst, src, trackedPod.size)) //-V575
            scheduleTrackChanged(eid, *podCidx);
          // we can, of course, just scheduleTrackChanged, but currently data is in cache already, so it is faster
        }
      }
      // createdSet(createdBits, trackedPod.archComponentId);//we don't call it, as we assume it won't be needed anyway
      ++podCidx;
    }
  }

  if (hasCreatableOrTracked & COMPONENT_TYPE_NON_TRIVIAL_CREATE)
  {
    const size_t *templHasData;
    const uint8_t *templInitialData = initialData;
    {
      // hide reference to template
      auto &doNotUseTempl = templates.getTemplate(entDescs[eid.index()].template_id);
      templHasData = doNotUseTempl.hasData.get();
    }
    // first create creatables
    {
      PROFILE_CREATE(constructors);
      for (auto &component : archetypes.getCreatables(newArchetype))
      {
        if (createdGet(createdBits, component.archComponentId))
          continue;
          // we don't call it, as we assume it won't be needed anyway
          // createdSet(createdBits, component.archComponentId);
#if DAECS_EXTENSIVE_CHECKS
        ComponentType ctype = componentTypes.getTypeInfo(dataComponents.getComponentById(component.originalCidx).componentType);
#endif
        DAECS_EXT_ASSERT(component.size == archetypes.getComponentSizeUnsafe(newArchetype, component.archComponentId));
        DAECS_EXT_ASSERT(component.dataOffset == archetypes.getComponentOfsUnsafe(newArchetype, component.archComponentId));
        DAECS_EXT_ASSERT(ctype.flags & COMPONENT_TYPE_NON_TRIVIAL_CREATE);
        DAECS_EXT_ASSERT(component.trackedFromOfs == archetypes.getComponentInitialOfs(newArchetype, component.archComponentId));

#if DAECS_EXTENSIVE_CHECKS
        creatingEntityTop.createdCindex =
          component.originalCidx - !(ctype.flags & COMPONENT_TYPE_CREATE_ON_TEMPL_INSTANTIATE); // allow shared comps to read itself
                                                                                                // (for weak_ptr & such)
#endif
        entCreatProf.start(component.originalCidx);
        void *__restrict cData =
          archetypes.getComponentDataUnsafeOfsNoCheck(newArchetype, component.dataOffset, component.size, chunkId, idInChunk);
        ComponentTypeManager *typeManager = componentTypes.getTypeManager(component.typeIndex);
        if (!InstantiatedTemplate::isInited(templHasData, component.archComponentId) ||
            !typeManager->copy(cData, templInitialData + component.trackedFromOfs, component.originalCidx, eid))
        {
          typeManager->create(cData, *this, eid, map, component.originalCidx);
        }
        entCreatProf.end();
      }
    }

    // now create trackable creatables
    // if it is just from template, nothing to check
    if ((hasCreatableOrTracked & Archetypes::HAS_TRACKED_COMPONENT)) // todo: check if any non-constructed components left after
                                                                     // recreate
    {
      PROFILE_CREATE(tracked_constructors);
#if DAECS_EXTENSIVE_CHECKS
      creatingEntityTop.createdCindex = componentsCount - 1;
#endif
      for (auto &component : archetypes.getCreatableTrackeds(newArchetype))
      {
        DAECS_EXT_ASSERT(componentTypes.getTypeInfo(dataComponents.getComponentById(component.originalCidx).componentType).flags &
                         COMPONENT_TYPE_NON_TRIVIAL_CREATE);
        if (!createdGet(createdBits, component.archComponentId)) // it can be created if it is recreating entity
        {
          void *__restrict copyData =
            archetypes.getComponentDataUnsafeOfsNoCheck(newArchetype, component.dataOffset, component.size, chunkId, idInChunk);
          const void *__restrict fromData =
            archetypes.getComponentDataUnsafeOfsNoCheck(newArchetype, component.trackedFromOfs, component.size, chunkId, idInChunk);
          entCreatProf.start(component.originalCidx);
          ComponentTypeManager *typeManager = componentTypes.getTypeManager(component.typeIndex);
          if (DAGOR_UNLIKELY(!typeManager->copy(copyData, fromData, component.originalCidx, eid))) // trying to create copy constructor
          {
            typeManager->create(copyData, *this, eid, map, component.originalCidx);
            if (!typeManager->assign(copyData, fromData))
              typeManager->replicateCompare(copyData, fromData);
          }
          entCreatProf.end();
        }
        else
        {
#if DAECS_EXTENSIVE_CHECKS
          if (!isRecreating) // sanity check
            logerr("creating already initialized copy. Replication code is invalid for (copy = %d) component", component.originalCidx);
#endif
          scheduleTrackChanged(eid, component.originalCidx);
          // this is the only potentially dangerous change. It was typeManager->replicateCompare(copyData, fromData);
        }
      }
    }
  }
  // #endif
  if (createdBits)
    creationAllocator.deallocate((uint8_t *)createdBits, allocatedSize);

// todo: optimization: if total entities count in newArchetype is 1 - 'enable' all potentially affected es and queries (which had
// nothing to do before that)
#if DAECS_EXTENSIVE_CHECKS
  creatingEntityTop = oldCreatingTop; // we should not call notifyESEventHandlersAppear on different archetype!
#endif

  {
    PROFILE_CREATE(creation_events);
    notifyESEventHandlers(eid, newArchetype, isRecreating ? ENTITY_RECREATION_ES : ENTITY_CREATION_ES);
    if (isRecreating)
      notifyESEventHandlersAppear(eid, oldArchetype, newArchetype);
  }

  // EventEntity[Re]Created/EventComponentsAppear considered to be part of creation process,
  // while completion callback is not, therefore execute it after aftermentioned events
  if (creation_cb)
  {
    PROFILE_CREATE(creation_cb);
    creation_cb(eid);
  }

  auto it = findLoadingEntityEvents(eid);
  if (DAGOR_UNLIKELY(it != eventsForLoadingEntities.end()))
  {
    PROFILE_CREATE(loading_events);
    DAECS_EXT_ASSERT(it->eid == eid);
    decltype(LoadingEntityEvents::events) loadingEvents(eastl::move(it->events));
    eventsForLoadingEntities.erase(it);
    processEventsAnyway(~0u, loadingEvents);
  }
}

inline bool EntityManager::validateInitializer(template_t templId, ComponentsInitializer &comp_init) const
{
  if (templId >= templates.size())
  {
    logerr("invalid templateId %d", templId);
    return false;
  }
#if DAECS_EXTENSIVE_CHECKS
  uint32_t archetype = templates.getTemplate(templId).archetype;
  G_UNUSED(archetype);
#endif
  // validateInitializer can not be completely omitted
  for (auto initIt = comp_init.end() - 1, initEnd = comp_init.begin() - 1; initIt != initEnd; --initIt)
  {
    if (initIt->cIndex == INVALID_COMPONENT_INDEX)
    {
      component_index_t initializerIndex = dataComponents.findComponentId(initIt->name);
      if (DAGOR_UNLIKELY(initializerIndex == INVALID_COMPONENT_INDEX))
      {
#if DAECS_EXTENSIVE_CHECKS
        static eastl::vector_set<template_t> warnedTemplates;
        if (warnedTemplates.insert(templId).second)
        {
          const char *name = templateDB.getComponentName(initIt->name);
          logwarn("%s<0x%X> component is not known in template <%s>", name ? name : "", initIt->name, getTemplateName(templId));
        }
#endif
        continue;
      }
      initIt->cIndex = initializerIndex;
      DataComponent destComponent;
      // verify type and 'not a copy' (should never happen in production)!
      destComponent = dataComponents.getComponentById(initIt->cIndex);
      if (DAGOR_UNLIKELY(
            initIt->second.getUserType() != destComponent.componentTypeName || (destComponent.flags & DataComponent::IS_COPY)))
      {
        logerr("initializer <%s|0x%X> type mismatch %s initializer, <%s> in template <%s>",
          dataComponents.getComponentNameById(initIt->cIndex), initIt->name, componentTypes.findTypeName(initIt->second.getUserType()),
          componentTypes.getTypeNameById(destComponent.componentType), getTemplateName(templId));
        comp_init.erase_unsorted(initIt);
      }
    }
    else
    {
#if DAECS_EXTENSIVE_CHECKS
      DAECS_EXT_ASSERTF(initIt->cIndex == dataComponents.findComponentId(initIt->name),
        "componentIndex %d for <%s|0x%X> is incorrect, should be %d", initIt->cIndex,
        dataComponents.getComponentNameById(initIt->cIndex), initIt->name, dataComponents.findComponentId(initIt->name));
#endif
    }
#if DAECS_EXTENSIVE_CHECKS
    // todo: store initializerIndex in ComponentsInitializer parallel array.
    if (DAGOR_UNLIKELY(archetypes.getArchetypeComponentIdUnsafe(archetype, initIt->cIndex) == INVALID_ARCHETYPE_COMPONENT_ID))
    {
      component_t name = dataComponents.getComponentTpById(initIt->cIndex);
      auto flagsIt = getTemplateDB().info().componentFlags.find(name);
      auto tagIt = getTemplateDB().info().componentTags.find(name);
      if (((flagsIt == getTemplateDB().info().componentFlags.end()) || (flagsIt->second & DataComponent::DONT_REPLICATE) == 0) &&
          (tagIt != getTemplateDB().info().componentTags.end()) &&
          !filter_needed(tagIt->second.c_str(), getTemplateDB().info().filterTags))
        logerr("<%s|0x%X> component is filtered out in template <%s>, misbevaior is possible on replication",
          dataComponents.getComponentNameById(initIt->cIndex), name, getTemplateName(templId));
      continue;
    }
#endif
  }
  return true;
}

EntityManager::RequestResources EntityManager::requestResources(EntityId eid, archetype_t oldArchetype, template_t templId,
  const ComponentsInitializer &initializer, RequestResourcesType type)
{
  G_UNUSED(oldArchetype);
  const archetype_t archetype = templates.getTemplate(templId).archetype;
  if (!(archetypes.getArchetypeCombinedTypeFlags(archetype) & COMPONENT_TYPE_NEED_RESOURCES))
    return RequestResources::AlreadyLoaded;

  CurrentlyRequesting *oldRequestingTop = requestingTop;
  CurrentlyRequesting creating(eid, templId, oldArchetype, archetype, initializer);
  requestingTop = &creating;
  ResourceRequestCb rcb(*this);
  const bool recreating = oldArchetype != INVALID_ARCHETYPE;
  // todo: better skip initializer with resources as well
  // currently we don't it can potentially lead to loading of unneeded resources
  for (auto &component : archetypes.getComponentsWithResources(archetype))
  {
    component_index_t cIndex = component.cIndex;
    if (!recreating || archetypes.getArchetypeComponentIdUnsafe(oldArchetype, cIndex) == INVALID_ARCHETYPE_COMPONENT_ID)
      componentTypes.getTypeManager(component.typeIndex)->requestResources(dataComponents.getComponentNameById(cIndex), rcb);
  }
  requestingTop = oldRequestingTop;

  if (rcb.requestedResources.empty())
    return RequestResources::AlreadyLoaded;

  if (!filter_out_loaded_gameres(rcb.requestedResources, (type == RequestResourcesType::CHECK_ONLY) ? 0 : 1024))
    return RequestResources::AlreadyLoaded;
  else if (type == RequestResourcesType::CHECK_ONLY)
    return RequestResources::Error;
  else if (type == RequestResourcesType::ASYNC)
  {
    DAECS_EXT_ASSERT(!rcb.requestedResources.empty());
    if (!requestedResources.empty())
    {
      for (auto &&name : rcb.requestedResources)
        requestedResources.emplace(eastl::move(name)); // todo: this is actually allocation for each string
    }
    else
      eastl::swap(requestedResources, rcb.requestedResources);
    resourceEntities.push_back(eid);
    loadingEntities[eid]++;
    return RequestResources::Scheduled;
  }
  else // sync
    return load_gameres_list(eastl::move(rcb.requestedResources)) ? RequestResources::Loaded : RequestResources::Error;
}

EntityId EntityManager::createEntitySync(template_t templId, ComponentsInitializer &&initializer, ComponentsMap &&map)
{
  if (!validateInitializer(templId, initializer))
    return ecs::INVALID_ENTITY_ID;
  EntityId eid = allocateOneEid();
  const RequestResources result = requestResources(eid, INVALID_ARCHETYPE, templId, initializer, RequestResourcesType::SYNC);
  if (DAGOR_UNLIKELY(result != RequestResources::AlreadyLoaded))
  {
#if DAECS_EXTENSIVE_CHECKS
    if (result == RequestResources::Loaded)
    {
      // Assume that it's acceptable for dedicated to create entities with resources synchronously (e.g. in order to enforce creation
      // order)
      int ll = getTemplateDB().info().filterTags.count(ECS_HASH(ASYNC_RESOURCES).hash) ? LOGLEVEL_ERR : LOGLEVEL_WARN;
      logmessage(ll, "sync creation of entity %d eid (%s) with resources", eid, getTemplateName(templId));
    }
#endif
    if (result == RequestResources::Error)
    {
      logerr("sync creation of entity %d eid (%s) failed, as some resources are missing", eid, getTemplateName(templId));
      destroyEntityImmediate(eid);
      return INVALID_ENTITY_ID;
    }
  }
  createEntityInternal(eid, templId, eastl::move(initializer), eastl::move(map), create_entity_async_cb_t());
  return eid;
}

ecs::archetype_t EntityManager::getArchetypeByTemplateId(ecs::template_t templId) const
{
  return templates.getTemplate(templId).archetype;
}

const char *EntityManager::getTemplateName(ecs::template_t template_id) const
{
  if (template_id >= templates.templateDbId.size())
    return nullptr;
  auto id = templates.templateDbId[template_id];
  return id < getTemplateDB().templates.size() ? getTemplateDB().templates[id].getName() : "!invalid! !name!";
}

template_t EntityManager::instantiateTemplate(int id)
{
  TIME_PROFILE_DEV(instantiateTemplate);
  // Add all components from templates
  const Template &templ = getTemplateDB().templates[id];
  if (!templ.canInstantiate())
  {
    logerr("Entity of template <%s> can not be created as it has missing dependencies", templ.getName());
    templ.reportInvalidDependencies(getTemplateDB());
    return INVALID_TEMPLATE_INDEX;
  }
  DA_PROFILE_TAG(template, templ.getName());
  FRAMEMEM_REGION;
  eastl::vector_set<component_index_t, eastl::less<component_index_t>, framemem_allocator> templComponentsIndices; // we better sort
                                                                                                                   // after that, but
                                                                                                                   // in pairs
  eastl::vector_set<component_index_t, eastl::less<component_index_t>, framemem_allocator> templReplicatedIndices; // we better sort
                                                                                                                   // after that, but
                                                                                                                   // in pairs
  SmallTab<const ChildComponent *, framemem_allocator> templComponentsData;
  templComponentsIndices.reserve(templ.components.size()); // todo: we can precalc that to exact!//fixme:
  templComponentsData.reserve(templ.components.size());
  templComponentsIndices.insert(0);
  templComponentsData.push_back(nullptr);
  {
    TIME_PROFILE_DEV(iterate_template_parents);
    getTemplateDB().iterate_template_parents(templ, [&](const Template &p) {
      for (auto templateIt = p.getComponentsMap().begin(), e = p.getComponentsMap().end(); templateIt != e; ++templateIt)
      {
        const auto &it = *templateIt;
        // component_flags_t flag = isCopy ? DataComponent::IS_COPY : 0;
        const component_t compHash = it.first;
        component_index_t comp = dataComponents.findComponentId(compHash);
        type_index_t typeIdx;
        component_flags_t flag = 0;
        if (DAGOR_LIKELY(comp != INVALID_COMPONENT_INDEX))
        {
          auto compInfo = dataComponents.getComponentById(comp);
          typeIdx = compInfo.componentType;
          flag = compInfo.flags;
          if (it.second.getUserType() != 0 && compInfo.componentTypeName != it.second.getUserType())
          {
            logerr("Component %s|0x%X in template %s has incorrect type of 0x%X (registered with <%s|0x%X>)",
              getTemplateDB().getComponentName(compHash), compHash, p.getName(), it.second.getUserType(),
              componentTypes.getTypeNameById(typeIdx), typeIdx);
            continue;
          }
        }
        else
        {
          HashedConstString hName = HashedConstString{getTemplateDB().getComponentName(compHash), compHash};
          dag::Span<component_t> dependencies(nullptr, 0); // todo:actually we can create component dependencies on the fly
          const auto fi = getTemplateDB().info().componentFlags.find(hName.hash);
          flag |= (fi != getTemplateDB().info().componentFlags.end()) ? fi->second : 0;
          const component_type_t tp = it.second.getUserType() ? it.second.getUserType() : getTemplateDB().getComponentType(compHash);
          typeIdx = componentTypes.findType(tp);
          comp = createComponent(hName, typeIdx, dag::Span<component_t>(), nullptr, flag);

          if (comp == INVALID_COMPONENT_INDEX)
            continue;
        }
        if (templComponentsIndices.find(comp) != templComponentsIndices.end()) // already added
          continue;
        const component_index_t *ciAt = templComponentsIndices.emplace(comp).first;
        const size_t at = eastl::distance(templComponentsIndices.cbegin(), ciAt);
        templComponentsData.insert(templComponentsData.begin() + at, &templateIt->second);

        if (componentTypes.getTypeInfo(typeIdx).size != 0) // do not process tags
        {
          uint32_t tempFlags = templ.getRegExpInheritedFlags(compHash, templateDB);
          if (tempFlags)
          {
            // this is needed only on server and on client with validation - i.e. in dev mode
            // todo: skip on release client, to optimize mem usage
            if ((tempFlags & FLAG_REPLICATED) && should_replicate(comp, *this))
              templReplicatedIndices.insert(comp);
            component_index_t compCopy = createComponent(HashedConstString{NULL, compHash}, typeIdx, dag::Span<component_t>(), nullptr,
              flag | DataComponent::IS_COPY);
            const component_index_t *copyCidx =
              templComponentsIndices.emplace_hint(templComponentsIndices.cbegin() + at + 1, compCopy);
            const uint32_t copyAt = eastl::distance(templComponentsIndices.cbegin(), copyCidx);
            templComponentsData.insert(templComponentsData.cbegin() + copyAt, nullptr);
          }
        }
      }
    });
  }
  const uint32_t oldArchetypesCount = archetypes.size(), oldArchetypesCapacity = archetypes.capacity();
  G_UNUSED(oldArchetypesCapacity); // only used in dev mode
  const template_t ret = templates.createTemplate(archetypes, id, templComponentsIndices.cbegin(), templComponentsIndices.size(),
    templReplicatedIndices.cbegin(), templReplicatedIndices.size(), templComponentsData.cbegin(), dataComponents, componentTypes);

#if DAECS_EXTENSIVE_CHECKS
  debug("template %d <%s> has archetype %d", ret, templ.getName(), templates.getTemplate(ret).archetype);
#endif
  if (templates.hasReplication(ret)) // otherwise this components won't be replicated anyway
  {
    for (auto ri : templReplicatedIndices)
      canBeReplicated.set(ri, true);
  }
  // todo: free framemem here

  if (oldArchetypesCount != archetypes.size())
  {
    DAECS_EXT_ASSERT(oldArchetypesCount + 1 == archetypes.size());
    DAECS_EXT_ASSERTF(oldArchetypesCapacity == archetypes.capacity() || !isConstrainedMTMode(),
      "resize of archetypes array is not safe in constrainedMT mode");
    updateAllQueries();
  }
  TemplateDB::InstantiatedTemplateInfo ti{ret, templ.isSingletonDB(templateDB)};
  getMutableTemplateDB().instantiatedTemplates[id] = ti;
  return ret;
}

template_t EntityManager::templateByName(const char *templ_name, EntityId eid)
{
  int id = getMutableTemplateDB().buildTemplateIdByName(templ_name);
  if (DAGOR_UNLIKELY(id == -1))
  {
    logerr("template <%s> is not in database", templ_name);
    return INVALID_TEMPLATE_INDEX;
  }
  auto templId = getTemplateDB().instantiatedTemplates[id];
  if (DAGOR_UNLIKELY(templId.singleton))
  {
    const EntityId singletonEid = hasSingletoneEntity(templ_name);
    if (singletonEid != eid && singletonEid != INVALID_ENTITY_ID)
    {
      logerr("Singleton <%s> has been already created", templ_name);
      return INVALID_TEMPLATE_INDEX;
    }
  }
  if (DAGOR_LIKELY(templId.t != INVALID_TEMPLATE_INDEX))
    return templId.t;
  return instantiateTemplate(id);
}

EntityId EntityManager::createEntitySync(const char *templ_name, ComponentsInitializer &&initializer, ComponentsMap &&map)
{
  ScopedMTMutex lock(isConstrainedMTMode(), ownerThreadId, creationMutex);
  auto tId = templateByName(templ_name);
  if (bool(lock))
  {
#if DAECS_EXTENSIVE_CHECKS
    LOGERR_ONCE("sync (re)creation is not valid in constrainedMt mode. Create <%s>", templ_name);
#endif
    if (updateAllQueriesAnyMT())
      logerr("Race in sync creation. We have to invalidate queries array, due to sync creation");
  }
  return createEntitySync(tId, eastl::move(initializer), eastl::move(map));
}

EntityId EntityManager::createEntityAsync(const char *templ_name, ComponentsInitializer &&initializer, ComponentsMap &&map,
  create_entity_async_cb_t &&cb)
{
  ScopedMTMutex lock(isConstrainedMTMode(), ownerThreadId, creationMutex);
  EntityId eid = allocateOneEidDelayed(isConstrainedMTMode());
  emplaceCreate(eid, DelayedEntityCreation::Op::Create, templ_name, eastl::move(initializer), eastl::move(map), eastl::move(cb));
  return eid;
  // return createEntityAsync(templateByName(templ_name, EntityId()), eastl::move(initializer), eastl::move(map), eastl::move(cb));
}

void EntityManager::forceServerEidGeneration(EntityId from_eid)
{
  ScopedMTMutex lock(isConstrainedMTMode(), ownerThreadId, creationMutex);
  unsigned ei = from_eid.index();
  if (DAGOR_UNLIKELY(entDescs[ei].archetype != INVALID_ARCHETYPE)) // already allocated (this is considered erroneous code path)
  {
    EntityId peid(make_eid(ei, entDescs[ei].generation));
    logerr("%s attempt to allocate eid %d while slot already occupied with %d<%s>. Did client created replicating entity?",
      __FUNCTION__, (ecs::entity_id_t)from_eid, (ecs::entity_id_t)peid, this->getEntityTemplateName(peid));
    destroyEntityImmediate(peid);
  }
  entDescs[ei].generation = get_generation(from_eid);
}

bool EntityManager::collapseRecreate(EntityId eid, const char *template_name, ComponentsInitializer &cinit, ComponentsMap &cmap,
  create_entity_async_cb_t &&creation_cb)
{
  for (auto ci = delayedCreationQueue.end() - 1, ce = delayedCreationQueue.begin() - 1; ci != ce; --ci)
    for (auto cr = ci->end() - 1, cre = ci->begin() - 1; cr != cre; --cr)
      if (DAGOR_UNLIKELY(cr->eid == eid))
      {
        if (cr->currentlyCreating)
          return true; // don't try to optimize currently creating entity (e.g. if we get here from disappear events)
        else if (!cr->isToDestroy())
        {
#if DAECS_EXTENSIVE_CHECKS
          // template names in re-creates might be too long (hence use id)
          debug("%s %d templ %s -> %s", __FUNCTION__, eid, cr->templateName.c_str(), template_name);
#endif
          eastl::move(cinit.begin(), cinit.end(), eastl::back_inserter(static_cast<BaseComponentsInitializer &>(cr->compInit)));
          for (auto &ckv : static_cast<BaseComponentsMap &>(cmap))
            static_cast<BaseComponentsMap &>(cr->compMap).emplace(eastl::move(ckv.first), eastl::move(ckv.second));
          cr->templateName = template_name;
          cr->templ = INVALID_TEMPLATE_INDEX;
          if (!cr->cb)
            cr->cb = eastl::move(creation_cb);
          else if (creation_cb)
          {
            DAECS_EXT_FAST_ASSERT(cr->cb && creation_cb);
            auto cb2 = [cbA = eastl::move(cr->cb), cbB = eastl::move(creation_cb)](EntityId eid) {
              cbA(eid);
              cbB(eid);
            };
            cr->cb = eastl::move(cb2);
          }
        }
        else
          ; // ignore this re-create if entity is already scheduled to be destroyed
        return false;
      }
  return true;
}

EntityId EntityManager::reCreateEntityFromAsync(EntityId from_eid, const char *templ_name, ComponentsInitializer &&initializer,
  ComponentsMap &&map, create_entity_async_cb_t &&cb)
{
  if (!doesEntityExist(from_eid))
    return ecs::INVALID_ENTITY_ID;
  ScopedMTMutex lock(isConstrainedMTMode(), ownerThreadId, creationMutex);
  const uint32_t idx = from_eid.index();
  if (entDescs.isCurrentlyCreating(idx) && !collapseRecreate(from_eid, templ_name, initializer, map, eastl::move(cb)))
    return from_eid;
  emplaceCreate(from_eid, DelayedEntityCreation::Op::Create, templ_name, eastl::move(initializer), eastl::move(map), eastl::move(cb));
  return from_eid;
}

EntityId EntityManager::allocateOneEid()
{
  bool reserved = eidsReservationMode;
  auto &findices = reserved ? freeIndicesReserved : freeIndices;
  unsigned idx;
  if (findices.size() <= MINIMUM_FREE_INDICES)
  {
    if (!reserved)
    {
    alloc_non_reserved:
      idx = entDescs.push_back();
    }
    else if (DAGOR_LIKELY(nextResevedEidIndex <= MAX_RESERVED_EID_IDX_CONST))
      idx = nextResevedEidIndex++;
    else if (!freeIndicesReserved.empty())
      goto alloc_free_indices;
    else
    {
      LOGERR_ONCE("Exhausted reserved eids indexes range [1..%d]", MAX_RESERVED_EID_IDX_CONST);
      exhaustedReservedIndices = true;
      reserved = false;
      goto alloc_non_reserved;
    }
    DAECS_EXT_FAST_ASSERT(idx < (1 << ENTITY_INDEX_BITS));
    entDescs[idx].generation = entDescs.globalGen;
  }
  else
  {
  alloc_free_indices:
    idx = findices.front() & ENTITY_INDEX_MASK;
    G_FAST_ASSERT(idx < entDescs.size());
    findices.pop_front();
  }
  DAECS_EXT_ASSERTF(reserved ? (idx <= MAX_RESERVED_EID_IDX_CONST) : (idx > MAX_RESERVED_EID_IDX_CONST), "reserved %d, idx=%d",
    reserved, idx);
  G_FAST_ASSERT(entDescs[idx].archetype == INVALID_ARCHETYPE);
  return EntityId(make_eid(idx, entDescs[idx].generation));
}

EntityId EntityManager::allocateOneEidDelayed(bool delayed)
{
  bool reserved = eidsReservationMode;
  auto &findices = reserved ? freeIndicesReserved : freeIndices;
  unsigned idx;
  if (findices.size() <= MINIMUM_FREE_INDICES)
  {
    if (!reserved)
    {
    alloc_non_reserved:
      idx = delayed ? entDescs.push_back_delayed() : entDescs.push_back();
    }
    else if (DAGOR_LIKELY(nextResevedEidIndex <= MAX_RESERVED_EID_IDX_CONST))
      idx = nextResevedEidIndex++;
    else if (!freeIndicesReserved.empty())
      goto alloc_free_indices;
    else
    {
      LOGERR_ONCE("Exhausted reserved eids indexes range [1..%d]", MAX_RESERVED_EID_IDX_CONST);
      exhaustedReservedIndices = true;
      reserved = false;
      goto alloc_non_reserved;
    }
    DAECS_EXT_FAST_ASSERT(idx < (1 << ENTITY_INDEX_BITS));
    if (idx < entDescs.allocated_size())
      entDescs[idx].generation = entDescs.globalGen;
  }
  else
  {
  alloc_free_indices:
    idx = findices.front() & ENTITY_INDEX_MASK;
    G_FAST_ASSERT(idx < entDescs.allocated_size());
    findices.pop_front();
  }
  DAECS_EXT_ASSERTF(reserved ? (idx <= MAX_RESERVED_EID_IDX_CONST) : (idx > MAX_RESERVED_EID_IDX_CONST), "reserved %d, idx=%d",
    reserved, idx);
  return entDescs.makeEid(idx);
}

void EntityManager::allocateInvalid()
{
  const bool oldMode = eidsReservationMode;
  eidsReservationMode = true;
  EntityId invalidEid = allocateOneEid(); // invalid eid creation
  eidsReservationMode = oldMode;
  DAECS_EXT_ASSERT(invalidEid.index() == INVALID_ENTITY_ID.index());
  G_UNUSED(invalidEid);
  entDescs[0].generation = eastl::numeric_limits<decltype(entDescs[0].generation)>::max();
  DAECS_EXT_ASSERT(!doesEntityExist(INVALID_ENTITY_ID));
}

int EntityManager::getNumEntities() const
{
  return int(nextResevedEidIndex - freeIndicesReserved.size() - 1) + // -1 for INVALID_ENTITY_ID
         int(entDescs.size() - size_t(MAX_RESERVED_EID_IDX_CONST + 1) - freeIndices.size());
}

CompileTimeQueryDesc *CompileTimeQueryDesc::tail = nullptr;

void EntityManager::initCompileTimeQueries()
{
  for (CompileTimeQueryDesc *sd = CompileTimeQueryDesc::tail; sd; sd = sd->next)
    sd->query = createUnresolvedQuery(*sd);
}

void init_profiler_tokens();

EntityManager::EntityManager() : templateDB(*this)
{
  ownerThreadId = get_main_thread_id(); // use main thread as default to keep same logic
  init_profiler_tokens();
  delayedCreationQueue.reserve(16); // avoid memory reallocation. 16 is enough for 2mln of entities.
  initializeCreationQueue();
  {
    ArchetypesQuery::offset_type_t q = ArchetypesQuery::INVALID_OFFSET;
    G_UNUSED(q);
    G_ASSERTF(ArchetypesQuery::offset_type_t(q + 1) == 0, "this assumption is used in fillQuery");
  }
  entDescs.reserve(MAX_RESERVED_EID_IDX_CONST + 4096); // this allows up to 4096 entites beyond reserved
  entDescs.resize(MAX_RESERVED_EID_IDX_CONST + 1);
  allocateInvalid();
  componentTypes.initialize();
  dataComponents.initialize(componentTypes);
  uint32_t biggestType = 0;
  for (int i = 0; i < componentTypes.getTypeCount(); ++i)
    biggestType = max(biggestType, (uint32_t)componentTypes.getTypeInfo(i).size);
  zeroMem.reset(new uint8_t[(biggestType + 15) & ~15]);
  debug("biggest type size is %d, zeroMem %p", biggestType, zeroMem.get());
  initCompileTimeQueries();
  resetEsOrder();
}

EntityManager::EntityManager(const EntityManager &from) : ownerThreadId(from.ownerThreadId), templateDB(*this)
{
  init_profiler_tokens();
  delayedCreationQueue.reserve(16); // avoid memory reallocation. 16 is enough for 2mln of entities.
  initializeCreationQueue();
  {
    ArchetypesQuery::offset_type_t q = ArchetypesQuery::INVALID_OFFSET;
    G_UNUSED(q);
    G_ASSERTF(ArchetypesQuery::offset_type_t(q + 1) == 0, "this assumption is used in fillQuery");
  }
  entDescs.reserve(MAX_RESERVED_EID_IDX_CONST + 4096); // this allows up to 4096 entites beyond reserved
  entDescs.resize(MAX_RESERVED_EID_IDX_CONST + 1);
  allocateInvalid();
  componentTypes.initialize();
  dataComponents.initialize(componentTypes);
  uint32_t biggestType = 0;
  for (int i = 0; i < componentTypes.getTypeCount(); ++i)
    biggestType = max(biggestType, (uint32_t)componentTypes.getTypeInfo(i).size);
  zeroMem.reset(new uint8_t[(biggestType + 15) & ~15]);
  debug("biggest type size is %d, zeroMem %p", biggestType, zeroMem.get());
  copyFrom(from);
  dataComponents.componentToLT = from.dataComponents.componentToLT;
  dataComponents.componentToLT.iterate([](uint64_t, LTComponentList *component) { component->info.cidx = INVALID_COMPONENT_INDEX; });
}

void EntityManager::clear()
{
  broadcastEventImmediate(EventEntityManagerBeforeClear());

  // this is practical limitation if we would use native stack for event recursion, since we allocate around 1kb of stack for each
  // sendEvent.
  static const int max_destroy_attempts = 1024;
  int clearAttempts = max_destroy_attempts;
  decltype(EntityDesc::generation) minGen = eastl::numeric_limits<decltype(EntityDesc::generation)>::max(), maxGen = 0;
  bool destroyed = false;
  do
  {
    destroyed = false;
    clearCreationQueue();
    sendQueuedEvents(1024);
    // actually we need to destroy all components instead
    // so can just iterate over archetypes, then their chunks, then their entities, and call removeFromArchetype
    // todo!:
    loadingEntities.clear();
    for (int lasti = (int)entDescs.allocated_size() - 1; lasti > 0; lasti--) // zero index is INVALID_ENTITY_ID
    {
      EntityDesc entDesc = entDescs[lasti];
      if (entDesc.archetype != INVALID_ARCHETYPE)
      {
        destroyed = true;
        destroyEntityImmediate(EntityId(make_eid(lasti, entDesc.generation)));
      }
      minGen = eastl::min(minGen, entDesc.generation);
      maxGen = eastl::max(maxGen, entDesc.generation);
    }
  } while ((destroyed || deferredEventsCount) && --clearAttempts);

  if (clearAttempts == 0)
  {
    logerr("infinite recursion during destruction of ECS, spend %d attempts, %d deferred Events left", max_destroy_attempts,
      deferredEventsCount);
    destroyEvents(eventsStorage); // destroy all other events left
  }
  else
    nextResevedEidIndex = 0;
  clearCreationQueue();

  if (creationAllocator.calcMemAllocated() > 0)
    logerr("there is allocated memory on clear");
  creationAllocator.clear();
  for (auto &e : eventsForLoadingEntities)
    destroyEvents(e.events);
  eventsForLoadingEntities.clear();
  templates.clear(archetypes, dataComponents, componentTypes);
  templateDB.clear();
  canBeReplicated.clear();
  archetypes.clear(); // may be just shrink-to-fit them (remove empty chunks, measure initial size)
  // dataComponents.clear();//do we have to clear data components at all? we'd better sort them based on usage in archetypes...
  // componentTypes.clear();//do we have to clear componentTypes at all? we definetly need garbage collection (call destroy on created
  // fabriques)
  entDescs.clear(); // Intentionally before ctm's clear in order to invalidate all eid handles
  for (int i = 0, ei = componentTypes.getTypeCount(); i < ei; ++i)
    if (ComponentTypeManager *ctm = componentTypes.getTypeManager(i))
      ctm->clear();
  freeIndices.clear();
  freeIndicesReserved.clear();
  esOrder.clear();
  esSkip.clear();
  clear_and_shrink(esList);
  esForAllEntities.clear();
  for (auto &q : esListQueries)
    if (q)
      destroyQuery(q);
  esListQueries.clear();
  clearQueries(); // as long as we clear archetypes
  queryToEsMap.clear();
  esUpdates.clear();
  eidTrackingQueue.clear();
  archetypeTrackingQueue.clear();
  trackQueryIndices.clear();
  queryScheduled.clear();

  ++lastEsGen;
  resetEsOrder();

  if ((int)min(minGen, entDescs.globalGen) > 255 - (int)max(entDescs.globalGen, maxGen))
    entDescs.globalGen = 0;
  else
    entDescs.globalGen = maxGen;

  entDescs.resize(MAX_RESERVED_EID_IDX_CONST + 1);
  nextResevedEidIndex = 0;
  allocateInvalid();

  someLoadedEntitiesHasErrors = false;
  lastUpdatedCreationQueueGen = INITIAL_CREATION_QUEUE_GEN;
  lastTrackedCount = 0;
  broadcastEventImmediate(EventEntityManagerAfterClear());
}

EntityManager::~EntityManager()
{
  // this is mostly to become ::clear()
  clear();
  debug("EntityManager destroyed");
}

void EntityManager::copyFrom(const EntityManager &from)
{
  getEventsDbMutable() = from.getEventsDb();

  clearQueries();
  // clear this arrays, because clearQueries keeps them
  resolvedQueries.clear();
  archetypeQueries.clear();
  archetypeEidQueries.clear();
  // also clear this, because it is not cleared by clearQueries
  queryDescs.clear();
  queriesReferences.clear();
  queriesGenerations.clear();
  int queryIdx = 0;
  for (uint32_t qi = 0u; qi < from.getQueriesCount(); qi++)
  {
    auto qid = from.getQuery(qi);
    auto desc = from.getQueryDesc(qid);
    auto name = from.getQueryName(qid);
    if (name == nullptr)
      continue;
    ecs::NamedQueryDesc namedDesk(name, desc.componentsRW, desc.componentsRO, desc.componentsRQ, desc.componentsNO);
    createQuery(namedDesk);
    queriesGenerations[queryIdx] = from.queriesGenerations[queryIdx];
    queriesReferences[queryIdx] = from.queriesReferences[queryIdx];
    ++queryIdx;
  }
  esTags = from.esTags;
  resetEsOrder();
  esListQueries = from.esListQueries;
}

inline void EntityManager::destroyEntityImmediate(EntityId eid)
{
  const unsigned idx = eid.index();
  if (idx >= entDescs.allocated_size() || entDescs[idx].generation != eid.generation())
    return;
  // warning! we can't destroy entities while creating them, we have to finish first
  ecs::archetype_t arch = entDescs[idx].archetype;
  if (arch != INVALID_ARCHETYPE) // otherwise it wasn't yet created, so we don't want to let everyone now it was destroyed
    notifyESEventHandlers(eid, arch, ENTITY_DESTROY_ES);

  VALIDATE_DESTROYING_GET(eid);

  auto loadingIt = find_loading_entity(eid);
  if (loadingIt != loadingEntities.end())
  {
    // immediately destroy loading entity
    if (--loadingIt->second == 0)
      loadingEntities.erase(loadingIt);
    auto itEvents = findLoadingEntityEvents(eid);
    if (itEvents != eventsForLoadingEntities.end())
    {
      destroyEvents(itEvents->events);
      eventsForLoadingEntities.erase(itEvents);
    }
  }
  auto &entDesc = entDescs[idx];
  entDesc.generation++;
  DAECS_VALIDATE_ARCHETYPE(arch);
  removeFromArchetype(arch, entDesc.chunkId, entDesc.idInChunk, [](int, component_index_t) { return false; });
  auto findices = (eid.index() <= MAX_RESERVED_EID_IDX_CONST) ? ((eid.index() < nextResevedEidIndex) ? &freeIndicesReserved : nullptr)
                                                              : &freeIndices;
  if (findices)
  {
#if DAECS_EXTENSIVE_CHECKS
    for (auto it = findices->begin(), ite = findices->end(); it != ite; ++it)
      G_ASSERTF(((*it) & ENTITY_INDEX_MASK) != idx, "eid %d<%s> index %d is already in freeIndices (occupied with %d)",
        (ecs::entity_id_t)eid, this->getEntityTemplateName(eid), idx, *it);
#endif
    findices->push_back((entity_id_t)eid);
  }
  entDescs[idx].reset();
}


void EntityManager::flushGameResRequests()
{
  if (resourceEntities.empty())
    return;
  auto tmpResEntities = eastl::move(resourceEntities);
  auto tmpReqRes = eastl::move(requestedResources);

  place_gameres_request(eastl::move(tmpResEntities), eastl::move(tmpReqRes));
}

void EntityManager::onEntitiesLoaded(dag::ConstSpan<EntityId> ents, bool all_ok)
{
  if (!ents.size())
    return;
  if (!all_ok)
    someLoadedEntitiesHasErrors = true;                     // will cause slow path off error reporting
  lastUpdatedCreationQueueGen = INITIAL_CREATION_QUEUE_GEN; // some entities were loaded
  for (const EntityId *__restrict eit = ents.end() - 1, *__restrict end = ents.begin() - 1; eit != end; --eit) // we iterate in reverse
                                                                                                               // order to decrease
                                                                                                               // cost of erase in
                                                                                                               // loadingEntities
  {
    EntityId eid = *eit;
    if (!doesEntityExist(eid))
      continue;
    auto it = find_loading_entity(eid);
    if (it == loadingEntities.end())
    {
      // Could be if resource loading jobs cross ::clear call.
      // To consider: mark such jobs as invalid to avoid calling this?
      logwarn("%s eid <%d> not exist in loadingEntities", __FUNCTION__, (ecs::entity_id_t)eid);
      continue;
    }
    if (--it->second == 0)
      loadingEntities.erase(it);
  }
}

//! we allow at least 20 events per tick (and during the tick in non-deferred-only strategy)
static const uint32_t minEventsPerTick = 20;
//! we allow up to 50% more than average for each frame (small spikes). Should be >1.
static const float minAverageAllowance = 1.5f;
void EntityManager::performDeferredEvents(bool flush_all)
{
  TIME_PROFILE(ecs_deferred_events);
  average_tick_events = 0.8f * float(average_tick_events) + 0.2f * float(current_tick_events + deferredEventsCount);
  average_tick_events_uint = uint32_t(average_tick_events * minAverageAllowance) + minEventsPerTick;
  unsigned minimum_to_perform = max((int)average_tick_events_uint, (int)minEventsPerTick);
  minimum_to_perform = flush_all ? eastl::numeric_limits<decltype(minimum_to_perform)>::max() : minimum_to_perform;
  sendQueuedEvents(minimum_to_perform);
  current_tick_events = 0;
}

bool EntityManager::validateResources(EntityId eid, archetype_t old_archetype, template_t templ,
  const ComponentsInitializer &initializer)
{
  const RequestResources result = requestResources(eid, old_archetype, templ, initializer, RequestResourcesType::CHECK_ONLY);
  if (result == RequestResources::Error)
  {
    logerr("Creation of entity %d<%s> is not possible, as some resources are missing", eid, getTemplateName(templ));
    return false;
  }
  return true;
}

void EntityManager::DelayedEntityCreation::clear()
{
  compInit.clear();
  decltype(compMap)().swap(compMap);
  cb = nullptr;
  templateName.clear();
  templateName.shrink_to_fit();
}

bool EntityManager::createQueuedEntitiesOOL()
{
  if (createOrDestroyGen == INVALID_CREATION_QUEUE_GEN)
    ++createOrDestroyGen; // sentinel value of INVALID_CREATION_QUEUE_GEN
  const uint32_t currentTop = createOrDestroyGen;
  // we shouldn't try to create anything, if we re already creating, or if nothing has changed since last time we have been here
  // since we always write to top Chunk, current amount of chunks and writeTo position is full representation of queue state
  if (lastUpdatedCreationQueueGen == currentTop || lastUpdatedCreationQueueGen == INVALID_CREATION_QUEUE_GEN)
    return false;
  lastUpdatedCreationQueueGen = INVALID_CREATION_QUEUE_GEN;
  TIME_PROFILE(ecs_create_queued_entities);
  struct Destroy
  {
    EntityId eid;
    uint32_t sortBy;
  };
  eastl::fixed_vector<Destroy, 32, true, framemem_allocator> destroyNow; // use sorting queue, so we reduce amount of memory movements
  auto destroyQueued = [&destroyNow, this]() {
    if (destroyNow.empty())
      return;
    stlsort::sort(destroyNow.begin(), destroyNow.end(), [](auto &a, auto &b) { return a.sortBy > b.sortBy; });
    for (auto &i : destroyNow)
      destroyEntityImmediate(i.eid);
    destroyNow.clear();
  };

  entDescs.addDelayed(); // allocate delayed ids. Should be the only one called
  DAECS_EXT_FAST_ASSERT(!delayedCreationQueue.empty());
  int ci = 0, cEnd = delayedCreationQueue.size();
  do
  {
    DelayedEntityCreation *firstLoading = nullptr;
    auto chunkBegin = delayedCreationQueue[ci].begin(), chunkEnd = delayedCreationQueue[ci].end();
    for (auto it = chunkBegin; it != chunkEnd; ++it)
    {
      auto &ce = *it;
      ecs::EntityId eid = ce.eid;
      if (!eid) // Entity was already created/destroyed
        continue;
      const uint32_t idx = eid.index();
      // debug("eid %d idx %d, isCreating %d %d",
      //   eid, idx, entDescs.isCurrentlyCreating(eid.index()), entDescs.currentlyCreatingEntitiesCnt[idx]);
      const bool exist = entDescs.doesEntityExist(idx, eid.generation());
      if (ce.isToDestroy())
      {
        if (exist)
        {
          const auto &entDesc = entDescs[idx];
          destroyNow.emplace_back(Destroy{eid, uint32_t(entDesc.archetype) << 16 | entDesc.idInChunk}); // simple
                                                                                                        // destroyEntityImmediate(eid);
                                                                                                        // would work, but is
                                                                                                        // suboptimal on mass
                                                                                                        // destruction
        }
      }
      else
      {
        if (exist)
        {
          if (!destroyNow.empty())
          {
            destroyQueued();
            if (!doesEntityExist(eid)) // do check again since destroyQueued() might already destroyed this entity. todo: do optimized
                                       // check
              continue;
          }
          const auto archetype = entDescs[idx].archetype;
          // todo: better iterate over all first. That will decrease number of calls to validate queries, collapse recreate, and
          // increase instruction cache locality
          bool entityResourcesLoaded = !someLoadedEntitiesHasErrors;
          if (ce.templ == INVALID_TEMPLATE_INDEX)
          {
            ce.templ = templateByName(ce.templateName.c_str(), ce.eid);
            if (ce.templ == INVALID_TEMPLATE_INDEX || !validateInitializer(ce.templ, ce.compInit))
            {
              logerr("Entity %@ with template %s, could not be created, %s", ce.eid, ce.templateName.c_str(),
                ce.templ == INVALID_TEMPLATE_INDEX ? "as template is incorrect" : "as initializer is incorrect");
              ce.clear();
              ce.eid = INVALID_ENTITY_ID; // Mark as already created/destroyed
              // todo: call callback with error
              continue;
            }

            const RequestResources result = requestResources(ce.eid, archetype, ce.templ, ce.compInit, RequestResourcesType::ASYNC);
            if (result == RequestResources::Scheduled)
            {
              if (!firstLoading)
                firstLoading = &ce;
              continue;
            }
            else
            {
              DAECS_EXT_ASSERT(result == RequestResources::AlreadyLoaded);
              // validateResources call should be skipped if all resources are already loaded
              entityResourcesLoaded = true;
            }
          }
          else if (find_loading_entity(eid) != loadingEntities.end())
          {
            if (!firstLoading)
              firstLoading = &ce;
            continue;
          }
          if (entityResourcesLoaded || validateResources(eid, archetype, ce.templ, ce.compInit)) //-V1051
          {
#if REQUEST_RESOURCES_CAN_RECREATE_ENTITIES
            if (ce.templ == INVALID_TEMPLATE_INDEX) //-V547
            {
              // this mean that validateResources caused recreation of this entity!
              --it;
              continue;
            }
#endif
            ce.currentlyCreating = true;
            createEntityInternal(eid, ce.templ, eastl::move(ce.compInit), eastl::move(ce.compMap), eastl::move(ce.cb));
            if (firstLoading) // Otherwise it won't be destroyed until end of loading preceding entities
              ce.clear();
          }
        }
        DAECS_EXT_ASSERTF(entDescs.isCurrentlyCreating(eid.index()), "%d", eid);
        entDescs.decreaseCreating(eid.index());
      }
      ce.eid = INVALID_ENTITY_ID; // Mark as already created/destroyed
    }

    auto &chunk = delayedCreationQueue[ci]; // we have to re-read chunk data, as during create/destroy vector could be resized
    DAECS_EXT_ASSERT(chunkBegin == chunk.begin());
    auto clearTo = firstLoading ? firstLoading : chunkEnd;
    chunk.readFrom = clearTo - chunk.queue;
    if (chunk.empty())
      chunk.reset();
    for (auto i = chunkBegin; i != clearTo; ++i)
      i->~DelayedEntityCreation();
  } while (++ci < cEnd);

  destroyQueued();
  const bool wereAdded = currentTop != createOrDestroyGen;

  if (DAGOR_UNLIKELY(delayedCreationQueue.size() > 1))
  {
    delayedCreationQueue.erase(
      eastl::remove_if(delayedCreationQueue.begin() + 1, delayedCreationQueue.end(), [](auto &a) { return a.empty(); }),
      delayedCreationQueue.end());
    if (delayedCreationQueue.back().full())
      delayedCreationQueue.emplace_back(delayedCreationQueue.back().next_capacity());
  }

  someLoadedEntitiesHasErrors = false; // all errors were already checked
  lastUpdatedCreationQueueGen = currentTop;
  // If something was added during previous step - add it back
  return wereAdded;
}

#if DAGOR_DBGLEVEL == 0 && defined(_MSC_VER)
#pragma warning(push, 1)
#pragma warning(disable : 4701) // compiler is unable to realize that  movedFrom, movedTo, movedAt, movedCount can not be uninited if
                                // we reach that line
#endif
void EntityManager::defragmentArchetypes()
{
  G_STATIC_ASSERT(sizeof(Archetype) <= 32);
  G_ASSERT_RETURN(!isConstrainedMTMode(), );
  const uint32_t archetypesCount = archetypes.size();
  if (!archetypesCount)
    return;
  TIME_PROFILE(ecs_gc);
  uint64_t reft = profile_ref_ticks();
  chunk_type_t movedFrom, movedTo;
  uint32_t movedAt, movedCount;
#if DAGOR_DBGLEVEL != 0
  // compiler is unable to realize that  movedFrom, movedTo, movedAt, movedCount can not be uninited if we reach that line
  movedFrom = movedTo = 0;
  movedAt = movedCount = 0;
#endif
  DataComponentManager::DefragResult defrag = DataComponentManager::NO_ACTION;
  // we still defragment only one chunk in only one archetype each frame
  // but now we try to find it faster (skipping up to 16 defragmented archetypes each tick)
  int attempts_to_find_defrag_archetype = eastl::min((uint32_t)64, archetypesCount); // while we check
  do
  {
    defragmentArchetypeId = (defragmentArchetypeId + 1) % archetypesCount;
#if DAECS_EXTENSIVE_CHECKS
    validateArchetypeES(defragmentArchetypeId);
#endif
    if (archetypes.shouldDefragment(defragmentArchetypeId))
    {
      defrag = archetypes.defragment(defragmentArchetypeId, movedFrom, movedTo, movedAt, movedCount);
      if (defrag != DataComponentManager::NO_ACTION)
        break;
    }
  } while (--attempts_to_find_defrag_archetype && !((attempts_to_find_defrag_archetype & 15) == 0 && profile_usec_passed(reft, 2)));

  if (defrag == DataComponentManager::NO_ACTION)
    return;
  if (defrag == DataComponentManager::DATA_MOVED)
  {
    TIME_PROFILE(ecs_gc_move_data);
    G_ASSERTF(movedTo < archetypes.getArchetype(defragmentArchetypeId).manager.getChunksCount(), "%d<%d", movedTo,
      archetypes.getArchetype(defragmentArchetypeId).manager.getChunksCount());
    const auto &chunk = archetypes.getArchetype(defragmentArchetypeId).manager.getChunk(movedTo);
    G_ASSERTF(chunk.getUsed() == movedAt + movedCount, "chunk.getUsed() = %d, moved = %d + %d", chunk.getUsed(), movedAt, movedCount);

    EntityId *__restrict eid = (EntityId *__restrict)chunk.getCompDataUnsafe(0);
    eid += movedAt;
    EntityId *__restrict start = eid;
    G_UNUSED(start);
    for (EntityId *__restrict eidE = eid + movedCount; eid != eidE; ++eid)
    {
      EntityDesc &entDesc = entDescs[eid->index()];
      DAECS_EXT_ASSERT(entDesc.idInChunk == eid - start && entDesc.chunkId == movedFrom);
      DAECS_EXT_ASSERT(entDesc.archetype == defragmentArchetypeId);
      entDesc.idInChunk = entDesc.idInChunk + movedAt;
      entDesc.chunkId = movedTo;
    }
  }
  else if (defrag == DataComponentManager::CHUNK_REMOVE)
  {
    TIME_PROFILE(ecs_gc_remove_chunk);
    auto &archetype = archetypes.getArchetype(defragmentArchetypeId);
    for (uint32_t chunkI = movedFrom + 1, chunkE = archetype.manager.getChunksCount(); chunkI < chunkE; ++chunkI)
    {
      const auto &chunk = archetype.manager.getChunk(chunkI);
      const uint32_t chunkUsed = chunk.getUsed();
      if (!chunkUsed)
        continue;
      G_ASSERTF(chunk.data && chunk.getUsed() <= chunk.getCapacity(), "%p %d %d", chunk.data, chunk.getUsed(), chunk.getCapacity());
      for (EntityId *__restrict eid = (EntityId *__restrict)chunk.getCompDataUnsafe(0), *__restrict eidE = eid + chunkUsed;
           eid != eidE; ++eid)
      {
        EntityDesc &entDesc = entDescs[eid->index()];
        DAECS_EXT_ASSERTF(entDesc.idInChunk == eid - (EntityId *)chunk.getCompDataUnsafe(0) && entDesc.chunkId == chunkI,
          "eid %d: entDesc.idInChunk = %d entDesc.chunkId = %d should be id = %d chunk= %d, generation %d == %d", entity_id_t(*eid),
          entDesc.idInChunk, entDesc.chunkId, eid - (EntityId *)chunk.getCompDataUnsafe(0), chunkI, eid->generation(),
          entDesc.generation);
        DAECS_EXT_ASSERTF(eid->generation() == entDesc.generation, "eid %d: generation %d != %d", entity_id_t(*eid), eid->generation(),
          entDesc.generation);
        DAECS_EXT_ASSERTF(entDesc.archetype == defragmentArchetypeId, "archetype = %d should be %d", entDesc.archetype,
          defragmentArchetypeId);
        entDesc.chunkId--;
      }
    }
    archetype.manager.removeChunk(movedFrom);
  }
#if DAECS_EXTENSIVE_CHECKS
  if (profile_usec_passed(reft, 1000))
    debug("defrag of archetype = %d took %d us", defragmentArchetypeId, profile_time_usec(reft));
#endif
}
#if DAGOR_DBGLEVEL == 0 && defined(_MSC_VER)
#pragma warning(pop)
#endif

void EntityManager::performDelayedCreation(bool flush_all)
{
  bool wereAdded;
  uint32_t iterations = 32; // we limit maximum amount of iterations, to avoid inifinite loop (halting problem)
  do
  {
    wereAdded = createQueuedEntities();
  } while (flush_all && iterations-- && (wereAdded));
}

void EntityManager::tick(bool flush_all)
{
  G_ASSERT_RETURN(!isConstrainedMTMode(), );
  TIME_PROFILE(ecs_tick);
  // ofc, it is error to reset nested counters if we are within query, but it is illegal to call tick within query and not likely to
  // happen. what can happen is:
  //  * running query/es from thread without constrainedMT mode
  //  * handled exception/panic in within query
  //  and in that case we better attempt to 'recover', than just ignore all future track changes, etc
  if (nestedQuery != 0)
  {
    logerr("tick within query/es isn't allowed, or nestedQuery=%d leaked", nestedQuery);
    nestedQuery = 0;
  }

  updateCurrentUpdateMaxJobs();
  performTrackChanges(flush_all);
  performDelayedCreation(flush_all);

#if DAECS_EXTENSIVE_CHECKS
  if (creationAllocator.calcMemAllocated() > 0)
    logerr("there is allocated memory on tick");
#endif
  performDeferredEvents(flush_all);
  flushGameResRequests();
  performDelayedCreation(flush_all); // we call it twice, since after performDeferredEvents there could be new
  defragmentArchetypes();
  maintainQueries();
}

const char *EntityManager::getEntityFutureTemplateName(EntityId eid) const
{
  ScopedMTMutex lock(isConstrainedMTMode(), ownerThreadId, creationMutex);
  const unsigned idx = eid.index();

  if (!entDescs.doesEntityExist(idx, eid.generation()))
    return nullptr;
  for (auto ci = delayedCreationQueue.rbegin(), ce = delayedCreationQueue.rend(); ci != ce; ++ci)
    for (auto i = ci->end() - 1, e = ci->begin() - 1; i != e; --i)
      if (i->eid == eid)
      {
        DAECS_EXT_ASSERT(i->op <= DelayedEntityCreation::Op::Create); // currently add is not supported
        return i->isToDestroy() ? nullptr : i->templateName.c_str();
      }
  return idx < entDescs.allocated_size() ? getTemplateName(entDescs[idx].template_id) : nullptr;
}

bool Template::isSingleton() const { return isSingletonDB(g_entity_mgr->getTemplateDB()); }
const ChildComponent &Template::getComponent(const HashedConstString &hash_name) const
{
  return getComponent(hash_name, g_entity_mgr->getTemplateDB());
}
bool Template::hasComponent(const HashedConstString &hash_name) const
{
  return hasComponent(hash_name, g_entity_mgr->getTemplateDB());
}

extern const uint32_t MAX_RESERVED_EID_IDX = MAX_RESERVED_EID_IDX_CONST; // To consider: unsatisfied/weak link time dep in order be
                                                                         // able configure it per game?
};                                                                       // namespace ecs

using namespace ecs;
#define ECS_DECL_CORE_EVENT ECS_REGISTER_EVENT
ECS_DECL_ALL_CORE_EVENTS
