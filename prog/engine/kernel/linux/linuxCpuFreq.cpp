// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <perfMon/dag_cpuFreq.h>
#include <time.h>

static constexpr int64_t NS_TICK_DIV = 1U;
static constexpr int64_t TICKS_NSEC = 1U / NS_TICK_DIV;
static constexpr int64_t TICKS_USEC = 1000U / NS_TICK_DIV;
static constexpr int64_t TICKS_MSEC = 1000000U / NS_TICK_DIV;
static constexpr int64_t SEC_MUL = 1000000000ULL / NS_TICK_DIV;

static bool measured = false;
static int64_t startup_ref_time = 0;
static int64_t startup_localtime_ticks = 0;

static inline int64_t _timespec_to_ticks(struct timespec tm) { return int64_t(tm.tv_sec) * SEC_MUL + tm.tv_nsec / NS_TICK_DIV; }

static int64_t ref_localtime_ticks()
{
  struct timespec ts;
  clock_gettime(CLOCK_REALTIME, &ts);

#if _TARGET_C3

#else
  tzset();
  ts.tv_sec -= timezone; // seconds West of UTC
#endif

  return _timespec_to_ticks(ts);
}


void measure_cpu_freq(bool)
{
  if (measured)
    return;
  measured = true;
  startup_ref_time = ref_time_ticks();
  startup_localtime_ticks = ref_localtime_ticks();
}

int64_t ref_time_ticks()
{
  struct timespec tm;

#if _TARGET_C3



#else
  clock_gettime(CLOCK_MONOTONIC, &tm);
#endif
  return _timespec_to_ticks(tm);
}

int64_t rel_ref_time_ticks(int64_t ref, int time_usec) { return ref + time_usec * TICKS_USEC; }

int64_t ref_ticks_frequency() { return SEC_MUL; }

int64_t ref_time_delta_to_nsec(int64_t ref) { return ref / TICKS_NSEC; }
int64_t ref_time_delta_to_usec(int64_t ref) { return ref / TICKS_USEC; }

int64_t get_time_nsec(int64_t ref) { return ref_time_delta_to_nsec(ref_time_ticks() - ref); }
int get_time_usec(int64_t ref) { return ref_time_delta_to_usec(ref_time_ticks() - ref); }

int get_time_msec() { return (ref_time_ticks() - startup_ref_time) / TICKS_MSEC; }

int64_t time_msec_to_localtime(int64_t t) { return t + startup_localtime_ticks / TICKS_MSEC; }

#define EXPORT_PULL dll_pull_kernel_cpuFreq
#include <supp/exportPull.h>
