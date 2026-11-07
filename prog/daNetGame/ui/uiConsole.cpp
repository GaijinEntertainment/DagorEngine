// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "uiShared.h"
#include "uiRender.h"
#include "loadingUi.h"

#include <util/dag_console.h>
#include <util/dag_string.h>

#include <daRg/dag_guiScene.h>
#include <daRg/dag_guiGlobals.h>

#include "ui/overlay.h"


using namespace console;
static bool ui_console_handler(const char *argv[], int argc) // move it somewhere else
{
  if (argc < 1)
    return false;

  int found = 0;
  CONSOLE_CHECK_NAME("ui", "darg_profiler_overlay_ui", 1, 2)
  {
    if (overlay_ui::gui_scene())
    {
      bool on = (argc > 1) ? to_bool(argv[1]) : !overlay_ui::gui_scene()->getProfiler();
      overlay_ui::gui_scene()->activateProfiler(on);
    }
  }
  CONSOLE_CHECK_NAME("ui", "reload_pictures", 1, 1)
  {
    uishared::before_device_reset();
    uishared::after_device_reset();
  }
  CONSOLE_CHECK_NAME("ui", "fully_covering", 1, 2)
  {
    bool on = (argc > 1) ? to_bool(argv[1]) : !loading_ui::is_fully_covering();
    loading_ui::set_fully_covering(on);
  }
  CONSOLE_CHECK_NAME("ui", "mt", 1, 2)
  {
    if (argc > 1)
      uirender::multithreaded = to_bool(argv[1]);
    console::print_d(uirender::multithreaded ? "UI render is multi-threaded" : "UI render is single-threaded");
  }
  CONSOLE_CHECK_NAME("ui", "beforeRenderMt", 1, 2)
  {
    if (argc > 1)
      uirender::beforeRenderMultithreaded = to_bool(argv[1]);
    console::print_d(
      uirender::beforeRenderMultithreaded ? "UI before render is multi-threaded" : "UI before render is single-threaded");
  }
  return found;
}

REGISTER_CONSOLE_HANDLER(ui_console_handler);
