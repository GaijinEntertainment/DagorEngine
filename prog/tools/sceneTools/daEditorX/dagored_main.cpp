// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "de_appwnd.h"
#include "de_batch.h"
#include <EditorCore/ec_IEditorCore.h>
#include <EditorCore/ec_imguiInitialization.h>
#include <EditorCore/ec_input.h>

#include <workCycle/dag_gameSettings.h>
#include <startup/dag_globalSettings.h>
#include <startup/dag_fatalHandler.inc.cpp>
#include <drv/3d/dag_info.h>
#include <debug/dag_logSys.h>
#include <debug/dag_debug.h>
#include <debug/dag_except.h>
#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_cpuJobs.h>

#include <EditorCore/ec_mainWindow.h>
#include <EditorCore/ec_wndGlobal.h>
#include <libTools/util/strUtil.h>
#include <libTools/dtx/ddsxPlugin.h>

#include <winGuiWrapper/wgw_dialogs.h>

#include <util/dag_oaHashNameMap.h>
#include <util/dag_delayedAction.h>
#include <stdio.h>
#include <time.h>
#undef ERROR

#include "splashScreen.h"

#include <perfMon/dag_cpuFreq.h>
#include <perfMon/dag_daProfilerSettings.h>

#if _TARGET_PC_LINUX
#include <unistd.h>
#endif

