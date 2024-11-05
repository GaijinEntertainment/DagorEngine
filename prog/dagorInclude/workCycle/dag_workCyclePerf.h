//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <perfMon/dag_cpuFreq.h>
#include <debug/dag_debug.h>

namespace workcycleperf
{
#if DAGOR_DBGLEVEL > 0
//! read-only values
extern bool debug_on;
extern int64_t ref_frame_start;

//! debug mode enable/disable; takes effect at start of every frame
void enable_debug(bool on);

//! returns time passed since frame start, in microseconds
inline int get_frame_timepos_usec() { return debug_on ? get_time_usec(ref_frame_start) : 0; }

//! outputs checkpoint with timestamp
inline void cpu_cp(const char *str, int ln)
{
  if (debug_on)
    debug(" -- %d: %s,%d", get_time_usec(ref_frame_start), str, ln);
}
template <typename... Args>
void log(const Args &...args)
{
  if (debug_on)
    debug(args...);
}
#else
constexpr bool debug_on = false;

inline void enable_debug(bool) {}
inline int get_frame_timepos_usec() { return 0; }
inline void cpu_cp(const char *, int) {}
template <typename... Args>
void log(const Args &...)
{}
#endif

extern bool cpu_only_cycle_record_enabled;
extern int64_t ref_cpu_only_cycle_start;
extern int last_cpu_only_cycle_time_usec;
extern double summed_cpu_only_cycle_time;
extern uint64_t num_cpu_only_cycle;

inline void mark_cpu_only_cycle_start()
{
  if (!cpu_only_cycle_record_enabled)
    return;

  ref_cpu_only_cycle_start = ref_time_ticks();
}

inline void mark_cpu_only_cycle_end()
{
  if (!cpu_only_cycle_record_enabled)
    return;

  if (ref_cpu_only_cycle_start != 0)
  {
    last_cpu_only_cycle_time_usec = get_time_usec(ref_cpu_only_cycle_start);
    summed_cpu_only_cycle_time += last_cpu_only_cycle_time_usec;
    num_cpu_only_cycle++;
  }
}

inline void mark_cpu_only_cycle_pause()
{
  if (!cpu_only_cycle_record_enabled)
    return;

  if (ref_cpu_only_cycle_start != 0)
  {
    last_cpu_only_cycle_time_usec = get_time_usec(ref_cpu_only_cycle_start);
    summed_cpu_only_cycle_time += last_cpu_only_cycle_time_usec;
  }
}

void reset_summed_cpu_only_cycle_time();
float get_avg_cpu_only_cycle_time_usec();
} // namespace workcycleperf
