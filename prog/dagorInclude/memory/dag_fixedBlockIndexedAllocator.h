//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

// this is very simple and totally not thread-safe allocator which allocates memory in blocks of fixed size
// it uses buddy-allocator (i.e. allocating memory in chunks growing in power-of-2)
// maximum chunk size is limited to 32768 (can be made bigger, but in case of regular allocator 32768 blocks are 512kb!)
// if allocating (and de-allocating) in order, performance will be 10X times faster than dlmalloc (for blocks of 16 bytes)
// if de-allocating out of order, performance will be 2X times faster than dlmalloc (for blocks of 16 bytes)
// the best property of it, is that it is allocating memory tight (no service information or padding)
// minimum block size is 2 bytes (i.e. if you will use it for smaller blocks, it will waste space)
// NOTE! it doesn't keep alignment of 16 if block size is not 16 bytes aligned!
// if you want to align to 16 block of different size, you can use align_to_if_bigger
// if you want to have such alignment (probably for bigger chunks, use aligned block size)

// unlike FixedBlockAllocator, it operates on indices, rather than void*
// it can at maximum allocates 131072 chunks, which mean that at most it is 4Gb of blocks (if minChunkSize is 32768)
// or at minumum 131057*32768 + 245760 = 4294721536 blocks

// intentionally made with minimum dependencies (we actually need base types and G_ASSERT only, besides memcpy/memset)


#include <memory.h>
#include <string.h>
#include <dag/dag_Vector.h>

#include <memory/dag_fixedBlockChunk.h>

#if DAGOR_DBGLEVEL > 0 && _DEBUG_TAB_
#define FBA_POISON_MEMORY 1
#if defined(_M_IX86_FP) && _M_IX86_FP == 0
#define FBA_MIN4K(x) x
#else
#define FBA_MIN4K(x) min<unsigned>(x, 4096u)
#endif
#define FBA_DEBUG_FILL_MEM(p, sz, ptrn)                                   \
  for (int *i = (int *)(p), *e = i + FBA_MIN4K(sz / sizeof(int)); i < e;) \
  *i++ = ptrn
#else
#define FBA_DEBUG_FILL_MEM(p, sz, ptrn)
#define FBA_POISON_MEMORY 0
#endif

struct FixedBlockAllocatorIndexed
{
protected:
  typedef FixedBlockChunk Chunk;

public:
  uint32_t allocateOneBlock();   // of block size
  bool freeOneBlock(uint32_t b); // return false, if block doesn't belong to allocator. Warning, double delete isn't checked in dev
                                 // build (would require iteration)
  uint32_t allocateContiguousBlocks(uint16_t block_cnt);     // of block size * block_cnt total
  bool freeContiguousBlocks(uint32_t b, uint16_t block_cnt); // return false, if block doesn't belong to allocator. Warning, double
                                                             // delete isn't checked in dev build (would require iteration)

  bool isValidIndex(uint32_t index) const; // return not false if index resides in ranges controlled by this allocator
  uint32_t getNextIndex() const;

  int clear(); // return number of blocks that was freed
  uint32_t getBlockSize() const { return blockSize; }
  void getMemoryStats(uint32_t &chunks_count, uint32_t &allocated_mem, uint32_t &used_mem);
  void *getPointer(uint32_t index); // returns nullptr on incorrect Index
  const void *getPointer(uint32_t index) const { return const_cast<FixedBlockAllocatorIndexed *>(this)->getPointer(index); }
  void *getPointerUnsafe(uint32_t index); // asserts on incorrect Index
  const void *getPointerUnsafe(uint32_t index) const
  {
    return const_cast<FixedBlockAllocatorIndexed *>(this)->getPointerUnsafe(index);
  }

  // memory is 16 byte only if block_size is 16 bytes aligned. If you need bigger than alignment blocks to be aligned - align memory by
  // this function
  static constexpr uint32_t align_to_if_bigger(uint32_t block, uint32_t alignment);

