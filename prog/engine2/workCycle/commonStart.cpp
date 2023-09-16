#include <workCycle/dag_startupModules.h>
#include <startup/dag_restart.h>
#include <generic/dag_initOnDemand.h>
#include "workCyclePriv.h"

void dagor_common_startup()
{
  workcycle_internal::set_priority(true);

  startup_game(RESTART_ALL);

  //== this rproc should execute autoexec.con
  // main_rproc.demandInit();
  // add_restart_proc(main_rproc);
}
