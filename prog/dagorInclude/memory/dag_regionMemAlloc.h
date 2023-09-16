//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <debug/dag_assert.h>

struct RegionMemPool;
struct RegionMemPoolHeader;
class RegionMemAlloc;


struct RegionMemPoolHeader
{
  typedef char __EightChars[8];

  RegionMemPool *next;
  size_t used;
  __EightChars data;
};

struct RegionMemPool : public RegionMemPoolHeader
{
  static constexpr int REGMEM_HDRSZ = sizeof(RegionMemPoolHeader) - sizeof(RegionMemPoolHeader::__EightChars);

  //! allocates memory in pool with specified alignment
  static inline void *alloc(RegionMemPool *p, size_t sz, size_t align, int min_gran)
  {
    if (sz == 0)
      return NULL;

    size_t pool_sz = midmem->getSize(p) - REGMEM_HDRSZ;
    size_t used = (int)((uintptr_t(p->data) + p->used + align - 1) / align * align - uintptr_t(p->data));

    while (used + sz > pool_sz)
    {
      if (!p->next)
      {
        if (!min_gran)
          return nullptr;
        p->next = createPool(sz + align < min_gran ? min_gran : sz + align);
      }
      p = p->next;
      pool_sz = midmem->getSize(p) - REGMEM_HDRSZ;
      used = (int)((uintptr_t(p->data) + p->used + align - 1) / align * align - uintptr_t(p->data));
    }

    void *ptr = p->data + used;
    p->used = used + sz;
    return ptr;
  }

  //! clears pool chain to be reused for new allocations
  static void clear(RegionMemPool *p)
  {
    while (p)
    {
      p->used = 0;
      p = p->next;
    }
  }

  //! create new pool with usable size at least 'sz'
  static RegionMemPool *createPool(size_t sz)
  {
    RegionMemPool *p = (RegionMemPool *)midmem->tryAlloc(sz + REGMEM_HDRSZ);
    if (p)
    {
      p->next = NULL;
      p->used = 0;
    }
    return p;
  }

  //! deletes 'pool' and all pools linked to it
  static void deletePool(RegionMemPool *p)
  {
    while (p)
    {
      RegionMemPool *next = p->next;
      midmem->free(p);
      p = next;
    }
  }

  //! returns total used data size
  static size_t getPoolUsedSize(const RegionMemPool *p)
  {
    size_t total = 0;
    for (; p; p = p->next)
      total += p->used;
    return total;
  }

  //! returns total memory size allocated from system
  static size_t getPoolAllocatedSize(const RegionMemPool *p)
  {
    size_t total = 0;
    for (; p; p = p->next)
      total += midmem->getSize((void *)p);
    return total;
  }

  //! returns number of pools used
  static unsigned getPoolsCount(const RegionMemPool *p)
  {
    unsigned cnt = 0;
    for (; p; p = p->next)
      cnt++;
    return cnt;
  }
};


class RegionMemAlloc : public IMemAlloc
{
public:
  //! creates allocator with no working RegionMemPool
  RegionMemAlloc() = default;

  // creates allocator with auto-delete pool of size 'init_sz'
  RegionMemAlloc(int init_sz, int next_sz = 16 << 10) { initPool(init_sz, next_sz); }

  //! creates allocator with and sets working pool (which can be optionally auto-deleted)
  RegionMemAlloc(RegionMemPool *p, bool auto_delete) { setPool(p, auto_delete); }

  //! destructor, also frees pool if delPool=true
  ~RegionMemAlloc() { releasePool(); }

  // returns pointer to pool and clears current working pool
  RegionMemPool *takePool()
  {
    RegionMemPool *p = pool;
    pool = NULL;
    lastPtr = NULL;
    lastPtrSz = 0;
    return p;
  }

  //! returns read-only pointer to pool
  const RegionMemPool *getPool() const { return pool; }

  // clears current working pool (deletes pool if auto-delete active)
  void releasePool()
  {
    if (delPool && pool)
      RegionMemPool::deletePool(pool);
    pool = NULL;
    lastPtr = NULL;
    lastPtrSz = 0;
  }

