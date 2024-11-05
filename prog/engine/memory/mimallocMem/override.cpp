// Copyright (C) Gaijin Games KFT.  All rights reserved.

#if !(_TARGET_C1 | _TARGET_C2)
#include <malloc.h>
#endif
#include <mimalloc.h>

#ifdef _MSC_VER
#if _MSC_VER >= 1900
#define MEMEXP  extern "C" _CRTRESTRICT
#define MEMEXP2 extern "C"
#define MEMEXP3 extern "C"
#elif _MSC_VER >= 1400
#define MEMEXP  extern "C" _CRTRESTRICT _CRTNOALIAS
#define MEMEXP2 extern "C" _CRTNOALIAS
#define MEMEXP3 extern "C"
#else
#define MEMEXP  extern "C"
#define MEMEXP2 extern "C"
#define MEMEXP3 extern "C"
#endif
#else
#define MEMEXP  extern "C"
#define MEMEXP2 extern "C"
#define MEMEXP3 extern "C"
#endif

MEMEXP void *__cdecl malloc(size_t sz) { return mi_malloc(sz); }
MEMEXP void *__cdecl calloc(size_t n, size_t c) { return mi_calloc(n, c); }
MEMEXP void *__cdecl realloc(void *p, size_t s) { return mi_realloc(p, s); }
MEMEXP2 void __cdecl free(void *ptr) { mi_free(ptr); }
MEMEXP3 size_t __cdecl _msize(void *ptr) { return mi_usable_size(ptr); }
MEMEXP3 size_t __cdecl _msize_base(void *ptr) { return _msize(ptr); }
MEMEXP void *__cdecl _aligned_malloc(size_t sz, size_t al) { return mi_malloc_aligned(sz, al); }
MEMEXP2 void __cdecl _aligned_free(void *ptr) { mi_free(ptr); }
MEMEXP void *__cdecl _aligned_realloc(void *ptr, size_t sz, size_t al) { return mi_realloc_aligned(ptr, sz, al); }
MEMEXP2 size_t __cdecl _aligned_msize(void *ptr, size_t, size_t) { return mi_usable_size(ptr); }
MEMEXP3 void *__cdecl _expand(void *ptr, size_t sz) { return mi_expand(ptr, sz); }

#if defined(_MSC_VER) && _MSC_VER >= 1400
MEMEXP void *__cdecl _recalloc(void *ptr, size_t nelem, size_t elemsz) { return mi_recalloc(ptr, nelem, elemsz); }
MEMEXP void *__cdecl _recalloc_base(void *ptr, size_t nelem, size_t elemsz) { return _recalloc(ptr, nelem, elemsz); }
MEMEXP3 void *__cdecl _calloc_impl(size_t nelem, size_t elemsz) { return calloc(nelem, elemsz); }
#include <windows.h>

extern "C"
{
  void *_crtheap = (void *)1;
  int __cdecl _heap_init(void) { return 1; }
  void __cdecl _heap_term(void) {}
  void __cdecl _initp_heap_handler(void *) {}
}

#if defined(_MSC_VER) && _MSC_VER >= 1900
MEMEXP2 HANDLE __acrt_heap = nullptr;
MEMEXP2 bool __cdecl __acrt_initialize_heap() { return true; }
MEMEXP2 bool __cdecl __acrt_uninitialize_heap(bool const /* terminating */) { return true; }
MEMEXP2 intptr_t __cdecl _get_heap_handle() { return reinterpret_cast<intptr_t>(__acrt_heap); }
MEMEXP2 HANDLE __acrt_getheap() { return __acrt_heap; }

MEMEXP void *__cdecl _malloc_base(size_t const size) { return malloc(size); }
MEMEXP void *__cdecl _calloc_base(size_t const count, size_t const size) { return calloc(count, size); }
MEMEXP void *__cdecl _realloc_base(void *const block, size_t const size) { return realloc(block, size); }
MEMEXP2 void __cdecl _free_base(void *const block) { return free(block); }
#endif

#endif
