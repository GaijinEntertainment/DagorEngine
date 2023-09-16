#include "workCyclePriv.h"

namespace workcycle_internal
{
void idle_loop() {}
void set_title(const char *, bool) {}
#if _TARGET_PC_LINUX
void set_title_tooltip(const char *, const char *, bool) {}
#endif
} // namespace workcycle_internal

#ifdef WORKCYCLE_PERF_STUB
namespace workcycleperf
{
float get_avg_cpu_only_cycle_time_usec() { return 0.0f; }
void reset_summed_cpu_only_cycle_time() {}
} // namespace workcycleperf
#endif
