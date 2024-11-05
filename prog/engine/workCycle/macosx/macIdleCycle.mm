// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <workCycle/dag_workCycle.h>
#include <workCycle/dag_delayedAction.h>
#include <perfMon/dag_statDrv.h>
#import <Cocoa/Cocoa.h>

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
  perform_regular_actions_for_idle_cycle();
  dagor_process_sys_messages(input_only);
}
