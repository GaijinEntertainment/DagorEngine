// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "workCyclePriv.h"

struct Display;

namespace workcycle_internal
{
void idle_loop() {}
void set_title(const char *, bool) {}
#if _TARGET_PC_LINUX
Display *get_root_display() { return nullptr; }
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
