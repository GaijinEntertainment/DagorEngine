#pragma once

#include "block.h"

#include <generic/dag_tab.h>
#include <generic/dag_carray.h>
#include <util/dag_stlqsort.h>


struct BlockPoolAllocator
{
  static constexpr int MAX_BLOCKS_POOLS = 48;
  static constexpr int ALIGNMENT = 3;
  struct FreeBlock
  {
    int start, size;
    FreeBlock() : start(0), size(0) {}
    FreeBlock(int st, int sz) : start(st), size(sz) {}
  };

  struct Pool
  {
    Tab<FreeBlock> blocks;
    int size;
  };
  carray<Pool, MAX_BLOCKS_POOLS> blockPools;
  enum
  {
    FRAGMENTED = 0,
    BIGGEST = 1,
    TOTAL
  };
  struct CurrentFreeBlock
  {
    int poolId, blockId;
    CurrentFreeBlock() : poolId(-1), blockId(-1) {}
    CurrentFreeBlock(int p, int b) : poolId(p), blockId(b) {}
  } currentFreeBlock[TOTAL];

  FreeBlock *getFreeBlock(const CurrentFreeBlock &block) const
  {
    return block.blockId < 0 ? nullptr : const_cast<FreeBlock *>(&blockPools[block.poolId].blocks[block.blockId]);
  }
  FreeBlock *getFragmentedFreeBlock() const { return getFreeBlock(currentFreeBlock[FRAGMENTED]); }
  FreeBlock *getBiggestFreeBlock() const { return getFreeBlock(currentFreeBlock[BIGGEST]); }
  // int currentTexture, currentTextureFreeBlock;
  void free(int size, AllocatedPoolBlock &block)
  {
    if (block.pool < 0 || block.pool >= blockPools.size())
    {
      // G_ASSERT(size == 0);
      return; // attempt free on freed block
    }
    if (size < 0 || block.size < 0)
    {
      logerr("trying to free negative size %d, blocksize %d", size, block.size);
    }
    G_ASSERTF(block.size == align_size(block.start, size), "trying to free %d out of %d", size, block.size);
    if (block.size != align_size(block.start, size))
      logerr("trying to free %d out of %d", size, block.size);
    (void)size;
    if (block.size > 0)
      blockPools[block.pool].blocks.push_back(FreeBlock(block.start, block.size));
    block = AllocatedPoolBlock();
  } // return
  static int align_start(int start) { return start + (ALIGNMENT - start % ALIGNMENT) % ALIGNMENT; }
  static int align_size(int start, int size) { return size + (ALIGNMENT - start % ALIGNMENT) % ALIGNMENT; }
  int freeBlockAlignedSize() const { return align_size(getFragmentedFreeBlock()->start, getFragmentedFreeBlock()->size); }
  bool allocate(int size, AllocatedPoolBlock &block) // return
  {
    G_ASSERT(block.pool < 0);
    G_ASSERT(size > 0);
    if (size <= 0)
      return true;
    if (!getFragmentedFreeBlock() || align_size(getFragmentedFreeBlock()->start, size) > getFragmentedFreeBlock()->size)
    {
      defragmentation();
      if (align_size(getFragmentedFreeBlock()->start, size) > getFragmentedFreeBlock()->size)
        currentFreeBlock[FRAGMENTED] = currentFreeBlock[BIGGEST];
    }
    return allocateInternal(size, block);
  }

  void close()
  {
    currentFreeBlock[FRAGMENTED] = currentFreeBlock[BIGGEST] = CurrentFreeBlock();
    for (int i = 0; i < blockPools.size(); ++i)
      clear_and_shrink(blockPools[i].blocks);
  }

  void init(int blockSize, int blocksPoolsCnt)
  {
    close();
    G_ASSERT(blocksPoolsCnt <= MAX_BLOCKS_POOLS);
    blocksPoolsCnt = min(blocksPoolsCnt, (int)MAX_BLOCKS_POOLS);
    for (int i = 0; i < blocksPoolsCnt; ++i)
    {
      blockPools[i].blocks.push_back(FreeBlock(0, blockSize));
      blockPools[i].size = blockSize;
    }
    if (blocksPoolsCnt)
      currentFreeBlock[FRAGMENTED] = currentFreeBlock[BIGGEST] = CurrentFreeBlock(0, 0);
  }

