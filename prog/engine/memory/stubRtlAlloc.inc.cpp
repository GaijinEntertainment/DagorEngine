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
#pragma warning(disable : 4273)
#else
#define MEMEXP  extern "C"
#define MEMEXP2 extern "C"
#define MEMEXP3 extern "C"
#endif

#ifdef _CRT_NOEXCEPT
#define MEMEXP_NOEXCEPT _CRT_NOEXCEPT
#else
#define MEMEXP_NOEXCEPT
#endif

#if (_TARGET_PC_WIN | _TARGET_XBOX) && (USE_DLMALLOC | USE_IMPORTED_ALLOC)
#include <malloc.h>
#include <debug/dag_debug.h>

#define DAG_OVERRIDE_MALLOC_FAMILY  1
#define DAG_IMPLEMENT_STRDUP_FAMILY 1
#define DAG_DECLARE_IMPORT_STUBS    _DLL

#if defined(_MSC_VER) && _MSC_VER >= 1400
extern "C"
{
  void *_crtheap = (void *)1;
  int __cdecl _heap_init(void) { return 1; }
  void __cdecl _heap_term(void) {}
  void __cdecl _initp_heap_handler(void *) {}
}

#if defined(_MSC_VER) && _MSC_VER >= 1900
MEMEXP2 HANDLE __acrt_heap = nullptr;
MEMEXP2 bool __cdecl __acrt_initialize_heap()
{
  real_init_memory();
  return true;
}
MEMEXP2 bool __cdecl __acrt_uninitialize_heap(bool const /* terminating */) { return true; }
MEMEXP2 intptr_t __cdecl _get_heap_handle() { return reinterpret_cast<intptr_t>(__acrt_heap); }
MEMEXP2 HANDLE __acrt_getheap() { return __acrt_heap; }
#endif

#endif
static inline void init_acrt_heap() {}

#elif (_TARGET_PC_WIN | _TARGET_XBOX) && _TARGET_STATIC_LIB
#include <corecrt_malloc.h>

#define DAG_OVERRIDE_MALLOC_FAMILY  1
#define DAG_IMPLEMENT_STRDUP_FAMILY 1
#define DAG_DECLARE_IMPORT_STUBS    _DLL

extern "C" HANDLE __acrt_heap;

#if !USE_MIMALLOC
MEMEXP void *__cdecl _recalloc_base(void *ptr, size_t nelem, size_t elemsz) { return dagmem_recalloc_base(ptr, nelem, elemsz); }
__inline static bool is_contraction_possible(size_t const old_size) throw()
{
  // Check if object allocated on low fragmentation heap. The LFH can only allocate blocks up to 16KB in size.
  if (old_size <= 0x4000)
  {
    LONG heap_type = -1;
    if (!HeapQueryInformation(__acrt_heap, HeapCompatibilityInformation, &heap_type, sizeof(heap_type), nullptr))
      return false;
    return heap_type != 2;
  }
  return true; // Contraction possible for objects not on the LFH
}
extern "C" __declspec(noinline) void *__cdecl _expand_base(void *const block, size_t const size)
{
  if (!block)
    return nullptr;
  if (size > _HEAP_MAXREQ)
    return nullptr;

  size_t const old_size = static_cast<size_t>(HeapSize(__acrt_heap, 0, block));
  size_t const new_size = size == 0 ? 1 : size;

  void *new_block = HeapReAlloc(__acrt_heap, HEAP_REALLOC_IN_PLACE_ONLY, block, new_size);
  if (new_block != nullptr)
    return new_block;

  // If a failure to contract was caused by platform limitations, just return the original block.
  if (new_size <= old_size && !is_contraction_possible(old_size))
    return block;
  return nullptr;
}

extern "C" __declspec(noinline) size_t __cdecl _msize_base(void *const block) MEMEXP_NOEXCEPT
{
  return block ? static_cast<size_t>(HeapSize(__acrt_heap, 0, block)) : static_cast<size_t>(-1);
}
#endif

