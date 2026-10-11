// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

// std::condition_variable bucket impl of address wait, for platforms (or OS versions) without native one

#include <stdint.h>
#include <mutex>
#include <condition_variable>
#include <util/dag_globDef.h>
#include <osApiWrappers/dag_atomic.h>

namespace addr_wait_cv
{
inline uint64_t splitmix64(uint64_t z)
{
  z = (z ^ (z >> 30)) * UINT64_C(0xBF58476D1CE4E5B9);
  z = (z ^ (z >> 27)) * UINT64_C(0x94D049BB133111EB);
  return z ^ (z >> 31);
}

// Shared bucket cv, optimized for expensive cond var creation/destruction
struct alignas(128) WaitTableEntry
{
  std::mutex mtx;
  std::condition_variable cv;
  int numWaiters = 0;
};
inline WaitTableEntry waitTable[256];

inline WaitTableEntry &get_wait_table_entry(const void *addr)
{
  return waitTable[splitmix64(uintptr_t(addr) >> 2) & (countof(waitTable) - 1)];
}

inline void wait(volatile uint32_t *addr, const uint32_t *cmpaddr, int wait_ms)
{
  if (interlocked_acquire_load(*addr) != *cmpaddr)
    return;

  auto &entry = get_wait_table_entry((const void *)addr);
  std::unique_lock lock(entry.mtx);

  // Re-check under lock: closes lost-wake race with wakers. Do NOT remove.
  if (interlocked_acquire_load(*addr) != *cmpaddr)
    return;

  entry.numWaiters++;

  // Note: deliberately ignore spurious wake-ups since caller is assumed to be expecting them
  if (wait_ms < 0) [[likely]]
    entry.cv.wait(lock);
  else
    entry.cv.wait_for(lock, std::chrono::milliseconds(wait_ms));

  entry.numWaiters--;
}

inline void wake(volatile uint32_t *addr)
{
  auto &entry = get_wait_table_entry((const void *)addr);
  std::unique_lock lock(entry.mtx);
  if (entry.numWaiters)
    entry.cv.notify_all();
}
} // namespace addr_wait_cv
