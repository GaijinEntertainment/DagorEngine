//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <util/dag_hash.h>
#include <osApiWrappers/dag_atomic.h>
#include <debug/dag_assert.h>

#if DAGOR_DBGLEVEL > 0

// Asserts when scopes tagged with the same name overlap, from another thread or by
// reentry on the same one: it verifies external serialization, it is not a lock.
// Same-name scopes share one counter across functions and translation units.
template <uint32_t name_hash>
struct ConcurrencyChecker
{
  static inline volatile int busy = 0;

  ConcurrencyChecker(const char *name)
  {
    G_ASSERTF(interlocked_increment(busy) == 1, "Concurrency checker caught a race with %s!", name);
  }
  ~ConcurrencyChecker() { interlocked_decrement(busy); }
};
#define CONCURRENCY_CHECKER(name) ConcurrencyChecker<str_hash_fnv1(#name)> concurrencyChecker_##name(#name)
#else
#define CONCURRENCY_CHECKER(name) ((void)0)
#endif
