//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <util/dag_globDef.h>
#include <util/dag_stdint.h>
#include <memory/dag_memBase.h>
#include <EASTL/utility.h> // swap

// sizeof FixedBlockChunkData is 8 + sizeof(char*)

struct FixedBlockChunkData
{
  typedef uint16_t free_list_t;
  static constexpr int MAX_CHUNK_SIZE = 32768; // there is enough bits 15 bit used for valid index, 16 bit freeBlock, 16 bit chunkSize
  char *blocks = nullptr;
  uint32_t used = 0;
  uint16_t chunkSize = 0;
  uint16_t freeBlock = 0;

  uint32_t getChunkSize() const { return chunkSize; } // at least of size one. could be used with shifts if pow-of-2
  uint32_t getUsedSize() const { return used; }       // at least one block should be always allocated (for 64k blocks)
  free_list_t getFreeHead() const { return freeBlock; }
  int spaceLeft() const { return getChunkSize() - getUsedSize(); }
  // uint32_t getInitialEnd() const { return initialEnd; }
  free_list_t allocateOne(uint32_t block_size) // we need to pass blocks and block_size, so we can use freelist within blocks
  {
    G_ASSERTF(spaceLeft() > 0, "FixedAllocator %p: chunk is full!", this);
    used++;
    const uint32_t chunkSz = getChunkSize();
    const free_list_t freeHead = getFreeHead();
    G_ASSERTF(freeHead < chunkSz, "FixedAllocator %p:free linked list block in chunk is invalid (%d>=%d)", this, freeBlock, chunkSz);
    G_ASSERTF_RETURN(freeHead != chunkSz, -1, "FixedAllocator %p:no free blocks in chunk, while chunk is not full", this);
    freeBlock = *(free_list_t *)(blocks + block_size * freeHead);
    G_ASSERTF(freeBlock < chunkSz || getUsedSize() == chunkSz, "FixedAllocator %p:free linked list block in chunk is invalid (%d>=%d)",
      this, freeBlock, chunkSz);

    return freeHead;
  }
  uint32_t allocateBlocks(uint32_t block_size, uint32_t block_cnt) // we need to pass blocks and block_size, so we can use freelist
                                                                   // within blocks
  {
    G_ASSERTF(spaceLeft() >= block_cnt, "FixedAllocator %p: chunk is full!", this);
    const uint32_t chunkSz = getChunkSize();
    uint32_t preFirstFree = chunkSz, firstFree = getFreeHead(), lastFree = firstFree;
    G_ASSERTF(firstFree < chunkSz, "FixedAllocator %p:free linked list block in chunk is invalid (%d>=%d)", this, freeBlock, chunkSz);
    while (lastFree - firstFree + 1 < block_cnt)
    {
      free_list_t next_b = *(free_list_t *)(blocks + block_size * lastFree);
      if (next_b >= chunkSz)
        return chunkSz;
      else if (next_b == lastFree + 1)
        lastFree++;
      else
      {
        preFirstFree = lastFree;
        firstFree = lastFree = next_b;
      }
    }

    if (preFirstFree == chunkSz)
      freeBlock = *(free_list_t *)(blocks + block_size * lastFree);
    else
      *(free_list_t *)(blocks + block_size * preFirstFree) = *(free_list_t *)(blocks + block_size * lastFree);
    used += block_cnt;
    return firstFree;
  }
  bool freeBlocks(free_list_t block, uint32_t block_size, uint32_t block_cnt) // we need to pass blocks and block_size, so we can use
                                                                              // freelist within blocks
  {
    G_ASSERTF_RETURN(block < getChunkSize() && block_cnt <= getUsedSize(), false, "FixedAllocator %p: block is out of range", this);
#if DAGOR_DBGLEVEL > 1
    for (free_list_t check = freeBlock, chunkSz = getChunkSize(); check < chunkSz;
         check = *(free_list_t *)(blocks + block_size * check))
      if (check >= block && check < block + block_cnt)
        G_ASSERTF_RETURN(0, false, "FixedAllocator %p(blockSize=%d): block (%d;%d) is already deleted!", this, block_size, block,
          block_cnt);
#endif
    used -= block_cnt;
    block += block_cnt - 1;
    do
    {
      *(free_list_t *__restrict)(blocks + block_size * block) = freeBlock; // old head of linked goes to freed block
      freeBlock = block;                                                   // change head of linked list
      --block;
    } while (--block_cnt);
    return used == 0;
  }

