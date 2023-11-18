#include "de_appwnd.h"
#include "de_batch.h"
#include <dllPluginCore/core.h>

#include <workCycle/dag_gameSettings.h>
#include <startup/dag_globalSettings.h>
#include <startup/dag_fatalHandler.inc.cpp>
#include <3d/dag_drv3d.h>
#include <debug/dag_logSys.h>
#include <debug/dag_debug.h>
#include <debug/dag_except.h>
#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_cpuJobs.h>

#include <libTools/util/strUtil.h>
#include <libTools/dtx/ddsxPlugin.h>

#include <sepGui/wndGlobal.h>
#include <winGuiWrapper/wgw_busy.h>
#include <winGuiWrapper/wgw_dialogs.h>

#include <util/dag_oaHashNameMap.h>
#include <util/dag_delayedAction.h>
#include <windows.h>
#include <winreg.h>
#include <stdio.h>
#undef ERROR

#include "splashScreen.h"

#include <perfMon/dag_cpuFreq.h>
#include <perfMon/dag_daProfilerSettings.h>
extern "C" const char *dagor_get_build_stamp_str(char *buf, size_t bufsz, const char *suffix);

static InitOnDemand<SplashScreen> screen;

namespace sgg
{
const char *get_start_path();
};

const char *de3_drv_name = "auto";
bool de3_late_scene_select = false;


bool dabuild_usage_allowed = true;
bool minimize_dabuild_usage = false;
bool src_assets_scan_allowed = true;
OAHashNameMap<true> cmdline_force_enabled_plugins, cmdline_force_disabled_plugins;
FastNameMap cmdline_include_dll_re, cmdline_exclude_dll_re;

void (*on_batch_exit)() = NULL;
static DeBatch *global_batch = NULL;
static String global_batch_fn;
static bool batch_exit_done = false;

static void on_global_batch_exit()
{
  if (batch_exit_done)
    return;
  batch_exit_done = true;
  CoolConsole &con = DAGORED2->getConsole();

  bool ok = (!global_batch->numErrors && !con.getGlobalErrorsCounter() && !con.getGlobalFatalsCounter());

  debug("batch finished: %s (%d, %d, %d)", ok ? "OK" : "FAILED", global_batch->numErrors, con.getGlobalErrorsCounter(),
    con.getGlobalFatalsCounter());
  con.saveToFile(global_batch_fn + (ok ? ".log" : ".err"));
}

static void quiet_report_fatal_error(const char *title, const char *msg, const char *call_stack)
{
  if (on_batch_exit && !batch_exit_done)
  {
    batch_exit_done = true;
    CoolConsole &con = DAGORED2->getConsole();
    debug("fatal error during batch: FAILED (%d, %d, %d)", global_batch->numErrors, con.getGlobalErrorsCounter(),
      con.getGlobalFatalsCounter());
    con.saveToFile(global_batch_fn + ".err");
  }
  flush_debug_file();
  fflush(stdout);
  fflush(stderr);
  send_event_error(msg, call_stack);
  _exit(13);
}

class AppManager : public IWndManagerEventHandler
{
public:
  virtual void onInit(IWndManager *manager)
  {
    G_ASSERT(manager);

    const char *openFname = NULL;
    const char *batchFname = NULL;
    const char *useWorkspace = NULL;

    // test switches
    for (int i = 1; i < __argc; ++i)
    {
      if (__argv[i][0] != '/' && __argv[i][0] != '-' && ::dd_file_exist(__argv[i]))
      {
        const char *ext = ::get_file_ext(__argv[i]);

        if (ext && !stricmp(ext, "dcmd"))
          batchFname = __argv[i];
        else if (!openFname)
          openFname = __argv[i];
      }
      if (strnicmp(__argv[i], "-ws:", 4) == 0)
        useWorkspace = __argv[i] + 4;
    }

    //----------------------------------

    app = new DagorEdAppWindow(manager, openFname);

    ::dgs_pre_shutdown_handler = &clearBusy;
    app->init();

    //----------------------------------
#if _TARGET_64BIT
    int pc = ddsx::load_plugins(::make_full_path(sgg::get_exe_path_full(), "../bin64/plugins/ddsx"));
#else
    int pc = ddsx::load_plugins(::make_full_path(sgg::get_exe_path_full(), "../bin/plugins/ddsx"));
#endif
    debug("loaded %d DDSx export plugin(s)", pc);

    if (batchFname)
    {
      DeBatch batch;
      if (d3d::get_driver_code().is(!d3d::stub))
        dgs_report_fatal_error = quiet_report_fatal_error;

      global_batch_fn = ::make_full_path(sgg::get_start_path(), batchFname);
      global_batch = &batch;
      on_batch_exit = &on_global_batch_exit;

      debug("start batch: %s", global_batch_fn.str());
      SplashScreen::kill();
      batch.runBatch(global_batch_fn);
    }
    else
    {
      SplashScreen::kill();
      DAGOR_TRY { DAGORED2->startWithWorkspace(useWorkspace); }
      DAGOR_CATCH(...)
      {
        if (DAGORED2)
          DAGORED2->getConsole().addMessage(ILogWriter::ERROR, "DaEditorX failed to start");
        wingw::message_box(wingw::MBS_EXCL, "Fatal error", "DaEditorX failed to start");
        flush_debug_file();
        fflush(stdout);
        fflush(stderr);
        _exit(13);
      }
      DAGORED2->preparePluginsListmenu();
    }
  }


