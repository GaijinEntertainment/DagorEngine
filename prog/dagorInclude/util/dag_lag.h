//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <osApiWrappers/dag_threads.h>

#include <supp/dag_define_KRNLIMP.h>
KRNLIMP extern void (*dgs_lag_handler)(const char *marker_name, float time_ms);
#include <supp/dag_undef_KRNLIMP.h>

#if DAGOR_DBGLEVEL < 1 && !DAGOR_FORCE_LOGS
#define BEGIN_LAG(name) ((void)(0))
#define END_LAG(name)   ((void)(0))
#else
#define BEGIN_LAG(name)                                                                                                   \
  const int numFrames##name = 5;                                                                                          \
  const int thresholdTimeUs##name = 100000;                                                                               \
  const int thresholdRatio##name = 3;                                                                                     \
  static int64_t times##name[numFrames##name + 1]; /* One additional element to workaround a bug in Edit and Continue. */ \
  static int64_t sumTime##name = 0;                                                                                       \
  static int timeNo##name = 0;                                                                                            \
  static bool isFilled##name = false;                                                                                     \
  volatile int64_t ref##name = ref_time_ticks();

#define END_LAG(name)                                                                                                              \
  volatile int64_t time##name = get_time_usec(ref##name);                                                                          \
  if (isFilled##name && time##name > thresholdTimeUs##name && time##name > thresholdRatio##name * sumTime##name / numFrames##name) \
  {                                                                                                                                \
    if (dgs_lag_handler)                                                                                                           \
      (*dgs_lag_handler)(#name, time##name / 1000.f);                                                                              \
  }                                                                                                                                \
  sumTime##name += time##name;                                                                                                     \
  if (isFilled##name)                                                                                                              \
    sumTime##name -= times##name[timeNo##name];                                                                                    \
  times##name[timeNo##name++] = time##name;                                                                                        \
  if (timeNo##name >= numFrames##name)                                                                                             \
  {                                                                                                                                \
    timeNo##name = 0;                                                                                                              \
    isFilled##name = true;                                                                                                         \
  }
#endif


class ScopedLagTimer
{
  int timeoutMs;
  int startTime;
  const char *name;

public:
  ScopedLagTimer(const char *static_name, int timeout_ms = 100);
  ~ScopedLagTimer();
};


#if DAGOR_DBGLEVEL > 0

class LagTestThread : public DaThread
{
public:
  volatile int startWaitMs;
  volatile int stopWaitMs;
  bool printStack;

  LagTestThread() :
    DaThread("LagTestThread", DEFAULT_STACK_SZ, 0, WORKER_THREADS_AFFINITY_MASK), startWaitMs(-1), stopWaitMs(-1), printStack(false)
  {}

  void begin(int wait_ms, bool print_stack = false);
  int end(bool print_time = true);

protected:
  virtual void execute();
};

extern LagTestThread lag_test;

#endif // DAGOR_DBGLEVEL > 0
