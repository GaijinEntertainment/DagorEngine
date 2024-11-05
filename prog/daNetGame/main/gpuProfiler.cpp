// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <perfMon/dag_statDrv.h>
#include <util/dag_globDef.h>
#include "input/globInput.h"

#if TIME_PROFILER_ENABLED
#include <gui/dag_visualLog.h>

// we enable everything.
static constexpr uint32_t profile_all =
  da_profiler::EVENTS | da_profiler::TAGS | da_profiler::GPU | da_profiler::SAMPLING | da_profiler::SAVE_SPIKES;

static inline bool is_enabled() { return (da_profiler::get_current_mode() & da_profiler::EVENTS); }
static inline bool is_currently_enabled() { return (da_profiler::get_active_mode() & da_profiler::EVENTS); }
void switchIngameProfiler(bool enable)
{
  if (enable)
  {
    da_profiler::add_mode(profile_all);
    visuallog::logmsg("Profiler enabled. Press Left_Alt + F11 to disable and F11 to dump performance to log.", E3DCOLOR(255, 255, 255),
      1);
  }
  else
  {
    da_profiler::remove_mode(profile_all);
    visuallog::logmsg("Profiler disabled. Press Left_Alt + F11 to enable it again.", E3DCOLOR(255, 255, 255), 1);
  }
}

void dumpIngameProfiler()
{
  da_profiler::request_dump();
  if (is_currently_enabled())
    visuallog::logmsg("Performance profiler info saved near logfile", E3DCOLOR(255, 255, 255), 1);
}
#endif

bool glob_input_gpu_profiler_handle_F11_key(bool alt_modif)
{
#if TIME_PROFILER_ENABLED
  if (alt_modif)
    switchIngameProfiler(!is_enabled());
  else
  {
    if (is_enabled())
      dumpIngameProfiler();
  }
  return true; // processed, do not pass to shortcuts
#else
  G_UNUSED(alt_modif);
  return false;
#endif
}
