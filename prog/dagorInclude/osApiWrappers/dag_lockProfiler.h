//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <perfMon/dag_statDrv.h>
#include <perfMon/dag_perfTimer.h>

typedef uint64_t lock_start_t;
#if DA_PROFILER_ENABLED && DAGOR_DBGLEVEL > 0 // for performance reasons, none of wait profiling is enabled in release builds
#define LOCK_PROFILER_ENABLED 1

#if _TARGET_ANDROID | _TARGET_IOS
static constexpr unsigned DEFAULT_LOCK_PROFILER_USEC_THRESHOLD = 4;
#else
static constexpr unsigned DEFAULT_LOCK_PROFILER_USEC_THRESHOLD = 1;
#endif

__forceinline lock_start_t dagor_lock_profiler_start() { return profile_ref_ticks(); }
__forceinline void dagor_lock_profiler_interval(lock_start_t interval, da_profiler::desc_id_t desc,
  uint32_t usec_threshold = DEFAULT_LOCK_PROFILER_USEC_THRESHOLD)
{
  if (interval > usec_threshold * profile_ticks_per_usec())
  {
    lock_start_t end = dagor_lock_profiler_start();
    DA_PROFILE_LEAF_EVENT(desc, end - interval, end);
  }
  (void)desc;
}
__forceinline void dagor_lock_profiler_stop(lock_start_t start, da_profiler::desc_id_t desc,
  uint32_t usec_threshold = DEFAULT_LOCK_PROFILER_USEC_THRESHOLD)
{
  uint64_t end = profile_ref_ticks();
  if (end > start + usec_threshold * profile_ticks_per_usec())
  {
    DA_PROFILE_LEAF_EVENT(desc, start, end);
  }
  (void)desc;
}
template <da_profiler::desc_id_t desc, uint32_t usec_threshold = DEFAULT_LOCK_PROFILER_USEC_THRESHOLD>
struct ScopeLockProfiler
{
  lock_start_t reft = dagor_lock_profiler_start();
  ScopeLockProfiler() = default;
  ~ScopeLockProfiler() { dagor_lock_profiler_stop(reft, desc, usec_threshold); }
};

template <uint32_t usec_threshold>
struct ScopeLockProfiler<da_profiler::NoDesc, usec_threshold>
{
  lock_start_t reft = dagor_lock_profiler_start();
  da_profiler::desc_id_t desc;
  ScopeLockProfiler(da_profiler::desc_id_t d) : desc(d) {}
  ~ScopeLockProfiler() { dagor_lock_profiler_stop(reft, desc, usec_threshold); }
};
#else
#define LOCK_PROFILER_ENABLED 0
template <da_profiler::desc_id_t desc, uint32_t usec_threshold = 1>
struct ScopeLockProfiler
{
  ScopeLockProfiler() = default;
  ScopeLockProfiler(da_profiler::desc_id_t) {}
};
__forceinline lock_start_t dagor_lock_profiler_start() { return 0; }
__forceinline void dagor_lock_profiler_interval(lock_start_t, da_profiler::desc_id_t, uint32_t = 1) {}
__forceinline void dagor_lock_profiler_stop(lock_start_t, da_profiler::desc_id_t, uint32_t = 1) {}
#endif
