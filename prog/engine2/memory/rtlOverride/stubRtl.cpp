#ifndef __GNUC__
#include <malloc.h>
#endif
#include <string.h>

class IMemAlloc
{
public:
  virtual void destroy() = 0;
  virtual bool isEmpty() = 0;
  virtual unsigned getSize(void *p) = 0;
  virtual void *alloc(size_t sz) = 0;
  virtual void *tryAlloc(size_t sz) = 0;
  virtual void *allocAligned(size_t sz, size_t alignment) = 0;
  virtual bool resizeInplace(void *p, size_t sz) = 0;
  virtual void *realloc(void *p, size_t sz) = 0;
  virtual void free(void *p) = 0;
  virtual void freeAligned(void *p) = 0;
};

#if _TARGET_PC_LINUX
#define _STD_RTL_MEMORY 1
#endif

#include <supp/dag_define_COREIMP.h>
extern "C" KRNLIMP void *memalloc_default(size_t sz);
extern "C" KRNLIMP void memfree_default(void *);
extern "C" KRNLIMP int memresizeinplace_default(void *p, size_t sz);
extern "C" KRNLIMP void *memrealloc_default(void *, size_t sz);
extern "C" KRNLIMP void *memcalloc_default(size_t, size_t);
extern "C" KRNLIMP void memfree_anywhere(void *p);

#if !_TARGET_STATIC_LIB
extern KRNLIMP IMemAlloc *defaultmem;
#endif
#include <supp/dag_undef_COREIMP.h>

#if !_TARGET_STATIC_LIB
#include "../dlmalloc-setup.h"
#if _TARGET_PC_WIN
#include <windows.h>
#endif

static inline void real_init_memory() {}
static inline void *mt_dlmemalign(size_t sz, size_t al) { return defaultmem->allocAligned(sz, al); }
static inline void *mt_dlrealloc(void *ptr, size_t sz) { return defaultmem->realloc(ptr, sz); }
#if _TARGET_PC_WIN
extern "C" __declspec(dllimport) void *mt_dlmalloc_crt(size_t bytes);
extern "C" __declspec(dllexport) void *mt_dlmemalign_crt(size_t bytes, size_t alignment);
extern "C" __declspec(dllimport) void *mt_dlrealloc_crt(void *mem, size_t bytes);
extern "C" __declspec(dllimport) void mt_dlfree_crt(void *mem);
#endif

#if defined(_STD_RTL_MEMORY)
static inline size_t dlmalloc_usable_size(void *mem) { return defaultmem->getSize(mem); }
#else
// extracts from dlmalloc code, for fastest dlmalloc_usable_size()
typedef struct malloc_chunk
{
  size_t prev_foot; /* Size of previous chunk (if free).  */
  size_t head;      /* Size and inuse bits. */
  malloc_chunk *fd; /* double links -- used only if free. */
  malloc_chunk *bk;
} *mchunkptr;

#define SIZE_T_SIZE (sizeof(size_t))

#define SIZE_T_ONE       ((size_t)1)
#define SIZE_T_TWO       ((size_t)2)
#define SIZE_T_FOUR      ((size_t)4)
#define TWO_SIZE_T_SIZES (SIZE_T_SIZE << 1)

#if FOOTERS
#define CHUNK_OVERHEAD (TWO_SIZE_T_SIZES)
#else
#define CHUNK_OVERHEAD (SIZE_T_SIZE)
#endif

#define MMAP_CHUNK_OVERHEAD (TWO_SIZE_T_SIZES)

#define PINUSE_BIT (SIZE_T_ONE)
#define CINUSE_BIT (SIZE_T_TWO)
#define FLAG4_BIT  (SIZE_T_FOUR)
#define INUSE_BITS (PINUSE_BIT | CINUSE_BIT)
#define FLAG_BITS  (PINUSE_BIT | CINUSE_BIT | FLAG4_BIT)

#define CHUNKSIZE(p)    ((p)->head & ~(FLAG_BITS))
#define IS_INUSE(p)     (((p)->head & INUSE_BITS) != PINUSE_BIT)
#define IS_MMAPPED(p)   (((p)->head & INUSE_BITS) == 0)
#define OVERHEAD_FOR(p) (IS_MMAPPED(p) ? MMAP_CHUNK_OVERHEAD : CHUNK_OVERHEAD)

#define MEM2CHUNK(mem) ((mchunkptr)((char *)(mem)-TWO_SIZE_T_SIZES))

static inline size_t dlmalloc_usable_size(void *mem)
{
  if (mem != 0)
  {
    mchunkptr p = MEM2CHUNK(mem);
    if (IS_INUSE(p))
      return CHUNKSIZE(p) - OVERHEAD_FOR(p);
  }
  return 0;
}
#endif

// implement stubs for different compiler runtime
#include "../stubRtlAlloc.inc.cpp"
#endif

int pull_rtlOverride_stubRtl = 0;
