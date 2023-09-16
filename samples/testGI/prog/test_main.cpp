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
#include <humanInput/dag_hiJoystick.h>
#include <ioSys/dag_dataBlock.h>
#include <perfMon/dag_cpuFreq.h>
#include <osApiWrappers/dag_progGlobals.h>
#include <osApiWrappers/dag_files.h>
#include <osApiWrappers/dag_cpuJobs.h>
#include <debug/dag_debug.h>
#include "test_main.h"

#include <gui/dag_guiStartup.h>
#include <de3_guiManager.h>
#include <de3_ICamera.h>
#include <de3_loghandler.h>

#if _TARGET_XBOX360
#include <startup/dag_xboxMain.inc.cpp>
#elif _TARGET_C0


#elif _TARGET_C1

#elif _TARGET_APPLE
#include <osApiWrappers/dag_basePath.h>
#elif _TARGET_ANDROID
#include <startup/dag_androidMain.inc.cpp>
#else
#include <startup/dag_winMain.inc.cpp>
#endif

static DataBlock *global_settings_blk = NULL;
static const DataBlock *get_glob_settings_blk() { return global_settings_blk; }
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
  debug_ctx("shutdown!");
  reset_game_resources();
  ddsx::shutdown_tex_pack2_data();
  shutdown_game(RESTART_INPUT);
  shutdown_game(RESTART_ALL);
}

int DagorWinMain(int nCmdShow, bool /*debugmode*/)
{
  dd_add_base_path(df_get_real_folder_name("./"));

  ::measure_cpu_freq();
  ::dgs_dont_use_cpu_in_background = true;
  ::dgs_post_shutdown_handler = post_shutdown_handler;

  // prepare global settings datablock
  global_settings_blk = new DataBlock;
  dgs_get_settings = &get_glob_settings_blk;
  global_settings_blk->load("settings.blk");

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
#if _TARGET_PC_WIN | _TARGET_PC_MACOSX
  ::dagor_init_keyboard_win();
  ::dagor_init_mouse_win();
#endif
#elif _TARGET_IOS | _TARGET_ANDROID
  ::dagor_init_keyboard_win();
  ::dagor_init_mouse_win();
  ::dagor_init_joystick();
#else
  ::dagor_init_joystick();
  if (global_cls_drv_joy)
    global_cls_drv_joy->enableAutoDefaultJoystick(true);
#endif
  ::startup_game(RESTART_ALL);
  shaders_register_console(true);

  const char *sh_bindump_prefix = global_settings_blk->getStr("shaders", "compiledShaders/game");
  ::startup_shaders(sh_bindump_prefix);
  ::startup_game(RESTART_ALL);
  dagor_use_reversed_depth(true);
  ShaderGlobal::enableAutoBlockChange(true);
  ShaderGlobal::set_int(get_shader_variable_id("in_editor", true), 0);
  ShaderGlobal::set_vars_from_blk(*global_settings_blk->getBlockByNameEx("shaderVar"), true);

  ::dagor_common_startup();

  bool drawFps = global_settings_blk->getBool("draw_fps", false);
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
  game_demo_init();

  ::dagor_reset_spent_work_time();
  for (; !quitted;) // infinite cycle
  {
    ::dagor_work_cycle();
  }
  return 0;
}
