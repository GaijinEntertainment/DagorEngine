//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

// https://en.wikipedia.org/wiki/Test_and_test-and-set lock
// it is basically exactly same as pthread_spinlock
// cache friendly spinlock inline only implementation
// see also https://rigtorp.se/spinlock/

#include <osApiWrappers/dag_atomic.h>
#include <osApiWrappers/dag_miscApi.h> //for yield and spin wait
#include <util/dag_compilerDefs.h>

// will sleep after few iterations, preferrable
inline void ttas_spinlock_lock(volatile int &lock, int taken = 1, int free = 0)
{
  // Optimistically assume the lock is free on the first try
  if (DAGOR_UNLIKELY(interlocked_exchange_acquire(lock, taken) != free))
    do
    {
      // Wait for lock to be released without generating cache misses
      // Issue X86 PAUSE or ARM YIELD instruction to reduce contention between
      // hyper-threads
      spin_wait_no_profile([&]() { return interlocked_relaxed_load(lock) != free; });
      // Note: intentionally use CMPXCHG here intead of XCHG as otherwise clang seems to refuse
      // put away this contended loop from critical code path
    } while (interlocked_compare_exchange(lock, taken, free) != free);
}

// busy loop, never sleeps
// do _not_ use it, unless you are absolutely sure you have to.
// it is not "faster" in any normal case, and if contention happens - it can be extremely slow and even dangerous
//  can only be used in these conditions:
//    * you know you have competing threads all in different cores
//    * you should not be waiting for anything long inside any lock, i.e. operations inside lock are trivial, not using io/other
//    resources
//    * basically, when you only try to protect from thread scheduling sleeps anyway
inline void ttas_spinlock_busylock(volatile int &lock, int taken = 1, int free = 0)
{
  for (;;)
  {
    // Optimistically assume the lock is free on the first try
    if (interlocked_exchange_acquire(lock, taken) == free)
      return;
    // Wait for lock to be released without generating cache misses
    // Issue X86 PAUSE or ARM YIELD instruction to reduce contention between
    // hyper-threads
    do
    {
      cpu_yield();
    } while (interlocked_relaxed_load(lock) != free);
  }
}

__forceinline void ttas_spinlock_unlock(volatile int &lock, int free = 0) { interlocked_release_store(lock, free); }

__forceinline bool ttas_spinlock_trylock(volatile int &lock, int taken = 1, int free = 0)
{
  // First do a relaxed load to check if lock is free in order to prevent
  // unnecessary cache misses if someone does while(!try_lock()) sleep (instead of doing lock!)
  return interlocked_relaxed_load(lock) == free && interlocked_exchange_acquire(lock, taken) == free;
}

// optimistic, just try exchange, we assume it is already taken
__forceinline bool ttas_spinlock_trylock_fast(volatile int &lock, int taken = 1, int free = 0)
{
  return interlocked_exchange_acquire(lock, taken) == free;
}