#if _TARGET_XBOX
#include <xmem.h>
extern "C" HANDLE __acrt_heap = XMemGetTitleHeap();
static inline void init_acrt_heap() { __acrt_heap = XMemGetTitleHeap(); }
#elif _DLL
extern "C" HANDLE __acrt_heap = GetProcessHeap();
static inline void init_acrt_heap() { __acrt_heap = GetProcessHeap(); }
#else
static inline void init_acrt_heap() {}
#endif

#else
static inline void init_acrt_heap() {}
#endif

#if DAG_OVERRIDE_MALLOC_FAMILY
MEMEXP3 size_t __cdecl _msize(void *ptr) { return dagmem_malloc_usable_size(ptr); }
MEMEXP2 size_t __cdecl _aligned_msize(void *ptr, size_t, size_t) { return dagmem_malloc_usable_size(ptr); }

#if MEM_DEBUGALLOC <= 0
MEMEXP void *__cdecl malloc(size_t const size) { return dagmem_malloc(size); }
MEMEXP void *__cdecl calloc(size_t const count, size_t const size) { return dagmem_calloc(count, size); }
MEMEXP void *__cdecl realloc(void *const block, size_t const size) { return dagmem_realloc(block, size); }
MEMEXP void *__cdecl _recalloc(void *ptr, size_t nelem, size_t elemsz) { return dagmem_recalloc(ptr, nelem, elemsz); }
MEMEXP3 void *__cdecl _expand(void *const block, size_t const size) { return dagmem_expand(block, size) ? block : nullptr; }
MEMEXP2 void __cdecl free(void *const block) { return dagmem_free(block); }

MEMEXP void *__cdecl _aligned_malloc(size_t sz, size_t al) { return dagmem_memalign(al, sz); }
MEMEXP2 void __cdecl _aligned_free(void *ptr) { dagmem_free_aligned(ptr); }
MEMEXP void *__cdecl _aligned_realloc(void *p, size_t sz, size_t al)
{
  void *r = dagmem_memalign(al, sz);
  if (r && p)
  {
    size_t psz = dagmem_malloc_usable_size(p);
    memcpy(r, p, (sz < psz) ? sz : psz);
    dagmem_free_aligned(p);
  }
  return r;
}

#else
MEMEXP void *__cdecl malloc(size_t sz) { return stdmem->alloc(sz); }
MEMEXP void *__cdecl calloc(size_t nelem, size_t elemsz)
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
MEMEXP void *__cdecl realloc(void *ptr, size_t sz) { return stdmem->realloc(ptr, sz); }
MEMEXP void *__cdecl _recalloc(void *ptr, size_t nelem, size_t elemsz)
{
  size_t old_size = dagmem_malloc_usable_size(ptr);
  size_t new_size = nelem * elemsz;

  void *new_p = stdmem->realloc(ptr, new_size);

  // If the reallocation succeeded and the new block is larger, zero-fill the new bytes:
  if (new_p && old_size < new_size)
    memset(static_cast<char *>(new_p) + old_size, 0, new_size - old_size);
  return new_p;
}
MEMEXP3 void *__cdecl _expand(void *const block, size_t const size) { return stdmem->resizeInplace(block, size) ? block : nullptr; }
MEMEXP2 void __cdecl free(void *ptr) { return stdmem->free(ptr); }

MEMEXP void *__cdecl _aligned_malloc(size_t sz, size_t al) { return stdmem->allocAligned(sz, al); }
MEMEXP2 void __cdecl _aligned_free(void *ptr) { stdmem->freeAligned(ptr); }
MEMEXP void *__cdecl _aligned_realloc(void *p, size_t sz, size_t al)
{
  void *r = stdmem->allocAligned(sz, al);
  if (r && p)
  {
    size_t psz = stdmem->getSize(p);
    memcpy(r, p, (sz < psz) ? sz : psz);
    stdmem->freeAligned(p);
  }
  return r;
}

#endif
MEMEXP3 void *__cdecl _calloc_impl(size_t nelem, size_t elemsz) { return calloc(nelem, elemsz); }