  FixedBlockAllocatorIndexed() = default; // If this ctor is used then init() has to be called before first allocation
  FixedBlockAllocatorIndexed(uint32_t block_size, uint16_t min_chunk_size_in_blocks);
  FixedBlockAllocatorIndexed(FixedBlockAllocatorIndexed &&);
  FixedBlockAllocatorIndexed &operator=(FixedBlockAllocatorIndexed &&);
  FixedBlockAllocatorIndexed(const FixedBlockAllocatorIndexed &) = delete;
  FixedBlockAllocatorIndexed &operator=(const FixedBlockAllocatorIndexed &) = delete;
  ~FixedBlockAllocatorIndexed();

  void init(uint32_t block_size, uint16_t min_chunk_size_in_blocks); // Need to be called if default ctor is used. Warn: can be called
                                                                     // only before first allocation
  bool isInited() const { return getBlockSize() != 0u; }             // return false if init or non default ctor wasn't called


protected:
  dag::Vector<Chunk> chunks;
  uint32_t blockSize = 0;
  uint32_t allocatingHead = ~0u;
  uint32_t freeBlocksLeft = 0;
  uint16_t minChunkSize = 0;
  // we have free 16 bits of padding

  Chunk &validateAllocatingHead(uint16_t block_cnt)
  {
    if (allocatingHead != ~0u)
    {
      auto &c = chunks[allocatingHead];
      if (c.spaceLeft() >= block_cnt)
        return c;
    }
    return makeAllocatingHead(block_cnt);
  }

  DAGOR_NOINLINE
  Chunk &makeAllocatingHead(uint16_t block_cnt)
  {
    if (freeBlocksLeft >= block_cnt)
    {
      for (Chunk *c = chunks.end() - 1, *b = chunks.begin() - 1; c != b; --c)
        if (c->spaceLeft() >= block_cnt)
        {
          allocatingHead = c - (b + 1);
          return *c;
        }
    }
    return addChunk(getNextSize(block_cnt));
  }

  DAGOR_NOINLINE
  void popChunkHead()
  {
    G_FAST_ASSERT(!chunks.empty());
    G_ASSERT(chunks.back().getUsedSize() == 0);
    freeBlocksLeft -= chunks.back().getChunkSize();
    chunks.pop_back();
    allocatingHead = chunks.size() - 1;
  }

  Chunk &addChunk(uint32_t sz)
  {
    freeBlocksLeft += sz;
    chunks.push_back(Chunk(blockSize, sz));
    allocatingHead = chunks.size() - 1;
    G_ASSERT(allocatingHead < MAX_CHUNKS);
    return chunks.back();
  }

  uint32_t getNextSize(uint16_t block_cnt) const
  {
    G_FAST_ASSERT(block_cnt <= MAX_BLOCK_SIZE);
    G_STATIC_ASSERT(MAX_BLOCK_SIZE <= Chunk::MAX_CHUNK_SIZE);
    return max<uint32_t>(chunks.empty() ? minChunkSize : min<uint32_t>(chunks.back().getChunkSize() * 2, MAX_BLOCK_SIZE), block_cnt);
  }

  DAGOR_NOINLINE
  uint32_t allocateOOLContiguousBlocks(uint16_t block_cnt)
  {
    for (int i = chunks.size() - 1; i != -1; --i)
    {
      auto &chunk = chunks[i];
      if (allocatingHead != i && chunk.spaceLeft() >= block_cnt)
      {
        const uint32_t blocks = chunk.allocateBlocks(blockSize, block_cnt);
        if (blocks != chunk.getChunkSize())
        {
          allocatingHead = i;
          return blocks;
        }
      }
    }
    Chunk &chunk = addChunk(getNextSize(block_cnt));
    return chunk.allocateBlocks(blockSize, block_cnt);
  }

  enum
  {
    BLOCK_INDEX_BITS = 15,
    MAX_BLOCK_SIZE = (1 << BLOCK_INDEX_BITS),
    BLOCK_MASK = MAX_BLOCK_SIZE - 1,
    MAX_CHUNKS = 1 << (32 - BLOCK_INDEX_BITS)
  };
  static uint32_t make_index(uint16_t block_index, uint32_t chunk_index)
  {
    G_FAST_ASSERT(block_index <= BLOCK_MASK);
    return block_index | (chunk_index << BLOCK_INDEX_BITS);
  }
  static void decompose_index(uint32_t index, uint16_t &block_index, uint32_t &chunk_index)
  {
    block_index = index & BLOCK_MASK;
    chunk_index = index >> BLOCK_INDEX_BITS;
  }
};

