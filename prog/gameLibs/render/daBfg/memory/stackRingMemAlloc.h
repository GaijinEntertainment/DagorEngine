#pragma once

#include <EASTL/allocator.h>
#include <EASTL/numeric_limits.h>
#include <EASTL/algorithm.h>
#include <EASTL/utility.h>
#include <debug/dag_assert.h>

namespace dabfg
{

inline constexpr size_t aligned_size(size_t sz, size_t align) { return (sz + (align - 1)) & ~(align - 1); }

inline char *aligned_ptr(char *ptr, size_t align)
{
  return reinterpret_cast<char *>(((reinterpret_cast<uintptr_t>(ptr) + (align - 1)) & ~(align - 1)));
}

struct BlockHeader
{
  // The negative sign indicates that the block was freed
  int16_t prevBlockSize;
};

inline constexpr size_t DEFAULT_ALLOCATOR_SIZE = 32768u;
inline constexpr size_t DEFAULT_ALIGNMENT = 16u;
template <size_t capacity = DEFAULT_ALLOCATOR_SIZE, size_t align = DEFAULT_ALIGNMENT, typename DefaultAllocator = EASTLAllocatorType>
class StackRingMemAlloc : private DefaultAllocator
{

  static constexpr size_t ALIGNED_HEADER_SIZE = aligned_size(sizeof(BlockHeader), align);

  BlockHeader readBlockHeader(const char *ptr)
  {
    BlockHeader header;
    memcpy(&header, ptr, sizeof(BlockHeader));
    return header;
  }

  void writeBlockHeader(char *ptr, const BlockHeader *header) { memcpy(ptr, header, sizeof(BlockHeader)); }

public:
  StackRingMemAlloc() { mem = reinterpret_cast<char *>(this->allocate(capacity)); }

  ~StackRingMemAlloc() { this->deallocate(reinterpret_cast<void *>(mem), capacity); }

  void *alloc(size_t sz) { return allocAligned(sz, align); };

  void *allocAligned(size_t sz, size_t alignment)
  {
    G_ASSERT_RETURN(mem, nullptr);
    alignment = eastl::max(alignment, align);
    char *top = mem + size;
    char *alignedResult = aligned_ptr(top + ALIGNED_HEADER_SIZE, alignment);
    const size_t alignedSize = aligned_size(sz, alignment);
    size_t blockSize = alignedResult - top + alignedSize;
    G_FAST_ASSERT(blockSize <= capacity);
    // In general looping memory is not safe, because we can start overwriting old data.
    // But this allocator is supposed to be used for strings in framegraph.
    // And in this case it will not cause such a problem,
    // because strings are allocated during compilation, which means that old
    // names from previous compilations will be freed.
    if (size + blockSize > capacity)
    {
      top = mem;
      size = 0;
      lastBlockOffset = 0;
      alignedResult = aligned_ptr(top + ALIGNED_HEADER_SIZE, alignment);
      blockSize = alignedResult - top + alignedSize;
    }
    G_FAST_ASSERT(blockSize <= eastl::numeric_limits<int16_t>::max());
    BlockHeader header{static_cast<int16_t>(size - lastBlockOffset)};
    writeBlockHeader(top, &header);
    size += blockSize;
    lastBlockOffset = top - mem;
    G_FAST_ASSERT(alignedResult + alignedSize == mem + size);
    return alignedResult;
  }

  void free(void *p)
  {
    if (!p)
      return;

    const auto blockMem = reinterpret_cast<char *>(p);
    char *blockPtr = blockMem - ALIGNED_HEADER_SIZE;
    BlockHeader blockHeader = readBlockHeader(blockPtr);
    G_FAST_ASSERT(blockMem - sizeof(BlockHeader) >= mem);
    G_FAST_ASSERT(blockMem - sizeof(BlockHeader) - mem <= capacity);
    // Mark that block as free by setting its size to a negative value.
    blockHeader.prevBlockSize = -blockHeader.prevBlockSize;
    writeBlockHeader(blockPtr, &blockHeader);
    const uint32_t blockOffset = blockPtr - mem;
    // Decrement top if free block is on top of the stack.
    // We may decrement the top several times if the blocks below have also been released.
    // b1        b2        b3    b4    b5                  top
    // v         v         v     v     v                    v
    // [occupied][occupied][free][free][occupied last block]
    //
    // After free(b5)
    //
    // b1        b2       top
    // v         v         v
    // [occupied][occupied]
    //
    if (blockOffset == lastBlockOffset)
    {
      char *freeBlockPtr = blockPtr;
      BlockHeader freeBlock = blockHeader;
      while (freeBlock.prevBlockSize < 0)
      {
        size = freeBlockPtr - mem;
        freeBlockPtr -= -freeBlock.prevBlockSize;
        freeBlock = readBlockHeader(freeBlockPtr);
      }
      lastBlockOffset = freeBlockPtr - mem;
    }
  }

private:
  size_t size = 0;
  size_t lastBlockOffset = 0;
  char *mem = nullptr;
};

} // namespace dabfg