#if DAGOR_DBGLEVEL >= 2 // replacement for debug CRT
MEMEXP3 size_t __cdecl _msize_dbg(void *p, int const) { return dagmem_malloc_usable_size(p); }
MEMEXP3 void *__cdecl _malloc_dbg(size_t size, int, const char *, int) { return dagmem_malloc_base(size); }
MEMEXP3 void *__cdecl _calloc_dbg(size_t nelem, size_t elemsz, int, const char *, int) { return dagmem_calloc_base(nelem, elemsz); }
MEMEXP3 void *__cdecl _realloc_dbg(void *p, size_t sz, int, const char *, int) { return dagmem_realloc_base(p, sz); }
MEMEXP3 void *__cdecl _recalloc_dbg(void *p, size_t nelem, size_t elemsz, int, const char *, int)
{
  return dagmem_recalloc_base(p, nelem, elemsz);
}
MEMEXP3 void *__cdecl _expand_dbg(void *p, size_t sz, int, const char *, int) { return dagmem_expand(p, sz) ? p : nullptr; }
MEMEXP3 void __cdecl _free_dbg(void *const p, int const) { dagmem_free_base(p); }
MEMEXP3 _CRT_DUMP_CLIENT __cdecl _CrtGetDumpClient() { return nullptr; }
MEMEXP3 _CRT_DUMP_CLIENT __cdecl _CrtSetDumpClient(_CRT_DUMP_CLIENT const /*new_client*/) { return nullptr; }
MEMEXP3 int __cdecl _CrtSetDbgFlag(int const /*new_bits*/) { return 0; }
MEMEXP3 int __cdecl _CrtDumpMemoryLeaks() { return FALSE; }
MEMEXP3 long __cdecl _CrtSetBreakAlloc(long const /*new_break_alloc*/) { return 0; }
#endif
#endif

#if DAG_IMPLEMENT_STRDUP_FAMILY
MEMEXP3 char *__cdecl _strdup(const char *s)
{
  if (!s)
    return nullptr;

  size_t const size = strlen(s) + 1;
  char *const new_s = static_cast<char *>(malloc(size));
  if (!new_s)
    return nullptr;

  strcpy_s(new_s, size, s);
  return new_s;
}

MEMEXP3 wchar_t *__cdecl _wcsdup(const wchar_t *s)
{
  if (!s)
    return nullptr;

  size_t const size = wcslen(s) + 1;
  wchar_t *const new_s = static_cast<wchar_t *>(malloc(size * sizeof(wchar_t)));
  if (!new_s)
    return nullptr;

  wcscpy_s(new_s, size, s);
  return new_s;
}
#if DAGOR_DBGLEVEL >= 2 // replacement for debug CRT
MEMEXP3 char *__cdecl _strdup_dbg(const char *s, int, const char *, int) { return _strdup(s); }
MEMEXP3 wchar_t *__cdecl _wcsdup_dbg(const wchar_t *s, int, const char *, int) { return _wcsdup(s); }
#endif
#endif

#if DAG_DECLARE_IMPORT_STUBS
MEMEXP3 size_t(__cdecl *__imp__msize)(void *ptr) = &dagmem_malloc_usable_size;
MEMEXP3 void *(__cdecl *__imp_malloc)(size_t const size) = &malloc;
MEMEXP3 void *(__cdecl *__imp_calloc)(size_t const count, size_t const size) = &calloc;
MEMEXP3 void *(__cdecl *__imp_realloc)(void *const block, size_t const size) = &realloc;
MEMEXP3 void *(__cdecl *__imp__recalloc)(void *ptr, size_t nelem, size_t elemsz) = &_recalloc;
MEMEXP3 void *(__cdecl *__imp__expand)(void *const block, size_t const size) = &_expand;
MEMEXP3 void(__cdecl *__imp_free)(void *const block) = &free;
MEMEXP3 void *(__cdecl *__imp__aligned_malloc)(size_t sz, size_t al) = &_aligned_malloc;
MEMEXP3 void *(__cdecl *__imp__aligned_realloc)(void *ptr, size_t sz, size_t) = &_aligned_realloc;
MEMEXP3 void(__cdecl *__imp__aligned_free)(void *ptr) = &_aligned_free;
MEMEXP3 size_t(__cdecl *__imp__aligned_msize)(void *ptr, size_t, size_t) = &_aligned_msize;
#if DAG_IMPLEMENT_STRDUP_FAMILY
MEMEXP3 char *(__cdecl *__imp__strdup)(const char *s) = &_strdup;
MEMEXP3 wchar_t *(__cdecl *__imp__wcsdup)(const wchar_t *s) = &_wcsdup;
#endif
#endif

