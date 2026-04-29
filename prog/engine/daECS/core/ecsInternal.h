// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daECS/core/entityManager.h>
#include "archetypesInternal.h"

namespace ecs
{

__forceinline void EntityManager::removeDataFromArchetype(const uint32_t archetype, const chunk_type_t chunkId,
  const uint32_t idInChunk)
{
  uint32_t movedId;
  if (archetypes.delEntity(archetype, chunkId, idInChunk, movedId))
  {
    EntityId movedEid = *(EntityId *)archetypes.getComponentDataUnsafe(archetype, 0, sizeof(EntityId), chunkId, idInChunk); // it is
                                                                                                                            // already
                                                                                                                            // located
                                                                                                                            // in that
                                                                                                                            // place
#if DAECS_EXTENSIVE_CHECKS
    if (!doesEntityExist(movedEid))
    {
      logerr("new entity in index is already destroyed! archetype=%d eid =%d chunkId=%d, idInChunk=%d, chunksCnt %d, chunkUsed = %d",
        archetype, ecs::entity_id_t(movedEid), chunkId, idInChunk, archetypes.getArchetype(archetype).manager.getChunksCount(),
        chunkId < archetypes.getArchetype(archetype).manager.getChunksCount()
          ? archetypes.getArchetype(archetype).manager.getChunks()[chunkId].getUsed()
          : -2);
    }
#endif
    const unsigned movedIdx = movedEid.index();
    if (movedIdx < entDescs.size()) // paranoid
    {
      DAECS_EXT_ASSERT(entDescs[movedIdx].chunkId == chunkId && entDescs[movedIdx].archetype == archetype);
      DAECS_EXT_ASSERT(entDescs[movedIdx].generation == movedEid.generation());
      entDescs[movedIdx].idInChunk = idInChunk;
    }
  }
}

#if defined(_MSC_VER) && !defined(__clang__)
#define INLINE_LAMBDA
#else
#define INLINE_LAMBDA __attribute__((always_inline))
#endif

template <typename FilterUsed>
inline void EntityManager::destroyComponents(const uint32_t archetype, const chunk_type_t chunkId, const uint32_t id,
  FilterUsed filter)
{
  if (!(archetypes.getArchetypeCombinedTypeFlags(archetype) & COMPONENT_TYPE_NON_TRIVIAL_CREATE))
    return;
  auto destructComponent = [&](component_index_t cIndex, auto ci) INLINE_LAMBDA {
    type_index_t tIndex = ci->typeIndex;
    ComponentType typeInfo = componentTypes.getTypeInfo(tIndex);
    if (!filter(ci->archComponentId, cIndex))
    {
      auto typeManager = componentTypes.getTypeManager(tIndex);
      DAECS_EXT_ASSERT_RETURN(typeManager, );
      void *__restrict cData = archetypes.getComponentDataUnsafe(archetype, ci->archComponentId, typeInfo.size, chunkId, id);
      typeManager->destroy(cData);
    }
  };
  auto &creatablesTracked = archetypes.getCreatableTrackeds(archetype); // can be empty
  for (auto ci = creatablesTracked.crbegin(), ce = creatablesTracked.crend(); ci != ce; ++ci)
    destructComponent(archetypes.getComponentUnsafe(archetype, ci->archComponentId), ci);
  auto &creatables = archetypes.getCreatables(archetype);
  DAECS_EXT_ASSERT(!creatables.empty()); // can not be empty as we have combined flag of having creatables
  for (auto ci = creatables.end() - 1, ce = creatables.begin(); ci >= ce; --ci)
    destructComponent(ci->originalCidx, ci);
}

template <typename FilterUsed>
inline void EntityManager::removeFromArchetype(const uint32_t archetype, const chunk_type_t chunkId, const uint32_t idInChunk,
  FilterUsed filter)
{
  DAECS_VALIDATE_ARCHETYPE(archetype);
  if (archetype == INVALID_ARCHETYPE)
    return;
  // destroy from DataManager
  destroyComponents(archetype, chunkId, idInChunk, filter);
  removeDataFromArchetype(archetype, chunkId, idInChunk);
}

}; // namespace ecs