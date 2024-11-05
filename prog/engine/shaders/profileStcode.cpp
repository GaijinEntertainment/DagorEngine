// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "profileStcode.h"

#include <util/dag_runningWindowAccumulator.h>
#include <startup/dag_globalSettings.h>
#include <debug/dag_debug.h>

namespace stcode::profile
{

static RunningSampleAverageAccumulator<uint64_t, 512> usec_per_frame_accum;

void add_time_usec(uint64_t usec) { usec_per_frame_accum.addSample(usec, dagor_frame_no()); }
void dump_avg_time()
{
  double usec = usec_per_frame_accum.getLatestAvg();
  debug("STCODE (dynamic): %.4lfms average per %d frames", 0.001 * usec, usec_per_frame_accum.getWindowSize());
}

} // namespace stcode::profile