#if (_TARGET_PC_WIN | _TARGET_XBOX) && (USE_MIMALLOC || USE_DLMALLOC || USE_IMPORTED_ALLOC) && defined(_MSC_VER) && _MSC_VER >= 1900
MEMEXP3 size_t __cdecl _msize_base(void *ptr) MEMEXP_NOEXCEPT { return dagmem_malloc_usable_size(ptr); }
MEMEXP void *__cdecl _malloc_base(size_t const size) { return dagmem_malloc_base(size); }
MEMEXP void *__cdecl _calloc_base(size_t const count, size_t const size) { return dagmem_calloc_base(count, size); }
MEMEXP void *__cdecl _realloc_base(void *const block, size_t const size) { return dagmem_realloc_base(block, size); }
MEMEXP void *__cdecl _recalloc_base(void *block, size_t nelem, size_t elemsz) { return dagmem_recalloc_base(block, nelem, elemsz); }
MEMEXP2 void __cdecl _free_base(void *const block) { dagmem_free_base(block); }
#if DAG_DECLARE_IMPORT_STUBS
MEMEXP3 size_t(__cdecl *__imp__msize_base)(void *ptr) = &_msize_base;
MEMEXP3 void *(__cdecl *__imp__malloc_base)(size_t const size) = &_malloc_base;
MEMEXP3 void *(__cdecl *__imp__calloc_base)(size_t const count, size_t const size) = &_calloc_base;
MEMEXP3 void *(__cdecl *__imp__realloc_base)(void *const block, size_t const size) = &_realloc_base;
MEMEXP3 void *(__cdecl *__imp__recalloc_base)(void *const block, size_t nelem, size_t elemsz) = &_recalloc_base;
MEMEXP3 void(__cdecl *__imp__free_base)(void *const block) = &_free_base;
#endif
#endif

#if _TARGET_APPLE && (USE_RTL_ALLOC | USE_IMPORTED_ALLOC) && !DAGOR_ADDRESS_SANITIZER && !DAGOR_THREAD_SANITIZER
#include <dlfcn.h>
#include <errno.h>
#define DAG_IMPLEMENT_STRDUP_FAMILY 1

#if USE_RTL_ALLOC
// Forward-declare resolver trampolines
static size_t dagmem_malloc_usable_size_resolve(const void *p);
static void *dagmem_malloc_resolve(size_t sz);
static void *dagmem_calloc_resolve(size_t n, size_t sz);
static int dagmem_posix_memalign_resolve(void **out_p, size_t alignment, size_t sz);
static void *dagmem_aligned_alloc_resolve(size_t alignment, size_t sz);
static void *dagmem_valloc_resolve(size_t sz);
static void *dagmem_realloc_resolve(void *p, size_t sz);
static void dagmem_free_resolve(void *p);

// Zone-based fallbacks (safe during dlsym, bypasses interpose)
static size_t dagmem_malloc_usable_size_fallback(const void *p) { return p ? 16 : 0; }
static void *dagmem_malloc_fallback(size_t sz) { return malloc_zone_malloc(malloc_default_zone(), sz); }
static void *dagmem_calloc_fallback(size_t n, size_t sz) { return malloc_zone_calloc(malloc_default_zone(), n, sz); }
static int dagmem_posix_memalign_fallback(void **out_p, size_t al, size_t sz)
{
  return (*out_p = malloc_zone_memalign(malloc_default_zone(), al, sz)) ? 0 : ENOMEM;
}
static void *dagmem_aligned_alloc_fallback(size_t al, size_t sz) { return malloc_zone_memalign(malloc_default_zone(), al, sz); }
static void *dagmem_valloc_fallback(size_t sz) { return malloc_zone_valloc(malloc_default_zone(), sz); }
static void *dagmem_realloc_fallback(void *p, size_t sz) { return malloc_zone_realloc(malloc_default_zone(), p, sz); }
static void dagmem_free_fallback(void *p) { p ? malloc_zone_free(malloc_default_zone(), p) : (void)0; }

