// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <util/dag_compilerDefs.h>
#include "mallocMinAlignment.h"

#if USE_DLMALLOC // dlmalloc allocator
#include <dlmalloc/dlmalloc-2.8.4.h>
static inline size_t dagmem_malloc_usable_size(void *p) { return dlmalloc_usable_size(p); }
static inline void *dagmem_malloc_base(size_t sz) { return dlmalloc(sz); }
static inline void *dagmem_calloc_base(size_t elem_count, size_t elem_sz) { return dlcalloc(elem_count, elem_sz); }
static inline void *dagmem_memalign_base(size_t alignment, size_t sz) { return dlmemalign(alignment, sz); }
static inline void *dagmem_valloc_base(size_t sz) { return dlvalloc(sz); }
static inline void *dagmem_realloc_base(void *p, size_t sz) { return dlrealloc(p, sz); }
static inline void *dagmem_expand_base(void *p, size_t sz) { return dlresize_inplace(p, sz); }
static inline void dagmem_free_base(void *p) { dlfree(p); }
extern "C" size_t get_memory_committed();
extern "C" size_t get_memory_committed_max();

#elif USE_MIMALLOC // mimalloc allocator
#include <mimalloc.h>

inline size_t dagmem_malloc_usable_size(void *p) { return mi_usable_size(p); }
inline void *dagmem_malloc_base(size_t sz) { return mi_malloc(sz); }
inline void *dagmem_realloc_base(void *p, size_t sz) { return mi_realloc(p, sz); }
inline void *dagmem_calloc_base(size_t n, size_t c) { return mi_calloc(n, c); }
inline void dagmem_free_base(void *p) { mi_free(p); }
inline void *dagmem_expand_base(void *p, size_t sz) { return mi_expand(p, sz); }
inline void *dagmem_memalign_base(size_t al, size_t sz) { return mi_malloc_aligned(sz, al); }
inline void *dagmem_valloc_base(size_t sz) { return mi_malloc_aligned(sz, 4096); }

#elif USE_RTL_ALLOC // pure RTL-based allocator
#if _TARGET_APPLE
#include <malloc/malloc.h>
#if !DAGOR_ADDRESS_SANITIZER && !DAGOR_THREAD_SANITIZER
extern void *(*dagmem_orig_ptr_malloc)(size_t);
extern void *(*dagmem_orig_ptr_calloc)(size_t, size_t);
extern int (*dagmem_orig_ptr_posix_memalign)(void **, size_t, size_t);
extern void *(*dagmem_orig_ptr_aligned_alloc)(size_t, size_t);
extern void *(*dagmem_orig_ptr_valloc)(size_t);
extern void *(*dagmem_orig_ptr_realloc)(void *, size_t);
extern void (*dagmem_orig_ptr_free)(void *);
#define MALLOC_BASE         dagmem_orig_ptr_malloc
#define CALLOC_BASE         dagmem_orig_ptr_calloc
#define POSIX_MEMALIGN_BASE dagmem_orig_ptr_posix_memalign
#define ALIGNED_ALLOC_BASE  dagmem_orig_ptr_aligned_alloc
#define VALLOC_BASE         dagmem_orig_ptr_valloc
#define REALLOC_BASE        dagmem_orig_ptr_realloc
#define FREE_BASE           dagmem_orig_ptr_free

#define DAGMEM_BASE_REDIRECTOR_FUNCS_DEFINED 1
#endif

#elif _TARGET_PC_LINUX
#include <malloc.h>
#if !DAGOR_ADDRESS_SANITIZER && !DAGOR_THREAD_SANITIZER
extern "C" void *__libc_malloc(size_t);
extern "C" void *__libc_calloc(size_t, size_t);
extern "C" void *__libc_memalign(size_t, size_t);
extern "C" void *__libc_valloc(size_t);
extern "C" void *__libc_realloc(void *, size_t);
extern "C" void __libc_free(void *);
#define MALLOC_BASE   __libc_malloc
#define CALLOC_BASE   __libc_calloc
#define MEMALIGN_BASE __libc_memalign
#define VALLOC_BASE   __libc_valloc
#define REALLOC_BASE  __libc_realloc
#define FREE_BASE     __libc_free

#define DAGMEM_BASE_REDIRECTOR_FUNCS_DEFINED 1
#endif

#elif _TARGET_C3

#endif
#include <string.h> // memcpy
#ifdef __GNUC__
#include <stdlib.h>
#else
#include <malloc.h>
#endif
#if _TARGET_ANDROID
#include <android/api-level.h>
#if __ANDROID_API__ > 19 || defined(__clang__)
inline void *valloc(size_t sz) { return memalign(4096, sz); }
#endif
#endif

#if !DAGMEM_BASE_REDIRECTOR_FUNCS_DEFINED
#define MALLOC_BASE         malloc
#define CALLOC_BASE         calloc
#define MEMALIGN_BASE       memalign
#define POSIX_MEMALIGN_BASE posix_memalign
#define ALIGNED_ALLOC_BASE  aligned_alloc
#define VALLOC_BASE         valloc
#define REALLOC_BASE        realloc
#define FREE_BASE           free
#endif

#if _TARGET_C1 | _TARGET_C2 | _TARGET_PC_LINUX | _TARGET_C3
inline size_t dagmem_malloc_usable_size(void *p) { return malloc_usable_size(p); }
#elif _TARGET_APPLE
inline size_t dagmem_malloc_usable_size(void *p) { return malloc_size(p); }
#elif _TARGET_ANDROID
inline size_t dagmem_malloc_usable_size(void *p) { return malloc_usable_size(p); }
#endif

