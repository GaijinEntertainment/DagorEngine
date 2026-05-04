// Copyright (C) Gaijin Games KFT.  All rights reserved.

#if _TARGET_PC_WIN
#include <windows.h>
#define DAG_OVERRIDE_MALLOC_FAMILY  1
#define DAG_IMPLEMENT_STRDUP_FAMILY 1
#define DAG_DECLARE_IMPORT_STUBS    _DLL
#elif _TARGET_APPLE
#include <malloc/malloc.h>
static void real_init_memory() {}
#endif
#include "../mallocMinAlignment.h"


#if USE_DLMALLOC
#include <dlmalloc/dlmalloc_usable_size.h>
#else
static inline size_t dagmem_malloc_usable_size(void *mem) { return stdmem->getSize(mem); }
#endif

#if !_TARGET_PC_LINUX
#include <string.h>
static inline void *dagmem_malloc(size_t sz) { return stdmem->alloc(sz); }
static inline void *dagmem_calloc(size_t nelem, size_t elemsz)
{
#if MALLOC_MIN_ALIGNMENT < 16
  void *ptr = stdmem->allocAligned(nelem * elemsz, MALLOC_MIN_ALIGNMENT);
#else
  void *ptr = stdmem->alloc(nelem * elemsz);
#endif
  if (ptr)
    memset(ptr, 0, nelem * elemsz);
  return ptr;
}
static inline void *dagmem_realloc(void *ptr, size_t sz) { return stdmem->realloc(ptr, sz); }
static inline void *dagmem_recalloc(void *ptr, size_t nelem, size_t elemsz)
{
  size_t old_size = dagmem_malloc_usable_size(ptr);
  size_t new_size = nelem * elemsz;

  void *new_p = stdmem->realloc(ptr, new_size);

  // If the reallocation succeeded and the new block is larger, zero-fill the new bytes:
  if (new_p && old_size < new_size)
    memset(static_cast<char *>(new_p) + old_size, 0, new_size - old_size);
  return new_p;
}
static inline void *dagmem_expand(void *const block, size_t const size)
{
  return stdmem->resizeInplace(block, size) ? block : nullptr;
}
static inline void dagmem_free(void *ptr) { stdmem->free(ptr); }
static inline void *dagmem_memalign(size_t al, size_t sz) { return stdmem->allocAligned(sz, al); }
static inline void dagmem_free_aligned(void *ptr) { stdmem->freeAligned(ptr); }

// implement stubs for different compiler runtime
#include "../stubRtlAlloc.inc.cpp"
#if _TARGET_PC_WIN && USE_RTL_ALLOC
MEMEXP3 size_t __cdecl _msize_base(void *const mem) MEMEXP_NOEXCEPT { return dagmem_malloc_usable_size(mem); }
MEMEXP void *__cdecl _recalloc_base(void *ptr, size_t nelem, size_t elemsz) { return _recalloc(ptr, nelem, elemsz); }
#endif
#endif

int pull_rtlOverride_stubRtl = 0;
