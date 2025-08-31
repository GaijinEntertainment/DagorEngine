// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "profileStcode.h"

#include <util/dag_runningWindowAccumulator.h>
#include <startup/dag_globalSettings.h>
#include <osApiWrappers/dag_spinlock.h>
#include <debug/dag_debug.h>

namespace stcode::profile
{

static OSSpinlock g_spinlock;
static RunningSampleAverageAccumulator<uint64_t, 512> g_dyn_usec_per_frame_accum DAG_TS_GUARDED_BY(g_spinlock);
static RunningSampleAverageAccumulator<uint64_t, 512> g_st_usec_per_frame_accum DAG_TS_GUARDED_BY(g_spinlock);

static_assert(decltype(g_dyn_usec_per_frame_accum)::WINDOW_SIZE == decltype(g_st_usec_per_frame_accum)::WINDOW_SIZE);
static constexpr int WINDOW_SIZE = decltype(g_dyn_usec_per_frame_accum)::WINDOW_SIZE;

void add_dyncode_time_usec(uint64_t usec)
{
  OSSpinlockScopedLock lock{g_spinlock};
  g_dyn_usec_per_frame_accum.addSample(usec, dagor_frame_no());
}
void add_blkcode_time_usec(uint64_t usec)
{
  OSSpinlockScopedLock lock{g_spinlock};
  g_st_usec_per_frame_accum.addSample(usec, dagor_frame_no());
}

void dump_avg_time()
{
  double stusec, dynusec;
  {
    OSSpinlockScopedLock lock{g_spinlock};
    dynusec = g_dyn_usec_per_frame_accum.getLatestAvg();
    stusec = g_st_usec_per_frame_accum.getLatestAvg();
  }

  debug("STCODE: all: %.4lfms, dynamic: %.4lfms, stblk: %.4lfms, average per %d frames", 0.001 * (dynusec + stusec), 0.001 * dynusec,
    0.001 * stusec, WINDOW_SIZE);
}

} // namespace stcode::profile
