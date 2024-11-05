// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <workCycle/dag_workCyclePerf.h>

namespace workcycleperf
{
bool cpu_only_cycle_record_enabled = false;
int64_t ref_cpu_only_cycle_start = 0;
int last_cpu_only_cycle_time_usec = 0;
double summed_cpu_only_cycle_time = 0.0;
uint64_t num_cpu_only_cycle = 0;

float get_avg_cpu_only_cycle_time_usec() { return num_cpu_only_cycle != 0 ? summed_cpu_only_cycle_time / num_cpu_only_cycle : 0.0f; }

void reset_summed_cpu_only_cycle_time()
{
  summed_cpu_only_cycle_time = 0.0;
  num_cpu_only_cycle = 0;
}
} // namespace workcycleperf
