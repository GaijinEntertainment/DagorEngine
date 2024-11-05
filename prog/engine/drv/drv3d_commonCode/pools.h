// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/fixed_vector.h>
#include <EASTL/unique_ptr.h>
#include <generic/dag_span.h>
#include <generic/dag_tabExt.h>
#include <osApiWrappers/dag_critSec.h>
#include <stdio.h>
#include <string.h>
#include "keyMapPools.h"

namespace drv3d_generic
{
static constexpr int BAD_HANDLE = -1;
// T has it's following containts/limitations:
//  * memcpy relocatable (i.e doesn't has self refertial pointers)
//  * LSB is 0 (e.g. if [first member of] T is pointer then it's lower bit is zero (for reasonably aligned T))
template <typename T, int InitialCapacity = 128>
struct PodPool
{
  union Entry
  {
    T elem;
    int link; // LSB is set if this Entry is free
    bool isUsed() const { return (link & 1) == 0; }
  };

  Entry *entries = nullptr;                                   //[_max]
  eastl::fixed_vector<eastl::unique_ptr<Entry[]>, 1> garbage; // entries to be deleted
#if DAGOR_DBGLEVEL > 0
  const char *debugName = nullptr;
#endif

  int freeList = BAD_HANDLE;
  int totalElements = 0;
  int usedElements = 0;

  PodPool(const char *dbg_name = nullptr)
#if DAGOR_DBGLEVEL > 0
    :
    debugName(dbg_name)
#endif
  {
    G_UNUSED(dbg_name);
  }

  void destroy()
  {
    delete[] entries;
    entries = NULL;
    garbage.clear(/*freeOverflow*/ true);
    freeList = BAD_HANDLE;
    totalElements = 0;
    usedElements = 0;
  }

  ~PodPool() { destroy(); }

  void clearGarbage() { garbage.clear(); }

  int size() { return totalElements; }

  int totalUsed() { return usedElements; }

  bool isIndexValid(int index) { return (unsigned)index < totalElements; }

  T &operator[](int index)
  {
    G_FAST_ASSERT(isIndexValid(index));
    return entries[index].elem;
  }

  bool reserve(int elements)
  {
    G_ASSERT(elements > 0);

    if (elements <= totalElements)
      return true;

#if DAGOR_DBGLEVEL > 0
    debug("%s reserve %u -> %u elements", debugName, totalElements, elements);
#endif

    Entry *newEntries = new Entry[elements];
    if (totalElements > 0)
    {
      G_FAST_ASSERT(entries != NULL);
      memcpy(newEntries, entries, sizeof(Entry) * totalElements);
      garbage.emplace_back(entries);
    }

    for (int i = totalElements; i < elements; i++)
      newEntries[i].link = ((i + 1) << 1) | 1;
    newEntries[elements - 1].link = (freeList << 1) | 1;

    entries = newEntries;
    freeList = totalElements;
    totalElements = elements;
    return true;
  }

  int alloc()
  {
    if (totalElements == usedElements)
      reserve(totalElements ? (totalElements + totalElements / 2) : InitialCapacity); // x1.5 grow factor

    G_FAST_ASSERT(freeList >= 0);
    int index = freeList;
    freeList = entries[index].link >> 1; // unlink from free list
    usedElements++;

    return index;
  }

  bool isIndexInFreeListUnsafe(int idx) const
  {
    int index = freeList;
    while (index != BAD_HANDLE)
    {
      if (idx == index)
        return true;
      index = entries[index].link >> 1;
    }
    return false;
  }

  bool isEntryUsed(int i) const { return entries[i].isUsed(); }
  void fillEntryAsInvalid(int i) { entries[i].link |= 1; }
};

// lock garantiee both valid structure and contents
template <typename T, int InitialCapacity = 128>
struct PodPoolWithLock : protected PodPool<T, InitialCapacity>
{
  typedef PodPool<T, InitialCapacity> PodPoolType;

protected:
  mutable CritSecStorage critSec;
  using PodPoolType::destroy;
  using PodPoolType::entries;
  using PodPoolType::freeList;
  using PodPoolType::reserve;
  using PodPoolType::usedElements;

public:
  // Must be locked beforehand.

  using PodPoolType::alloc;
  using PodPoolType::operator[];
  using PodPoolType::clearGarbage;
  using PodPoolType::fillEntryAsInvalid;
  using PodPoolType::isEntryUsed;
  using PodPoolType::isIndexValid;
  using PodPoolType::size;
  using PodPoolType::totalUsed;


  class AutoLock;
  friend class AutoLock;
  class AutoLock
  {
  private:
    void *pCritSec;
    AutoLock(const AutoLock &) = delete;
    AutoLock &operator=(const AutoLock &) = delete;
    friend struct PodPoolWithLock;

