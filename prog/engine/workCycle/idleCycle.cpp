// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <supp/_platform.h>
#include <workCycle/dag_workCycle.h>
#include <workCycle/dag_gameSettings.h>
#include <startup/dag_demoMode.h>
#include <util/dag_globDef.h>
#include <util/dag_delayedAction.h>
#include <osApiWrappers/dag_wndProcComponent.h>
#include <osApiWrappers/dag_miscApi.h>
#include <debug/dag_debug.h>
#include "workCyclePriv.h"
#include <3d/dag_lowLatency.h>
#include <startup/dag_globalSettings.h>

#if _TARGET_GDK
#include <osApiWrappers/gdk/queues.h>
#endif

#if _TARGET_PC_WIN
#include <workCycle/threadedWindow.h>
#endif

#if _TARGET_XBOX
extern void update_xbox_keyboard();
#elif _TARGET_PC_WIN
static void update_xbox_keyboard() {}
#endif

#if _TARGET_PC_WIN
static bool pump_messages(bool input_only)
{
  TIME_PROFILE(pump_messages);
  bool msg_processed = false;
  if (windows::process_main_thread_messages(input_only, msg_processed))
    return msg_processed;

  static MSG msg;
  bool were_events = false;

  while (PeekMessageW(&msg, NULL, 0, 0, PM_NOREMOVE | (input_only ? PM_QS_INPUT : 0)))
  {
    if (!GetMessageW(&msg, NULL, 0, 0))
    {
      if (!input_only)
        quit_game(msg.wParam);
    }
    // Special message created by ReflexStats for latency measurement
    if (lowlatency::feed_latency_input(msg.message))
      continue;
    TranslateMessage(&msg);
    DispatchMessageW(&msg);
    were_events = true;
  }

  return were_events;
}
#elif _TARGET_XBOX
namespace gdk
{
extern bool process_main_thread_messages();
}
static bool pump_messages(bool)
{
  TIME_PROFILE(pump_messages);
  return gdk::process_main_thread_messages();
}
#endif

void dagor_process_sys_messages(bool input_only)
{
#if _TARGET_PC_WIN | _TARGET_XBOX
  bool were_events = pump_messages(input_only);

  update_xbox_keyboard();

  if (were_events)
  {
    TIME_PROFILE(additional_perform_wnd_proc_components);
    intptr_t result;
    ::perform_wnd_proc_components(NULL, 0, 0, 0, result);
  }

#if _TARGET_GDK
  gdk::dispatch_queue(gdk::get_default_queue());
#endif

  if (::dgs_dont_use_cpu_in_background && !::dgs_app_active)
  {
    TIME_PROFILE(application_inactive_sleep);
    sleep_msec(100);
  }

#if _TARGET_PC_WIN
  if (dagor_demo_check_idle_timeout())
  {
    debug("QUIT GAME: demo idle timeout");
    quit_game(0, false);
  }
#endif
#elif _TARGET_PC_LINUX | _TARGET_XBOX
  if (input_only) //== implement properly [to be usable]
    return;
  workcycle_internal::idle_loop();
#else
  (void)(input_only);
#endif
}

void dagor_idle_cycle(bool input_only)
{
  TIME_PROFILE(dagor_idle_cycle);
  perform_regular_actions_for_idle_cycle();
  dagor_process_sys_messages(input_only);
}
