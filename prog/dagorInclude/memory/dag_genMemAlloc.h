//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <memory/dag_mem.h>


/// @addtogroup memory
/// @{


/// @file
/// Allocator classes for SmallTab.


#define DECLARE_MEMALLOC(NAME, MEM)                                                                                           \
  struct NAME                                                                                                                 \
  {                                                                                                                           \
    NAME() {}                                                                                                                 \
    NAME(const char *) {}                                                                                                     \
    static inline IMemAlloc *getMem() { return MEM; }                                                                         \
    static inline void *alloc(int sz) { return MEM->alloc(sz); }                                                              \
    static inline void free(void *p) { return MEM->free(p); }                                                                 \
    static inline void *allocate(size_t n, int /*flags*/ = 0) { return MEM->alloc(n); }                                       \
    static inline void *allocate(size_t n, size_t al, size_t /*ofs*/, int /*flags*/ = 0) { return MEM->allocAligned(n, al); } \
    static inline void deallocate(void *p, size_t) { MEM->free(p); }                                                          \
    static inline bool resizeInplace(void *p, size_t sz) { return MEM->resizeInplace(p, sz); }                                \
    static inline void *realloc(void *p, size_t sz) { return MEM->realloc(p, sz); }                                           \
    static inline void set_name(const char *) {}                                                                              \
  }

DECLARE_MEMALLOC(MidmemAlloc, midmem);
DECLARE_MEMALLOC(InimemAlloc, inimem);
DECLARE_MEMALLOC(TmpmemAlloc, tmpmem);
DECLARE_MEMALLOC(StrmemAlloc, strmem);
DECLARE_MEMALLOC(UimemAlloc, uimem);

#undef DECLARE_MEMALLOC

/// @}
