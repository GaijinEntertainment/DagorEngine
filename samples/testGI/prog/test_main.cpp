// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <shaders/dag_shaders.h>
#include <shaders/dag_shaderBlock.h>
#include <startup/dag_restart.h>
#include <startup/dag_globalSettings.h>
#include <startup/dag_inpDevClsDrv.h>
#include <startup/dag_startupTex.h>
#include <startup/dag_fatalHandler.inc.cpp>
#include <gameRes/dag_gameResSystem.h>
#include <workCycle/dag_startupModules.h>
#include <workCycle/dag_gameSettings.h>
#include <workCycle/dag_workCycle.h>
#include <workCycle/dag_gameScene.h>
#include <3d/dag_texPackMgr2.h>
#include <drv/hid/dag_hiJoystick.h>
#include <ioSys/dag_dataBlock.h>
#include <perfMon/dag_cpuFreq.h>
#include <util/dag_threadPool.h>
#include <osApiWrappers/dag_threads.h>
#include <osApiWrappers/dag_progGlobals.h>
#include <osApiWrappers/dag_files.h>
#include <osApiWrappers/dag_cpuJobs.h>
#include <debug/dag_debug.h>
#include "test_main.h"

#include <gui/dag_guiStartup.h>
#include <de3_guiManager.h>
#include <de3_ICamera.h>
#include <de3_loghandler.h>
#include <perfMon/dag_daProfilerSettings.h>

#if _TARGET_PC_WIN
#include <windows.h>
#include <startup/dag_leakDetector.inc.cpp>
#endif

#if _TARGET_C1 | _TARGET_C2

#elif _TARGET_XBOX
#include <startup/dag_xboxMain.inc.cpp>
#elif _TARGET_APPLE
#include <osApiWrappers/dag_basePath.h>
#elif _TARGET_ANDROID
#include <startup/dag_androidMain.inc.cpp>
#elif _TARGET_PC_WIN
#include <startup/dag_winMain.inc.cpp>
#elif _TARGET_PC_LINUX
#include <startup/dag_linuxMain.inc.cpp>
#endif

static InitOnDemand<De3GuiMgrDrawFps> gui_mgr;
namespace ddsx
{
extern void shutdown_tex_pack2_data();
}

void DagorWinMainInit(int, bool) {}
static bool quitted = false;
static void post_shutdown_handler()
{
  dagor_select_game_scene(NULL);
  quitted = true;
  DEBUG_CTX("shutdown!");
  reset_game_resources();
  ddsx::shutdown_tex_pack2_data();
  shutdown_game(RESTART_INPUT);
  shutdown_game(RESTART_ALL);
  threadpool::shutdown();
  cpujobs::term(true, 1000);
}


static void init_threads(const DataBlock &cfg)
{
  int coreCount = cpujobs::get_core_count();
#if _TARGET_XBOX
  int num_workers = coreCount - 1;
#else
  bool freeMainThreadHT = coreCount > 4;                      // for main thread's hyper-threading neighbour
  int defNumWorkers = coreCount - (freeMainThreadHT ? 3 : 2); // 1 for main & 1 for IO threads
  int num_workers = max(cfg.getInt("numWorkThreads", defNumWorkers), 0);
#endif
  int stack_size = cfg.getInt("workThreadsStackSize", 128 << 10);
#if DAGOR_DBGLEVEL > 1
  stack_size *= 2;
#endif
  threadpool::init(num_workers, 2048, stack_size);
  debug("threadpool inited for %d workers", num_workers);
#if _TARGET_PC_WIN || _TARGET_XBOX
  if (cfg.getBool("boostMainThreadPriority", true)) // boost main thread priority in order to overrule loading threads (resources,
                                                    // textures, etc...)
    DaThread::applyThisThreadPriority(THREAD_PRIORITY_ABOVE_NORMAL); // we increase main thread priority by one step. We only support
                                                                     // two priorities: high and low
  G_VERIFY(SetThreadAffinityMask(GetCurrentThread(), MAIN_THREAD_AFFINITY));
#endif
#if _TARGET_C1

#endif
}
static int log_callback(int lev_tag, const char * /*fmt*/, const void * /*arg*/, int /*anum*/, const char * /*ctx_file*/,
  int /*ctx_line*/)
{
  if (lev_tag == LOGLEVEL_ERR || lev_tag == LOGLEVEL_FATAL)
    debug_dump_stack();
  return 1;
}