  bool freeOneBlock(free_list_t block, uint32_t block_size)
  {
    G_ASSERTF_RETURN(block < getChunkSize() && getUsedSize(), false, "FixedAllocator %p: block is out of range", this);
#if DAGOR_DBGLEVEL > 1
    for (free_list_t check = freeBlock, chunkSz = getChunkSize(); check < chunkSz;
         check = *(free_list_t *)(blocks + block_size * check))
      if (check == block)
        G_ASSERTF_RETURN(0, false, "FixedAllocator %p(blockSize=%d): block (%d) is already deleted!", this, block_size, block);
#endif
    *(free_list_t *__restrict)(blocks + block_size * block) = freeBlock; // old head of linked goes to freed block
    freeBlock = block;                                                   // change head of linked list
    return --used == 0;
  }

  FixedBlockChunkData() = default;
  FixedBlockChunkData(FixedBlockChunkData &&a)
  {
    memcpy(this, &a, sizeof(FixedBlockChunkData));
    a.blocks = nullptr;
  }
  // Chunk &operator =(Chunk &&a){swap(*this, a);}
  FixedBlockChunkData &operator=(const FixedBlockChunkData &) = delete;
  FixedBlockChunkData(const FixedBlockChunkData &) = delete;
  void create(char *b, uint32_t block_size, uint32_t chunk_size)
  {
    G_ASSERT(chunk_size <= MAX_CHUNK_SIZE);
    blocks = b;
    chunkSize = chunk_size < MAX_CHUNK_SIZE ? chunk_size : MAX_CHUNK_SIZE;
    setFree(block_size);
  }
  void setFree(uint32_t block_size)
  {
    G_FAST_ASSERT(block_size >= sizeof(freeBlock));
    freeBlock = 0;
    char *b = blocks;
    for (free_list_t i = 0, e = chunkSize; i < e; ++i, b += block_size)
      *((free_list_t *)b) = i + 1;
  }

protected:
  ~FixedBlockChunkData() {}
};

struct FixedBlockChunk : public FixedBlockChunkData
{
  ~FixedBlockChunk() { memfree_anywhere(blocks); }
  FixedBlockChunk() : FixedBlockChunkData() {}
  FixedBlockChunk(FixedBlockChunk &&a) : FixedBlockChunkData((FixedBlockChunkData &&)a) {}
  FixedBlockChunk &operator=(const FixedBlockChunk &) = delete;
  FixedBlockChunk &operator=(FixedBlockChunk &&a)
  {
    G_STATIC_ASSERT(sizeof(FixedBlockChunk) == sizeof(FixedBlockChunkData));
    eastl::swap(a.blocks, blocks);
    eastl::swap(a.used, used);
    eastl::swap(a.chunkSize, chunkSize);
    eastl::swap(a.freeBlock, freeBlock);
    return *this;
  }
  FixedBlockChunk(const FixedBlockChunk &) = delete;
  FixedBlockChunk(uint32_t block_size, uint32_t chunk_size)
  {
    G_ASSERT(chunk_size <= MAX_CHUNK_SIZE);
    chunkSize = chunk_size < MAX_CHUNK_SIZE ? chunk_size : MAX_CHUNK_SIZE;
    blocks = (char *)memalloc(block_size * chunkSize, tmpmem);
    setFree(block_size);
  }
};