  int add_pool(int blockSize) // return poolNo or -1 if everything is busy
  {
    for (int i = 0; i < blockPools.size(); ++i)
    {
      if (blockPools[i].blocks.size())
        continue;
      blockPools[i].blocks.push_back(FreeBlock(0, blockSize));
      currentFreeBlock[BIGGEST] = CurrentFreeBlock(i, 0);
      return i;
    }
    return -1;
  }

protected:
  // private
  bool allocateInternal(int size, AllocatedPoolBlock &block)
  {
    FreeBlock *freeBlock = getFragmentedFreeBlock();
    if (!freeBlock)
      return false;
    if (align_size(freeBlock->start, size) > freeBlock->size)
    {
      return false;
    }

    block.pool = currentFreeBlock[FRAGMENTED].poolId;
    block.start = freeBlock->start;
    block.size = align_size(freeBlock->start, size);
    G_ASSERT(size > 0);
    G_ASSERT(block.size > 0 && block.size >= size);
    freeBlock->start += block.size;
    freeBlock->size -= block.size;
    G_ASSERT(freeBlock->size >= 0);
    return true;
  }
  struct BlockAscend
  {
    bool operator()(const FreeBlock &a, const FreeBlock &b) const { return a.start < b.start; }
  };
  void selectBest(CurrentFreeBlock *current, int pool, int block)
  {
    int blockSize = blockPools[pool].blocks[block].size;
    if (current[BIGGEST].poolId < 0 || getFreeBlock(current[BIGGEST])->size < blockSize)
      current[BIGGEST] = CurrentFreeBlock(pool, block);
    if (blockSize != blockPools[pool].size)
      if (current[FRAGMENTED].poolId < 0 || getFreeBlock(current[FRAGMENTED])->size < blockSize)
        current[FRAGMENTED] = CurrentFreeBlock(pool, block);
  }
  void defragmentation()
  {
    CurrentFreeBlock nextFree[TOTAL];
    for (int i = 0; i < blockPools.size(); ++i)
    {
      if (!blockPools[i].blocks.size())
        continue;
      if (blockPools[i].blocks.size() == 1)
      {
        selectBest(nextFree, i, 0);
        continue;
      }
      // concatenate blocks
      Tab<FreeBlock> newBlocks;
      stlsort::sort(blockPools[i].blocks.data(), blockPools[i].blocks.data() + blockPools[i].blocks.size(), BlockAscend());
      newBlocks.reserve(blockPools[i].blocks.size());
      newBlocks.push_back(blockPools[i].blocks[0]);
      int maxBlockSz = newBlocks.back().size, maxBlockId = 0;
      for (int fi = 1; fi < blockPools[i].blocks.size(); ++fi)
      {
        if (blockPools[i].blocks[fi].size == 0)
          continue; // empty block
        if (newBlocks.back().start + newBlocks.back().size > blockPools[i].blocks[fi].start || blockPools[i].blocks[fi].size < 0)
        {
          logerr("inconsistent data! pool = %d blocks = %d, current block = %d (start=%d size = %d)"
                 ", concatenated block start = %d, size = %d",
            i, blockPools[i].blocks.size(), fi, blockPools[i].blocks[fi].start, blockPools[i].blocks[fi].size, newBlocks.back().start,
            newBlocks.back().size);
          for (int fi2 = 0; fi2 < blockPools[i].blocks.size(); ++fi2)
          {
            logerr("block:[%d] start=%d size = %d", fi2, blockPools[i].blocks[fi2].start, blockPools[i].blocks[fi2].size);
          }
        }
        G_ASSERT(newBlocks.back().start + newBlocks.back().size <= blockPools[i].blocks[fi].start);
        if (blockPools[i].blocks[fi].size <= 0)
          continue; //== should not ever happen!
        if (newBlocks.back().start + newBlocks.back().size == blockPools[i].blocks[fi].start)
          newBlocks.back().size += blockPools[i].blocks[fi].size;
        else
          newBlocks.push_back(blockPools[i].blocks[fi]);

        if (maxBlockSz < newBlocks.back().size)
        {
          maxBlockSz = newBlocks.back().size;
          maxBlockId = newBlocks.size() - 1;
        }
      }
      blockPools[i].blocks = newBlocks;
      selectBest(nextFree, i, maxBlockId);
    }
    currentFreeBlock[FRAGMENTED] = nextFree[FRAGMENTED];
    currentFreeBlock[BIGGEST] = nextFree[BIGGEST];
    if (!getFragmentedFreeBlock())
      currentFreeBlock[FRAGMENTED] = currentFreeBlock[BIGGEST];
  }
};