// Initialize to resolvers (never NULL - eliminates branch from hot path)
void *(*dagmem_orig_ptr_malloc)(size_t) = &dagmem_malloc_resolve;
void *(*dagmem_orig_ptr_calloc)(size_t, size_t) = &dagmem_calloc_resolve;
int (*dagmem_orig_ptr_posix_memalign)(void **, size_t, size_t) = &dagmem_posix_memalign_resolve;
void *(*dagmem_orig_ptr_aligned_alloc)(size_t, size_t) = &dagmem_aligned_alloc_resolve;
void *(*dagmem_orig_ptr_valloc)(size_t) = &dagmem_valloc_resolve;
void *(*dagmem_orig_ptr_realloc)(void *, size_t) = &dagmem_realloc_resolve;
void (*dagmem_orig_ptr_free)(void *) = &dagmem_free_resolve;

static void dagmem_resolve_all_originals()
{
  // Step 1: install zone fallbacks (protects against dlsym calling malloc)
  dagmem_orig_ptr_malloc = &dagmem_malloc_fallback;
  dagmem_orig_ptr_calloc = &dagmem_calloc_fallback;
  dagmem_orig_ptr_posix_memalign = &dagmem_posix_memalign_fallback;
  dagmem_orig_ptr_aligned_alloc = &dagmem_aligned_alloc_fallback;
  dagmem_orig_ptr_valloc = &dagmem_valloc_resolve;
  dagmem_orig_ptr_realloc = &dagmem_realloc_fallback;
  dagmem_orig_ptr_free = &dagmem_free_fallback;

  // Step 2: resolve real functions (if dlsym calls malloc, it safely hits the fallbacks)
  auto m = (void *(*)(size_t))dlsym(RTLD_NEXT, "malloc");
  auto c = (void *(*)(size_t, size_t))dlsym(RTLD_NEXT, "calloc");
  auto pm = (int (*)(void **, size_t, size_t))dlsym(RTLD_NEXT, "posix_memalign");
  auto aa = (void *(*)(size_t, size_t))dlsym(RTLD_NEXT, "aligned_alloc");
  auto v = (void *(*)(size_t))dlsym(RTLD_NEXT, "valloc");
  auto r = (void *(*)(void *, size_t))dlsym(RTLD_NEXT, "realloc");
  auto f = (void (*)(void *))dlsym(RTLD_NEXT, "free");

  // Step 3: install real ones (keep fallbacks if dlsym somehow fails)
  if (m)
    dagmem_orig_ptr_malloc = m;
  if (c)
    dagmem_orig_ptr_calloc = c;
  if (pm)
    dagmem_orig_ptr_posix_memalign = pm;
  if (aa)
    dagmem_orig_ptr_aligned_alloc = aa;
  if (v)
    dagmem_orig_ptr_valloc = v;
  if (r)
    dagmem_orig_ptr_realloc = r;
  if (f)
    dagmem_orig_ptr_free = f;

  // Step 4: init memory manager early (placement new only, no malloc)
  real_init_memory();
}

