#include <workCycle/dag_workCycle.h>
#include <workCycle/dag_delayedAction.h>
#include <perfMon/dag_statDrv.h>
#import <QuartzCore/QuartzCore.h>

void dagor_process_sys_messages(bool input_only)
{
  if (input_only) //== implement properly [to be usable]
    return;

  if (dagor_workcycle_depth > 1)  // Must process messages expicitly while the root dagor_work_cycle is bocked by a WaitBox.
    while (CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0, TRUE) == kCFRunLoopRunHandledSource)
    {
    }
}

void dagor_idle_cycle(bool input_only)
{
  TIME_PROFILE(dagor_idle_cycle);
  perform_regular_actions_for_idle_cycle();
  dagor_process_sys_messages(input_only);
}
