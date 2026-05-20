//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <perfMon/dag_daProfilerToken.h>
#include <osApiWrappers/dag_threadSafety.h>
#include <osApiWrappers/dag_miscApi.h>
#include <osApiWrappers/dag_atomic.h>

// all primives in this file are very simple read-write locks designed around spinlock/atomics
// they are small (8 bytes), light-weighed, non-reentrant and NOT intent to be used in heavy contention cases!
// if you need "fair" or reentrant RWlocks, use OSReadWriteLock or OSReentrantReadWriteLock
// which are based on OS primitives (and they use limited OS resources).
// use spinlock primitives only on low contention cases, especially low "write"-contention cases


// Warn: not reentrant (recursive), use OSReentrantReadWriteLock if you need it
// This mimics OSReadWriteLock class interface over spinlock and atomic int16.
// this is Raynal's read-preferring RWlock, with additional explicit writers preferential
// compared to SpinLockReadWriteLock it is one relaxed_load more costly for lockRead, and two atomic counter cost for lockWrite
// this prioritize writers but still there is absolutely no ordering,
// meaning both writers and readers obtain access regardless on "when" they tried to.
// so, if there is enough writers contention, no any reader will ever gain access

struct DAG_TS_CAPABILITY("mutex") SpinLockReadWriteLock
{
  SpinLockReadWriteLock() = default;
  void lockRead(::da_profiler::desc_id_t profile_token = ::da_profiler::DescReadLock) DAG_TS_ACQUIRE_SHARED()
  {
    // first we try to wait till there are no 'waiting' writers, this will prioritize writers
    if (DAGOR_LIKELY(interlocked_relaxed_load(waitingWritersCount) == 0))
    {
      if (interlocked_increment(writerReadersWord) < WRITER_BIT)
        return;
      ScopeLockProfiler<da_profiler::NoDesc> lp(profile_token);
      spin_wait_no_profile([&] { return interlocked_acquire_load(writerReadersWord) >= WRITER_BIT; });
    }
    else
    {
      ScopeLockProfiler<da_profiler::NoDesc> lp(profile_token);
      spin_wait_no_profile([&] { return interlocked_acquire_load(waitingWritersCount) != 0; });
      if (interlocked_increment(writerReadersWord) >= WRITER_BIT)
        spin_wait_no_profile([&] { return interlocked_acquire_load(writerReadersWord) >= WRITER_BIT; });
    }
  }
  void unlockRead() DAG_TS_RELEASE_SHARED() { interlocked_decrement(writerReadersWord); }
  void lockWrite(::da_profiler::desc_id_t profile_token = ::da_profiler::DescWriteLock) DAG_TS_ACQUIRE()
  {
    interlocked_increment(waitingWritersCount);
    if (!tryLockWrite())
    {
      ScopeLockProfiler<da_profiler::NoDesc> lp(profile_token);
      do
        spin_wait_no_profile([&] { return interlocked_acquire_load(writerReadersWord) != 0; });
      while (!tryLockWrite());
    }
  }
  void unlockWrite() DAG_TS_RELEASE()
  {
    interlocked_and(writerReadersWord, ~WRITER_BIT);
    interlocked_decrement(waitingWritersCount);
  }

protected:
  static constexpr uint32_t WRITER_BIT = 1UL << 30;
  bool tryLockWrite() { return interlocked_compare_exchange(writerReadersWord, WRITER_BIT, 0) == 0; }
  SpinLockReadWriteLock(const SpinLockReadWriteLock &) = delete;
  SpinLockReadWriteLock &operator=(const SpinLockReadWriteLock &) = delete;
  volatile uint32_t writerReadersWord = 0, waitingWritersCount = 0;
};


// This one is even simpler, just 4 bytes.
// this is Raynal's read-preferring RWlock, without additional explicit writers preferential
// compared to SpinLockReadWriteLock it is one relaxed_load less costly for lockRead
// this prioritize readers but still there is absolutely no ordering,
// so, it should not ever be used on any writer contention case
// it is only implied to be used in a situation when there is first Write (load) with probably occasional readers
// and then only readers, without writes
struct DAG_TS_CAPABILITY("mutex") NoWritersSpinLockReadWriteLock
{
  NoWritersSpinLockReadWriteLock() = default;

  void lockRead(::da_profiler::desc_id_t profile_token = ::da_profiler::DescReadLock) DAG_TS_ACQUIRE_SHARED()
  {
    // Fast path: bump reader count. If WRITER_BIT wasn't set, we're in.
    if (DAGOR_LIKELY((interlocked_increment(writerReadersWord) & WRITER_BIT) == 0))
      return;
    // Writer is pending or holding. Back out our bump and wait for WRITER_BIT to clear.
    interlocked_decrement(writerReadersWord);
    ScopeLockProfiler<da_profiler::NoDesc> lp(profile_token);
    for (;;)
    {
      spin_wait_no_profile([&] { return (interlocked_acquire_load(writerReadersWord) & WRITER_BIT) != 0; });
      if ((interlocked_increment(writerReadersWord) & WRITER_BIT) == 0)
        return;
      interlocked_decrement(writerReadersWord);
    }
  }

  void unlockRead() DAG_TS_RELEASE_SHARED() { interlocked_decrement(writerReadersWord); }

  void lockWrite(::da_profiler::desc_id_t profile_token = ::da_profiler::DescWriteLock) DAG_TS_ACQUIRE()
  {
    // Fast path: no readers, no writer.
    if (DAGOR_LIKELY(tryLockWrite()))
      return;
    ScopeLockProfiler<da_profiler::NoDesc> lp(profile_token);
    // Step 1: claim WRITER_BIT (excludes other writers, blocks new readers).
    // Unlike tryLockWrite, this succeeds even with readers present, so existing
    // readers drain while new ones are turned away.
    for (;;)
    {
      uint32_t s = interlocked_relaxed_load(writerReadersWord);
      if ((s & WRITER_BIT) == 0 && interlocked_compare_exchange(writerReadersWord, s | WRITER_BIT, s) == s)
        break;
      spin_wait_no_profile([&] { return (interlocked_acquire_load(writerReadersWord) & WRITER_BIT) != 0; });
    }
    // Step 2: wait for in-flight readers to drain.
    spin_wait_no_profile([&] { return (interlocked_acquire_load(writerReadersWord) & ~WRITER_BIT) != 0; });
  }

  void unlockWrite() DAG_TS_RELEASE() { interlocked_and(writerReadersWord, ~WRITER_BIT); }

protected:
  static constexpr uint32_t WRITER_BIT = 1UL << 30;
  bool tryLockWrite() { return interlocked_compare_exchange(writerReadersWord, WRITER_BIT, 0) == 0; }
  NoWritersSpinLockReadWriteLock(const NoWritersSpinLockReadWriteLock &) = delete;
  NoWritersSpinLockReadWriteLock &operator=(const NoWritersSpinLockReadWriteLock &) = delete;
  volatile uint32_t writerReadersWord = 0;
};