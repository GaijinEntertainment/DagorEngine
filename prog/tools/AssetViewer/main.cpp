// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "av_appwnd.h"

#include <EditorCore/ec_imguiInitialization.h>
#include <EditorCore/ec_input.h>
#include <EditorCore/ec_mainWindow.h>
#include <libTools/util/strUtil.h>
#include <workCycle/dag_gameSettings.h>
#include <startup/dag_globalSettings.h>
#include <startup/dag_fatalHandler.inc.cpp>
#include <osApiWrappers/dag_cpuJobs.h>
#include <osApiWrappers/dag_direct.h>
#include <debug/dag_debug.h>

#include <perfMon/dag_cpuFreq.h>
#include <perfMon/dag_daProfilerSettings.h>
#include <perfMon/dag_daProfiler.h>
#include <util/dag_threadPool.h>
#include <time.h>
extern "C" const char *dagor_get_build_stamp_str(char *buf, size_t bufsz, const char *suffix);
String get_global_av_settings_file_path();

bool src_assets_scan_allowed = true;
const char *av2_drv_name = "auto";
static const char *open_app_blk = NULL;

class AppManager : public IWndManagerEventHandler
{
public:
  void onInit(IWndManager *manager) override
  {
    G_ASSERT(manager);
    const char *useWorkspace = nullptr;
    for (int i = 1; i < ::dgs_argc; ++i)
      if (::dgs_argv[i][0] != '/' && ::dgs_argv[i][0] != '-')
        continue;
      else if (strnicmp(::dgs_argv[i], "-ws:", 4) == 0)
        useWorkspace = ::dgs_argv[i] + 4;

    app = new AssetViewerApp(manager, open_app_blk);
    ::dgs_pre_shutdown_handler = &clearBusy;
    app->init(useWorkspace);
  }

  bool onClose() override
  {
    G_ASSERT(app);
    return app->canClose();
  }

  void onDestroy() override { del_it(app); }

  static void clearBusy() { ec_set_busy(false); }

private:
  AssetViewerApp *app;
};


void DagorWinMainInit(int, bool) {}

IEditorCoreEngine *IEditorCoreEngine::__global_instance = NULL;

static E3DCOLOR load_window_background_color()
{
  DataBlock settingsBlock;
  dblk::load(settingsBlock, get_global_av_settings_file_path(), dblk::ReadFlag::ROBUST);
  const DataBlock *settingsThemeBlock = settingsBlock.getBlockByNameEx("theme");
  const char *themeName = settingsThemeBlock->getStr("name", "light");
  return editor_core_load_window_background_color(String::mk_str_cat(themeName, ".blk"));
}

int DagorWinMain(int /*nCmdShow*/, bool /*debugmode*/)
{
  char stamp_buf[256], start_time_buf[32] = {0};
  time_t start_at_time = time(nullptr);
  const char *asctimeResult = asctime(gmtime(&start_at_time));
  strcpy(start_time_buf, asctimeResult ? asctimeResult : "???");
  if (char *p = strchr(start_time_buf, '\n'))
    *p = '\0';
  debug("%s [started at %s UTC+0]\n", dagor_get_build_stamp_str(stamp_buf, sizeof(stamp_buf), ""), start_time_buf);

  AppManager appManager;
  for (int i = 1; i < dgs_argc; i++)
    if (stricmp(dgs_argv[i], "-no_src_assets") == 0)
      src_assets_scan_allowed = false;
    else if (strnicmp(dgs_argv[i], "-drv:", 5) == 0)
      av2_drv_name = dgs_argv[i] + 5;
    else if (!open_app_blk && trail_strcmp(dgs_argv[i], "application.blk") && dd_file_exist(dgs_argv[i]))
      open_app_blk = dgs_argv[i];

  cpujobs::init();
  threadpool::init(eastl::min(cpujobs::get_core_count(), 64), 1024, 128 << 10);
  dagor_install_dev_fatal_handler(NULL);

  da_profiler::tick_frame();

  EditorMainWindow mainWindow(appManager);
  mainWindow.run("AssetViewer2", "AssetViewerIcon", nullptr, load_window_background_color());

  da_profiler::shutdown();

  return 0;
}

#define __UNLIMITED_BASE_PATH 1
#define DAGOR_NO_DPI_AWARE    -1

#if _TARGET_PC_WIN
#include <startup/dag_winMain.inc.cpp>
#elif _TARGET_PC_LINUX
#include <startup/dag_linuxMain.inc.cpp>
#else
#error "Unsupported platform"
#endif
