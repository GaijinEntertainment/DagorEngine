
#ifndef __DAGOR_MEMORY_STD_RTL_MEM_USAGE_H
#define __DAGOR_MEMORY_STD_RTL_MEM_USAGE_H
#pragma once

#if _TARGET_C1 | _TARGET_C2 | _TARGET_APPLE | _TARGET_PC_LINUX | _TARGET_ANDROID | _TARGET_C3
#define _STD_RTL_MEMORY 1
#elif defined(__clang__) && defined(__has_feature)
#if __has_feature(address_sanitizer)
#define _STD_RTL_MEMORY 1
#endif
#else
// uncomment next line to compile with VC RTL memory manager
//  #define _STD_RTL_MEMORY 1
#endif

#if _TARGET_PC_WIN
#define _DLMALLOC_MSPACE 1
#endif

#if defined(_STD_RTL_MEMORY) && defined(__cplusplus)
#if _TARGET_APPLE
#include <malloc/malloc.h>
#elif _TARGET_PC_LINUX
#include <malloc.h>
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
#endif

#if _TARGET_C1 | _TARGET_C2 | _TARGET_PC_LINUX | _TARGET_C3
inline size_t sys_malloc_usable_size(void *p) { return malloc_usable_size(p); }
#elif _TARGET_APPLE
inline size_t sys_malloc_usable_size(void *p) { return malloc_size(p); }
#elif _TARGET_ANDROID
#if __ANDROID_API__ > 19 || defined(__clang__)
inline void *valloc(size_t sz) { return memalign(4096, sz); }
#endif
inline size_t sys_malloc_usable_size(void *p) { return malloc_usable_size(p); }
#elif _TARGET_PC_WIN && !_TARGET_64BIT && !USE_MIMALLOC
inline size_t sys_malloc_usable_size(void *p) { return p ? _aligned_msize(p, 16, 0) : 0; }
#else
inline size_t sys_malloc_usable_size(void *p) { return p ? _msize(p) : 0; }
#endif

#if _TARGET_PC_LINUX | _TARGET_ANDROID
#if _TARGET_64BIT
inline void *dlmalloc(size_t sz) { return malloc(sz); }
inline void *dlrealloc(void *p, size_t sz) { return realloc(p, sz); }
#else
#error 32bit not supported
#endif // _TARGET_64BIT
#elif _TARGET_PC_WIN && !_TARGET_64BIT && !USE_MIMALLOC
inline void *dlmalloc(size_t sz) { return _aligned_malloc(sz, 16); }
#elif _TARGET_C3

#else
inline void *dlmalloc(size_t sz) { return malloc(sz); }
#endif

#if _TARGET_PC_WIN && !_TARGET_64BIT && !USE_MIMALLOC
inline void dlfree(void *p) { return _aligned_free(p); }
inline void *dlrealloc(void *p, size_t sz) { return _aligned_realloc(p, sz, 16); }
#elif _TARGET_PC_LINUX | _TARGET_ANDROID
inline void dlfree(void *p) { return free(p); }
#else
inline void dlfree(void *p) { return free(p); }
inline void *dlrealloc(void *p, size_t sz) { return realloc(p, sz); }
#endif

#if (_TARGET_PC_WIN | _TARGET_XBOX) && !USE_MIMALLOC
inline void dlfree_aligned(void *p) { return _aligned_free(p); }
inline size_t dlmalloc_usable_size_aligned(void *p)
{
  // _aligned_msize() returns weird values when alignment is unknown, so we calculate usable size ourself
  static constexpr int PTR_SZ = sizeof(void *);
  if (!p)
    return 0;
  /* in non-debug builds ptr is one position behind memblock */
  /* the value in ptr is the start of the real allocated block */
  /* after dereference ptr points to the beginning of the allocated block */
  uintptr_t *ptrPtr = ((uintptr_t *)((uintptr_t(p) & ~(PTR_SZ - 1)) - PTR_SZ));
  uintptr_t ptr = *ptrPtr;

#if DAGOR_DBGLEVEL > 1
  {
/* but in debug builds struct _AlignMemBlockHdr goes before the allocated block */
/* it consists of a pointer to the beginning of the block */
/* and a gap consisting of sizeof(void *) bytes filled with 0xED value */
#ifdef _TARGET_64BIT
    constexpr const uintptr_t aligned_nomansland = 0xEDEDEDEDEDEDEDEDull;
#else
    constexpr const uintptr_t aligned_nomansland = 0xEDEDEDEDul;
#endif
    if (ptr == aligned_nomansland)
    {
      ptrPtr--;
      ptr = *ptrPtr;
    }
  }
#endif

  return _msize((void *)ptr) - (uintptr_t(p) - ptr);
}
#else
inline void dlfree_aligned(void *p) { return free(p); }
inline size_t dlmalloc_usable_size_aligned(void *p) { return sys_malloc_usable_size(p); }
#endif

#if _TARGET_C1 | _TARGET_C2


#elif _TARGET_PC_WIN | _TARGET_XBOX
inline void *dlmemalign(size_t alignment, size_t sz) { return _aligned_malloc(sz, alignment); }
inline void *dlvalloc(size_t sz) { return _aligned_malloc(sz, 4096); }
#elif _TARGET_PC_LINUX | _TARGET_ANDROID
inline void *dlmemalign(size_t alignment, size_t sz) { return memalign(alignment, sz); }
inline void *dlvalloc(size_t sz) { return valloc(sz); }
#elif _TARGET_C3


#else
inline void *dlmemalign(size_t alignment, size_t sz)
{
  if (alignment <= 16)
    return malloc(sz);
  if (alignment <= 4096)
    return valloc(sz);
  return NULL;
}
inline void *dlvalloc(size_t sz) { return valloc(sz); }
#endif
#endif

#if !defined(_STD_RTL_MEMORY) && defined(__cplusplus)
extern "C" size_t get_memory_committed();
extern "C" size_t get_memory_committed_max();

#if !_TARGET_STATIC_LIB
extern "C" __declspec(dllexport) void *mt_dlmalloc_crt(size_t bytes);
extern "C" __declspec(dllexport) void *mt_dlmemalign_crt(size_t bytes, size_t alignment);
extern "C" __declspec(dllexport) void *mt_dlrealloc_crt(void *mem, size_t bytes);
extern "C" __declspec(dllexport) void mt_dlfree_crt(void *mem);
#endif

extern "C" void dlfree(void *);
inline void dlfree_aligned(void *p) { dlfree(p); }

extern "C" size_t dlmalloc_usable_size(void *);
inline size_t dlmalloc_usable_size_aligned(void *p) { return dlmalloc_usable_size(p); }
#endif

extern int pull_rtlOverride_stubRtl;

#endif
