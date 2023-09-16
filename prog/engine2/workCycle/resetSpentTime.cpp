#include "workCyclePriv.h"
#include <workCycle/dag_workCycle.h>
#include <perfMon/dag_cpuFreq.h>
#include <osApiWrappers/dag_atomic.h>

volatile int64_t workcycle_internal::last_wc_time = 0;
volatile int64_t workcycle_internal::last_draw_time = 0;
float workcycle_internal::gametimeElapsed = 0.0f;

void dagor_reset_spent_work_time()
{
  workcycle_internal::last_wc_time = ref_time_ticks_qpc();
  workcycle_internal::last_draw_time = workcycle_internal::last_wc_time;
  workcycle_internal::gametimeElapsed = 0.0f;
  interlocked_release_store(workcycle_internal::lastFrameTime, -1);
}
