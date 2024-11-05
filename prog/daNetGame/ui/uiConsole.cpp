// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "uiShared.h"
#include "uiRender.h"
#include "userUi.h"
#include "loadingUi.h"

#include <util/dag_console.h>
#include <util/dag_string.h>

#include <daRg/dag_guiScene.h>
#include <daRg/dag_guiGlobals.h>

#include <quirrel/sqStackChecker.h>

#include "ui/overlay.h"


using namespace console;
static bool ui_console_handler(const char *argv[], int argc) // move it somewhere else
{
  if (argc < 1)
    return false;

  int found = 0;
  CONSOLE_CHECK_NAME("ui", "darg_profiler_user_ui", 1, 2)
  {
    if (user_ui::gui_scene)
    {
      bool on = (argc > 1) ? to_bool(argv[1]) : !user_ui::gui_scene->getProfiler();
      user_ui::gui_scene->activateProfiler(on);
    }
  }
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
  CONSOLE_CHECK_NAME("user_ui", "reload", 1, 2)
  {
    bool hard = argc > 1 ? to_bool(argv[1]) : false;
    user_ui::reload_user_ui_script(hard);
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
  CONSOLE_CHECK_NAME("user_ui", "gc", 1, 1)
  {
    if (user_ui::gui_scene && user_ui::gui_scene->getScriptVM())
    {
      HSQUIRRELVM vm = user_ui::gui_scene->getScriptVM();

      SqStackChecker stackChecck(vm);

      SQInteger res = sq_collectgarbage(vm);
      console::print_d("%d reference cycles found and deleted", int(res));

      if (SQ_SUCCEEDED(sq_resurrectunreachable(vm)))
      {
        if (sq_gettype(vm, -1) == OT_NULL)
          console::print_d("No unreachable objects");
        else
        {
          SQInteger sz = sq_getsize(vm, -1);
          console::print_d("%d unreachable objects", sz);
        }
        sq_pop(vm, 1);
      }
    }
  }
  return found;
}

REGISTER_CONSOLE_HANDLER(ui_console_handler);
