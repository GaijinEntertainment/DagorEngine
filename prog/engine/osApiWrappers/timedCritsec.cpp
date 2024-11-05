// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <osApiWrappers/dag_critSec.h>
#include <util/dag_globDef.h>
#include <perfMon/dag_cpuFreq.h>
#include <osApiWrappers/dag_miscApi.h>
#include <osApiWrappers/dag_atomic.h>
#include <osApiWrappers/dag_lockProfiler.h>
#include <string.h>
#include "critsec.h"

bool try_timed_enter_critical_section(void *p, int timeout_ms, const char *waiter_perf_name)
{
#if LOCK_PROFILER_ENABLED
  DA_PROFILE_EVENT_DESC(::da_profiler::DescCritsec);
#endif
  csimpl::type *cc = csimpl::get(p);
  G_ASSERT(cc && "critical section is NULL!");
#if HAVE_WINAPI

  bool locked = TryEnterCriticalSection(cc) != FALSE;
  if (!locked)
  {
    int deadline_ms = get_time_msec() + timeout_ms;
    sleep_usec(0); // first sleep 0, may be that will be enough
    while ((locked = (TryEnterCriticalSection(cc) != FALSE)) == false && get_time_msec() < deadline_ms)
      sleep_usec(1000); // use sleep_usec to avoid profiling
  }
#else

#if _TARGET_PC_LINUX
  struct timespec ts;
  clock_gettime(CLOCK_REALTIME, &ts);
  ts.tv_sec += timeout_ms / 1000;              // seconds
  ts.tv_nsec += (timeout_ms % 1000) * 1000000; // nanoseconds

  int result = pthread_mutex_timedlock(cc, &ts);
#elif _TARGET_C1 || _TARGET_C2

#else
  int result = pthread_mutex_trylock(cc);
  if (result != 0)
  {
    sleep_usec(0); // first try just scheduler
    for (; (result = pthread_mutex_trylock(cc)) != 0 && timeout_ms > 0; timeout_ms -= 1)
    {
      if (result == EBUSY)
        sleep_usec(1000); // use sleep_usec to avoid profiling
    }
  }
#endif
  bool locked = result == 0;
  if (locked)
    csimpl::lockcount::increment(p);
#endif
  G_UNUSED(waiter_perf_name);
  return locked;
}

#define EXPORT_PULL dll_pull_osapiwrappers_timed_critsec
#include <supp/exportPull.h>
