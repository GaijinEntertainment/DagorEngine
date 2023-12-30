#include <workCycle/dag_workCycle.h>
#include "workCyclePriv.h"
#include <debug/dag_fatal.h>

unsigned dagor_game_act_rate = 100;
float dagor_game_act_time = 0.01;
unsigned dagor_game_time_step_usec = 10000;
float dagor_game_time_scale = 1.0;

void dagor_set_game_act_rate(int rate)
{
  if (rate == 0 || rate > 10000)
    DAG_FATAL("incorrect act rate: %d", rate);
  else if (rate < 0)
  {
    // set variable-act-rate mode
    workcycle_internal::minVariableRateActTimeUsec = 1000000 / (-rate);
    return;
  }

  ::dagor_game_act_rate = rate;
  ::dagor_game_act_time = 1.0f / rate;
  ::dagor_game_time_step_usec = 1000000 / rate;
  workcycle_internal::minVariableRateActTimeUsec = 0;
}

int dagor_get_game_act_rate()
{
  if (workcycle_internal::minVariableRateActTimeUsec)
    return -1000000 / workcycle_internal::minVariableRateActTimeUsec;
  return dagor_game_act_rate;
}
