//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <util/dag_oaHashNameMap.h>
#include <osApiWrappers/dag_rwLock.h>
#include <osApiWrappers/dag_atomic.h>

// thread safe FastNameMap (with read-write lock)
template <bool case_insensitive = false, typename RWLockMutexClass = OSReadWriteLock>
struct FastNameMapTS : protected OAHashNameMap<case_insensitive>
{
protected:
  uint32_t namesCount = 0;
  typedef OAHashNameMap<case_insensitive> BaseNameMap;
  mutable RWLockMutexClass lock; // Note: non reentrant

public:
  int getNameId(const char *name, size_t name_len, typename BaseNameMap::hash_t hash) const
  {
    lockRd();
    int id = BaseNameMap::getNameId(name, name_len, hash);
    unlockRd();
    return id;
  }
  int getNameId(const char *name, size_t name_len) const
  {
    return getNameId(name, name_len, BaseNameMap::string_hash(name, name_len));
  }
  int getNameId(const char *name) const { return getNameId(name, strlen(name)); }
  int addNameId(const char *name, size_t name_len, typename BaseNameMap::hash_t hash) // optimized version. addNameId doesn't call for
                                                                                      // getNameId
  {
    lockRd();
    int it = -1;
    bool foundCollision = false;
    if (DAGOR_LIKELY(BaseNameMap::noCollisions()))
    {
      it = BaseNameMap::hashToStringId.findOr(hash, -1);
      if (DAGOR_UNLIKELY(it != -1 && !BaseNameMap::string_equal(name, name_len, it)))
      {
        foundCollision = true;
        it = -1;
      }
    }
    else
      it =
        BaseNameMap::hashToStringId.findOr(hash, -1, [&, this](uint32_t id) { return BaseNameMap::string_equal(name, name_len, id); });
    unlockRd();

    if (it == -1)
    {
      lockWr();
      if (foundCollision)
        BaseNameMap::hasCollisions() = 1;
      uint32_t id = BaseNameMap::addString(name, name_len);
      BaseNameMap::hashToStringId.emplace(hash, eastl::move(id));
      interlocked_increment(namesCount);
      unlockWr();
      return (int)id;
    }
    return it;
  }
  int addNameId(const char *name, size_t name_len) { return addNameId(name, name_len, BaseNameMap::string_hash(name, name_len)); }
  int addNameId(const char *name) { return name ? addNameId(name, strlen(name)) : -1; }

  // almost full copy paste of addNameId
  // it is same as getName(addNameId(name)), but saves one lock (which can be expensive, especially in case of contention)
  const char *internName(const char *name, size_t name_len, typename BaseNameMap::hash_t hash) // optimized version. addNameId doesn't
                                                                                               // call for getNameId
  {
    lockRd();
    int it = -1;
    bool foundCollision = false;
    if (DAGOR_LIKELY(BaseNameMap::noCollisions()))
    {
      it = BaseNameMap::hashToStringId.findOr(hash, -1);
      if (DAGOR_UNLIKELY(it != -1 && !BaseNameMap::string_equal(name, name_len, it)))
      {
        foundCollision = true;
        it = -1;
      }
    }
    else
      it =
        BaseNameMap::hashToStringId.findOr(hash, -1, [&, this](uint32_t id) { return BaseNameMap::string_equal(name, name_len, id); });
    if (it != -1)
      name = BaseNameMap::getName(it);
    unlockRd();

    if (it == -1)
    {
      lockWr();
      if (foundCollision)
        BaseNameMap::hasCollisions() = 1;
      uint32_t id = BaseNameMap::addString(name, name_len);
      BaseNameMap::hashToStringId.emplace(hash, eastl::move(id));
      name = BaseNameMap::getName(id);
      interlocked_increment(namesCount);
      unlockWr();
      return name;
    }
    return name;
  }
  const char *internName(const char *name)
  {
    size_t name_len = strlen(name);
    return internName(name, name_len, BaseNameMap::string_hash(name, name_len));
  }

  uint32_t nameCountRelaxed() const { return interlocked_relaxed_load(namesCount); }
  uint32_t nameCountAcquire() const { return interlocked_acquire_load(namesCount); }
  uint32_t nameCount() const { return nameCountAcquire(); }
  const char *getName(int name_id) const
  {
    lockRd();
    const char *s = BaseNameMap::getName(name_id);
    unlockRd();
    return s;
  }
  void reset(bool erase_only = false)
  {
    lockWr();
    BaseNameMap::reset(erase_only);
    interlocked_relaxed_store(namesCount, 0);
    unlockWr();
  }
  void clear() { reset(false); }
  void shrink_to_fit()
  {
    lockWr();
    BaseNameMap::shrink_to_fit();
    unlockWr();
  }
  void memInfo(size_t &used, size_t &allocated)
  {
    lockRd();
    allocated = BaseNameMap::totalAllocated();
    used = BaseNameMap::totalUsed();
    unlockRd();
  }

  template <typename Cb>
  uint32_t iterate(Cb cb)
  {
    lockRd();
    uint32_t count = iterate_names((const BaseNameMap &)*this, cb);
    unlockRd();
    return count;
  }

private:
  void lockRd() const DAG_TS_ACQUIRE_SHARED(lock) { lock.lockRead(); }
  void unlockRd() const DAG_TS_RELEASE_SHARED(lock) { lock.unlockRead(); }
  void lockWr() const DAG_TS_ACQUIRE(lock) { lock.lockWrite(); }
  void unlockWr() const DAG_TS_RELEASE(lock) { lock.unlockWrite(); }
};
