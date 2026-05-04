// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daECS/core/internal/archetypes.h>
#include "dataComponentManagerInt.h"
namespace ecs
{

inline void Archetypes::allocateEntity(uint32_t archetype, chunk_type_t &chunkId, uint32_t &id)
{
  DAECS_EXT_ASSERT(archetype < archetypes.size() && getArchetypeComponentOfsUnsafe(archetype) < archetypeComponents.size());
  Archetype &arch = archetypes.get<ARCHETYPE>()[archetype]; // -V758
  arch.manager.allocateEmpty(chunkId, id, arch.entitySize);
}

inline void Archetypes::entityAllocated(uint32_t archetype, chunk_type_t chunkId)
{
  archetypes.get<ARCHETYPE>()[archetype].manager.allocated(chunkId);
}

inline void Archetypes::addEntity(uint32_t archetype, chunk_type_t &chunkId, uint32_t &id, const uint8_t *__restrict initial_data,
  const uint16_t *__restrict offsets)
{
  DAECS_EXT_ASSERT(archetype < archetypes.size() && getArchetypeComponentOfsUnsafe(archetype) < archetypeComponents.size());
  Archetype &arch = archetypes.get<ARCHETYPE>()[archetype]; // -V758
  arch.manager.allocate(chunkId, id, arch.entitySize, initial_data, componentDataSizes(archetype), offsets);
}

inline bool Archetypes::delEntity(uint32_t archetype, chunk_type_t chunkId, uint32_t id, uint32_t &movedChunkIndex)
{
  DAECS_EXT_ASSERT(archetype < archetypes.size());
  Archetype &arch = archetypes.get<ARCHETYPE>()[archetype]; // -V758
  if (arch.manager.removeFromChunk(chunkId, id, arch.entitySize,
        &eastl::get<DATA_SIZE>(archetypeComponents[getArchetypeComponentOfsUnsafe(archetype)]), movedChunkIndex))
    return true;
  return false;
}

inline DataComponentManager::DefragResult Archetypes::defragment(uint32_t arch, chunk_type_t &moved_from_chunk,
  chunk_type_t &moved_to_chunk, uint32_t &moved_at_id, uint32_t &moved_cnt)
{
  Archetype &archetype = eastl::get<ARCHETYPE>(archetypes[arch]);
  const uint32_t baseOfs = getArchetypeComponentOfsUnsafe(arch);
  return archetype.manager.defragment(&eastl::get<DATA_SIZE>(archetypeComponents[baseOfs]),
    &eastl::get<DATA_OFFSET>(archetypeComponents[baseOfs]), archetype.componentsCnt, archetype.entitySize, moved_from_chunk,
    moved_to_chunk, moved_at_id, moved_cnt);
}


} // namespace ecs