// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <util/dag_console.h>
#include <util/dag_string.h>
#include <daRg/dag_guiGlobals.h>
#include <daRg/dag_element.h>

#include "guiScene.h"

using namespace console;

static bool darg_console_handler(const char *argv[], int argc)
{
  if (argc < 1)
    return false;

  int found = 0;
  CONSOLE_CHECK_NAME("darg", "dbgframe", 2, 2)
  {
#if DARG_USE_DBGCOLOR
    String msg;
    if (darg::set_debug_frames_mode_by_str(argv[1]))
      msg.printf(32, "Mode set to %d", darg::debug_frames_mode);
    else
      msg.printf(32, "Invalid mode (keeping %d intact)", darg::debug_frames_mode);
    console::print_d(msg);
#else
    console::print_d("App was built without dbgframe support");
#endif
  }
  CONSOLE_CHECK_NAME("darg", "perf_stats", 1, 2)
  {
    bool reset = argc > 1 && strcmp(argv[1], "reset") == 0;
    darg::dump_all_gui_scenes_perf_stats(reset);
    console::print_d("daRg perf stats dump scheduled to debug log (written on next scene update)%s",
      reset ? ", counters will be reset" : "");
  }
  CONSOLE_CHECK_NAME("darg", "profiler", 2, 2)
  {
    bool on = strcmp(argv[1], "on") == 0 || strcmp(argv[1], "1") == 0;
    darg::set_all_gui_scenes_profiler(on);
    console::print_d("daRg per-stage profiler %s for all scenes", on ? "enabled" : "disabled");
  }
  return found;
}


REGISTER_CONSOLE_HANDLER(darg_console_handler);
