#pragma once
#include <util/dag_compilerDefs.h>
#include "typesAndLimits.h"
#include "asserts.h"
#include <util/dag_stdint.h>
#include <generic/dag_span.h>
#include <generic/dag_smallTab.h>
#include <debug/dag_assert.h>
#include <EASTL/vector.h>
#include <EASTL/unique_ptr.h>
#include <osApiWrappers/dag_virtualMem.h>
// #include <math/dag_mathUtils.h>

#ifndef RESTRICT_FUN
#ifdef _MSC_VER
#define RESTRICT_FUN __declspec(restrict)
#else
#define RESTRICT_FUN
#endif
#endif

namespace ecs
{
// all data components to be stored in a very simple way.
// one chunk (persumably allocated per template) have all of data components in linear order, separated by capacity
#pragma pack(push)
#pragma pack(4) // we make it tight, so Archetype size is 32 bytes (in 64 bit)
struct DataComponentManager
{
protected:
  struct Chunk;

public:
  enum
  {
    MAX_CAPACITY_BITS = ECS_MAX_CHUNK_ID_BITS,
    MAX_CAPACITY = (1 << MAX_CAPACITY_BITS),
    USED_BITS = MAX_CAPACITY_BITS + 1, // we need to store up to MAX_CAPACITY, so we need +1 bit
    INITIAL_ENTIIES_BITS = 0
  };
#if !_TARGET_64BIT
  enum
  {
    EMPTY_CAPACITY = MAX_CAPACITY_BITS + 1,
    MAX_CAPACITY_MASK = (1 << EMPTY_CAPACITY) - 1
  };
#else
  static constexpr int EMPTY_CAPACITY = 32;
#endif
  enum DefragResult
  {
    NO_ACTION,
    DATA_MOVED,
    CHUNK_REMOVE
  };

  DataComponentManager() = default;
  DataComponentManager(DataComponentManager &&) = default;
  DataComponentManager &operator=(DataComponentManager &&) = default;
  DataComponentManager(const DataComponentManager &) = delete;
  DataComponentManager &operator=(const DataComponentManager &) = delete;
  ~DataComponentManager() = default;
  DataComponentManager(uint32_t /*entity_size*/, uint8_t initial_bits) : initialBits(initial_bits), currentCapacityBits(initial_bits)
  {
    setEmpty();
  }
  void setEmpty()
  {
    G_ASSERT_RETURN(totalEntitiesUsed == 0, );
    aliasedChunks.getSingleChunk().swap(Chunk(0, EMPTY_CAPACITY));
    totalEntitiesCapacity = 0;
  }
  // capacity in entity
  uint32_t getTotalEntities() const { return totalEntitiesUsed; }
  DefragResult defragment(const uint16_t *__restrict component_sz, const uint16_t *__restrict component_ofs,
    const uint32_t components_cnt, const uint32_t entity_size, chunk_type_t &moved_from_chunk, chunk_type_t &moved_to_chunk,
    uint32_t &moved_at_id, uint32_t &moved_cnt);

  RESTRICT_FUN
  void *__restrict getDataUnsafeNoCheck(uint32_t ofs, uint32_t sz, chunk_type_t chunkId, uint32_t id) const;
  RESTRICT_FUN
  void *__restrict getDataUnsafe(uint32_t ofs, uint32_t sz, chunk_type_t chunkId, uint32_t id) const;
  RESTRICT_FUN
  void *__restrict getData(uint32_t ofs, uint32_t sz, chunk_type_t chunkId, uint32_t id) const;
  uint32_t allocateChunk(const uint32_t entity_size, uint8_t capacity_bits);

  Chunk &allocateEmptyInNewChunk(chunk_type_t &chunkId, uint32_t &id, uint32_t entity_size);
  Chunk &allocateEmpty(chunk_type_t &chunkId, uint32_t &id, uint32_t entity_size);
  void allocated(chunk_type_t chunkId);
  void allocate(chunk_type_t &chunkId, uint32_t &id, uint32_t entity_size, const uint8_t *__restrict data,
    const uint16_t *__restrict component_sz, const uint16_t *__restrict component_ofs);

  bool removeFromChunk(chunk_type_t chunkId, uint32_t index, uint32_t entity_size, const uint16_t *__restrict component_sz,
    uint32_t &moved_index); // moved_index has became index