using editorcore_extapi::dagConsole;

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
  void onInit(IWndManager *manager) override
  {
    G_ASSERT(manager);

    const char *openFname = NULL;
    const char *batchFname = NULL;
    const char *useWorkspace = NULL;

    // test switches
    for (int i = 1; i < dgs_argc; ++i)
    {
      if (dgs_argv[i][0] != '/' && dgs_argv[i][0] != '-')
      {
        const char *ext = ::get_file_ext(dgs_argv[i]);

        if (ext && !stricmp(ext, "dcmd"))
        {
          if (::dd_file_exist(dgs_argv[i]))
            batchFname = dgs_argv[i];
          else
            logwarn("Batch file \"%s\" does not exist!", dgs_argv[i]);
        }
      }
      if (strnicmp(dgs_argv[i], "-ws:", 4) == 0)
        useWorkspace = dgs_argv[i] + 4;
      else if (!openFname && trail_strcmp(dgs_argv[i], ".level.blk") && ::dd_file_exist(dgs_argv[i]))
        openFname = dgs_argv[i];
    }

    //----------------------------------

    app = new DagorEdAppWindow(manager, openFname);

    ::dgs_pre_shutdown_handler = &clearBusy;
    app->init();

    //----------------------------------
    int pc = ddsx::load_plugins(::make_full_path(sgg::get_exe_path_full(), "plugins/ddsx"));
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
    else if (d3d::get_driver_code().is(d3d::stub))
    {
      SplashScreen::kill();
      const char *message = "The stub driver can only be used in batch mode but no batch file (a file with .dcmd extension) has been "
                            "specified in the command line arguments.";
      logerr("%s", message);
      wingw::message_box(wingw::MBS_EXCL, "Fatal error", message);
      _exit(14);
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


  bool onClose() override
  {
    G_ASSERT(app);
    return app->canClose();
  }


  void onDestroy() override { del_it(app); }

  bool onDropFiles(const dag::Vector<String> &files) { return app && app->onDropFiles(files); }

  static void clearBusy()
  {
    ec_set_busy(false);
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

static E3DCOLOR load_window_background_color()
{
  const String settingsPath = make_full_path(sgg::get_exe_path_full(), "../.local/de3_settings.blk");
  DataBlock settingsBlock;
  dblk::load(settingsBlock, settingsPath, dblk::ReadFlag::ROBUST);
  const DataBlock *settingsThemeBlock = settingsBlock.getBlockByNameEx("theme");
  const char *themeName = settingsThemeBlock->getStr("name", "light");
  return editor_core_load_window_background_color(String::mk_str_cat(themeName, ".blk"));
}

int DagorWinMain(int nCmdShow, bool /*debugmode*/)
{
  AppManager appManager;

  cpujobs::init();
  dagor_install_dev_fatal_handler(NULL);
  prev_dev_fatal_handler = dgs_fatal_handler;
  dgs_fatal_handler = de3_default_fatal_handler;

  EditorMainWindow mainWindow(appManager);
  mainWindow.run(
    "DaEditorX", "RES_DE_ICON", [&appManager](const dag::Vector<String> &files) -> bool { return appManager.onDropFiles(files); },
    load_window_background_color());

  return 0;
}

extern bool nvtt_estimate_single_rgb_color_only;

void DagorWinMainInit(int, bool)
{
  screen.demandInit();
  nvtt_estimate_single_rgb_color_only = true;

  bool classic_dbg = false;
  for (int i = 1; i < dgs_argc; i++)
    if (stricmp(dgs_argv[i], "-debug:classic") == 0)
    {
      classic_dbg = true;
      break;
    }
    else if (stricmp(dgs_argv[i], "-no_dabuild") == 0)
      dabuild_usage_allowed = false;
    else if (stricmp(dgs_argv[i], "-min_dabuild") == 0)
      minimize_dabuild_usage = true;
    else if (stricmp(dgs_argv[i], "-no_src_assets") == 0)
      src_assets_scan_allowed = false;
    else if (strnicmp(dgs_argv[i], "-enable:", 8) == 0)
      cmdline_force_enabled_plugins.addNameId(dgs_argv[i] + 8);
    else if (strnicmp(dgs_argv[i], "-disable:", 9) == 0)
      cmdline_force_disabled_plugins.addNameId(dgs_argv[i] + 9);
    else if (strnicmp(dgs_argv[i], "-include_dll_re:", 16) == 0)
      cmdline_include_dll_re.addNameId(dgs_argv[i] + 16);
    else if (strnicmp(dgs_argv[i], "-exclude_dll_re:", 16) == 0)
      cmdline_exclude_dll_re.addNameId(dgs_argv[i] + 16);
    else if (strnicmp(dgs_argv[i], "-drv:", 5) == 0)
    {
      de3_drv_name = dgs_argv[i] + 5;
      if (strcmp(de3_drv_name, "stub") == 0)
        minimize_dabuild_usage = true;
    }
    else if (stricmp(dgs_argv[i], "-lateSceneSelect") == 0)
      de3_late_scene_select = true;
    else if (strnicmp(dgs_argv[i], "-profiler:", 10) == 0)
    {
      DataBlock profilerSettings;
      profilerSettings.addStr("profiler", dgs_argv[i] + 10);
      da_profiler::set_profiling_settings(profilerSettings);
    }

  if (classic_dbg)
    start_classic_debug_system("debug");
  else
    start_debug_system(dgs_argv[0]);

  char stamp_buf[256], start_time_buf[32] = {0};
  time_t start_at_time = time(nullptr);
  const char *asctimeResult = asctime(gmtime(&start_at_time));
  strcpy(start_time_buf, asctimeResult ? asctimeResult : "???");
  if (char *p = strchr(start_time_buf, '\n'))
    *p = '\0';
  debug("%s [started at %s UTC+0]\n", dagor_get_build_stamp_str(stamp_buf, sizeof(stamp_buf), ""), start_time_buf);
  if (d3d::get_driver_code().is(d3d::stub))
    dgs_report_fatal_error = quiet_report_fatal_error;
  if (!dabuild_usage_allowed)
    debug("dabuild-cache is disabled");
  else if (minimize_dabuild_usage)
    debug("dabuild-cache is minimized (build only textures that are not scanned from *.dxp.bin; all resources are read from *.grp)");
}

#if _TARGET_STATIC_LIB
void daeditor3_init_globals(IDagorEd2Engine &) {}
String editorcore_extapi::make_full_start_path(const char *rel_path) { return ::make_full_path(sgg::get_exe_path(), rel_path); }
#include <namedPtr.cpp>
#endif

#define __UNLIMITED_BASE_PATH 1
#define __DEBUG_FILEPATH      NULL
#define DAGOR_NO_DPI_AWARE    -1

#if _TARGET_PC_WIN
#include <startup/dag_winMain.inc.cpp>
#elif _TARGET_PC_LINUX
#include <startup/dag_linuxMain.inc.cpp>
#else
#error "Unsupported platform"
#endif