  virtual bool onClose()
  {
    G_ASSERT(app);
    return app->canClose();
  }


  virtual void onDestroy() { del_it(app); }

  static void clearBusy()
  {
    wingw::set_busy(false);
    SplashScreen::kill();
  }

private:
  DagorEdAppWindow *app;
};

static bool (*prev_dev_fatal_handler)(const char *msg, const char *call_stack, const char *file, int line) = NULL;
static void delayed_con_error(void *arg)
{
  String *s = (String *)arg;
  if (dagConsole)
  {
    DAGORED2->getConsole().addMessage(ILogWriter::ERROR, s->str());
    DAGORED2->getConsole().showConsole(true);
  }
  delete s;
}
bool de3_default_fatal_handler(const char *msg, const char *call_stack, const char *file, int line)
{
  send_event_error(msg, call_stack);

  if (!is_main_thread())
  {
    String *s =
      (strncmp(msg, "assert failed in", 16) == 0) ? new String(msg) : new String(0, "FATAL: %s,%d:\n    %s", file, line, msg);
    add_delayed_callback(delayed_con_error, s);
    logerr("skip fatal from non-main thread; message will be posted to console from main thread");
    return false;
  }
  if (dagConsole)
  {
    if (strncmp(msg, "assert failed in", 16) == 0)
      DAGORED2->getConsole().addMessage(ILogWriter::ERROR, msg);
    else
      DAGORED2->getConsole().addMessage(ILogWriter::ERROR, "FATAL: %s,%d:\n    %s", file, line, msg);
    DAGORED2->getConsole().showConsole(true);
  }
  return prev_dev_fatal_handler ? prev_dev_fatal_handler(msg, call_stack, file, line) : true;
}

int DagorWinMain(int nCmdShow, bool /*debugmode*/)
{
  AppManager appManager;

  cpujobs::init();
  dagor_install_dev_fatal_handler(NULL);
  prev_dev_fatal_handler = dgs_fatal_handler;
  dgs_fatal_handler = de3_default_fatal_handler;

  IWndManager *wndManager = IWndManager::createManager(&appManager);
  wndManager->run(1024, 768, "DaEditorX", "RES_DE_ICON", WSI_MAXIMIZED);

  delete wndManager;
  return 0;
}

#define __UNLIMITED_BASE_PATH 1
#define __DEBUG_FILEPATH      NULL
#define DAGOR_NO_DPI_AWARE    -1
#include <startup/dag_winMain.inc.cpp>

extern bool nvtt_estimate_single_rgb_color_only;

void DagorWinMainInit(int, bool)
{
  screen.demandInit();
  nvtt_estimate_single_rgb_color_only = true;

  bool classic_dbg = false;
  for (int i = 1; i < __argc; i++)
    if (stricmp(__argv[i], "-debug:classic") == 0)
    {
      classic_dbg = true;
      break;
    }
    else if (stricmp(__argv[i], "-no_dabuild") == 0)
      dabuild_usage_allowed = false;
    else if (stricmp(__argv[i], "-min_dabuild") == 0)
      minimize_dabuild_usage = true;
    else if (stricmp(__argv[i], "-no_src_assets") == 0)
      src_assets_scan_allowed = false;
    else if (strnicmp(__argv[i], "-enable:", 8) == 0)
      cmdline_force_enabled_plugins.addNameId(__argv[i] + 8);
    else if (strnicmp(__argv[i], "-disable:", 9) == 0)
      cmdline_force_disabled_plugins.addNameId(__argv[i] + 9);
    else if (strnicmp(__argv[i], "-include_dll_re:", 16) == 0)
      cmdline_include_dll_re.addNameId(__argv[i] + 16);
    else if (strnicmp(__argv[i], "-exclude_dll_re:", 16) == 0)
      cmdline_exclude_dll_re.addNameId(__argv[i] + 16);
    else if (strnicmp(__argv[i], "-drv:", 5) == 0)
    {
      de3_drv_name = __argv[i] + 5;
      if (strcmp(de3_drv_name, "stub") == 0)
        minimize_dabuild_usage = true;
    }
    else if (stricmp(__argv[i], "-lateSceneSelect") == 0)
      de3_late_scene_select = true;
    else if (strnicmp(__argv[i], "-profiler:", 10) == 0)
    {
      DataBlock profilerSettings;
      profilerSettings.addStr("profiler", __argv[i] + 10);
      da_profiler::set_profiling_settings(profilerSettings);
    }

  if (classic_dbg)
    start_classic_debug_system("debug");
  else
    start_debug_system(__argv[0]);

  char stamp_buf[256], start_time_buf[32] = {0};
  time_t start_at_time = time(nullptr);
  if (asctime_s(start_time_buf, sizeof(start_time_buf), gmtime(&start_at_time)) != 0)
    strcpy(start_time_buf, "???");
  else if (char *p = strchr(start_time_buf, '\n'))
    *p = '\0';
  debug("%s [started at %s UTC+0]\n", dagor_get_build_stamp_str(stamp_buf, sizeof(stamp_buf), ""), start_time_buf);
  if (d3d::get_driver_code().is(d3d::stub))
    dgs_report_fatal_error = quiet_report_fatal_error;
  if (!dabuild_usage_allowed)
    debug("dabuild-cache is disabled");
  else if (minimize_dabuild_usage)
    debug("dabuild-cache is minimized (build only textures that are not scanned from *.dxp.bin; all resources are read from *.grp)");
}
