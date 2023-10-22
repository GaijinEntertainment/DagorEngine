#if (DAGOR_DBGLEVEL < 2) && !defined(_STD_RTL_MEMORY) && !_TARGET_XBOX // Config != dbg
#include <malloc.h>
#include <debug/dag_debug.h>

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

MEMEXP void *__cdecl malloc(size_t sz)
{
  real_init_memory();
  void *p = mt_dlmalloc_crt(sz);
  if (!p && sz)
    logerr("malloc(%llu) failed!", sz);
  return p;
}

MEMEXP void *__cdecl calloc(size_t nelem, size_t elemsz)
{
  real_init_memory();
  void *ptr = mt_dlmalloc_crt(nelem * elemsz);
  if (!ptr && nelem * elemsz)
    logerr("calloc(%llu,%llu) failed!", nelem, elemsz);
  if (ptr)
    memset(ptr, 0, nelem * elemsz);
  return ptr;
}

MEMEXP void *__cdecl realloc(void *ptr, size_t sz)
{
  real_init_memory();
  void *p = mt_dlrealloc_crt(ptr, sz);
  if (!p && sz)
    logerr("realloc(%p,%llu) failed!", ptr, sz);
  return p;
}
MEMEXP2 void __cdecl free(void *ptr)
{
  real_init_memory();
  mt_dlfree_crt(ptr);
}
MEMEXP3 size_t __cdecl _msize(void *ptr) { return dlmalloc_usable_size(ptr); }
MEMEXP3 size_t __cdecl _msize_base(void *ptr) { return _msize(ptr); }

#if MEM_DEBUGALLOC <= 0
MEMEXP void *__cdecl _aligned_malloc(size_t sz, size_t al) { return mt_dlmemalign(sz, al); }
MEMEXP2 void __cdecl _aligned_free(void *ptr) { memfree_anywhere(ptr); }
MEMEXP void *__cdecl _aligned_realloc(void *ptr, size_t sz, size_t) { return mt_dlrealloc(ptr, sz); }
MEMEXP2 size_t __cdecl _aligned_msize(void *ptr, size_t, size_t) { return dlmalloc_usable_size(ptr); }

#else
MEMEXP void *__cdecl _aligned_malloc(size_t sz, size_t al)
{
  void *p = mt_dlmemalign_crt(sz, al);
  if (!p && sz)
    logerr("_aligned_malloc(%llu, %d) failed!", sz, al);
  return p;
}
MEMEXP2 void __cdecl _aligned_free(void *ptr) { mt_dlfree_crt(ptr); }
MEMEXP void *__cdecl _aligned_realloc(void *ptr, size_t sz, size_t) { return mt_dlrealloc_crt(ptr, sz); }
MEMEXP2 size_t __cdecl _aligned_msize(void *, size_t, size_t) { return -1; }
#endif

#if defined(_MSC_VER) && _MSC_VER >= 1400
MEMEXP void *__cdecl _recalloc(void *ptr, size_t nelem, size_t elemsz)
{
  real_init_memory();
  void *p = mt_dlrealloc_crt(ptr, elemsz * nelem);
  if (!p && elemsz * nelem)
    logerr("recalloc(%p,%llu,%llu) failed!", ptr, nelem, elemsz);
  return p;
}
MEMEXP void *__cdecl _recalloc_base(void *ptr, size_t nelem, size_t elemsz) { return _recalloc(ptr, nelem, elemsz); }
MEMEXP3 void *__cdecl _calloc_impl(size_t nelem, size_t elemsz) { return calloc(nelem, elemsz); }

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
#endif
