//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <perfMon/dag_daProfilerToken.h>
#include <osApiWrappers/dag_threadSafety.h>
#include <osApiWrappers/dag_miscApi.h>
#include <osApiWrappers/dag_atomic.h>
#include <util/dag_compilerDefs.h>

// all primives in this file are very simple read-write locks designed around spinlock/atomics
// they are small (4 bytes), light-weighed, non-reentrant and NOT intent to be used in heavy contention cases!
// if you need "fair" or reentrant RWlocks, use OSReadWriteLock or OSReentrantReadWriteLock
// which are based on OS primitives (and they use limited OS resources).
// use spinlock primitives only on low contention cases, especially low "write"-contention cases


// Warn: not reentrant (recursive), use OSReentrantReadWriteLock if you need it
// This mimics OSReadWriteLock class interface over a single atomic word.
// Writer-preferring: a waiting writer turns away new readers, so with enough write contention readers starve.
// There is no ordering otherwise: waiters obtain access regardless of when they tried to.
struct DAG_TS_CAPABILITY("mutex") SpinLockReadWriteLock
{
  SpinLockReadWriteLock() = default;
  void lockRead(::da_profiler::desc_id_t profile_token = ::da_profiler::DescReadLock) DAG_TS_ACQUIRE_SHARED()
  {
    if (interlocked_increment(state) < WRITER_ONE) [[likely]]
      return;
    lockReadContended(profile_token);
  }
  void unlockRead() DAG_TS_RELEASE_SHARED() { interlocked_decrement(state); }

  void lockWrite(::da_profiler::desc_id_t profile_token = ::da_profiler::DescWriteLock) DAG_TS_ACQUIRE()
  {
    if (tryLockWrite()) [[likely]]
      return;
    ScopeLockProfiler<da_profiler::NoDesc> lp(profile_token);
    interlocked_add(state, WRITER_ONE); // announce: new readers back off, active ones drain
    for (;;)
    {
      uint32_t s = interlocked_acquire_load(state);
      if ((s & (READERS_MASK | LOCKED)) == 0 && interlocked_compare_exchange(state, s | LOCKED, s) == s)
        return;
      spin_wait_no_profile([&] { return (interlocked_acquire_load(state) & (READERS_MASK | LOCKED)) != 0; });
    }
  }
  void unlockWrite() DAG_TS_RELEASE() { interlocked_add(state, 0u - (LOCKED | WRITER_ONE)); }

protected:
  // state: bits 0..15 active readers, 16..30 waiting or owning writers, 31 write owned
  // at most 65535 reader increments (incl. temporary rejected ones) and 32767 registered writers (incl. the owner)
  static constexpr uint32_t WRITER_ONE = 1u << 16, READERS_MASK = WRITER_ONE - 1, LOCKED = 1u << 31;
  bool tryLockWrite() { return interlocked_compare_exchange(state, LOCKED | WRITER_ONE, 0u) == 0; }
  DAGOR_NOINLINE void lockReadContended(::da_profiler::desc_id_t profile_token) DAG_TS_ACQUIRE_SHARED()
  {
    interlocked_decrement(state); // a waiting reader must not count as active, or writers never drain
    ScopeLockProfiler<da_profiler::NoDesc> lp(profile_token);
    for (;;)
    {
      spin_wait_no_profile([&] { return interlocked_acquire_load(state) >= WRITER_ONE; });
      if (interlocked_increment(state) < WRITER_ONE)
        return;
      interlocked_decrement(state);
    }
  }
  SpinLockReadWriteLock(const SpinLockReadWriteLock &) = delete;
  SpinLockReadWriteLock &operator=(const SpinLockReadWriteLock &) = delete;
  volatile uint32_t state = 0;
};

using NoWritersSpinLockReadWriteLock = SpinLockReadWriteLock;
