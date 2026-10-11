//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <osApiWrappers/dag_spinlock.h>
#include <osApiWrappers/dag_critSec.h>
#include <osApiWrappers/dag_threadSafety.h>
#if !DAG_SPINLOCK_IS_FUTEX_MUTEX
#include <pthread.h>
#endif

namespace dag
{

class DAG_TS_CAPABILITY("mutex") mutex
{
public:
  constexpr mutex() noexcept = default;
#if !DAG_SPINLOCK_IS_FUTEX_MUTEX
  ~mutex() { pthread_mutex_destroy(&mtx); }
#endif
  mutex(const mutex &) = delete;
  mutex &operator=(const mutex &) = delete;

#if DAG_SPINLOCK_IS_FUTEX_MUTEX
  void lock() DAG_TS_ACQUIRE() { os_spinlock_lock(&state, da_profiler::DescMutex); }
  bool try_lock() DAG_TS_TRY_ACQUIRE(true) { return os_spinlock_trylock(&state); }
  void unlock() DAG_TS_RELEASE() { os_spinlock_unlock(&state); }
#else
  void lock() DAG_TS_ACQUIRE() { pthread_mutex_lock(&mtx); }
  bool try_lock() DAG_TS_TRY_ACQUIRE(true) { return pthread_mutex_trylock(&mtx) == 0; }
  void unlock() DAG_TS_RELEASE() { pthread_mutex_unlock(&mtx); }
#endif

private:
#if DAG_SPINLOCK_IS_FUTEX_MUTEX
  os_spinlock_t state = dag::spinlock_internal::LOCK_IS_FREE;
#else
  pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
#endif
};

class DAG_TS_CAPABILITY("mutex") DAG_TS_REENTRANT_CAPABILITY recursive_mutex
{
public:
  recursive_mutex() = default;
  recursive_mutex(const recursive_mutex &) = delete;
  recursive_mutex &operator=(const recursive_mutex &) = delete;

  void lock() DAG_TS_ACQUIRE() { cs.lock(); }
  bool try_lock() DAG_TS_TRY_ACQUIRE(true) { return cs.tryLock(); }
  void unlock() DAG_TS_RELEASE() { cs.unlock(); }

private:
  WinCritSec cs;
};

template <typename Mutex>
class DAG_TS_SCOPED_CAPABILITY lock_guard
{
public:
  explicit lock_guard(Mutex &m) DAG_TS_ACQUIRE(m) : mtx(m) { mtx.lock(); }
  ~lock_guard() DAG_TS_RELEASE() { mtx.unlock(); }
  lock_guard(const lock_guard &) = delete;
  lock_guard &operator=(const lock_guard &) = delete;

private:
  Mutex &mtx;
};

} // namespace dag