inline FixedBlockAllocatorIndexed::~FixedBlockAllocatorIndexed()
{
  int nb = clear();
  G_UNUSED(nb);
  G_ASSERTF(nb == 0, "Memory leak in fixed block size allocator of block %d (leaked %d blocks)", blockSize, nb);
}

inline int FixedBlockAllocatorIndexed::clear()
{
  uint32_t nblocks = 0;
  for (auto &c : chunks)
    nblocks += c.getUsedSize();

  chunks.clear();
  freeBlocksLeft = 0;
  allocatingHead = ~0u;
  return nblocks;
}

inline uint32_t FixedBlockAllocatorIndexed::allocateOneBlock() // of block size
{
  G_FAST_ASSERT(isInited());
  auto &chunk = validateAllocatingHead(1);
  G_FAST_ASSERT(&chunk - chunks.begin() == allocatingHead);
  const uint32_t block = chunk.allocateOne(blockSize);
  FBA_DEBUG_FILL_MEM(chunk.blocks + block * blockSize, blockSize, 0x7ffdcdcd);
  freeBlocksLeft--;
  return make_index(block, allocatingHead);
}

inline uint32_t FixedBlockAllocatorIndexed::allocateContiguousBlocks(uint16_t block_cnt) // of block size * block_cnt total
{
  G_FAST_ASSERT(isInited());
  auto &chunk = validateAllocatingHead(block_cnt);
  G_FAST_ASSERT(&chunk - chunks.begin() == allocatingHead);
  uint32_t block = chunk.allocateBlocks(blockSize, block_cnt);
  if (DAGOR_UNLIKELY(block == chunk.getChunkSize()))
    block = allocateOOLContiguousBlocks(block_cnt);
  FBA_DEBUG_FILL_MEM(chunk.blocks + block * blockSize, blockSize * block_cnt, 0x7ffdcdcd);
  freeBlocksLeft -= block_cnt;
  return make_index(block, allocatingHead);
}

inline void FixedBlockAllocatorIndexed::getMemoryStats(uint32_t &chunks_count, uint32_t &allocated_mem, uint32_t &used_mem)
{
  chunks_count = chunks.size();
  used_mem = allocated_mem = 0;
  for (auto &c : chunks)
  {
    allocated_mem += c.getChunkSize();
    used_mem += c.getUsedSize();
  }
  allocated_mem *= blockSize;
  used_mem *= blockSize;
}

inline bool FixedBlockAllocatorIndexed::isValidIndex(uint32_t index) const
{
  uint32_t chunkId;
  uint16_t blockId;
  decompose_index(index, blockId, chunkId);
  return chunkId < chunks.size() && blockId < chunks[chunkId].getChunkSize();
}

inline uint32_t FixedBlockAllocatorIndexed::getNextIndex() const
{
  if (freeBlocksLeft)
  {
    if (chunks[allocatingHead].spaceLeft())
      return make_index(chunks[allocatingHead].getFreeHead(), allocatingHead);

    for (const Chunk *c = chunks.end() - 1, *b = chunks.begin() - 1; c != b; --c)
      if (c->spaceLeft())
        return make_index(c->getFreeHead(), c - (b + 1));
  }
  return make_index(0, chunks.size());
}

inline void *FixedBlockAllocatorIndexed::getPointer(uint32_t index)
{
  uint32_t chunkId;
  uint16_t blockId;
  decompose_index(index, blockId, chunkId);
  return (chunkId < chunks.size() && blockId < chunks[chunkId].getChunkSize()) ? chunks[chunkId].blocks + blockSize * blockId
                                                                               : nullptr;
}

