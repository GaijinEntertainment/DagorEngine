// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <util/dag_console.h>
#include <util/dag_string.h>
#include <daRg/dag_guiGlobals.h>
#include <daRg/dag_element.h>

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
  return found;
}


REGISTER_CONSOLE_HANDLER(darg_console_handler);
