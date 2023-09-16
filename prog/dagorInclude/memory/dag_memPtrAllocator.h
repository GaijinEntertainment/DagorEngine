//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <memory/dag_memBase.h> //IMemAlloc

namespace dag
{
struct MemPtrAllocator;
}
namespace eastl
{
class allocator;
}

// EASTL-compatible allocator interface with pointer to real allocator.
struct dag::MemPtrAllocator
{
  IMemAlloc *m = midmem;
  MemPtrAllocator() = default;
  MemPtrAllocator(IMemAlloc *a) : m(a) {}
  MemPtrAllocator(const char *) : m(midmem) {}

  void *allocate(size_t n, int /*flags*/ = 0) { return m->alloc(n); }
  void *allocate(size_t n, size_t alignment, size_t /*offset*/, int /*flags*/ = 0) { return m->allocAligned(n, alignment); }
  void deallocate(void *p, size_t) { m->free(p); }
  bool resizeInplace(void *p, size_t sz) { return m->resizeInplace(p, sz); }
  void *realloc(void *p, size_t sz) { return m->realloc(p, sz); }
  bool operator==(const MemPtrAllocator &a) const { return m == a.m; }
  bool operator==(const eastl::allocator &) const { return false; }
};
