// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "av_appwnd.h"

#include <EditorCore/ec_imguiInitialization.h>
#include <EditorCore/ec_input.h>
#include <EditorCore/ec_mainWindow.h>
#include <EditorCore/ec_startup.h>
#include <libTools/util/strUtil.h>
#include <workCycle/dag_gameSettings.h>
#include <startup/dag_globalSettings.h>
#include <startup/dag_fatalHandler.inc.cpp>
#include <osApiWrappers/dag_cpuJobs.h>
#include <osApiWrappers/dag_direct.h>
#include <debug/dag_debug.h>
#include <coolConsole/conBatch.h>

#include <perfMon/dag_cpuFreq.h>
#include <perfMon/dag_daProfilerSettings.h>
#include <perfMon/dag_daProfiler.h>
#include <util/dag_threadPool.h>

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
    const char *asyncBatch = nullptr;
    for (int i = 1; i < ::dgs_argc; ++i)
      if (::dgs_argv[i][0] != '/' && ::dgs_argv[i][0] != '-')
        continue;
      else if (strnicmp(::dgs_argv[i], "-ws:", 4) == 0)
        useWorkspace = ::dgs_argv[i] + 4;
      else if (strnicmp(dgs_argv[i], "-async_batch:", 13) == 0)
        asyncBatch = dgs_argv[i] + 13;

    app = new AssetViewerApp(manager, open_app_blk);
    ::dgs_pre_shutdown_handler = &clearBusy;
    app->init(useWorkspace);

    if (asyncBatch)
    {
      ConBatch batch;
      batch.runBatch(asyncBatch, true);
    }
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

int DagorWinMain(int /*nCmdShow*/, bool /*debugmode*/)
{
  ec_log_startup_info();

  AppManager appManager;
  for (int i = 1; i < dgs_argc; i++)
    if (stricmp(dgs_argv[i], "-no_src_assets") == 0)
      src_assets_scan_allowed = false;
    else if (strnicmp(dgs_argv[i], "-drv:", 5) == 0)
      av2_drv_name = dgs_argv[i] + 5;
    else if (!open_app_blk && trail_strcmp(dgs_argv[i], ".blk") && dd_file_exist(dgs_argv[i]))
      open_app_blk = dgs_argv[i];

  cpujobs::init();
  threadpool::init(eastl::min(cpujobs::get_core_count(), 64), 1024, 128 << 10);
  dagor_install_dev_fatal_handler(NULL);

  da_profiler::tick_frame();

  EditorMainWindow mainWindow(appManager);
  mainWindow.run(nullptr);

  da_profiler::shutdown();

  return 0;
}

#if _TARGET_STATIC_LIB
#include <namedPtr.cpp>
#endif

#define __UNLIMITED_BASE_PATH 1
#define DAGOR_NO_DPI_AWARE    -1

#if _TARGET_PC_WIN
#include <startup/dag_winMain.inc.cpp>
#elif _TARGET_PC_LINUX
#include <startup/dag_linuxMain.inc.cpp>
#else
#error "Unsupported platform"
#endif
