// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <util/dag_lag.h>
#include <util/dag_baseDef.h>
#include <debug/dag_debug.h>
#include <osApiWrappers/dag_stackHlp.h>
#include <perfMon/dag_cpuFreq.h>
#include <osApiWrappers/dag_cpuJobs.h>

static void default_lag_handler(const char *marker_name, float time_ms)
{
  // Silence compiler warning if !(DAGOR_DBGLEVEL > 0 || DAGOR_FORCE_LOGS)
  G_UNREFERENCED(marker_name);
  G_UNREFERENCED(time_ms);

  logmessage(_MAKE4C('LAG '), "WARNING: %s lag %.1fms", marker_name, time_ms);
}
void (*dgs_lag_handler)(const char *, float) = &default_lag_handler;


ScopedLagTimer::ScopedLagTimer(const char *static_name, int timeout_ms) : timeoutMs(timeout_ms)
{
  startTime = get_time_msec();
  name = static_name;
}

ScopedLagTimer::~ScopedLagTimer()
{
  int elapsed = get_time_msec() - startTime;
  if (elapsed >= timeoutMs)
    logmessage(_MAKE4C('LAG '), "WARNING: %s lag timeout %dms", name, elapsed);
}


#if DAGOR_DBGLEVEL > 0

void LagTestThread::begin(int wait_ms, bool print_stack)
{
  startWaitMs = get_time_msec();
  stopWaitMs = startWaitMs + wait_ms;
  printStack = print_stack;
  start();
}

int LagTestThread::end(bool print_time)
{
  int elapsed = get_time_msec() - startWaitMs;
  stopWaitMs = -1;
  startWaitMs = -1;

  if (elapsed > 1 && print_time)
    debug("Lag: %dms", elapsed);

  return elapsed;
}

void LagTestThread::execute()
{
  for (;;)
  {
    int localStartWaitMs = startWaitMs;
    int localStopWaitMs = stopWaitMs;
    if (localStopWaitMs > 0 && get_time_msec() > localStopWaitMs)
    {
      int elapsed = get_time_msec() - localStartWaitMs;

      if (printStack)
        dump_all_thread_callstacks();

      debug("Lag TIMEOUT: %dms", elapsed);
      stopWaitMs = -1;
    }
  }
}

LagTestThread lag_test;

#endif // DAGOR_DBGLEVEL > 0


#define EXPORT_PULL dll_pull_baseutil_lag
#include <supp/exportPull.h>
