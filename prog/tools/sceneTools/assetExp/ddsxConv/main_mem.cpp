// Copyright (C) Gaijin Games KFT.  All rights reserved.

#if _TARGET_PC_WIN | _TARGET_PC_LINUX
#include <malloc.h>
#elif _TARGET_PC_MACOSX
#include <stdlib.h>
#include <malloc/malloc.h>
#endif
#include <util/dag_stdint.h>

#if _TARGET_PC_WIN
#pragma warning(disable : 4074)
#pragma init_seg(compiler)

#elif _TARGET_PC_LINUX
static inline size_t _msize(void *p) { return malloc_usable_size(p); }
#elif _TARGET_PC_MACOSX
static inline size_t _msize(void *p) { return malloc_size(p); }
#endif

class RtlMainAllocator : public IMemAlloc
{
public:
  virtual void destroy() {}
  virtual size_t getSize(void *p) { return p ? _msize(p) : 0; }
  virtual void *alloc(size_t sz) { return ::malloc(sz); }
  virtual void *tryAlloc(size_t sz) { return ::malloc(sz); }
  virtual void *allocAligned(size_t sz, size_t alignment) { return alignment == 16 ? ::malloc(sz) : nullptr; }
  virtual void *realloc(void *p, size_t sz) { return ::realloc(p, sz); }
  virtual void free(void *p) { ::free(p); }
  void freeAligned(void *p) override { ::free(p); }
  virtual bool resizeInplace(void *p, size_t sz)
  {
#if _TARGET_PC_WIN
    return ::_expand(p, sz);
#else
    (void)p;
    return sz <= _msize(p);
#endif
  }
  virtual bool isEmpty() { return false; }
};
static RtlMainAllocator main_mem;
IMemAlloc *defaultmem = &main_mem;

extern "C" void *memalloc_default(size_t sz) { return defaultmem->alloc(sz); }
extern "C" void *memrealloc_default(void *p, size_t sz) { return defaultmem->realloc(p, sz); }
extern "C" void memfree_default(void *p) { defaultmem->free(p); }
extern "C" void memfree_anywhere(void *p) { ::free(p); }
