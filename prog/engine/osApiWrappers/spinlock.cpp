// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <osApiWrappers/dag_lockProfiler.h>
#include <osApiWrappers/dag_ttas_spinlock.h>
#include <osApiWrappers/dag_spinlock.h>
#include <util/dag_compilerDefs.h>
#include <util/dag_globDef.h>                  // min
#if _TARGET_PC_LINUX && DAGOR_THREAD_SANITIZER // we have to use pthread's spinlock API or it would be not recognized by TSAN
#include <pthread.h>
#define USE_NATIVE_PTHREAD_API 1
#else
#if _TARGET_PC_WIN || _TARGET_XBOX || _TARGET_C2
#include <osApiWrappers/dag_addressWait.h>
#endif
using namespace dag::spinlock_internal;
#endif

void os_spinlock_init(os_spinlock_t *lock)
{
#if USE_NATIVE_PTHREAD_API
  static_assert(sizeof(*lock) >= sizeof(pthread_spinlock_t), "bad os_spinlock_t type!");
  pthread_spin_init(lock, PTHREAD_PROCESS_PRIVATE);
#else
  interlocked_relaxed_store(*lock, LOCK_IS_FREE); // relaxed store is fine since this is an initializing store (even on weakly ordered
                                                  // arch like ARM)
#endif
}

void os_spinlock_destroy(os_spinlock_t *lock)
{
#if USE_NATIVE_PTHREAD_API
  pthread_spin_destroy(lock);
#else
  (void)(lock);
#endif
}

#if _TARGET_PC_WIN || _TARGET_XBOX || _TARGET_C2
void os_spinlock_unlock_contended(os_spinlock_t *lock) { os_wake_on_address_one((uint32_t *)lock); }
#endif

void os_spinlock_lock_contended(os_spinlock_t *lock, da_profiler::desc_id_t token)
{
  ScopeLockProfiler<da_profiler::NoDesc, /*usec_threshold*/ 0> lp(token);
#if USE_NATIVE_PTHREAD_API
  pthread_spin_lock(lock);
#elif _TARGET_PC_WIN || _TARGET_XBOX || _TARGET_C2
  // Phase 1: short spin, in case the holder is about to release
  for (int i = 0, ny = 1; i < 32; ++i) // Note: ~8-10us on 3.5GHz Zen2
  {
    if (interlocked_relaxed_load(*lock) == LOCK_IS_FREE &&
        interlocked_compare_exchange(*lock, LOCK_IS_TAKEN_NO_WAITERS, LOCK_IS_FREE) == LOCK_IS_FREE)
      return;
    for (int j = 0; j < ny; ++j)
      cpu_yield();
    ny = (min)(ny * 2, 16);
  }
  // Phase 2: declare ourselves contended and park
  auto waitOnAddr = os_get_native_wait_on_address_impl(); // May be null on win7
  uint32_t expectedWait = LOCK_IS_TAKEN_CONTENDED;
  while (1)
  {
    if (interlocked_exchange(*lock, LOCK_IS_TAKEN_CONTENDED) == LOCK_IS_FREE)
      return;
    if (waitOnAddr) [[likely]]
      waitOnAddr((uint32_t *)lock, &expectedWait, /*inf*/ -1);
    else
      spin_wait_no_profile([&]() { return interlocked_relaxed_load(*lock) != LOCK_IS_FREE; });
  }
#else
  ttas_spinlock_lock(*lock, LOCK_IS_TAKEN_NO_WAITERS, LOCK_IS_FREE);
#endif
}

#define EXPORT_PULL dll_pull_osapiwrappers_spinlock
#include <supp/exportPull.h>
