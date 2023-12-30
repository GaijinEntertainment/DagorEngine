#include <memory/dag_MspaceAlloc.h>
#include "stdRtlMemUsage.h"
#include <debug/dag_fatal.h>
#include <debug/dag_debug.h>

#define IS_OUR_PTR(p) (((char *)(p)) >= ((char *)mBase) && ((char *)p) < ((char *)mBaseEnd))

#if defined(_DLMALLOC_MSPACE) // dlmalloc-based implementation
#include "dlmalloc-2.8.4.h"

MspaceAlloc::MspaceAlloc(void *base, size_t capacity, bool fixed_) : mFixed(fixed_), mMemOwner(base == NULL)
{
  mBase = base ? base : memalloc(capacity, defaultmem);
  capacity = base ? capacity : defaultmem->getSize(mBase);
  mBaseEnd = (void *)((char *)mBase + capacity);
  rmSpace = create_mspace_with_base(mBase, capacity, false);
  G_ASSERTF(rmSpace, "%s can't create mspace at 0x%p capacity=%llu", __FUNCTION__, mBase, capacity);
}

MspaceAlloc::~MspaceAlloc() { release(false); }

void MspaceAlloc::release(bool reinit)
{
  if (rmSpace)
  {
    destroy_mspace(rmSpace);
    rmSpace = NULL;
  }

  if (reinit)
  {
    if (mBase)
    {
      rmSpace = create_mspace_with_base(mBase, (char *)mBaseEnd - (char *)mBase, false);
      G_ASSERTF(rmSpace, "%s can't create mspace at 0x%p capacity=%llu", __FUNCTION__, mBase, (char *)mBaseEnd - (char *)mBase);
    }
    else
      G_ASSERTF(0, "trying to call %s(true)after %s(false)", __FUNCTION__, __FUNCTION__);
  }
  else
  {
    if (mMemOwner && mBase)
      memfree(mBase, defaultmem);
    mBase = NULL;
  }
}

bool MspaceAlloc::isEmpty()
{
  struct mallinfo mi = mspace_mallinfo(rmSpace);
  return mi.uordblks == 0;
}

size_t MspaceAlloc::getSize(void *p) { return mspace_usable_size(p); }

void *MspaceAlloc::alloc(size_t sz)
{
  void *p = mspace_malloc(rmSpace, sz);

  if (mFixed)
  {
    if (p && !IS_OUR_PTR(p))
    {
      mspace_free(rmSpace, p);
      return NULL;
    }
  }
  else
  {
    if (!p)
      DAG_FATAL("Not enough memory to alloc %llu bytes", sz);
  }

  return p;
}

void *MspaceAlloc::tryAlloc(size_t sz)
{
  void *p = mspace_malloc(rmSpace, sz);
  if (mFixed && p && !IS_OUR_PTR(p))
  {
    mspace_free(rmSpace, p);
    p = NULL;
  }
  return p;
}

void *MspaceAlloc::allocAligned(size_t sz, size_t alignment)
{
  void *p = mspace_memalign(rmSpace, alignment ? alignment : 4096, sz);
  if (mFixed && p && !IS_OUR_PTR(p))
  {
    mspace_free(rmSpace, p);
    p = NULL;
  }
  return p;
}

bool MspaceAlloc::resizeInplace(void *p, size_t sz) { return sz <= mspace_usable_size(p); }

void *MspaceAlloc::realloc(void *p, size_t sz)
{
  p = mspace_realloc(rmSpace, p, sz);
  if (mFixed)
  {
    if (p && !IS_OUR_PTR(p))
    {
      mspace_free(rmSpace, p);
      return NULL;
    }
  }
  else
  {
    if (!p)
      DAG_FATAL("Not enough memory to realloc %llu bytes", sz);
  }
  return p;
}

void MspaceAlloc::free(void *p)
{
  if (p)
    mspace_free(rmSpace, p);
}
void MspaceAlloc::freeAligned(void *p) { free(p); }

void MspaceAlloc::getStatistics(size_t &system_allocated, size_t &mem_allocated)
{
  if (!rmSpace)
  {
    system_allocated = mem_allocated = 0;
    return;
  }
  struct mallinfo info = mspace_mallinfo(rmSpace);
  system_allocated = info.arena;
  mem_allocated = info.uordblks;
}

#else // dummy implementation using global heap

MspaceAlloc::MspaceAlloc(void *, size_t, bool) : mBase(NULL), mBaseEnd(NULL), rmSpace(NULL), mFixed(false), mMemOwner(false) {}

MspaceAlloc::~MspaceAlloc() {}
void MspaceAlloc::release(bool) {}
bool MspaceAlloc::isEmpty() { return false; }
size_t MspaceAlloc::getSize(void *p) { return defaultmem->getSize(p); }
void *MspaceAlloc::alloc(size_t sz) { return memalloc(sz, defaultmem); }
void *MspaceAlloc::tryAlloc(size_t sz) { return defaultmem->tryAlloc(sz); }
void *MspaceAlloc::allocAligned(size_t sz, size_t alignment) { return defaultmem->allocAligned(sz, alignment); }
bool MspaceAlloc::resizeInplace(void *p, size_t sz) { return defaultmem->resizeInplace(p, sz); }
void *MspaceAlloc::realloc(void *p, size_t sz) { return defaultmem->realloc(p, sz); }
void MspaceAlloc::free(void *p) { memfree(p, defaultmem); }
void MspaceAlloc::freeAligned(void *p) { defaultmem->freeAligned(p); }
void MspaceAlloc::getStatistics(size_t &a, size_t &b) { a = b = 0; }

#endif

MspaceAlloc *MspaceAlloc::create(void *base, size_t capacity, bool fixed) { return new MspaceAlloc(base, capacity, fixed); }
void MspaceAlloc::destroy() { delete this; }

#define EXPORT_PULL dll_pull_memory_mspaceAlloc
#include <supp/exportPull.h>