inline void *FixedBlockAllocatorIndexed::getPointerUnsafe(uint32_t index)
{
  uint32_t chunkId;
  uint16_t blockId;
  decompose_index(index, blockId, chunkId);
  G_ASSERTF(chunkId < chunks.size() && blockId < chunks[chunkId].getChunkSize(), "index %d (chunk %d < %d, block %d < %d)", index,
    chunkId, chunks.size(), blockId, chunkId < chunks.size() ? chunks[chunkId].getChunkSize() : -1);
  // todo: check if poisoned with 0x7ffddddd
  return chunks[chunkId].blocks + blockSize * blockId;
}

inline bool FixedBlockAllocatorIndexed::freeContiguousBlocks(uint32_t b, uint16_t block_cnt)
{
  G_FAST_ASSERT(isInited());
  if (!block_cnt)
    return false;
  uint32_t chunkId;
  uint16_t blockId;
  decompose_index(b, blockId, chunkId);
  G_ASSERT_RETURN(chunkId < chunks.size(), false);
  auto &chunk = chunks[chunkId];
  G_ASSERT_RETURN(blockId < chunk.getChunkSize() && block_cnt <= chunk.getUsedSize(), false);
  const uintptr_t blockPtr = blockId;
  FBA_DEBUG_FILL_MEM(chunk.blocks + blockId * blockSize, blockSize * block_cnt, 0x7ffddddd);
  if (DAGOR_LIKELY(!chunk.freeBlocks(blockId, blockSize, block_cnt)))
    return false;
  // chunk is empty removed.
  if (&chunk == &chunks.back())
    popChunkHead();
  return true;
}

inline bool FixedBlockAllocatorIndexed::freeOneBlock(uint32_t b)
{
  uint32_t chunkId;
  uint16_t blockId;
  decompose_index(b, blockId, chunkId);
  G_ASSERT_RETURN(chunkId < chunks.size(), false);
  auto &chunk = chunks[chunkId];
  G_ASSERT_RETURN(blockId < chunk.getChunkSize() && chunk.getUsedSize(), false);

  FBA_DEBUG_FILL_MEM(chunk.blocks + blockId * blockSize, blockSize, 0x7ffddddd);
  if (DAGOR_LIKELY(!chunk.freeOneBlock(blockId, blockSize)))
    return false;
  // chunk is empty removed.
  if (&chunk == &chunks.back())
    popChunkHead();
  return true;
}

inline FixedBlockAllocatorIndexed::FixedBlockAllocatorIndexed(uint32_t block_size, uint16_t min_chunk_size) :
  blockSize(max<uint32_t>(block_size, sizeof(Chunk::freeBlock))), minChunkSize(max<uint16_t>(min_chunk_size, 1))
{}

inline void FixedBlockAllocatorIndexed::init(uint32_t block_size, uint16_t min_chunk_size)
{
  G_ASSERTF(!blockSize, "%s should be called before first allocation", __FUNCTION__);
  blockSize = max<uint32_t>(block_size, sizeof(Chunk::freeBlock));
  minChunkSize = max<uint16_t>(min_chunk_size, 1);
}

inline FixedBlockAllocatorIndexed::FixedBlockAllocatorIndexed(FixedBlockAllocatorIndexed &&a) :
  chunks((dag::Vector<Chunk> &&)a.chunks),
  blockSize(a.blockSize),
  minChunkSize(a.minChunkSize),
  allocatingHead(a.allocatingHead),
  freeBlocksLeft(a.freeBlocksLeft)
{
  a.allocatingHead = ~0u;
  a.freeBlocksLeft = 0;
}

inline FixedBlockAllocatorIndexed &FixedBlockAllocatorIndexed::operator=(FixedBlockAllocatorIndexed &&a)
{
  eastl::swap(chunks, a.chunks);
  eastl::swap(blockSize, a.blockSize);
  eastl::swap(minChunkSize, a.minChunkSize);
  eastl::swap(allocatingHead, a.allocatingHead);
  eastl::swap(freeBlocksLeft, a.freeBlocksLeft);
  return *this;
}

inline constexpr uint32_t FixedBlockAllocatorIndexed::align_to_if_bigger(uint32_t block, uint32_t alignment)
{
  return block <= alignment ? block : alignment * ((block + alignment - 1) / alignment);
}
#undef FBA_DEBUG_FILL_MEM
#undef FBA_MIN4K
#undef FBA_POISON_MEMORY
