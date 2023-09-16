#pragma once

#include <memory/stackRingMemAlloc.h>

namespace dabfg
{

extern StackRingMemAlloc<DEFAULT_ALLOCATOR_SIZE, DEFAULT_ALIGNMENT> &get_per_compilation_memalloc();

class PerCompilationAllocator
{
public:
  explicit PerCompilationAllocator(const char * /*pName*/ = ""){};

  friend __forceinline bool operator==(PerCompilationAllocator, PerCompilationAllocator) { return true; }
  friend __forceinline bool operator!=(PerCompilationAllocator, PerCompilationAllocator) { return false; }

  void *allocate(size_t n, int /*flags*/ = 0) { return get_per_compilation_memalloc().alloc(n); };
  void *allocate(size_t n, size_t alignment, size_t /*offset*/, int /*flags*/ = 0)
  {
    return get_per_compilation_memalloc().allocAligned(n, alignment);
  };
  void deallocate(void *p, size_t /*n*/) { get_per_compilation_memalloc().free(p); };

  const char *get_name() const { return "framegraph_debug_allocator"; };
  void set_name(const char * /*pName*/){};
};

} // namespace dabfg