// Resolvers: first call resolves all, then tail-calls the now-installed real function
static void *dagmem_malloc_resolve(size_t sz)
{
  dagmem_resolve_all_originals();
  return dagmem_orig_ptr_malloc(sz);
}
static void *dagmem_calloc_resolve(size_t n, size_t sz)
{
  dagmem_resolve_all_originals();
  return dagmem_orig_ptr_calloc(n, sz);
}
static int dagmem_posix_memalign_resolve(void **out_p, size_t al, size_t sz)
{
  dagmem_resolve_all_originals();
  return dagmem_orig_ptr_posix_memalign(out_p, al, sz);
}
static void *dagmem_aligned_alloc_resolve(size_t al, size_t sz)
{
  dagmem_resolve_all_originals();
  return dagmem_orig_ptr_aligned_alloc(al, sz);
}
static void *dagmem_valloc_resolve(size_t sz)
{
  dagmem_resolve_all_originals();
  return dagmem_orig_ptr_valloc(sz);
}
static void *dagmem_realloc_resolve(void *p, size_t sz)
{
  dagmem_resolve_all_originals();
  return dagmem_orig_ptr_realloc(p, sz);
}
static void dagmem_free_resolve(void *p)
{
  dagmem_resolve_all_originals();
  dagmem_orig_ptr_free(p);
}
#endif

extern "C" void *malloc(size_t sz) { return dagmem_malloc(sz); }
extern "C" void *calloc(size_t n, size_t sz) { return dagmem_calloc(n, sz); }
extern "C" int posix_memalign(void **out_p, size_t al, size_t sz) { return (*out_p = dagmem_memalign(al, sz)) ? 0 : ENOMEM; }
extern "C" void *aligned_alloc(size_t al, size_t sz) { return dagmem_memalign(al, sz); }
extern "C" void *valloc(size_t sz) { return dagmem_memalign(4096, sz); }
extern "C" void *realloc(void *p, size_t sz) { return dagmem_realloc(p, sz); }
extern "C" void free(void *p) { dagmem_free(p); }

#endif // _TARGET_APPLE && USE_RTL_ALLOC && !DAGOR_ADDRESS_SANITIZER && !DAGOR_THREAD_SANITIZER

#if _TARGET_PC_LINUX && (USE_RTL_ALLOC | USE_IMPORTED_ALLOC) && !DAGOR_ADDRESS_SANITIZER && !DAGOR_THREAD_SANITIZER
#include <errno.h>
#define DAG_IMPLEMENT_STRDUP_FAMILY 1

// Override glibc's weak malloc/free/calloc/realloc symbols.
// dagmem_*_base() calls __libc_* directly, so no recursion.
extern "C" void *malloc(size_t sz) { return dagmem_malloc(sz); }
extern "C" void *calloc(size_t n, size_t sz) { return dagmem_calloc(n, sz); }
extern "C" void *memalign(size_t al, size_t sz) { return dagmem_memalign(al, sz); }
extern "C" void *valloc(size_t sz) { return dagmem_memalign(4096, sz); }
extern "C" int posix_memalign(void **pp, size_t al, size_t sz) { return (*pp = dagmem_memalign(al, sz)) ? 0 : ENOMEM; }
extern "C" void *realloc(void *p, size_t sz) { return dagmem_realloc(p, sz); }
extern "C" void free(void *p) { dagmem_free(p); }

#endif // _TARGET_PC_LINUX && USE_RTL_ALLOC && !DAGOR_ADDRESS_SANITIZER && !DAGOR_THREAD_SANITIZER

#if (_TARGET_C1 | _TARGET_C2)









#endif

#if DAG_IMPLEMENT_STRDUP_FAMILY && (_TARGET_PC_LINUX | _TARGET_APPLE | _TARGET_C1 | _TARGET_C2)
#include <wchar.h>
extern "C" char *strdup(const char *s)
{
#if !_TARGET_PC_LINUX
  if (!s)
    return nullptr;
#endif

  size_t size = strlen(s) + 1;
  char *new_s = static_cast<char *>(malloc(size));
  if (new_s)
    memcpy(new_s, s, size);
  return new_s;
}

extern "C" wchar_t *wcsdup(const wchar_t *s)
{
  if (!s)
    return nullptr;

  size_t size = wcslen(s) + 1;
  wchar_t *new_s = static_cast<wchar_t *>(malloc(size * sizeof(wchar_t)));
  if (new_s)
    memcpy(new_s, s, size * sizeof(wchar_t));
  return new_s;
}
#endif
