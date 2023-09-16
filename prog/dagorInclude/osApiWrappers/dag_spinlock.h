//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

typedef volatile int os_spinlock_t;

#include <supp/dag_define_COREIMP.h>
#include <perfMon/dag_daProfilerToken.h>

KRNLIMP void os_spinlock_init(os_spinlock_t *lock);
KRNLIMP void os_spinlock_destroy(os_spinlock_t *lock);
KRNLIMP void os_spinlock_lock_raw(os_spinlock_t *lock); // no profiling
KRNLIMP void os_spinlock_lock(os_spinlock_t *lock, da_profiler::desc_id_t token = da_profiler::DescSpinlock);
KRNLIMP bool os_spinlock_trylock(os_spinlock_t *lock);
KRNLIMP void os_spinlock_unlock(os_spinlock_t *lock);

class OSSpinlock
{
public:
  OSSpinlock() { os_spinlock_init(&spinlock); }
  ~OSSpinlock() { os_spinlock_destroy(&spinlock); }
  OSSpinlock(const OSSpinlock &) = delete;
  OSSpinlock &operator=(const OSSpinlock &) = delete;
  OSSpinlock(OSSpinlock &&) = default;
  OSSpinlock &operator=(OSSpinlock &&) = default;
  void lock(da_profiler::desc_id_t token = da_profiler::DescSpinlock) { os_spinlock_lock(&spinlock, token); }
  bool tryLock() { return os_spinlock_trylock(&spinlock); }
  void unlock() { os_spinlock_unlock(&spinlock); }

protected:
  friend class OSSpinlockScopedLock;
  os_spinlock_t spinlock;
};

class OSSpinlockScopedLock
{
  os_spinlock_t *mutex;

public:
  OSSpinlockScopedLock(os_spinlock_t &spmtx, da_profiler::desc_id_t token = da_profiler::DescSpinlock) : mutex{&spmtx}
  {
    os_spinlock_lock(mutex, token);
  }

  OSSpinlockScopedLock(os_spinlock_t *spmtx, da_profiler::desc_id_t token = da_profiler::DescSpinlock) : mutex{spmtx}
  {
    if (mutex)
      os_spinlock_lock(mutex, token);
  }
  OSSpinlockScopedLock(OSSpinlock &mtx, da_profiler::desc_id_t token = da_profiler::DescSpinlock) :
    OSSpinlockScopedLock(mtx.spinlock, token)
  {}
  OSSpinlockScopedLock(OSSpinlock *mtx, da_profiler::desc_id_t token = da_profiler::DescSpinlock) :
    OSSpinlockScopedLock(&mtx->spinlock, token)
  {}
  ~OSSpinlockScopedLock()
  {
    if (mutex)
      os_spinlock_unlock(mutex);
  }
};

class OSSpinlockUniqueLock
{
  OSSpinlock &mutex;
  bool locked = false;

public:
  struct DeferLock
  {};
  struct TryToLock
  {};
  struct AdoptLock
  {};

  OSSpinlockUniqueLock(OSSpinlock &mtx) : mutex{mtx}
  {
    mutex.lock();
    locked = true;
  }

  // Does not lock the mutex
  OSSpinlockUniqueLock(OSSpinlock &mtx, DeferLock) : mutex{mtx} {}

  // Uses tryLock on success the this is in locked state on fail this is in unlocked state
  OSSpinlockUniqueLock(OSSpinlock &mtx, TryToLock) : mutex{mtx} { locked = mutex.tryLock(); }

  // Assumes mtx is locked already so this state will be in locked state
  OSSpinlockUniqueLock(OSSpinlock &mtx, AdoptLock) : mutex{mtx}, locked{true} {}

  OSSpinlockUniqueLock(OSSpinlockUniqueLock &&other) : mutex{other.mutex}, locked{other.locked} { other.locked = false; }

  ~OSSpinlockUniqueLock()
  {
    if (locked)
      mutex.unlock();
  }
  // returns true if it had released the lock
  bool unlock()
  {
    if (locked)
    {
      mutex.unlock();
      locked = false;
      return true;
    }
    return false;
  }
  // returns true if it had acquired the lock
  bool lock()
  {
    if (!locked)
    {
      mutex.lock();
      locked = true;
      return true;
    }
    return false;
  }

  bool tryLock()
  {
    if (!locked)
    {
      locked = mutex.tryLock();
    }
    return locked;
  }

  bool ownsLock() const { return locked; }

  explicit operator bool() const { return ownsLock(); }
};

#include <supp/dag_undef_COREIMP.h>
