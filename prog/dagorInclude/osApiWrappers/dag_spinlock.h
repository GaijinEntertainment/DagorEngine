//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <supp/dag_define_KRNLIMP.h>
#include <util/dag_compilerDefs.h>
#include <osApiWrappers/dag_atomic.h>
#if _TARGET_PC_LINUX && DAGOR_THREAD_SANITIZER // we have to use pthread's spinlock API or it would be not recognized by TSAN
#include <pthread.h>
#endif
#include <perfMon/dag_daProfilerToken.h>
#include <osApiWrappers/dag_threadSafety.h>

namespace dag::spinlock_internal
{
enum
{
  LOCK_IS_FREE = 0,
  LOCK_IS_TAKEN_NO_WAITERS = 1,
  LOCK_IS_TAKEN_CONTENDED = 2
};
}

typedef volatile int os_spinlock_t;

KRNLIMP void os_spinlock_init(os_spinlock_t *lock);
KRNLIMP void os_spinlock_destroy(os_spinlock_t *lock);
inline bool os_spinlock_trylock(os_spinlock_t *lock)
{
  using namespace dag::spinlock_internal;
#if _TARGET_PC_LINUX && DAGOR_THREAD_SANITIZER
  static_assert(sizeof(*lock) >= sizeof(pthread_spinlock_t), "bad os_spinlock_t type!");
  return pthread_spin_trylock(lock) == 0;
#elif _TARGET_PC_WIN || _TARGET_XBOX || _TARGET_C2
  return interlocked_compare_exchange(*lock, LOCK_IS_TAKEN_NO_WAITERS, LOCK_IS_FREE) == LOCK_IS_FREE;
#else
  return interlocked_exchange(*lock, LOCK_IS_TAKEN_NO_WAITERS) == LOCK_IS_FREE;
#endif
}
KRNLIMP void DAGOR_NOINLINE os_spinlock_lock_contended(os_spinlock_t *lock, da_profiler::desc_id_t token);
#if _TARGET_PC_WIN || _TARGET_XBOX || _TARGET_C2
KRNLIMP void os_spinlock_unlock_contended(os_spinlock_t *lock);
#endif
inline void os_spinlock_lock(os_spinlock_t *lock, da_profiler::desc_id_t token = da_profiler::DescSpinlock)
{
  if (!os_spinlock_trylock(lock)) [[unlikely]]
    os_spinlock_lock_contended(lock, token);
}
inline void os_spinlock_unlock(os_spinlock_t *lock)
{
  using namespace dag::spinlock_internal;
#if _TARGET_PC_LINUX && DAGOR_THREAD_SANITIZER
  pthread_spin_unlock(lock);
#elif _TARGET_PC_WIN || _TARGET_XBOX || _TARGET_C2
  if (interlocked_exchange(*lock, LOCK_IS_FREE) == LOCK_IS_TAKEN_CONTENDED)
    os_spinlock_unlock_contended(lock);
#else
  interlocked_release_store(*lock, LOCK_IS_FREE);
#endif
}

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
