// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "main/console.h"
#include "main/app.h"
#include "main/main.h"
#include "main/gameLoad.h"
#include "net/net.h"
#include "game/gameLauncher.h"

#include <gui/dag_visConsole.h>
#include <gui/dag_visualLog.h>
#include <gui/dag_imgui.h>
#include <visualConsole/dag_visualConsole.h>
#include <consoleWindow/dag_consoleWindow.h>
#include <util/dag_console.h>
#include <osApiWrappers/dag_miscApi.h>
#include <perfMon/dag_statDrv.h>
#include "ui/overlay.h"
#include <memory/dag_dbgMem.h>
#include <debug/dag_memReport.h>
#include <perfMon/dag_pix.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_info.h>

using namespace console;

PULL_CONSOLE_PROC(def_app_console_handler)
PULL_CONSOLE_PROC(profiler_console_handler)
PULL_CONSOLE_PROC(ecs_console_handler)
PULL_CONSOLE_PROC(game_console_handler)
PULL_CONSOLE_PROC(phys_console_handler)
PULL_CONSOLE_PROC(sq_console_handler)
PULL_CONSOLE_PROC(das_console_handler)
PULL_CONSOLE_PROC(replay_console_handler)
#if DAGOR_DBGLEVEL > 0
PULL_CONSOLE_PROC(ri_collision_console_handler)
#endif

// use it only for app like console processors. move everything else elsewhere
static bool app_console_handler(const char *argv[], int argc)
{
  if (argc < 1)
    return false;
  int found = 0;
  CONSOLE_CHECK_NAME("app", "quit", 1, 1) { exit_game("console"); }
  CONSOLE_CHECK_NAME("app", "exit", 1, 1) { exit_game("console"); }
  CONSOLE_CHECK_NAME("app", "timeSpeed", 2, 2) { set_timespeed(to_real(argv[1])); }
  CONSOLE_CHECK_NAME("app", "toggle_pause", 1, 1) { toggle_pause(); }
  CONSOLE_CHECK_NAME("app", "vlog", 2, 2) { visuallog::logmsg(argv[1]); }
  CONSOLE_CHECK_NAME("log", "history", 1, 1)
  {
    for (const SimpleString &s : visuallog::getHistory())
      console::print(s.str());
  }
  CONSOLE_CHECK_NAME_EX("log", "max_lines", 2, 2, "sets how many lines can be shown in log at once", "<lines_count>")
  {
    visuallog::setMaxItems(to_int(argv[1]));
  }
  CONSOLE_CHECK_NAME("app", "fps_limit", 2, 2) { set_fps_limit(to_int(argv[1])); }
  CONSOLE_CHECK_NAME("app", "switch_scene", 1, 2) { sceneload::switch_scene(argc > 1 ? argv[1] : "", {}); }
  CONSOLE_CHECK_NAME("app", "switch_scene_and_update", 1, 2) { sceneload::switch_scene_and_update(argc > 1 ? argv[1] : ""); }
  CONSOLE_CHECK_NAME_EX("profiler", "capture_after_long_frames", 2, 5, "Captures frames after long frames",
    "<frame_interval_threshold_us> [frames] [capture_count_limit] [flags]")
  {
    CaptureAfterLongFrameParams params;
    params.thresholdUS = console::to_int(argv[1]);
    if (argc > 2)
      params.frames = console::to_int(argv[2]);
    if (argc > 3)
      params.captureCountLimit = console::to_int(argv[3]);
    if (argc > 4)
      params.flags = console::to_int(argv[4]);
    d3d_err(d3d::driver_command(Drv3dCommand::PIX_GPU_CAPTURE_AFTER_LONG_FRAMES, reinterpret_cast<void *>(&params)));
  }
  return found;
}

REGISTER_CONSOLE_HANDLER(app_console_handler);

void console::init_ingame() { console::init(); }

void console::term_ingame() { console::shutdown(); }

void console::update()
{
  if (auto drv = console::get_visual_driver())
    drv->update();
}

void console::render()
{
  if (auto drv = console::get_visual_driver())
    drv->render();
}

REGISTER_IMGUI_WINDOW("General", "Console commands and variables", console::imgui_window);
