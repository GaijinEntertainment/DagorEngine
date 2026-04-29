// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <memory/dag_MspaceAlloc.h>
#include <debug/dag_fatal.h>
#include <debug/dag_debug.h>

#if _TARGET_PC_WIN && !DAGOR_PREFER_HEAP_ALLOCATION
#include <windows.h>

/* static */
MspaceAlloc *MspaceAlloc::create(size_t initial_size)
{
  HANDLE heap = HeapCreate(HEAP_NO_SERIALIZE, initial_size, 0);
  G_ASSERT_RETURN(heap, nullptr);
  void *mptr = HeapAlloc(heap, 0, sizeof(MspaceAlloc));
  return new (mptr, _NEW_INPLACE) MspaceAlloc(heap);
}

void MspaceAlloc::destroy()
{
  if (heap)
    G_VERIFY(HeapDestroy(heap));
}

size_t MspaceAlloc::getSize(void *p) { return HeapSize(heap, 0, p); }

void *MspaceAlloc::alloc(size_t sz) { return HeapAlloc(heap, HEAP_GENERATE_EXCEPTIONS | HEAP_NO_SERIALIZE, sz); }

void *MspaceAlloc::tryAlloc(size_t sz) { return HeapAlloc(heap, 0, sz); }

void *MspaceAlloc::allocAligned(size_t sz, size_t algn)
{
  algn = algn ? algn : 4096;
  void *p = this->alloc(sz + algn);
  void *ptr = (char *)p + (algn - (uintptr_t(p) & (algn - 1)));
  ((void **)ptr)[-1] = p;
  return ptr;
}

bool MspaceAlloc::resizeInplace(void *p, size_t sz) { return sz <= getSize(p); }

void *MspaceAlloc::realloc(void *p, size_t sz)
{
  return p ? HeapReAlloc(heap, HEAP_GENERATE_EXCEPTIONS | HEAP_NO_SERIALIZE, p, sz) : this->alloc(sz);
}

void MspaceAlloc::free(void *p) { HeapFree(heap, 0, p); }

void MspaceAlloc::freeAligned(void *p)
{
  if (p)
    HeapFree(heap, 0, ((void **)p)[-1]);
}

#else // dummy implementation using global heap

size_t MspaceAlloc::getSize(void *p) { return defaultmem->getSize(p); }
void *MspaceAlloc::alloc(size_t sz) { return memalloc(sz, defaultmem); }
void *MspaceAlloc::tryAlloc(size_t sz) { return defaultmem->tryAlloc(sz); }
void *MspaceAlloc::allocAligned(size_t sz, size_t alignment) { return defaultmem->allocAligned(sz, alignment); }
bool MspaceAlloc::resizeInplace(void *p, size_t sz) { return defaultmem->resizeInplace(p, sz); }
void *MspaceAlloc::realloc(void *p, size_t sz) { return defaultmem->realloc(p, sz); }
void MspaceAlloc::free(void *p) { memfree(p, defaultmem); }
void MspaceAlloc::freeAligned(void *p) { defaultmem->freeAligned(p); }

MspaceAlloc *MspaceAlloc::create(size_t) { return new MspaceAlloc(0); }
void MspaceAlloc::destroy() { delete this; }
#endif


#define EXPORT_PULL dll_pull_memory_mspaceAlloc
#include <supp/exportPull.h>
