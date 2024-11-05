// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <math/dag_adjpow2.h>
#include <debug/dag_debug.h>
#include <daECS/core/internal/dataComponentManager.h>
#include <perfMon/dag_statDrv.h>
#include "dataComponentManagerInt.h"

namespace ecs
{
DAGOR_NOINLINE
DataComponentManager::Chunk &DataComponentManager::allocateEmptyInNewChunk(chunk_type_t &chunkId, uint32_t &id, uint32_t entity_size)
{
  // make it out of line, it is unhappy path
  // We use 'buddy allocator' (each new chunk is twice bigger then last, don't remove old chunks)
  // if creating entities by one, this will cause rather big amount of rather small chunks, which is not optimal for querying
  // however, we can optimize that, by using de-fragmentation (one archetype by frame, or something like that)
  if (totalEntitiesUsed < totalEntitiesCapacity && isUnlocked()) // there is free memory somewhere in some chunk
    workingChunk = findEmptyChunk();
  else // there is no sufficient memory, allocate new memory.
    workingChunk = allocateChunk(entity_size, currentCapacityBits + (totalEntitiesUsed < totalEntitiesCapacity ? 0 : 1));
  auto &chunk = getChunk(workingChunk);
  id = chunk.getUsed(); // we always add to the end
  chunkId = workingChunk;
  return chunk;
}

void DataComponentManager::verifyCachedData()
{
  G_STATIC_ASSERT(sizeof(DataComponentManager) <= 28);
#if DAECS_EXTENSIVE_CHECKS
  uint32_t countUsed = 0, countCapacity = 0;
  for (auto &chunk : getChunksConst())
  {
    countUsed += chunk.getUsed();
    countCapacity += chunk.getCapacity();
  }
  if (totalEntitiesUsed != countUsed || totalEntitiesCapacity != countCapacity)
  {
    logerr("chunks = %d, cached totalEntitiesUsed = %d != %d totalEntitiesCapacity = %d != %d", getChunksCount(), totalEntitiesUsed,
      countUsed, totalEntitiesCapacity, countCapacity);
    auto chunks = getChunksConst();
    for (int i = 0, e = chunks.size(); i < e; ++i)
      debug("%d:chunk = %d %d", i, chunks[i].getUsed(), chunks[i].getCapacity());

    totalEntitiesUsed = countUsed;
    totalEntitiesCapacity = countCapacity;
  }
#endif
}

bool DataComponentManager::shrinkToFitChunks(const uint16_t *__restrict component_sz, const uint16_t *__restrict component_ofs,
  const uint32_t components_cnt, const uint32_t entity_size, uint32_t chunks_to_defragment)
{
  if (totalEntitiesUsed == 0 && getChunksCount() == 1)
  {
    if (getChunks()[0].getCapacity() == 0)
      return false;
    currentCapacityBits = initialBits;
    setEmpty();
    return true;
  }
  uint32_t defragmentedChunks = chunks_to_defragment;
  auto chunks = getChunks();
  for (size_t ci = 0, ce = chunks.size(); ci < ce; ci++)
  {
    auto &chunk = chunks[ci];
    if (chunk.getCapacityBits() > initialBits && chunk.getCapacity() >= chunk.getUsed() * 2)
    {
      const uint32_t capacityBits = eastl::clamp(get_bigger_log2(chunk.getUsed()), (uint32_t)initialBits, (uint32_t)MAX_CAPACITY_BITS);
      if (capacityBits == chunk.getCapacityBits())
        continue;
      TIME_PROFILE(ecs_gc_shrink_to_fit);
      currentCapacityBits = eastl::clamp(get_bigger_log2(totalEntitiesUsed), (uint32_t)initialBits, (uint32_t)MAX_CAPACITY_BITS);
      Chunk newChunk(getAllocateSize(capacityBits, entity_size), capacityBits);
      newChunk.entitiesUsed = chunk.entitiesUsed;
      const uint32_t srcEntitiesCnt = chunk.getUsed();
      for (uint32_t ci = 0; ci < components_cnt; ++ci, component_sz++, component_ofs++)
      {
        const uint32_t csz = *component_sz;
        const uint32_t ofs = *component_ofs;
        memcpy(newChunk.getCompDataUnsafe(ofs), chunk.getCompDataUnsafe(ofs), csz * srcEntitiesCnt);
#if DAECS_EXTENSIVE_CHECKS
        memset(chunk.getCompDataUnsafe(ofs), 0xFF, csz * srcEntitiesCnt);
#endif
      }
      totalEntitiesCapacity -= chunk.getCapacity() - newChunk.getCapacity();
      chunk.swap(eastl::move(newChunk));
      if (--defragmentedChunks == 0)
        break; // defragment no more than x chunks
    }
  }
  verifyCachedData();
  return defragmentedChunks != chunks_to_defragment; // something was defragmented
}

DataComponentManager::DefragResult DataComponentManager::defragment(const uint16_t *__restrict component_sz,
  const uint16_t *__restrict component_ofs, const uint32_t components_cnt, const uint32_t entity_size, chunk_type_t &moved_from_chunk,
  chunk_type_t &moved_to_chunk, uint32_t &moved_at_id, uint32_t &moved_cnt)
{
  if (getChunksCount() <= 1) // nothing to defragment, it is already just one chunk
  {
    shrinkToFitChunks(component_sz, component_ofs, components_cnt, entity_size, 1);
    return NO_ACTION;
  }
  // remove empty chunk
  // there is no 'empty' chunk, let's find what to concatenate
  verifyCachedData();
#if DAECS_EXTENSIVE_CHECKS
  for (size_t i = 0, e = getChunksCount(); i < e; ++i)
  {
    G_ASSERTF(getChunk(i).getUsed() <= getChunk(i).getCapacity(), "chunk %d used %d cap %d<=%d", i, getChunk(i).getUsed(),
      getChunk(i).getCapacityBits(), currentCapacityBits);
  }
#endif

  for (size_t e = getChunksCount() - 1; e > 0; --e)
    if (getChunkUsed(e) == 0)
    {
      removeChunk(e);
    }
    else
      break;
  currentCapacityBits =
    eastl::clamp(get_log2i(getChunksConst().back().getCapacity()), (uint32_t)initialBits, (uint32_t)MAX_CAPACITY_BITS);
  verifyCachedData();
  workingChunk = 0;

  if (getChunksCount() == 1) // as for now we can't remove last available chunk, there should be always at least one
  {
    shrinkToFitChunks(component_sz, component_ofs, components_cnt, entity_size, 1);
    return NO_ACTION;
  }

  uint32_t maxFree = 0, maxCapacity = 0;
  chunk_type_t maxFreeChunk = workingChunk, maxCapacityChunk = workingChunk;
  uint32_t usedInNonMaxBlock = 0, nonMaxBlocks = 0;

#if DAECS_EXTENSIVE_CHECKS
  auto chunkPtr = getChunksPtr().begin();
#else
  auto chunkPtr = getChunksPtr();
#endif
  uint32_t maxCapacityChunkFree = chunkPtr[maxCapacityChunk].getFree();
  const uint32_t chunkE = getChunksCount();
  uint32_t chunkI = 0;
  do
  {
    auto &chunk = *(chunkPtr++);
    const auto chunkCapacity = chunk.getCapacity(), chunkFree = chunk.getFree(), chunkUsed = chunk.getUsed();
    if (chunkCapacity >= maxCapacity)
    {
      if (chunkCapacity > maxCapacity || chunkFree >= maxCapacityChunkFree)
      {
        maxCapacity = chunkCapacity;
        maxCapacityChunkFree = chunkFree;
        maxCapacityChunk = chunkI;
      }
    }
    if (chunkCapacity < MAX_CAPACITY)
    {
      usedInNonMaxBlock += chunk.getUsed();
      nonMaxBlocks++;
    }
    if (chunkUsed == 0) // this chunk is empty, just remove it!
    {
      moved_from_chunk = moved_to_chunk = chunkI;
      return CHUNK_REMOVE;
    }
    if (maxFree < chunkFree)
    {
      maxFreeChunk = chunkI;
      maxFree = chunkFree;
    }
  } while (++chunkI != chunkE);
  uint32_t minUsed = MAX_CAPACITY;
  chunk_type_t minUsedChunk = workingChunk;
  chunkPtr -= chunkE;
  chunkI = 0;
  do
  {
    auto &chunk = *(chunkPtr++);
    if (minUsed >= chunk.getUsed() && maxFreeChunk != chunkI) // minUsed greater or equal is intentionally. We'd better move from
                                                              // chunks of bigger index, reducing our work to do.
    {
      minUsedChunk = chunkI;
      minUsed = chunk.getUsed();
    }
  } while (++chunkI != chunkE);
  // there is not enough empty space left in any chunk, all data is occupied.
  chunkPtr -= chunkE;
  moved_from_chunk = minUsedChunk;
  workingChunk = maxFreeChunk;
  if (usedInNonMaxBlock > maxCapacity) // we can just fit all data in one new chunk, but we can't fit all of it in any of current
  {
    moved_to_chunk = allocateChunk(entity_size, get_bigger_log2(usedInNonMaxBlock));
    // DEBUG_CTX("moved_to_chunk = %d", moved_to_chunk);
  }
  else if (moved_from_chunk != maxCapacityChunk && minUsed <= chunkPtr[maxCapacityChunk].getFree()) // there is enough empty space left
                                                                                                    // in chunk with biggest capacity
  {
    moved_to_chunk = maxCapacityChunk;
    // DEBUG_CTX("moved_to_chunk = %d", moved_to_chunk);
  }
  else if (minUsed <= maxFree) // there is enough empty space left in some chunk
  {
    moved_to_chunk = maxFreeChunk;
    // DEBUG_CTX("moved_to_chunk = %d", moved_to_chunk);
  }
  else if (usedInNonMaxBlock > 0 && nonMaxBlocks > 1) // there is not enough empty space left anywhere, but there are some blocks we
                                                      // can collapse
  {
    moved_to_chunk = allocateChunk(entity_size, get_bigger_log2(usedInNonMaxBlock));
    // DEBUG_CTX("moved_to_chunk = %d", moved_to_chunk);
  }
  else
  {
    shrinkToFitChunks(component_sz, component_ofs, components_cnt, entity_size, 1);
    return NO_ACTION;
  }
  if (moved_to_chunk == moved_from_chunk)
  {
    logmessage(LOGLEVEL_WARN + (DAGOR_DBGLEVEL > 0 ? 1 : 0),
      "internal error moved_to_chunk == %d moved_from_chunk = %d minUsed=%d maxFree=%d minUsedChunk=%d"
      " maxCapacityChunk=%d (free = %d) usedInNonMaxBlock=%d nonMaxBlocks=%d maxCapacity=%d",
      moved_to_chunk, moved_from_chunk, minUsed, maxFree, minUsedChunk, maxCapacityChunk, maxCapacityChunkFree, usedInNonMaxBlock,
      nonMaxBlocks, maxCapacity);
    shrinkToFitChunks(component_sz, component_ofs, components_cnt, entity_size, 1);
    return NO_ACTION;
  }
#if DAECS_EXTENSIVE_CHECKS
  uint32_t entitySize = 0;
  for (uint32_t ci = 0; ci < components_cnt; ++ci)
    entitySize += component_sz[ci];
  G_ASSERT(entity_size == entitySize);
#endif
  // we can defragment chunk by adding it's data to more capable

  Chunk &fromChunk = getChunk(moved_from_chunk);
  Chunk &toChunk = getChunk(moved_to_chunk);
  const uint32_t srcEntitiesCnt = fromChunk.getUsed();
  const uint32_t dstEntitiesCnt = toChunk.getUsed();
  moved_cnt = srcEntitiesCnt;
  moved_at_id = dstEntitiesCnt;

  G_ASSERTF(toChunk.getFree() >= srcEntitiesCnt, "%d:srcEntitiesCnt = %d moving to %d:free %d", moved_from_chunk, srcEntitiesCnt,
    moved_to_chunk, toChunk.getFree());

  for (uint32_t ci = 0; ci < components_cnt; ++ci, component_sz++, component_ofs++)
  {
    const uint32_t csz = *component_sz;
    const uint32_t ofs = *component_ofs;
    memcpy(toChunk.getCompDataUnsafe(ofs) + csz * dstEntitiesCnt, fromChunk.getCompDataUnsafe(ofs), csz * srcEntitiesCnt);
#if DAECS_EXTENSIVE_CHECKS
    memset(fromChunk.getCompDataUnsafe(ofs), 0xFF, csz * srcEntitiesCnt);
#endif
  }
  toChunk.entitiesUsed += srcEntitiesCnt;
  // deallocate chunk
  totalEntitiesCapacity -= fromChunk.getCapacity();
  fromChunk.reset();

  workingChunk = 0;

  verifyCachedData();
  return DATA_MOVED;
}

}; // namespace ecs