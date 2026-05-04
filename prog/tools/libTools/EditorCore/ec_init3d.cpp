// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "ec_init3d.h"
#include <EditorCore/ec_wndGlobal.h>
#include <workCycle/dag_startupModules.h>
#include <workCycle/dag_gameSettings.h>
#include <workCycle/dag_workCycle.h>
#include <3d/dag_texMgr.h>
#include <startup/dag_globalSettings.h>
#include <startup/dag_restart.h>
#include <startup/dag_loadSettings.h>
#include <ioSys/dag_dataBlock.h>
#include <osApiWrappers/setProgGlobals.h>
#include <osApiWrappers/dag_progGlobals.h>
#include <util/dag_string.h>
#include <debug/dag_debug.h>

#if _TARGET_PC_WIN
#define _WIN32_WINNT 0x500
#include <windows.h>
#else
#define SW_SHOWMINIMIZED 0
#endif

extern void default_crt_init_kernel_lib();
extern void default_crt_init_core_lib();
extern void messagebox_win_report_fatal_error(const char *title, const char *msg, const char *call_stack);

namespace workcycle_internal
{
intptr_t main_window_proc(void *, unsigned, uintptr_t, intptr_t);
}

bool tools3d::inited = false;

bool tools3d::init(const char *drv_name, const DataBlock *blkTexStreaming, const char *caption, void *icon)
{
  if (inited)
    return true;

  dgs_load_settings_blk(true, String(260, "%s/startup_editors.blk", sgg::get_common_data_dir()), nullptr);

  DataBlock *global_settings_blk = const_cast<DataBlock *>(::dgs_get_settings());

  // Some textures are re-created when the resolution of the view changes,
  // and they might not be used, so to avoid the "texture ... was destroyed
  // but was never used in rendering" messages and messages reporting about
  // the view size change itself, we disable logging those messages in tools.
  global_settings_blk->addBlock("debug")->setBool("view_resizing_related_logging_enabled", false);

  // Do not allow window occlusion checking because here (in tools3d::init) there is a desktop sized invisible window
  // created that always occludes daEditorX's main window.
  if (DataBlock *dx_blk = global_settings_blk->addBlock("directx"))
  {
    dx_blk->setBool("winOcclusionCheckEnabled", false);
    dx_blk->setInt("max_genmip_tex_sz", 4096);
    dx_blk->setBool("immutable_textures", dx_blk->getBool("immutable_textures", false));
    dx_blk->setInt("inline_vb_size", dx_blk->getInt("inline_vb_size", 32 << 20));
    dx_blk->setInt("inline_ib_size", dx_blk->getInt("inline_ib_size", 2 << 20));
    dx_blk->setBool("ignore_resource_leaks_on_exit", true);
  }

  if (DataBlock *video_blk = global_settings_blk->addBlock("video"))
  {
    video_blk->setBool("windowed_ext", true);
    video_blk->setStr("mode", "resizablewindowed");
    video_blk->setBool("threadedWindow", false);
    video_blk->setInt("min_target_size", video_blk->getInt("min_target_size", 0));
    video_blk->setStr("instancing", video_blk->getStr("instancing", "auto"));
    video_blk->setStr("driver", drv_name ? drv_name : video_blk->getStr("driver", "auto"));
    video_blk->setInt("frameLimitBufferFramememKB", video_blk->getInt("frameLimitBufferFramememKB", 8 << 10));
  }

  global_settings_blk->addBlock("physx")->setBool("disable_hardware", true);

  // VideoRestartProc::startup() uses this graphics/limitfps setting but we set it to limitFps that the daNetGame-based
  // projects use.
  // (When using the daNetGame-based renderer dgs_limit_fps will be also set by app_start().)
  global_settings_blk->addBlock("graphics")->setBool("limitfps", global_settings_blk->getBool("limitFps", true));

  const DataBlock *debugBlock = ::dgs_get_settings()->getBlockByNameEx("debug");
  ::dgs_dont_use_cpu_in_background = debugBlock->getBool("dontUseCpuInBackground", true);
  if (!::dgs_dont_use_cpu_in_background)
  {
    logdbg("dagor_enable_idle_priority OFF");
    dagor_enable_idle_priority(false); // It is true by default.
  }

  if (blkTexStreaming)
  {
    global_settings_blk->removeBlock("texStreaming");
    DataBlock *b = global_settings_blk->addNewBlock(blkTexStreaming, "texStreaming");
    b->setBool("texLoadOnDemand", true);
    init_managed_textures_streaming_support();
  }

  // wgw_dialogs.cpp also uses the EDITOR_LAYOUT_DE_WINDOW window class to find the main window.
  // Create the window minimized to avoid showing the still unpainted window. The window is white on Windows and would
  // make an annoying flash.
  ::dagor_init_video("EDITOR_LAYOUT_DE_WINDOW", SW_SHOWMINIMIZED, icon, caption);

  ::dagor_common_startup();
  ::startup_game(RESTART_ALL);

  enable_tex_mgr_mt(true, 64 << 10);
  inited = true;
  return true;
}

void tools3d::destroy()
{
  if (!inited)
    return;

  debug("destroy 3d renderer");
  ::shutdown_game(RESTART_ALL);

  inited = false;
}