  public:
    AutoLock(PodPoolWithLock &op) : pCritSec(op.critSec) { ::enter_critical_section(pCritSec); }
    ~AutoLock() { ::leave_critical_section(pCritSec); }
  };

  PodPoolWithLock(const char *dbg_name) : PodPool<T, InitialCapacity>(dbg_name) //-V730
  {
    ::create_critical_section(critSec);
  }
  ~PodPoolWithLock() { ::destroy_critical_section(critSec); }
  void lock() const { ::enter_critical_section(critSec); }
  void unlock() const { ::leave_critical_section(critSec); }

  // returns handle, returns in locked state if allocation succeeded
  int safeAllocAndSet(T value)
  {
    lock();
    int index = alloc();
    G_FAST_ASSERT(index != BAD_HANDLE);
    entries[index].elem = value;
    G_FAST_ASSERT(entries[index].isUsed());
    unlock();
    return index;
  }

  void safeReleaseEntry(int index)
  {
    if (index == BAD_HANDLE)
      return;

    lock();
    G_ASSERT(isIndexValid(index));
    releaseEntryUnsafe(index);
    unlock();
  }

  bool safeReserve(int elements)
  {
    lock();
    bool res = reserve(elements);
    unlock();
    return res;
  }

  void safeDestroy()
  {
    lock();
    destroy();
    unlock();
  }


  bool isIndexInFreeList(int idx) const
  {
    lock();
    bool r = PodPool<T, InitialCapacity>::isIndexInFreeListUnsafe(idx);
    unlock();
    return r;
  }

  void releaseEntryUnsafe(int index)
  {
    G_FAST_ASSERT(isIndexValid(index));
    entries[index].link = (freeList << 1) | 1;
    freeList = index;
    G_VERIFY(--usedElements >= 0);
  }
};

// PodPool adapted to work with objects
// pool type must implement releaseObject method to clean up data
template <typename T>
struct ObjectProxyPtr
{
  T *obj;

  void destroyObject()
  {
    if (obj)
      obj->destroyObject();
    obj = NULL;
  }
};

template <typename T>
struct COMProxyPtr
{
  T *obj;

  void destroyObject()
  {
    if (obj)
      obj->Release();
    obj = NULL;
  }
};
template <typename T>
struct SimpleObjectProxyPtr
{
  T *obj;
  inline operator T *() const { return obj; }
  inline T &operator*() const { return *obj; }
  inline T *operator->() const { return obj; }
  inline T *get() const { return obj; }
  explicit operator bool() const { return obj != NULL; }
  void destroyObject() { del_it(obj); }
};

template <typename EntryType, int InitialCapacity = 128>
struct ObjectPoolWithLock : PodPoolWithLock<EntryType, InitialCapacity>
{
public:
  typedef PodPoolWithLock<EntryType, InitialCapacity> PodPoolWithLockType;
  using PodPoolWithLockType::clearGarbage;
  using PodPoolWithLockType::isEntryUsed;
  using PodPoolWithLockType::isIndexInFreeListUnsafe;
  using PodPoolWithLockType::isIndexValid;
  using PodPoolWithLockType::lock;
  using PodPoolWithLockType::releaseEntryUnsafe;
  using PodPoolWithLockType::safeDestroy;
  using PodPoolWithLockType::safeReleaseEntry;
  using PodPoolWithLockType::size;
  using PodPoolWithLockType::unlock;


  ObjectPoolWithLock(const char *dbg_name) : PodPoolWithLockType(dbg_name) {}
  ~ObjectPoolWithLock() { clear(); }

  bool release(int handle)
  {
    if (handle == BAD_HANDLE)
      return false;

    lock();
    G_FAST_ASSERT(isIndexValid(handle));
    (*this)[handle].destroyObject();
    releaseEntryUnsafe(handle);
    unlock();
    return true;
  }

  void clear()
  {
    lock();
    for (int i = 0; i < size(); i++)
      if (isEntryUsed(i))
        (*this)[i].destroyObject();
    PodPoolWithLockType::destroy();
    unlock();
  }
};

#define ITERATE_OVER_OBJECT_POOL(obj, index)                \
  obj.lock();                                               \
  for (int index = 0, sz = obj.size(); index < sz; index++) \
  {                                                         \
    if (obj.isEntryUsed(index))                             \
    {

#define ITERATE_OVER_OBJECT_POOL_RESTORE(obj) \
  }                                           \
  }                                           \
  obj.unlock();

#define ITERATE_OVER_OBJECT_POOL_DESTROY(obj) \
  }                                           \
  }                                           \
  obj.safeDestroy();                          \
  obj.unlock();

}