  RESTRICT_FUN uint8_t *__restrict getChunkData(chunk_type_t chunkId, uint32_t &stride) const;
  const uint32_t getChunksCount() const;
  const uint32_t getChunkCapacity(uint32_t chunkId) const;
  const uint32_t getChunkUsed(uint32_t chunkId) const;
  void removeChunk(uint32_t c);
  __forceinline dag::Span<Chunk> getChunks() { return aliasedChunks.getChunks(); }
  __forceinline dag::ConstSpan<Chunk> getChunksConst() const { return aliasedChunks.getChunksConst(); }
#if DAECS_EXTENSIVE_CHECKS
  __forceinline dag::Span<Chunk> getChunksPtr() { return getChunks(); }
  __forceinline dag::ConstSpan<Chunk> getChunksConstPtr() const { return getChunksConst(); }
#else
  RESTRICT_FUN __forceinline Chunk *getChunksPtr() { return aliasedChunks.getChunksPtr(); }
  RESTRICT_FUN __forceinline const Chunk *getChunksConstPtr() const { return aliasedChunks.getChunksConstPtr(); }
#endif
  const Chunk &getChunk(uint32_t chunk_id) const;
  Chunk &getChunk(uint32_t chunk_id);
  uint8_t lock() { return ++lockedOnRecreateCount; }
  uint8_t unlock() { return --lockedOnRecreateCount; }
  bool isUnlocked() const { return lockedOnRecreateCount == 0; }

protected:
  friend struct DataComponentsManagerAccess;
  void verifyCachedData();
  bool shrinkToFitChunks(const uint16_t *__restrict component_sz, const uint16_t *__restrict component_ofs,
    const uint32_t components_cnt, const uint32_t entity_size, uint32_t chunks_to_defragment);

  struct Chunk // sizeof(Chunk) == 8+1(12) in 32 bit, and, aligned 12+1 (16) in 64 bit. we can, of course, make it SOA, but as for now
               // just waste 4 bytes
  {
    friend class EntityManager;         // so we can get chunk data
    uint8_t *__restrict data = nullptr; // don't use unique_ptr, so we can make restrict
    // since in 64 bit we waster 4 bytes anyway, just use types as-is (uint32_t + uint8_t)
    // uint32_t entitiesUsed:USED_BITS;
    // uint32_t entitiesCapacityBits:5;
    // uint32_t unusedBits:7;//
    uint32_t entitiesUsed = 0;
    uint8_t entitiesCapacityBits = EMPTY_CAPACITY; //(1<<21)&((1<<22)-1)in 32 bit or uint32_t(1<<32) == 0 in 64 bit, mean empty empty
    uint32_t getUsed() const { return entitiesUsed; }
#if !_TARGET_64BIT
    uint32_t getCapacity() const { return (1U << entitiesCapacityBits) & MAX_CAPACITY_MASK; } // mask by maximum value
#else
    uint32_t getCapacity() const
    {
      return uint32_t(1LL << uint64_t(entitiesCapacityBits));
    } // truncating (1<<32) to 32 bits is always zero
#endif
    uint32_t getCapacityBits() const { return entitiesCapacityBits; }
    uint32_t getFree() const
    {
      G_ASSERTF(getUsed() <= getCapacity(), "used = %d capacity = %d", getUsed(), getCapacity());
      return getCapacity() - getUsed();
    }
    uint8_t *__restrict getData() const { return data; }
    uint8_t *__restrict getCompDataUnsafe(uint32_t ofs) const // pointer to all components of ofs
    {
      return getData() + (ofs << getCapacityBits());
    }
    Chunk(size_t total_size, uint8_t capacity_bits) : data(allocate(total_size)), entitiesCapacityBits(capacity_bits) {}

    // Rule-of-5
    Chunk(const Chunk &) = delete; // non_copyable
    void swap(Chunk &&a)
    {
      eastl::swap(data, a.data);
      eastl::swap(entitiesUsed, a.entitiesUsed);
      eastl::swap(entitiesCapacityBits, a.entitiesCapacityBits);
    }
    Chunk(Chunk &&a) { swap(eastl::move(a)); }
    Chunk() = default;
    Chunk &operator=(Chunk &&a)
    {
      swap(eastl::move(a));
      return *this;
    }
    void reset()
    {
      deallocate(data);
      data = nullptr;
      entitiesUsed = 0;
      entitiesCapacityBits = EMPTY_CAPACITY;
    }
    ~Chunk() { reset(); }

  protected:
#if _TARGET_64BIT && DAECS_EXTENSIVE_CHECKS && _TARGET_PC && !DAGOR_ADDRESS_SANITIZER // we only check this in 64 bit (enough virtual
                                                                                      // memory) and if asan is not enabled, otherwise
                                                                                      // it is covered by asan
    static __forceinline uint8_t *allocate(size_t sz)
    {
      if (sz == 0)
        return NULL;
      void *data = os_virtual_mem_alloc(sz + 16);
      os_virtual_mem_commit(data, sz + 16);
      *(size_t *)data = sz;
      return ((uint8_t *)data) + 16;
    }
    static __forceinline void deallocate(uint8_t *data)
    {
      if (data)
        os_virtual_mem_decommit(data - 16, *(size_t *)(data - 16)); // never free, so we can check AV
    }
#else
    static __forceinline uint8_t *allocate(size_t sz) { return sz ? new uint8_t[sz] : NULL; }
    static __forceinline void deallocate(uint8_t *data) { delete[] data; }
#endif
  };

