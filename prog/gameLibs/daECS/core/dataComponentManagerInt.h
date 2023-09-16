#pragma once

#include <daECS/core/internal/dataComponentManager.h>
#include "specialized_memcpy.h"
#include <vecmath/dag_vecMathDecl.h>

namespace ecs
{

__forceinline void DataComponentManager::addToChunk(Chunk &chunk, const uint32_t id_in_chunk, uint32_t entity_size,
  const uint8_t *__restrict data, const uint16_t *__restrict component_sz, const uint16_t *__restrict component_ofs)
{
  G_FAST_ASSERT(chunk.data);
  const uint32_t shift = chunk.getCapacityBits();
  for (uint8_t *__restrict compStream = chunk.getData(), *__restrict end = compStream + (entity_size << shift); compStream != end;
       component_sz++, component_ofs++)
  {
    const uint16_t csz = *component_sz;
    // optimize for typical sizes
    SPECIALIZE_MEMCPY_TO_SOA(csz, compStream, id_in_chunk, data + *component_ofs);
    compStream += csz << shift;
  }
}

inline uint32_t DataComponentManager::allocateChunk(const uint32_t entity_size, uint8_t capacity_bits)
{
  capacity_bits = eastl::clamp(capacity_bits, initialBits, (uint8_t)MAX_CAPACITY_BITS);
  currentCapacityBits = eastl::max(currentCapacityBits, capacity_bits);
  Chunk chunk(getAllocateSize(capacity_bits, entity_size), capacity_bits);
  totalEntitiesCapacity += chunk.getCapacity();
  if (!aliasedChunks.isArray() && aliasedChunks.getSingleChunk().data == NULL)
  {
    aliasedChunks.getSingleChunk().swap(eastl::move(chunk));
    return 0;
  }
  else
  {
    auto &chunksArray = aliasedChunks.getArray();
    chunksArray.emplace_back(eastl::move(chunk));
    return (uint32_t)chunksArray.size() - 1;
  }
}

inline void DataComponentManager::removeChunk(uint32_t c)
{
  if (c >= getChunksCount())
    return;
  G_ASSERT(totalEntitiesCapacity >= getChunk(c).getCapacity());
  if (EASTL_UNLIKELY(getChunkUsed(c)))
  {
    G_ASSERTF(0, "removing used chunk=%d(out of %d)! %d used", c, getChunksCount(), getChunkUsed(c));
    return;
  }
  else
    totalEntitiesCapacity = eastl::max((int)0, (int)totalEntitiesCapacity - (int)getChunk(c).getCapacity());
  aliasedChunks.eraseChunk(c);
  workingChunk = 0;
}

inline DataComponentManager::Chunk &DataComponentManager::allocateEmpty(chunk_type_t &chunkId, uint32_t &id, uint32_t entity_size)
{
  // select suitable chunk
  Chunk *__restrict chunk = &getChunk(workingChunk);
#if DAECS_EXTENSIVE_CHECKS
  G_ASSERT(chunk->getUsed() < eastl::numeric_limits<eastl::remove_reference<decltype(id)>::type>::max());
#endif
  const uint32_t cUsed = chunk->getUsed();
  if (EASTL_LIKELY(cUsed < chunk->getCapacity() && isUnlocked())) // current chunk has no sufficient memory
  {
    // do the allocation
    id = cUsed; // we always add to the end
    chunkId = workingChunk;
    return *chunk;
  }
  return allocateEmptyInNewChunk(chunkId, id, entity_size);
}

inline void DataComponentManager::allocated(chunk_type_t chunkId)
{
  unlock();
  totalEntitiesUsed++;
  getChunk(chunkId).entitiesUsed++; // re-read memory
}

inline void DataComponentManager::allocate(chunk_type_t &chunkId, uint32_t &id, uint32_t entity_size, const uint8_t *__restrict data,
  const uint16_t *__restrict component_sz, const uint16_t *__restrict component_ofs)
{
  // select suitable chunk
  Chunk &destChunk = allocateEmpty(chunkId, id, entity_size);
  addToChunk(destChunk, id, entity_size, data, component_sz, component_ofs);
  totalEntitiesUsed++;
  destChunk.entitiesUsed++;
}

inline bool DataComponentManager::removeFromChunk(chunk_type_t chunkId, uint32_t index, uint32_t entity_size,
  const uint16_t *__restrict component_sz,
  uint32_t &moved_index) // moved_index has became index
{
  DAECS_EXT_ASSERTF_RETURN(chunkId < getChunksCount(), false, "%d chunk < %d", chunkId, getChunksCount());
  Chunk &chunk = getChunksPtr()[chunkId];

  DAECS_EXT_ASSERTF_RETURN(index < chunk.getUsed(), false, "%d>=%d chunk %d", index, chunk.getUsed(), chunkId);

  --totalEntitiesUsed;
  --chunk.entitiesUsed;
  if (index == chunk.entitiesUsed)
    return false;

  const uint32_t shift = chunk.getCapacityBits();
  moved_index = chunk.entitiesUsed;
  for (uint8_t *compStream = chunk.getData(), *end = compStream + (entity_size << shift); compStream < end; component_sz++)
  {
    uint32_t csz = *component_sz;
    if (csz)
    {
      // optimize for typical sizes
      SPECIALIZE_MEMCPY_IN_SOA(csz, compStream, index, moved_index);
      compStream += csz << shift;
    }
  }
  return true;
}

}; // namespace ecs