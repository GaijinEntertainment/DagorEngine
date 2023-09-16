#pragma once
#include <EASTL/unique_ptr.h>
#include <EASTL/vector.h>

#ifndef RESTRICT_FUN
#ifdef _MSC_VER
#define RESTRICT_FUN __declspec(restrict)
#else
#define RESTRICT_FUN
#endif
#endif

// this is simple allocator, which works almost as region buffer (chain of chunks of equal size).
// however, if you work with it as stack (alloc0,alloc1,..allocN | deallocN,..alloc1,alloc0)
// it will perform like a stack (not allocating new data)
// it never deallocates memory, until Garbage collection called.
// it will always have at least one chunk of allocated data
// it always return pointers to aligned data (although we assume that new is aligned to same alignment. This assumption be very easily
// removed, see assert)

template <unsigned CHUNK_SIZE_BITS = 16, unsigned ALIGN_TO_BITS = 4>
struct CreationAllocator
{
  enum
  {
    ALIGN_TO = 1 << ALIGN_TO_BITS,
    ALIGN_TO_MASK = ALIGN_TO - 1
  }; // pow-of-two size
  enum
  {
    CHUNK_SIZE = 1 << CHUNK_SIZE_BITS,
    CHUNK_MASK = CHUNK_SIZE - 1
  }; // pow-of-two size
  struct CreationChunk
  {
    eastl::unique_ptr<uint8_t[]> data = eastl::make_unique<uint8_t[]>(CHUNK_SIZE);
    uint32_t allocated = 0;
    uint32_t freed = 0;
  };
  eastl::vector<CreationChunk> chunks;
  void pushChunk()
  {
    chunks.emplace_back();
    G_ASSERTF((uintptr_t(chunks.back().data.get()) & ALIGN_TO_MASK) == 0,
      "this assumption can be removed, by adjusting .allocated and .freed");
    // chunks.back().allocated += uintptr_t(chunks.back().data.get())&ALIGN_TO_MASK;
    // chunks.back().freed += uintptr_t(chunks.back().data.get())&ALIGN_TO_MASK;
  }
  CreationAllocator() { pushChunk(); }

  RESTRICT_FUN uint8_t *__restrict allocate(uint32_t size)
  {
    size = (size + ALIGN_TO_MASK) & ~ALIGN_TO_MASK; // align to 16
    G_ASSERT(size <= CHUNK_SIZE);
    if (chunks.back().allocated + size > CHUNK_SIZE)
      pushChunk();
    CreationChunk &chunk = chunks.back();
    uint8_t *data = chunk.data.get() + chunk.allocated;
    chunk.allocated += size;
    return data;
  }
  void deallocate(uint8_t *__restrict data, uint32_t size)
  {
    size = (size + ALIGN_TO_MASK) & ~ALIGN_TO_MASK; // align to 16
    eastl::find_if(chunks.rbegin(), chunks.rend(), [data, size](CreationChunk &a) -> bool {
      const uintptr_t ofs = (data - a.data.get());
      if ((ofs & ~uintptr_t(CHUNK_MASK)) != 0)
        return false;
      if (ofs == a.allocated - size) // just pop allocated data
        a.allocated = a.allocated - size;
      else
        a.freed += size;
      return true;
    });
  }
  void gc()
  {
    for (auto i = chunks.rbegin(), ei = chunks.rend() - 1; i != ei; ++i)
      if (i->freed == i->allocated || i->allocated == 0)
        chunks.erase_unsorted(i);
    G_ASSERT(chunks.size() >= 1);
    if (chunks.begin()->freed == chunks.begin()->allocated)
      chunks.begin()->allocated = chunks.begin()->freed = 0;
  }

  size_t calcMemAllocated() const // for debug/inspection
  {
    size_t sz = 0;
    for (const auto &ch : chunks)
      sz += ch.allocated;
    return sz;
  }
  size_t calcMemFreed() const // for debug/inspection
  {
    size_t sz = 0;
    for (const auto &ch : chunks)
      sz += ch.freed;
    return sz;
  }
  void clear()
  {
    chunks.clear();
    pushChunk();
  }
};