  uint32_t getAllocateSize(uint32_t newCapacityBits, uint32_t entity_size) const
  {
    return ((entity_size << newCapacityBits) + 12); // we always allocate 12 bytes more, so it is always safe to load point3 and float
                                                    // with ld/ldu
  }
  chunk_type_t findEmptyChunk() const
  {
    auto chunks = getChunksConst();
    for (auto i = chunks.begin(), e = chunks.end(); i != e; ++i)
      if (i->getUsed() < i->getCapacity())
        return eastl::distance(chunks.begin(), i);
    G_ASSERT(0);
    return -1;
  }
  void addToChunk(Chunk &chunk, uint32_t id, uint32_t entity_size, const uint8_t *__restrict data,
    const uint16_t *__restrict component_sz, const uint16_t *__restrict component_ofs);
  typedef SmallTab<Chunk, EASTLAllocatorType, uint16_t> ChunksArray;
  struct AliasedChunk
  {
    bool isArray() const { return (bool)container[sizeof(container) - 1]; }
    ChunksArray &getArray()
    {
      if (EASTL_LIKELY(!isArray()))
      {
        ChunksArray array;
        array.emplace_back(eastl::move(*(Chunk *)container));
        makeArray();
        ((ChunksArray *)container)->swap(array);
      }
      G_ASSERT(isArray());
      return *((ChunksArray *)container);
    }
    Chunk &getSingleChunk()
    {
      G_ASSERT(!isArray());
      return *((Chunk *)container);
    }
    __forceinline const Chunk *getChunksConstPtr() const
    {
      if (EASTL_UNLIKELY(isArray()))
        return ((ChunksArray *)container)->data();
      return (Chunk *)container;
    }
    __forceinline Chunk *getChunksPtr() { return const_cast<Chunk *>(getChunksConstPtr()); }
    __forceinline dag::Span<Chunk> getChunks()
    {
      if (EASTL_UNLIKELY(isArray()))
        return dag::Span<Chunk>(((ChunksArray *)container)->begin(), ((ChunksArray *)container)->size());
      return dag::Span<Chunk>((Chunk *)container, 1);
    }
    __forceinline dag::ConstSpan<Chunk> getChunksConst() const
    {
      if (EASTL_UNLIKELY(isArray()))
        return dag::ConstSpan<Chunk>(((ChunksArray *)container)->begin(), ((ChunksArray *)container)->size());
      return dag::ConstSpan<Chunk>((const Chunk *)container, 1);
    }
    __forceinline uint32_t getChunksCount() const
    {
      if (EASTL_UNLIKELY(isArray()))
        return ((ChunksArray *)container)->size();
      return 1;
    }
    void eraseChunk(uint32_t c)
    {
      G_ASSERT_RETURN(isArray() && c < getChunksCount(), );
      auto &chunksArray = *((ChunksArray *)container);
      chunksArray.erase(chunksArray.begin() + c);
      if (chunksArray.size() == 1)
      {
        Chunk chunk = eastl::move(chunksArray[0]);
        ((ChunksArray *)container)->~ChunksArray();
        makeSingleChunk();
        ((Chunk *)container)->swap(eastl::move(chunk));
      }
    }
    ~AliasedChunk()
    {
      if (isArray())
        ((ChunksArray *)container)->~ChunksArray();
      else
        ((Chunk *)container)->~Chunk();
    }
    AliasedChunk() { memset(container, 0, sizeof(container)); }
    AliasedChunk(AliasedChunk &&a)
    {
      memcpy(container, a.container, sizeof(container));
      memset(a.container, 0, sizeof(container));
    }
    AliasedChunk &operator=(AliasedChunk &&a)
    {
      G_STATIC_ASSERT(sizeof(AliasedChunk) <= 16);
      uint8_t temp[sizeof(container)];
      memcpy(temp, container, sizeof(container));
      memcpy(container, a.container, sizeof(container));
      memcpy(a.container, temp, sizeof(container));
      return *this;
    }
    AliasedChunk(const AliasedChunk &) = delete;
    AliasedChunk &operator=(const AliasedChunk &) = delete;

