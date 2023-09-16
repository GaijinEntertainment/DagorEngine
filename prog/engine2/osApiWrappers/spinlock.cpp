#include <osApiWrappers/dag_lockProfiler.h>
#include <osApiWrappers/dag_ttas_spinlock.h>
#include <osApiWrappers/dag_spinlock.h>
#if _TARGET_PC_LINUX && __SANITIZE_THREAD__ // we have to use pthread's spinlock API or it would be not recognized by TSAN
#include <pthread.h>
#define USE_NATIVE_PTHREAD_API 1
#else
enum
{
  LOCK_IS_FREE = 0,
  LOCK_IS_TAKEN = 1
};
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

void os_spinlock_lock(os_spinlock_t *lock, da_profiler::desc_id_t token)
{
#if LOCK_PROFILER_ENABLED // avoid profiling "no contention"
#if USE_NATIVE_PTHREAD_API
  if (pthread_spin_trylock(lock) == 0)
#else
  if (ttas_spinlock_trylock_fast(*lock, LOCK_IS_TAKEN, LOCK_IS_FREE))
#endif
    return;
#endif
  ScopeLockProfiler<da_profiler::NoDesc, /*usec_threshold*/ 0> lp(token); // threshold is intentionally zero. No contention case is
                                                                          // handled by trylock
#if USE_NATIVE_PTHREAD_API
  pthread_spin_lock(lock);
#else
  ttas_spinlock_lock(*lock, LOCK_IS_TAKEN, LOCK_IS_FREE);
#endif
}

bool os_spinlock_trylock(os_spinlock_t *lock)
{
#if USE_NATIVE_PTHREAD_API
  return pthread_spin_trylock(lock) == 0;
#else
  return ttas_spinlock_trylock_fast(*lock, LOCK_IS_TAKEN, LOCK_IS_FREE);
#endif
}

void os_spinlock_unlock(os_spinlock_t *lock)
{
#if USE_NATIVE_PTHREAD_API
  pthread_spin_unlock(lock);
#else
  ttas_spinlock_unlock(*lock, LOCK_IS_FREE);
#endif
}

#define EXPORT_PULL dll_pull_osapiwrappers_spinlock
#include <supp/exportPull.h>
