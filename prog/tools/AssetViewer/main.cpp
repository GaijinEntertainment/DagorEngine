#include "av_appwnd.h"

#include <winGuiWrapper/wgw_busy.h>
#include <workCycle/dag_gameSettings.h>
#include <startup/dag_globalSettings.h>
#include <startup/dag_fatalHandler.inc.cpp>
#include <osApiWrappers/dag_cpuJobs.h>
#include <osApiWrappers/dag_direct.h>
#include <debug/dag_debug.h>

#include <perfMon/dag_cpuFreq.h>
#include <perfMon/dag_daProfilerSettings.h>
#include <util/dag_threadPool.h>
#include <time.h>
extern "C" const char *dagor_get_build_stamp_str(char *buf, size_t bufsz, const char *suffix);

bool src_assets_scan_allowed = true;
const char *av2_drv_name = "auto";
static const char *open_app_blk = NULL;

class AppManager : public IWndManagerEventHandler
{
public:
  virtual void onInit(IWndManager *manager)
  {
    G_ASSERT(manager);

    app = new AssetViewerApp(manager, open_app_blk);
    ::dgs_pre_shutdown_handler = &clearBusy;
    app->init();
  }

  virtual bool onClose()
  {
    G_ASSERT(app);
    return app->canClose();
  }

  virtual void onDestroy() { del_it(app); }

  static void clearBusy() { wingw::set_busy(false); }

private:
  AssetViewerApp *app;
};


void DagorWinMainInit(int, bool) {}

IEditorCoreEngine *IEditorCoreEngine::__global_instance = NULL;

int DagorWinMain(int nCmdShow, bool /*debugmode*/)
{
  char stamp_buf[256], start_time_buf[32] = {0};
  time_t start_at_time = time(nullptr);
  if (asctime_s(start_time_buf, sizeof(start_time_buf), gmtime(&start_at_time)) != 0)
    strcpy(start_time_buf, "???");
  else if (char *p = strchr(start_time_buf, '\n'))
    *p = '\0';
  debug("%s [started at %s UTC+0]\n", dagor_get_build_stamp_str(stamp_buf, sizeof(stamp_buf), ""), start_time_buf);

  AppManager appManager;
  for (int i = 1; i < __argc; i++)
    if (stricmp(__argv[i], "-no_src_assets") == 0)
      src_assets_scan_allowed = false;
    else if (strnicmp(__argv[i], "-drv:", 5) == 0)
      av2_drv_name = __argv[i] + 5;
    else if (strnicmp(__argv[i], "-profiler:", 10) == 0)
    {
      DataBlock profilerSettings;
      profilerSettings.addStr("profiler", __argv[i] + 10);
      da_profiler::set_profiling_settings(profilerSettings);
    }
    else if (!open_app_blk && __argv[i][0] != '/' && __argv[i][0] != '-')
      open_app_blk = __argv[i];

  cpujobs::init();
  threadpool::init(eastl::min(cpujobs::get_core_count(), 64), 1024, 128 << 10);
  dagor_install_dev_fatal_handler(NULL);
  IWndManager *wndManager = IWndManager::createManager(&appManager);
  wndManager->run(1024, 768, "AssetViewer2", "AssetViewerIcon", WSI_MAXIMIZED);

  delete wndManager;
  return 0;
}

#define __UNLIMITED_BASE_PATH 1
#define DAGOR_NO_DPI_AWARE    -1
#include <startup/dag_winMain.inc.cpp>