  //! creates new working pool for allocator
  void initPool(int init_sz, int next_sz)
  {
    releasePool();
    pool = RegionMemPool::createPool(init_sz);
    nextSz = next_sz;
    delPool = true;
  }

  //! sets new working pool for allocator
  void setPool(RegionMemPool *p, bool auto_delete)
  {
    releasePool();
    pool = p;
    lastPtr = NULL;
    lastPtrSz = 0;
    delPool = auto_delete;
  }

  //! set/get alignment for allocations
  void setAlign(int align) { curAlign = align > 0 ? align : 4096; }
  int getAlign() const { return curAlign; }

  size_t getPoolUsedSize() const { return RegionMemPool::getPoolUsedSize(pool); }
  size_t getPoolAllocatedSize() const { return RegionMemPool::getPoolAllocatedSize(pool); }
  unsigned getPoolsCount() const { return RegionMemPool::getPoolsCount(pool); }

  // IMemAlloc interface
  void destroy() override { delete this; }
  bool isEmpty() override { return pool != NULL; }
  size_t getSize(void *p) override { return (p == lastPtr) ? lastPtrSz : 0; }
  void *alloc(size_t sz) override
  {
    void *p = RegionMemAlloc::tryAlloc(sz);
    G_ASSERTF(p, "alloc(%d): OOM while used %dK (of %dK allocated in %u pools)", sz, getPoolUsedSize() >> 10,
      getPoolAllocatedSize() >> 10, getPoolsCount());
    return p;
  }
  void *tryAlloc(size_t sz) override
  {
    lastPtr = pool ? RegionMemPool::alloc(pool, sz, curAlign, nextSz) : NULL;
    lastPtrSz = lastPtr ? sz : 0;
    return lastPtr;
  }
  void *allocAligned(size_t sz, size_t align) override
  {
    lastPtr = pool ? RegionMemPool::alloc(pool, sz, align, nextSz) : NULL;
    G_ASSERTF(lastPtr, "allocAligned(%d, %d): OOM while used %dK (of %dK allocated in %u pools)", sz, align, getPoolUsedSize() >> 10,
      getPoolAllocatedSize() >> 10, getPoolsCount());
    lastPtrSz = lastPtr ? sz : 0;
    return lastPtr;
  }
  void *realloc(void *p, size_t sz) override
  {
    if (!p)
      return alloc(sz);
    if (resizeInplace(p, sz))
      return p;
    void *p2 = alloc(sz);
    G_ASSERTF_RETURN(p2, nullptr, "realloc(%p, %d): OOM while used %dK (of %dK allocated in %u pools)", p, sz, getPoolUsedSize() >> 10,
      getPoolAllocatedSize() >> 10, getPoolsCount());
    size_t max_p_sz = (char *)p2 - (char *)p; //== we don't know size of previous block but know that max limit
    if (max_p_sz > sz)
      max_p_sz = sz;
    for (RegionMemPool *mp = pool; mp; mp = mp->next)
      if ((char *)p >= mp->data && (char *)p <= mp->data + mp->used)
      {
        size_t used_sz = mp->data + mp->used - (char *)p;
        if (max_p_sz > used_sz)
          max_p_sz = used_sz;
        break;
      }
    memcpy(p2, p, max_p_sz);
    return p2;
  }
  bool resizeInplace(void *p, size_t sz) override
  {
    if (p != lastPtr)
      return false;
    if (sz <= lastPtrSz)
      return true;
    for (RegionMemPool *mp = pool; mp; mp = mp->next)
      if ((char *)p >= mp->data && (char *)p <= mp->data + mp->used)
      {
        size_t pool_sz = midmem->getSize(mp) - mp->REGMEM_HDRSZ;
        if ((char *)p + sz > mp->data + pool_sz)
          return false;

        mp->used += sz - lastPtrSz;
        lastPtrSz = sz;
        return true;
      }
    return false;
  }

  void free(void * /*p*/) override {}
  void freeAligned(void * /*p*/) override {}

private:
  RegionMemPool *pool = nullptr;
  void *lastPtr = nullptr;
  size_t lastPtrSz = 0;
  int nextSz = 16 << 10;
  int curAlign = 16;
  bool delPool = true;
};