int DagorWinMain(int nCmdShow, bool /*debugmode*/)
{
  dd_add_base_path(df_get_real_folder_name("./"));

  ::measure_cpu_freq();
  ::dgs_dont_use_cpu_in_background = false;
  //::dgs_limit_fps = true;
  ::dgs_post_shutdown_handler = post_shutdown_handler;

  // prepare global settings datablock
  dgs_load_settings_blk(true, "settings.blk", nullptr, true, false, false);

  ::register_common_game_tex_factories();
  cpujobs::init();
  init_managed_textures_streaming_support();

  ddsx::set_decoder_workers_count(
    dgs_get_settings()->getBlockByNameEx("texStreaming")->getInt("numTexWorkThreads", cpujobs::get_core_count() - 1));
  ddsx::hq_tex_priority =
    dgs_get_settings()->getBlockByNameEx("texStreaming")->getInt("hqTexPrio", is_managed_textures_streaming_load_on_demand() ? 1 : 0);

  ::dagor_init_video("DagorWClass", nCmdShow, NULL, "Loading...");
  dagor_install_dev_fatal_handler(NULL);
  dagor_install_dev_log_handler_callback();

#if _TARGET_PC
  ::dagor_init_keyboard_win();
  ::dagor_init_mouse_win();
#elif _TARGET_IOS | _TARGET_ANDROID
  ::dagor_init_keyboard_win();
  ::dagor_init_mouse_win();
  ::dagor_init_joystick();
#else
#if _TARGET_C1 == 0 && _TARGET_C2 == 0
  ::dagor_init_mouse_null();
  ::dagor_init_keyboard_null();
#endif
  ::dagor_init_joystick();
  if (global_cls_drv_joy)
    global_cls_drv_joy->enableAutoDefaultJoystick(true);
#endif
  init_threads(*::dgs_get_settings());
  ::startup_game(RESTART_ALL);
  shaders_register_console(true);

  const char *sh_bindump_prefix = dgs_get_settings()->getStr("shaders", "compiledShaders/game");
  ::startup_shaders(sh_bindump_prefix);
  ::startup_game(RESTART_ALL);
  dagor_use_reversed_depth(true);
  // ShaderGlobal::enableAutoBlockChange(true);
  ShaderGlobal::set_int(get_shader_variable_id("in_editor", true), 0);
  ShaderGlobal::set_vars_from_blk(*dgs_get_settings()->getBlockByNameEx("shaderVar"), true);
  da_profiler::set_profiling_settings(*dgs_get_settings()->getBlockByNameEx("debug"));

  ::dagor_common_startup();

  bool drawFps = dgs_get_settings()->getBool("draw_fps", false);
  ::startup_gui_base("ui/fonts.blk");

  ::startup_game(RESTART_ALL);

  StdGuiRender::set_def_font_ht(0, floorf(StdGuiRender::screen_height() * dgs_get_settings()->getReal("useFontSize", 0.015) + 0.5f));
  if (drawFps)
  {
    gui_mgr.demandInit();
    ::dagor_gui_manager = gui_mgr;
    ::dgs_draw_fps = true;
  }

#if _TARGET_PC
  ::win32_set_window_title("testGI");
#endif
  debug_set_log_callback(&log_callback);
  game_demo_init();

  ::dagor_reset_spent_work_time();
  for (; !quitted;) // infinite cycle
  {
    ::dagor_work_cycle();
  }
  return 0;
}
