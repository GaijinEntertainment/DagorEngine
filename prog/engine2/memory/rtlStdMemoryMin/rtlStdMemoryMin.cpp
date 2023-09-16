#include <string.h>
#ifdef __GNUC__
#include <stdlib.h>
#endif
#if !defined(__GNUC__) || _TARGET_C3 || _TARGET_PC_LINUX
#include <malloc.h>
#endif

#if _TARGET_C1 | _TARGET_C2 | _TARGET_PC_LINUX | _TARGET_C3 | _TARGET_ANDROID
inline size_t sys_malloc_usable_size(void *p) { return malloc_usable_size(p); }
#elif _TARGET_APPLE
#include <malloc/malloc.h>
inline size_t sys_malloc_usable_size(void *p) { return malloc_size(p); }
#else
inline size_t sys_malloc_usable_size(void *p) { return p ? _msize(p) : 0; }
#endif

class MinRtlStdAllocator : public IMemAlloc
{
public:
  void destroy() override {}
  bool isEmpty() override { return false; }

  size_t getSize(void *p) override { return sys_malloc_usable_size(p); }

  void *alloc(size_t sz) override { return tryAlloc(sz); }
  void *tryAlloc(size_t sz) override { return ::malloc(sz); }
  void *allocAligned(size_t sz, size_t al) override { return al <= 8 ? alloc(sz) : nullptr; }
  bool resizeInplace(void *p, size_t sz) override { return false; }
  void *realloc(void *p, size_t sz) override { return ::realloc(p, sz); }
  void free(void *p) override { ::free(p); }
  void freeAligned(void *p) override { ::free(p); }
};

static MinRtlStdAllocator mem;
IMemAlloc *defaultmem = &mem;
IMemAlloc *midmem = &mem;
IMemAlloc *inimem = &mem;
IMemAlloc *tmpmem = &mem;
IMemAlloc *strmem = &mem;
IMemAlloc *globmem = &mem;

void memfree_anywhere(void *p) { free(p); }
char *str_dup(const char *s, IMemAlloc *a)
{
  if (!s)
    return NULL;
  size_t n = strlen(s) + 1;
  if (void *p = mem.alloc(n))
  {
    memcpy(p, s, n);
    return (char *)p;
  }
  return nullptr;
}
IMemAlloc *framemem_ptr() { return &mem; }
extern "C" void *memalloc_default(size_t sz) { return mem.alloc(sz); }
extern "C" void memfree_default(void *p) { mem.free(p); }
extern "C" void *memrealloc_default(void *p, size_t sz) { return mem.realloc(p, sz); }
extern "C" int memresizeinplace_default(void *p, size_t sz) { return 0; }