#if (_TARGET_PC_WIN | _TARGET_XBOX)
#include "msRtlAlignedHelper.h"
#else
inline void *dagmem_calloc_base(size_t elem_count, size_t elem_sz) { return CALLOC_BASE(elem_count, elem_sz); }
inline void dagmem_free_base(void *p) { return FREE_BASE(p); }
#endif

#if _TARGET_PC_LINUX | _TARGET_ANDROID
#if _TARGET_64BIT || (defined(__STDCPP_DEFAULT_NEW_ALIGNMENT__) && __STDCPP_DEFAULT_NEW_ALIGNMENT__ >= 16)
inline void *dagmem_malloc_base(size_t sz) { return MALLOC_BASE(sz); }
inline void *dagmem_realloc_base(void *p, size_t sz) { return REALLOC_BASE(p, sz); }
#else
inline void *dagmem_malloc_base(size_t sz) { return MEMALIGN_BASE(16, sz); }
inline void *dagmem_realloc_base(void *p, size_t sz)
{
  void *r = dagmem_malloc_base(sz);
  if (r && p)
  {
    size_t psz = dagmem_malloc_usable_size(p);
    memcpy(r, p, (sz < psz) ? sz : psz);
    dagmem_free_base(p);
  }
  return r;
}
#endif // _TARGET_64BIT
#elif _TARGET_C3


#elif !(_TARGET_PC_WIN | _TARGET_XBOX)
inline void *dagmem_malloc_base(size_t sz) { return MALLOC_BASE(sz); }
inline void *dagmem_realloc_base(void *p, size_t sz) { return REALLOC_BASE(p, sz); }
#endif

#if !(_TARGET_PC_WIN | _TARGET_XBOX)
inline void *dagmem_expand_base(void *p, size_t sz) { return sz <= dagmem_malloc_usable_size(p) ? p : nullptr; }
#endif

#if _TARGET_C1 | _TARGET_C2


#elif _TARGET_PC_WIN | _TARGET_XBOX
inline void *dagmem_valloc_base(size_t sz) { return dagmem_memalign_base(4096, sz); }
#elif _TARGET_PC_LINUX | _TARGET_ANDROID
inline void *dagmem_memalign_base(size_t alignment, size_t sz) { return MEMALIGN_BASE(alignment, sz); }
inline void *dagmem_valloc_base(size_t sz) { return VALLOC_BASE(sz); }
#elif _TARGET_C3


#elif _TARGET_APPLE
#if (_TARGET_IOS && __IPHONE_OS_VERSION_MIN_REQUIRED >= __IPHONE_13_0) || \
  (_TARGET_TVOS && __TV_OS_VERSION_MIN_REQUIRED >= __TVOS_13_0) ||        \
  (_TARGET_PC_MACOSX && __MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_15)
inline void *dagmem_memalign_base(size_t alignment, size_t sz)
{
  if (void *p = alignment > alignof(max_align_t) ? ALIGNED_ALLOC_BASE(alignment, sz) : MALLOC_BASE(sz))
    return p;
  return ALIGNED_ALLOC_BASE(alignment, (sz + alignment - 1) & ~(alignment - 1));
}
#else
inline void *dagmem_memalign_base(size_t alignment, size_t sz)
{
  if (alignment <= alignof(max_align_t))
    return MALLOC_BASE(sz);
  void *p;
  return POSIX_MEMALIGN_BASE(&p, alignment, sz) == 0 ? p : nullptr;
}
#endif
inline void *dagmem_valloc_base(size_t sz) { return VALLOC_BASE(sz); }
#endif

#elif USE_IMPORTED_ALLOC
#include <string.h>

extern IMemAlloc *imported_mem;

static inline size_t dagmem_malloc_usable_size(void *mem) { return imported_mem->getSize(mem); }
static inline void *dagmem_malloc_base(size_t sz) { return imported_mem->alloc(sz); }
static inline void *dagmem_calloc_base(size_t nelem, size_t elemsz)
{
#if MALLOC_MIN_ALIGNMENT < 16
  void *ptr = imported_mem->allocAligned(nelem * elemsz, MALLOC_MIN_ALIGNMENT);
#else
  void *ptr = imported_mem->alloc(nelem * elemsz);
#endif
  if (ptr)
    memset(ptr, 0, nelem * elemsz);
  return ptr;
}
static inline void *dagmem_memalign_base(size_t al, size_t sz) { return imported_mem->allocAligned(sz, al); }
static inline void *dagmem_valloc_base(size_t sz) { return dagmem_memalign_base(4096, sz); }
static inline void *dagmem_realloc_base(void *ptr, size_t sz) { return imported_mem->realloc(ptr, sz); }
static inline void *dagmem_expand_base(void *const block, size_t const size)
{
  return imported_mem->resizeInplace(block, size) ? block : nullptr;
}
static inline void dagmem_free_base(void *ptr) { imported_mem->free(ptr); }
#define DAGMEM_FREE_ALIGNED(PTR) imported_mem->freeAligned(PTR)
#endif

extern int pull_rtlOverride_stubRtl;

#undef DAGMEM_BASE_REDIRECTOR_FUNCS_DEFINED
#undef MALLOC_BASE
#undef CALLOC_BASE
#undef POSIX_MEMALIGN_BASE
#undef ALIGNED_ALLOC_BASE
#undef VALLOC_BASE
#undef REALLOC_BASE
#undef FREE_BASE