  protected:
    uint8_t container[eastl::max(sizeof(ChunksArray), sizeof(Chunk))] = {0}; // last byte is unused in both ChunksArray, and Chunk
    void setIsArray() { container[sizeof(container) - 1] = 1; }
    void setIsNotArray() { container[sizeof(container) - 1] = 0; }
    void makeArray()
    {
      new (container, _NEW_INPLACE) ChunksArray();
      setIsArray();
    }
    void makeSingleChunk()
    {
      new (container, _NEW_INPLACE) Chunk();
      setIsNotArray();
    }
  } aliasedChunks;
  // actually we need only ChunksArray (sizeof(ChunksArray) == sizeof(void*) + 2*2 == 8/16, due to alignment)
  // Moreover, if there is only one chunk (most common case!), it has same size as Chunk and can be aliased with it;
  // so we remove one indirection with that
  // actually, if it is only one chunk, we could alias data +entitiesUsed with chunk
  uint32_t totalEntitiesUsed = 0, totalEntitiesCapacity = 0; // 4+4 total entities&capacity used is up to 24 bit, so 16 bits wasted
  chunk_type_t workingChunk = 0;                             // 1 bytes
  uint8_t initialBits = 0, currentCapacityBits = 0;          // 2 bytes
  uint8_t lockedOnRecreateCount = 0;
};
#pragma pack(pop)

__forceinline const DataComponentManager::Chunk &DataComponentManager::getChunk(uint32_t chunk_id) const
{
  return getChunksConstPtr()[chunk_id];
}
__forceinline DataComponentManager::Chunk &DataComponentManager::getChunk(uint32_t chunk_id) { return getChunksPtr()[chunk_id]; }
__forceinline const uint32_t DataComponentManager::getChunksCount() const { return aliasedChunks.getChunksCount(); }
__forceinline const uint32_t DataComponentManager::getChunkCapacity(uint32_t chunkId) const
{
  return getChunksConstPtr()[chunkId].getCapacity();
}
__forceinline const uint32_t DataComponentManager::getChunkUsed(uint32_t chunkId) const
{
  return getChunksConstPtr()[chunkId].getUsed();
}

RESTRICT_FUN
__forceinline void *__restrict DataComponentManager::getDataUnsafeNoCheck(uint32_t ofs, uint32_t sz, chunk_type_t chunkId,
  uint32_t id) const
{
  DAECS_EXT_ASSERT(chunkId < getChunksConst().size());
  return getChunksConstPtr()[chunkId].getCompDataUnsafe(ofs) + id * sz;
}

RESTRICT_FUN
__forceinline void *__restrict DataComponentManager::getDataUnsafe(uint32_t ofs, uint32_t sz, chunk_type_t chunkId, uint32_t id) const
{
#if DAECS_EXTENSIVE_CHECKS
  auto chunks = getChunksConst();
  // we have to check for <= as it is the case for creating entity
  G_ASSERT(chunkId < chunks.size() && id < chunks[chunkId].getCapacity() && id <= chunks[chunkId].getUsed());
#endif
  return getChunksConstPtr()[chunkId].getCompDataUnsafe(ofs) + id * sz;
}

RESTRICT_FUN
inline void *__restrict DataComponentManager::getData(uint32_t ofs, uint32_t sz, chunk_type_t chunkId, uint32_t id) const
{
  auto chunks = getChunksConst();
  if (chunkId >= chunks.size() || id >= chunks[chunkId].getUsed())
    return nullptr;
  return chunks[chunkId].getCompDataUnsafe(ofs) + id * sz;
}

RESTRICT_FUN
inline uint8_t *__restrict DataComponentManager::getChunkData(chunk_type_t chunkId, uint32_t &stride) const
{
  auto &chunk = getChunk(chunkId);
  stride = chunk.getCapacity();
  return chunk.getData();
}

}; // namespace ecs

#undef RESTRICT_FUN
