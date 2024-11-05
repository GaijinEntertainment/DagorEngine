//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

typedef volatile int os_spinlock_t;

#include <supp/dag_define_KRNLIMP.h>
#include <perfMon/dag_daProfilerToken.h>
#include <osApiWrappers/dag_threadSafety.h>

KRNLIMP void os_spinlock_init(os_spinlock_t *lock);
KRNLIMP void os_spinlock_destroy(os_spinlock_t *lock);
KRNLIMP void os_spinlock_lock_raw(os_spinlock_t *lock); // no profiling
KRNLIMP void os_spinlock_lock(os_spinlock_t *lock, da_profiler::desc_id_t token = da_profiler::DescSpinlock);
KRNLIMP bool os_spinlock_trylock(os_spinlock_t *lock);
KRNLIMP void os_spinlock_unlock(os_spinlock_t *lock);

class DAG_TS_CAPABILITY("mutex") OSSpinlock
{
public:
  OSSpinlock() { os_spinlock_init(&spinlock); }
  ~OSSpinlock() { os_spinlock_destroy(&spinlock); }
  OSSpinlock(const OSSpinlock &) = delete;
  OSSpinlock &operator=(const OSSpinlock &) = delete;
  OSSpinlock(OSSpinlock &&) = default;
  OSSpinlock &operator=(OSSpinlock &&) = default;
  void lock(da_profiler::desc_id_t token = da_profiler::DescSpinlock) DAG_TS_ACQUIRE() { os_spinlock_lock(&spinlock, token); }
  bool tryLock() DAG_TS_TRY_ACQUIRE(true) { return os_spinlock_trylock(&spinlock); }
  void unlock() DAG_TS_RELEASE() { os_spinlock_unlock(&spinlock); }

protected:
  friend class OSSpinlockScopedLock;
  os_spinlock_t spinlock;
};

class DAG_TS_SCOPED_CAPABILITY OSSpinlockScopedLock
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
  OSSpinlockScopedLock(OSSpinlock &mtx, da_profiler::desc_id_t token = da_profiler::DescSpinlock) DAG_TS_ACQUIRE(mtx) :
    OSSpinlockScopedLock(mtx.spinlock, token)
  {}
  OSSpinlockScopedLock(OSSpinlock *mtx, da_profiler::desc_id_t token = da_profiler::DescSpinlock) DAG_TS_ACQUIRE(mtx) :
    OSSpinlockScopedLock(&mtx->spinlock, token)
  {}
  ~OSSpinlockScopedLock() DAG_TS_RELEASE()
  {
    if (mutex)
      os_spinlock_unlock(mutex);
  }
};

class DAG_TS_SCOPED_CAPABILITY OSSpinlockUniqueLock
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

  OSSpinlockUniqueLock(OSSpinlock &mtx) DAG_TS_ACQUIRE(mtx) : mutex{mtx}
  {
    mutex.lock();
    locked = true;
  }

  // Does not lock the mutex
  OSSpinlockUniqueLock(OSSpinlock &mtx, DeferLock) DAG_TS_EXCLUDES(mtx) : mutex{mtx} {}

  // Uses tryLock on success the this is in locked state on fail this is in unlocked state
  // NOTE: we lie about DAG_TS_ACQUIRE here because static analysis is not smart enough
  OSSpinlockUniqueLock(OSSpinlock &mtx, TryToLock) DAG_TS_ACQUIRE(mtx) : mutex{mtx} { locked = mutex.tryLock(); }

  // Assumes mtx is locked already so this state will be in locked state
  OSSpinlockUniqueLock(OSSpinlock &mtx, AdoptLock) DAG_TS_REQUIRES(mtx) : mutex{mtx}, locked{true} {}

  OSSpinlockUniqueLock(OSSpinlockUniqueLock &&other) : mutex{other.mutex}, locked{other.locked} { other.locked = false; }

  ~OSSpinlockUniqueLock() DAG_TS_RELEASE()
  {
    if (locked)
      mutex.unlock();
  }
  // returns true if it had released the lock
  bool unlock() DAG_TS_RELEASE()
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
  bool lock() DAG_TS_ACQUIRE()
  {
    if (!locked)
    {
      mutex.lock();
      locked = true;
      return true;
    }
    return false;
  }

  bool tryLock() DAG_TS_TRY_ACQUIRE(true)
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

#include <supp/dag_undef_KRNLIMP.h>
