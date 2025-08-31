// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <workCycle/dag_workCycle.h>
#include <workCycle/dag_delayedAction.h>
#include <perfMon/dag_statDrv.h>
#include <debug/dag_log.h>
#import <Cocoa/Cocoa.h>

static int last_cycle_time = 0;
static int cycle_warning_threshold_ms = 4000;

void dagor_process_sys_messages(bool input_only)
{
  if (input_only) //== implement properly [to be usable]
    return;
  NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
  while (NSEvent *event =
    [NSApp
#if __MAC_OS_X_VERSION_MAX_ALLOWED > 101200
      nextEventMatchingMask:NSEventMaskAny
#else
      nextEventMatchingMask:NSAnyEventMask
#endif
      untilDate:[NSDate distantPast]
      inMode:NSDefaultRunLoopMode dequeue:YES])
  {
    [NSApp sendEvent:event];
    [NSApp updateWindows];
  }
  [pool release];
}

void dagor_idle_cycle(bool input_only)
{
  TIME_PROFILE(dagor_idle_cycle);
  int curent_time = get_time_msec();
  if (last_cycle_time && (curent_time - last_cycle_time) > cycle_warning_threshold_ms)
    logwarn("[MAC] we haven't being idling for %d ms", (curent_time - last_cycle_time));
  perform_regular_actions_for_idle_cycle();
  int delayed_actions_time = get_time_msec();
  if ((delayed_actions_time - curent_time) > cycle_warning_threshold_ms)
    logwarn("[MAC] we have being executing delayed actions for %d ms", (delayed_actions_time - curent_time));
  dagor_process_sys_messages(input_only);
  last_cycle_time = get_time_msec();
  if ((last_cycle_time - delayed_actions_time) > cycle_warning_threshold_ms)
    logwarn("[MAC] we have being executing sys messages for %d ms", (last_cycle_time - delayed_actions_time));
}
