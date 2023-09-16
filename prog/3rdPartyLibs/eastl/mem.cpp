#include <EASTL/internal/config.h>
#include <stdint.h>

void* operator new[](size_t size, const char*, int, unsigned, const char*, int)
{
  return new char[size];
}

void* operator new[](size_t size, size_t alignment, size_t, const char*, int, unsigned, const char*, int)
{
  EA_UNUSED(alignment);
  EASTL_ASSERT(alignment <= 16); // alignment of 16 is guaranteed by dagor's implementation of memory allocator
  void *p = new char[size];
  EASTL_ASSERT((uintptr_t(p) & (alignment-1)) == 0);
  return p;
}
