// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_lock.h>
#include <3d/dag_picMgr.h>
#include <3d/dag_gpuConfig.h>
#include <debug/dag_debug.h>
#include <debug/dag_logSys.h>
#include <debug/dag_visualErrLog.h>
#include <debug/dag_traceInpDev.h>
#include <drv/hid/dag_hiGlobals.h>
#include <gameRes/dag_gameResSystem.h>
#include <gameRes/dag_stdGameRes.h>
#include <gameRes/dag_collisionResource.h>
#include <generic/dag_initOnDemand.h>
#include <gui/dag_stdGuiRender.h>
#include <ioSys/dag_dataBlock.h>
#include <ioSys/dag_dataBlockUtils.h>
#include <math/random/dag_random.h>
#include <memory/dag_framemem.h>
#include <osApiWrappers/dag_atomic.h>
#include <osApiWrappers/dag_cpuJobs.h>
#include <osApiWrappers/dag_files.h>
#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_progGlobals.h>
#include <osApiWrappers/dag_messageBox.h>
#include <osApiWrappers/dag_vromfs.h>
#include <osApiWrappers/dag_miscApi.h>
#include <osApiWrappers/dag_sockets.h>
#include <osApiWrappers/dag_threads.h>
#include <osApiWrappers/dag_pathDelim.h>
#include <perfMon/dag_cpuFreq.h>
#include <perfMon/dag_perfTimer.h>
#include <perfMon/dag_sleepPrecise.h>
#include <shaders/dag_shaderBlock.h>
#include <shaders/dag_shaderMeshTexLoadCtrl.h>
#include <statsd/statsd.h>
#include <startup/dag_globalSettings.h>
#include <startup/dag_restart.h>
#include <startup/dag_splashScreen.h>
#include <squirrel/memtrace.h>
#include <quirrel/sqplus/dag_sqAux.h>
#include <supp/dag_cpuControl.h>
#include <workCycle/dag_gameSettings.h>
#include <workCycle/dag_startupModules.h>
#include <workCycle/dag_workCycle.h>
#include <workCycle/dag_workCyclePerf.h>
#include <util/dag_delayedAction.h>
#include <util/dag_stlqsort.h>
#include <ecs/core/entityManager.h>
#include <rendInst/rendInstGen.h>
#include <asyncHTTPClient/asyncHTTPClient.h>
#include <asyncHTTPClient/curl_global.h>
#include <publicConfigs/publicConfigs.h>
#include <userSystemInfo/systemInfo.h>
#include <userSystemInfo/userSystemInfo.h>
#include <folders/folders.h>
#include <bindQuirrelEx/bindQuirrelEx.h>
#include <streaming/streaming.h>
#include <EASTL/functional.h>
#include <lagCatcher/lagCatcher.h>
#include <gui/dag_imgui.h>
#include <util/dag_localization.h>
#include <daScript/daScript.h>

#include "game/gameScripts.h"
#include "input/inputControls.h"
#include "main/appProfile.h"
#include "main/console.h"
#include "main/circuit.h"
#include "main/ecsUtils.h"
#include "main/gameLoad.h"
#include "main/fatal.h"
#include "main/localStorage.h"
#include "main/main.h"
#include "main/scriptDebug.h"
#include "main/settings.h"
#include "main/version.h"
#include "main/vromfs.h"
#include "main/webui.h"
#include "main/watchdog.h"
#include "main/storeApiEvents.h"
#include "render/animatedSplashScreen.h"
#include <render/hdrRender.h>
#include "net/dedicated.h"
#include "net/net.h"
#include "net/telemetry.h"
#include "render/renderer.h" // init_tex_streaming
#include "ui/userUi.h"
#include "level.h"
#include "logProcessing.h"

#include <sqrat.h>
#include <quirrel/frp/dag_frp.h>
#include <stdio.h>
#include <stdlib.h>
#include <crypto/ssl.h>

#if _TARGET_C1 | _TARGET_C2

#elif _TARGET_GDK
#include "platform/gdk/gdkServices.h"
#elif _TARGET_C3

#elif _TARGET_ANDROID
#include <ioSys/dag_fileIo.h>
#include "platform/android/android.h"
#include <crashlytics/firebase_crashlytics.h>
#elif _TARGET_IOS
#include <crashlytics/firebase_crashlytics.h>
extern void runIOSRunloop(const eastl::function<void()> &callback);
#endif

#if _TARGET_PC_WIN
#define NOGDI
#include <Windows.h>
#include <Shellapi.h>
#if defined(__clang__) && !defined(_M_ARM64)
#include <cpuid.h>
#else
#include <intrin.h>
#endif
#include <direct.h>
#elif _TARGET_PC_LINUX | _TARGET_APPLE
#include <unistd.h>
#include <signal.h>
#include <aio.h>
#endif
#define CURL_STATICLIB
#if !_TARGET_GDK
#include <curl/curl.h>
#endif

#include <gui/dag_guiStartup.h>
#include <drv/3d/dag_commands.h>
#include <drv/3d/dag_info.h>
#include <3d/dag_lowLatency.h>
#include <daRg/dag_renderObject.h>
#include <debug/dag_memReport.h>
#include <osApiWrappers/dag_basePath.h>
#include "updaterEvents.h"
#include <perfMon/dag_daProfilerSettings.h>

extern void dng_load_localization();

#if _TARGET_PC_WIN || _TARGET_PC_LINUX
#define __DAGOR_OVERRIDE_DEFAULT_ROOT_RELATIVE_PATH ".."
#endif

ECS_REGISTER_EVENT(VromfsUpdaterEarlyInitEvent)
ECS_REGISTER_EVENT(VromfsUpdaterShutdownEvent)
ECS_REGISTER_EVENT(CheckVromfsUpdaterRunningEvent, bool * /*dest_is_running*/)
ECS_REGISTER_EVENT(VromfsUpdateOnStartEvent)

ECS_BROADCAST_EVENT_TYPE(ContentConsistencySelfCheckEvent, bool * /*dest_check_ok*/)

ECS_REGISTER_EVENT(OnStoreApiEarlyInit)
ECS_REGISTER_EVENT(OnStoreApiInit)
ECS_REGISTER_EVENT(OnStoreApiTerm)
ECS_REGISTER_EVENT(OnStoreApiUpdate)
ECS_REGISTER_EVENT(StoreApiReportContentCorruption)

extern void init_shaders();
extern void init_res_factories();
extern void term_res_factories();

extern void start_local_profile_server();

const char *get_dir(const char *location) { return folders::get_path(location, ""); }

static void (*old_shutdown_handler)() = nullptr;

#if _TARGET_PC
static void init_early() // called from within main(), but before log system init and DagorWinMainInit
{
  if (dgs_get_argv("dumpversion")) // print version of app & exit
  {
    printf("%s\n", get_exe_version_str());
#if _TARGET_PC
    fflush(stdout);
    _exit(0); // don't call static dtors
#endif
  }
#if _TARGET_PC_LINUX
#ifdef __SANITIZE_THREAD__ // use big idle timeout to prevent exit of AIO threads which prevents TSAN suppressions to work
  aioinit aioi{20 /* aio_threads */, 64 /* aio_num */, 0, 0, 0, 0, 600 /* aio_idle_time*/, 0};
#else
  aioinit aioi{20 /* aio_threads */, 64 /* aio_num */, 0, 0, 0, 0, 30 /* aio_idle_time*/, 0};
#endif
  aio_init(&aioi);
#endif
}
#endif

#if DAGOR_DBGLEVEL > 0
#define __DEBUG_MODERN_PREFIX dng_get_log_prefix(false)
#define __DEBUG_DYNAMIC_LOGSYS
#define __DEBUG_CLASSIC_LOGSYS_FILEPATH dng_get_log_prefix(false)
#define __DEBUG_USE_CLASSIC_LOGSYS()    (dedicated::is_dedicated() || dgs_get_argv("stdout"))
#else
#define __DEBUG_FILEPATH dng_get_log_prefix(true)
#endif

static String get_log_dir()
{
  String dir(framemem_ptr());
  dir.printf(0, ".logs~%s", get_game_name());
  return dir;
}

#if _TARGET_PC
static String dng_get_log_prefix(bool do_crypt)
{
  init_early(); // side effect

  String prefix(framemem_ptr());
  if (::dgs_get_argv("stdout"))
  {
    prefix = "*";
    return prefix;
  }
  else
  {
    prefix = dedicated::setup_log();
    if (!prefix.empty())
      return prefix;
  }
  prefix.printf(0, "%s/", get_log_dir());
  if (!::dgs_get_argv("devMode") && do_crypt)
    prefix += "#";
  return prefix;
}
#endif

#if _TARGET_PC_WIN
#include <startup/dag_winMain.inc.cpp>
#elif _TARGET_PC_LINUX
#include <startup/dag_linuxMain.inc.cpp>
#elif _TARGET_ANDROID
#define DEBUG_ANDROID_CMDLINE_FILE "dagor_cmd.txt"
#include <startup/dag_androidMain.inc.cpp>
#elif _TARGET_C1 | _TARGET_C2

#elif _TARGET_XBOX
#include <startup/dag_xboxMain.inc.cpp>
#elif _TARGET_IOS
#elif _TARGET_APPLE
#elif _TARGET_C3


#else
#error "Unsupported platform"
#endif


namespace ddsx
{
extern void shutdown_tex_pack2_data();
}
static bool do_fatal_on_logerr_on_exit = false;
static bool initial_loading_complete = false;

static const char *quit_reason = nullptr;
#if _TARGET_PC_LINUX
static void signal_handler(int signum) // Warning: only async signal safe function calls from within!
{
  if (signum == SIGTERM)
    quit_reason = "SIGTERM";
  else if (signum == SIGINT)
    quit_reason = "SIGINT";
}
static void install_signal_handlers()
{
  signal(SIGTERM, signal_handler); // default kill ("kill <pid>")
  signal(SIGINT, signal_handler);  // CTRL+C
  signal(SIGPIPE, SIG_IGN);        // broken pipe
  lagcatcher::init_early();        // uses SIGUSR2 & SIGPROF
}
#else
static void install_signal_handlers() {}
#endif
void exit_game(const char *reason_static_str)
{
  G_ASSERT(reason_static_str);
  quit_reason = reason_static_str;
}

extern void init_device_reset();
extern void app_start();
extern void app_close();

#if _TARGET_ANDROID
static DataBlock preLoadSettings;

static void restoreConfigIfNotExist(const char *name)
{
  if (!dd_file_exists(name))
  {
    Tab<char> data;
    if (android_get_asset_data(name, data))
    {
      FullFileSaveCB cwr(name);
      cwr.write(data.data(), data_size(data));
    }
  }
}
#endif

class DaNetGameTexLoadController : public IShaderMatVdataTexLoadCtrl
{
public:
  virtual const char *preprocessMatVdataTexName(
    const char *original_tex_name, unsigned ctx_obj_type, const char * /*ctx_obj_name*/, String &tmp_storage)
  {
    if (ctx_obj_type == RendInstGameResClassId || ctx_obj_type == DynModelGameResClassId || ctx_obj_type == AnimCharGameResClassId)
    {
      if (get_managed_texture_id(original_tex_name) != BAD_TEXTUREID && strstr(original_tex_name, "_tomoe"))
        return original_tex_name;

      size_t len = strlen(original_tex_name);
      if (len > 0 && original_tex_name[len - 1] == '*')
      {
        tmp_storage.setStr(original_tex_name, len - 1);
        tmp_storage += "_tomoe*";
      }
      else
      {
        tmp_storage.setStr(original_tex_name);
        tmp_storage += "_tomoe";
      }

      if (get_managed_texture_id(tmp_storage.c_str()) != BAD_TEXTUREID)
        return tmp_storage.c_str();
    }
    return original_tex_name;
  }
};

static DaNetGameTexLoadController texLoadController;

extern size_t pull_mem_leak_detector;
extern size_t get_framework_pulls();
size_t framework_pulls = 0;


static void init_user_system_info_cache_dir()
{
  String uuid_cache_dir(folders::get_path("online_storage"));
  uuid_cache_dir += PATH_DELIM;
  uuid_cache_dir += "label";
  debug("UUID cache dir: %s", uuid_cache_dir.str());
  set_user_system_info_cache_dir(uuid_cache_dir.str());
}


void DagorWinMainInit(int, bool)
{
  install_signal_handlers(); // before any thread creation (incl. logging which might create thread)
  fatal::init(); // this inits errors reporting and shall be called as early as possible (i.e. before any other initialization code)
  DataBlock::singleBlockChecking = true;
#if _TARGET_ANDROID
  restoreConfigIfNotExist("config.blk");
#elif _TARGET_GDK
  init_gdk_services();
#elif _TARGET_C3

#endif
  folders::initialize(get_game_name());

#if _TARGET_ANDROID
  String configBlkFilename(get_config_filename(get_game_name()));
  restoreConfigIfNotExist(configBlkFilename);
  dagor_android_preload_settings_blk = &preLoadSettings;
  DataBlock &cfg = preLoadSettings;
  dgs_apply_changes_to_config(cfg, true);

  g_entity_mgr.demandInit();
  g_entity_mgr->broadcastEventImmediate(MobileSelfUpdateOnStartEvent{&cfg});

  folders::load_custom_folders(*cfg.getBlockByNameEx("folders"));
  init_user_system_info_cache_dir();
#endif
#if _TARGET_C3





#endif

  framework_pulls = get_framework_pulls() + pull_mem_leak_detector;
}

bool is_initial_loading_complete() { return initial_loading_complete; }

static void post_shutdown_handler()
{
  // non zero ptr might be used as threads interruption condition
  interlocked_compare_exchange_ptr(quit_reason, (const char *)"null", (const char *)nullptr);
  dgs_app_active = true; // Otherwise watchdog isn't working
  watchdog::change_mode(watchdog::/*UN*/ LOADING);

  DEBUG_CTX("shutdown because of '%s'!", quit_reason);

#if _TARGET_PC
  if (!dedicated::is_dedicated() && dgs_get_settings()->getBool("launchCountTelemetry", true))
  {
    get_settings_override_blk()->setBool("launchCorrectExit", true);
    save_settings(nullptr, false);
  }
#endif

  g_entity_mgr->broadcastEventImmediate(VromfsUpdaterShutdownEvent{});
  memreport::dump_memory_usage_report(0);
  animated_splash_screen_stop(); //-V779
  bindquirrel::term_logerr_interceptor();

#if _TARGET_C1 | _TARGET_C2

#endif
  pubcfg::shutdown();

  app_close();

#if _TARGET_GDK
  shutdown_gdk_services();
#endif

  stop_webui();
  reset_game_resources();
  gameres_rendinst_desc.reset();
  gameres_dynmodel_desc.reset();
  term_res_factories();
  ddsx::shutdown_tex_pack2_data();
  shutdown_game(RESTART_INPUT);
  shutdown_game(RESTART_ALL);
  stop_settings_async_saver_jobmgr();
  cpujobs::term(true);
  clear_settings();
  watchdog::shutdown();
  if (do_fatal_on_logerr_on_exit)
    visual_err_log_check_any();
  telemetry_shutdown();

  curl_global::shutdown();

  crypto::cleanup_ssl();

#if _TARGET_PC
  DaThread::terminate_all(/*wait*/ true, /*timeout*/ 7000); // wait for PurgeLogThread and similar threads
#endif

#if _TARGET_C3

#endif

  // invoke shutdown handler installed by platform
  if (old_shutdown_handler)
    old_shutdown_handler();
}

void set_window_title(const char *net_role)
{
#if _TARGET_PC
  String title(framemem_ptr());
  if (const char *nm = get_localized_text("windowTitle", ::dgs_get_settings()->getStr("windowTitle", get_game_name())))
    title.printf(0, "%s", nm);
#if DAGOR_DBGLEVEL > 0
  if (net_role)
    title.aprintf(0, " %s", net_role);
#endif
  ::win32_set_window_title_utf8(title);
#endif
  G_UNREFERENCED(net_role);
}

static void dump_log_initial(const char *cwd)
{
  String cmdLine(framemem_ptr());
  cmdLine.reserve(DAGOR_MAX_PATH);
#if _TARGET_PC_LINUX
  int rd = readlink("/proc/self/exe", cmdLine.data(), DAGOR_MAX_PATH - 1);
  if (rd > 0)
  {
    cmdLine.data()[rd] = '\0';
    char rpbuf[PATH_MAX];
    cmdLine = realpath(cmdLine.data(), rpbuf);
  }
  if (cmdLine.empty())
#endif
    cmdLine = dgs_argv[0];
  cmdLine += ' ';
  for (int i = 1; i < dgs_argc; ++i)
  {
    cmdLine += dgs_argv[i];
    cmdLine += i == dgs_argc - 1 ? "" : " ";
  };
  debug("%s", cmdLine.str());
#if _TARGET_PC
  debug("cwd: %s", cwd);
#else
  G_UNUSED(cwd);
#endif
  debug("version: %s", get_exe_version_str());
  time_t now = time(NULL);
  const char *strNow = asctime(gmtime(&now));
  debug("build_num: %d, started at %.*s UTC", get_build_number(), strlen(strNow) - 1, strNow);
}

static void init_game_res()
{
  debug("registering factories");

  ::set_gameres_sys_ver(2);
  init_res_factories();
  CollisionResource::registerFactory();
  rendinst::register_land_gameres_factory();
}

static inline void start_launcher()
{
#if _TARGET_PC_WIN
  ShellExecuteW(NULL, NULL, L"launcher.exe", NULL, NULL, SW_RESTORE);
#endif
}

static bool init_critical()
{
#if _TARGET_PC_WIN && _TARGET_64BIT
  bool ok = true;
#if _TARGET_SIMD_SSE >= 4
#ifdef __clang__ // __cpuid() clobbers registers under ASAN, so use clang-specific API instead
  unsigned cpu[4] = {0};
  __get_cpuid(1, &cpu[0], &cpu[1], &cpu[2], &cpu[3]);
#else
  int cpu[4] = {0};
  __cpuid(cpu, 1);
#endif
  // Note: there is a little sense in practice to check SSE4.1 here since clang uses it in static ctors anyway but be conservative here
  // To consider: move cpuid check/restart in static ctor of high priority?
  // https://en.wikipedia.org/wiki/CPUID#EAX=1:_Processor_Info_and_Feature_Bits
  static constexpr int SSE41_BIT = 1 << 19, POPCNT_BIT = 1 << 23;
  ok &= (cpu[2] & (SSE41_BIT | POPCNT_BIT)) == (SSE41_BIT | POPCNT_BIT);
#endif
  if (ok && os_sockets_init() == 0)
    atexit([]() { os_sockets_shutdown(); });
  else
  {
    if (ok) // don't try to load blk if cpuid is not correct (might e.g. crash on popcnt)
      commit_settings([](DataBlock &blk) { blk.setStr("clientBitType", "32bit"); });
    logwarn("Restart as win32 because of %s...", ok ? "WSAStartup fail" : "not supported CPU");
    ok = false;
    // Restart exe from win32 dir with same args
    eastl::wstring cmdl = GetCommandLineW();
    if (wchar_t *winXX = wcsstr(&cmdl[0], L"\\win64\\"))
    {
      winXX[4] = L'3';
      winXX[5] = L'2';
      STARTUPINFOW si = {0};
      si.cb = sizeof(si);
      PROCESS_INFORMATION pi = {0};
      if (CreateProcessW(NULL, &cmdl[0], NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi))
      {
        // Should wait here b/c some anti-cheat libs and other launchers might actively wait for this process
        WaitForSingleObject(pi.hProcess, INFINITE);
        return ok;
      }
    }
    // fallback path
    start_launcher();
  }
  return ok;
#else
  return true;
#endif
}

static unsigned min_frame_time_us = 0;
static bool allow_d3d_backend_frame_limiter = false;
void set_fps_limit(int max_fps)
{
#if _TARGET_IOS
  extern void setFPSLimit(int);
  setFPSLimit(max_fps);
#endif
  if (max_fps <= 0)
    min_frame_time_us = 0;
  else
    min_frame_time_us = 1e6 / max_fps;

  allow_d3d_backend_frame_limiter = dgs_get_settings()->getBlockByNameEx("video")->getBool("d3dFrameLimiter", false);
}

int get_fps_limit() { return min_frame_time_us ? 1e6 / min_frame_time_us : 0; }

void set_corrected_fps_limit(int fps_limit)
{
  const int fpsLimitFromSettings = [] {
    auto videoBlk = ::dgs_get_settings()->getBlockByNameEx("video");
    String platformFpsLimitName = String(32, "%sFpsLimit", get_platform_string_id());
    return videoBlk->paramExists(platformFpsLimitName) ? videoBlk->getInt(platformFpsLimitName) : videoBlk->getInt("fpsLimit", 0);
  }();
  if (fps_limit < 0)
    fps_limit = fpsLimitFromSettings;
  else if (fpsLimitFromSettings > 0)
    // parenthesis at eastl::min used  to avoid conflict with define of min in minwindef.h
    fps_limit = (eastl::min)(fps_limit, fpsLimitFromSettings);
  set_fps_limit(fps_limit);
}

struct FrameSleeper
{
  static constexpr unsigned MIN_LOADING_FRAME_TIME_US = 1e6 / 30.0;
  int64_t startRefTicks;

  // if allowed and d3d driver supports frame event callbacks
  // register wait callback on its backend,
  // to avoid frame rate instability
  // caused by internal driver queue being underfeeded
  bool registerOneTimeD3DBackendWait()
  {
    if (!allow_d3d_backend_frame_limiter)
      return false;

    struct D3DBackendWaiter final : public FrameEvents
    {
      int64_t startRefTicks = 0;
      void beginFrameEvent() override {}
      void endFrameEvent() override
      {
        FrameSleeper::waitForFrameRemainder(startRefTicks);
        startRefTicks = profile_ref_ticks();
      }
    };
    static D3DBackendWaiter waiter;

    // not all drivers support execution event callbacks
    return d3d::driver_command(Drv3dCommand::REGISTER_ONE_TIME_FRAME_EXECUTION_EVENT_CALLBACKS, &waiter);
  }

  static void waitForFrameRemainder(int64_t start_ref_ticks)
  {
    TIME_PROFILE(wait_for_target_fps_limit);

    bool load = sceneload::is_load_in_progress();
    if (unsigned minFrameTimeUs = load ? MIN_LOADING_FRAME_TIME_US : min_frame_time_us)
    {
      const unsigned frameTimeUs = (unsigned)profile_time_usec(start_ref_ticks);
      if (frameTimeUs < minFrameTimeUs)
      {
        if (load) // Note: imprecise FPS is fine during loading (i.e. don't waste CPU on active wait)
          sleep_msec((minFrameTimeUs - frameTimeUs) / 1000);
        else
          sleep_precise_usec(minFrameTimeUs - frameTimeUs);
      }
    }
  }

  FrameSleeper() { startRefTicks = registerOneTimeD3DBackendWait() ? 0 : profile_ref_ticks(); }

  ~FrameSleeper()
  {
    if (startRefTicks)
      waitForFrameRemainder(startRefTicks);
  }
};


static unsigned int quirrel_alloc_ignore_size = 0;

static void quirrel_huge_alloc_hook(unsigned int size, unsigned cur_threshold, HSQUIRRELVM vm)
{
  if (size > quirrel_alloc_ignore_size)
  {
    quirrel_alloc_ignore_size = size * 2;
    if (vm)
    {
      const char *callstack = nullptr;
      sqmemtrace::set_huge_alloc_threshold(256 << 20); // Prevent recursion
      if (SQ_SUCCEEDED(sqstd_formatcallstackstring(vm)))
        G_VERIFY(SQ_SUCCEEDED(sq_getstring(vm, -1, &callstack)));
      sqmemtrace::set_huge_alloc_threshold(cur_threshold);

      logerr("Huge allocation in quirrel (%d K) which is more then current threshold of %d K\n%s", size >> 10, cur_threshold >> 10,
        callstack ? callstack : "No call stack available");

      if (callstack)
        sq_pop(vm, 1);
    }
  }
}

SQInteger get_thread_id_func() { return SQInteger(get_current_thread_id()); }

#if _TARGET_PC
static void purge_obsolete_logs(const DataBlock &purgeLogCfg)
{
  int refms = get_time_msec();
  String logDir = get_log_dir();
  int64_t maxSize = purgeLogCfg.getInt64("maxSizeKB", 512 << 10) << 10;
  bool dryRun = purgeLogCfg.getBool("dryRun", DAGOR_DBGLEVEL > 0);
#if DAGOR_DBGLEVEL > 0
  bool logVerbose = purgeLogCfg.getBool("logVerbose", false);
#else
  constexpr bool logVerbose = false;
#endif
  time_t curTime = time(nullptr);
  int eraseTimeDelta = purgeLogCfg.getInt("eraseTimeDeltaSec", /*week*/ 7 * 24 * 60 * 60);
  time_t eraseTime = eraseTimeDelta ? (curTime - eraseTimeDelta) : 0;
  time_t timeLimit = curTime + purgeLogCfg.getInt("timeLimitDeltaSec", /*2min*/ 120);
  int64_t totalSize = 0, removedSize = 0;
  int nfound = 0, nremoved = 0;
  char tmpPath[DAGOR_MAX_PATH];
  const char *func = __FUNCTION__;
  auto removef = [&](const alefind_t &alf, bool bytime) {
    SNPRINTF(tmpPath, sizeof(tmpPath), "%s/%s", logDir.c_str(), alf.name);
    if (logVerbose)
    {
      if (bytime)
        debug("%s: %sremove '%s' of size=%dK because it's ctime %lld <= eraseTime %lld", func, dryRun ? "would " : "", tmpPath,
          alf.size >> 10, alf.mtime, eraseTime);
      else
        debug("%s: %sremove '%s' of size=%dK because it doesn't fit in size limit of %dK", func, dryRun ? "would " : "", tmpPath,
          alf.size >> 10, maxSize >> 10);
    }
    if (!dryRun)
      dd_erase(tmpPath);
    ++nremoved;
    removedSize += alf.size;
  };
  Tab<alefind_t> fileList;
  SNPRINTF(tmpPath, sizeof(tmpPath), "%s/*", logDir.c_str());
  alefind_t ff;
  if (::dd_find_first(tmpPath, DA_FILE, &ff)) // To consider: what about dirs?
  {
    do
    {
      ++nfound;
      if (eraseTime && ff.mtime <= eraseTime)
        removef(ff, /*bytime*/ true);
      else
      {
        fileList.push_back(ff);
        totalSize += ff.size;
      }
      if (interlocked_acquire_load_ptr(quit_reason) || timeLimit < time(nullptr))
      {
        maxSize = 0;
        break;
      }
    } while (dd_find_next(&ff));
  }
  else
  {
    logwarn("%s: failed to find any files at '%s'", func, logDir.c_str());
    return;
  }

  if (maxSize && totalSize > maxSize)
  {
    // sort from oldest to newest
    stlsort::sort_branchless(fileList.begin(), fileList.end(), [](const auto &a, const auto &b) { return a.mtime < b.mtime; });
    for (auto &it : fileList)
    {
      removef(it, /*bytime*/ false);
      if ((totalSize -= it.size) < maxSize)
        break;
      if (interlocked_acquire_load_ptr(quit_reason) || timeLimit < time(nullptr))
        break;
    }
  }

  debug("%s: %s %d/%d/%d files/total/KB at %d ms", func, dryRun ? "would remove" : "removed", nremoved, nfound, removedSize >> 10,
    get_time_msec() - refms);
}
static void start_purge_logs_thread()
{
  const DataBlock &purgeLogCfg = *dgs_get_settings()->getBlockByNameEx("purgeLog");
  if (purgeLogCfg.getBool("enabled", !dedicated::is_dedicated())) // in dev it's started in "dry run mode" by default
    execute_in_new_thread([&](auto) { purge_obsolete_logs(purgeLogCfg); }, "PurgeLogThread",
      DaThread::DEFAULT_STACK_SZ * 4, // for sorting
      cpujobs::DEFAULT_THREAD_PRIORITY + cpujobs::THREAD_PRIORITY_LOWER_STEP);
}
#else
static inline void start_purge_logs_thread() {}
#endif

int DagorWinMain(int nCmdShow, bool /*debugmode*/)
{
#if _TARGET_IOS
  // At this point we have Dagor2_AppDelegate initialized.
  // It's important to draw anything on the screen.
  g_entity_mgr.demandInit();
  g_entity_mgr->broadcastEventImmediate(MobileSelfUpdateOnStartEvent{nullptr});
#endif

  // tune delayed action max quota to avoid application not responding / lagging on delayed actions
  set_delayed_action_max_quota(::dgs_get_settings()->getInt("delayedActionsQuotaUs", 500000));

  debug_enable_thread_ids(true);
  memreport::dump_memory_usage_report(0);
#if _TARGET_PC
  char cwdbuf[DAGOR_MAX_PATH] = {0};
  dump_log_initial(getcwd(cwdbuf, sizeof(cwdbuf)));
#else
  dump_log_initial(NULL);
#endif

  if (!init_critical())
    return 2;
  verify_nan_finite_checks();
#if _TARGET_C1 | _TARGET_C2

#endif

  if (const char *circuit_name = circuit::init_name_early())
  {
    debug("early init circuit name <%s>", circuit_name);
  }
#if _TARGET_PC_LINUX
  G_ASSERT(cwdbuf[0]);
  dd_append_slash_c(cwdbuf);
  dd_remove_base_path("");
  dd_add_base_path(cwdbuf); // cwd is expected to point to game root at this point (see dagor_change_root_directory())
#endif
  g_entity_mgr.demandInit(); // init it early to be able to use broadcast events for init
  bool selfcheck_passed = true;
  g_entity_mgr->broadcastEventImmediate(ContentConsistencySelfCheckEvent{&selfcheck_passed});
  if (!selfcheck_passed)
    return 3;

  if (!net_init_early()) // do init network early as it might fail application startup (i.e. on server)
    return 1;

  g_entity_mgr->broadcastEventImmediate(VromfsUpdaterEarlyInitEvent{});

  crypto::init_ssl(crypto::InitSslFlags::InitLocking | crypto::InitSslFlags::InitDagorRandSeed);

  curl_global::init();


  g_entity_mgr->broadcastEventImmediate(OnStoreApiEarlyInit{});

  // do not add base path here, because empty one is already added
  ::measure_cpu_freq();
  ::dgs_dont_use_cpu_in_background = false;
  // we can't have reliable timings without correct delta times (especially important on dedicated)
  ::dagor_game_act_time_limited = false;
  old_shutdown_handler = ::dgs_post_shutdown_handler;
  ::dgs_post_shutdown_handler = post_shutdown_handler;
  dagor_enable_idle_priority(false);

  DataBlock::setRootIncludeResolver(NULL);

#if _TARGET_IOS
  const String gameData = folders::get_gamedata_dir();
  dd_add_base_path(gameData);
  // In case of bundle without resource we have to add downloads folder to base paths.
  // All content and sound will be there
  if (!dd_dir_exists("content"))
  {
    const String downloads = folders::get_downloads_dir();
    dd_add_base_path((downloads + "/").c_str());
    dd_dump_base_paths();
  }
#endif

#if _TARGET_C1 | _TARGET_C2

#endif
  load_settings(get_game_name(), /* resolve_tex_streaming */ false, /* use_on_reload_backup */ true);
  apply_vrom_list_difference(*dgs_get_settings()->getBlockByNameEx("vromList"), {});
  load_settings(get_game_name(), true); // we reload settings (with resolving tex_streaming config)

  // This must be called after settings load
  cpujobs::init(dgs_get_settings()->getBlockByNameEx("debug")->getInt("coreCount", -1), /*reserve_jobmgr_cores*/ false);

  const DataBlock &debugSettings = *dgs_get_settings()->getBlockByNameEx("debug");
  da_profiler::set_profiling_settings(debugSettings);
#if TIME_PROFILER_ENABLED && DAGOR_THREAD_SANITIZER
  // old profiler had a lot of races. Todo: check new one.
  da_profiler::set_mode(0);
#endif

  visual_err_log_setup(dedicated::is_dedicated()); // need to call it before start_purge_logs_thread to avoid datarace
  bindquirrel::setup_logerr_interceptor();

  dng_load_localization();

  start_purge_logs_thread(); // as soon as settings loaded & cpujobs inited
#if DAGOR_DBGLEVEL > 0
  if (const char *mask = ::dgs_get_settings()->getBlockByNameEx("debug")->getStr("assertOnLogerr", nullptr))
    enable_assert_on_logerrs(mask);
#endif

  g_entity_mgr->broadcastEventImmediate(OnStoreApiInit{});

#if DAGOR_DBGLEVEL > 0
  do_fatal_on_logerr_on_exit =
    dgs_get_settings()->getBlockByNameEx("debug")->getBool("fatalOnLogerrOnExit", !dedicated::is_dedicated());
#endif

#if (_TARGET_PC || _TARGET_C3) && DAGOR_DBGLEVEL > 0
  if (!dgs_get_settings()->getBlockByNameEx("debug")->getBool("vromfsFirstPriority", true))
  {
    ::vromfs_first_priority = false;
    debug("disabled vromfs_first_priority via settings. read files from real filesystem first");
  }
#endif

#if _TARGET_PC && DAGOR_DBGLEVEL > 0
  if (!dedicated::is_dedicated() && dgs_get_settings()->getBlockByNameEx("debug")->getBool("startLocalProfileServer", false))
  {
    start_local_profile_server();
  }
#endif

  workcycleperf::cpu_only_cycle_record_enabled =
    dgs_get_settings()->getBlockByNameEx("debug")->getBool("enableCpuOnlyCycleRecord", false);

  // before load or video start to be able use exe for fast compilation checks
  {
    // uncomment this line as soon as we found all asserts
    // g_entity_mgr->enableQueriesValidation(::dgs_get_settings()->getBool("ecs_validate_queries", bool(DAGOR_DBGLEVEL>0)));
    // g_entity_mgr->setInvalidateAttributes(::dgs_get_settings()->getBool("ecs_invalidate_attributes", bool(DAGOR_DBGLEVEL>1)));
    g_entity_mgr->setMaxUpdateJobs(::dgs_get_settings()->getInt("ecs_max_threads", 8));
  }

  systeminfo::dump_dll_names();

  folders::load_custom_folders(*dgs_get_settings()->getBlockByNameEx("folders"));
  init_user_system_info_cache_dir();
  start_settings_async_saver_jobmgr();
  if (!dgs_get_argv("nopubcfg"))
    pubcfg::init(folders::get_path("pubcfg"), "");

  sqmemtrace::set_huge_alloc_hook(quirrel_huge_alloc_hook,
    dgs_get_settings()->getBlockByNameEx("sqmemtrace")->getInt("huge_alloc_threshold", 3 << 20));

  init_tex_streaming();

  fatal::set_language();
  const char *title = get_localized_text("windowTitle", ::dgs_get_settings()->getStr("windowTitle", get_game_name()));
  fatal::set_product_title(title);

  ::dagor_init_video("DagorWClass", nCmdShow,
#if _TARGET_PC_WIN
    LoadIcon((HINSTANCE)win32_get_instance(), "AppIcon"),
#else
    NULL,
#endif
    "Loading...", get_game_name(), get_exe_version32());
  ::startup_game(RESTART_ALL);
  fatal::set_d3d_driver_data(d3d::get_driver_name(), d3d_get_vendor_name(d3d_get_gpu_cfg().primaryVendor));
#if _TARGET_ANDROID
  android::startup::set_android_graphics_preset();
#endif
#if _TARGET_ANDROID || _TARGET_IOS
  crashlytics::init(LOGLEVEL_WARN);
#endif
  enable_tex_mgr_mt(true, ::dgs_get_settings()->getBlockByNameEx("video")->getInt("maxTexCount", 8192));
  dagor_show_splash_screen(); // For system splash of sony consoles until the shaders are initalized
  init_shaders();
  ::startup_game(RESTART_ALL);
  watchdog::init();

  const char *imguiFontName = ::dgs_get_settings()->getBlockByNameEx("imgui")->getStr("fontName", "ui/imgui/roboto-regular.ttf");
  if (imguiFontName && *imguiFontName)
  {
    float imguiFontSize = ::dgs_get_settings()->getBlockByNameEx("imgui")->getReal("fontSize", 16.0f);
    DataBlock overrideBlk;
    overrideBlk.setStr("imgui_font_name", imguiFontName);
    overrideBlk.setReal("imgui_font_size", imguiFontSize);
    imgui_set_override_blk(overrideBlk);
  }

  class InitialLoadingThread final : public DaThread
  {
    das::daScriptEnvironment *bound = nullptr;

  public:
    InitialLoadingThread() : DaThread("InitialLoadingThread", 512 << 10, 0, WORKER_THREADS_AFFINITY_MASK)
    {
      das::daScriptEnvironment::ensure();
      bound = das::daScriptEnvironment::bound;
    }

    void waitWithSplashAnimation()
    {
      bool skipSplashScreenAnimation = dgs_get_settings()->getBool("skipSplashScreenAnimation", false);
      dagor_hide_splash_screen();

      animated_splash_screen_start();
      for (;;)
      {
        TIME_PROFILER_TICK();
        lowlatency::start_frame();
        const uint32_t lastDraw = get_time_msec();

        watchdog_kick();
        ::dagor_idle_cycle();
        cpujobs::release_done_jobs();
        perform_delayed_actions();
        if (!skipSplashScreenAnimation)
        {
          d3d::GpuAutoLock gpuLock;
          d3d::set_render_target();

          animated_splash_screen_draw(d3d::get_backbuffer_tex());
          ShaderElement::invalidate_cached_state_block();
          d3d::update_screen();
        }
        if (interlocked_acquire_load(terminating))
          break;
        sleep_msec(32 - clamp(get_time_msec() - lastDraw, 0u, 32u));
      }
      animated_splash_screen_stop();
    }

    void execThread()
    {
      memreport::dump_memory_usage_report(0);

#if _TARGET_C3

#endif

      if (!(app_profile::get().disableRemoteNetServices || app_profile::get().devMode))
        circuit::init();

      if (!circuit::get_name().empty())
      {
        init_event_log(circuit::get_name().data());
        init_statsd_common(circuit::get_name().data());
      }
      if (!dedicated::is_dedicated())
      {
        send_first_run_event();
#if _TARGET_PC
        if (dgs_get_settings()->getBool("launchCountTelemetry", true))
        {
          bool lastLaunchFailed = !dgs_get_settings()->getBool("launchCorrectExit", true);
          get_settings_override_blk()->setBool("incorrectExitWarning", lastLaunchFailed);
          get_settings_override_blk()->setBool("launchCorrectExit", false);
          save_settings(nullptr, false);
        }
#endif
      }

      controls::init_drivers();
      ::startup_game(RESTART_ALL);
      if (!dedicated::is_dedicated())
        statsd::counter("render.driver_init", 1,
          {{"d3d_driver", d3d::get_driver_name()}, {"gpu_vendor", d3d_get_vendor_name(d3d_get_gpu_cfg().primaryVendor)}});

      if (
        ::dgs_get_settings()->getBool("harmonizationRequired", false) || ::dgs_get_settings()->getBool("harmonizationEnabled", false))
        dagor_sm_tex_load_controller = &texLoadController;

      systeminfo::dump_sysinfo();
      init_game_res();

      if (dgs_get_settings()->getBlockByNameEx("texStreaming")->getBool("disableTexScan", false))
        ::set_default_tex_factory(get_stub_tex_factory());

      int bufSize = dgs_get_settings()->getBlockByNameEx("StdGuiRender")->getInt("dynBufferSize", 16);
      StdGuiRender::init_dynamic_buffers(bufSize << 10);
      ::dagor_common_startup();

      darg::register_std_rendobj_factories();
      darg::register_blur_rendobj_factories(false);

      if (!dedicated::is_dedicated())
      {
        DataBlock picMgrCfg(framemem_ptr());
        picMgrCfg.setBool("createAsyncLoadJobMgr", true);
        picMgrCfg.setBool("dynAtlasLazyAllocDef", true);
        picMgrCfg.setBool("fatalOnPicLoadFailed", false);
        picMgrCfg.setBool("searchBlkBeforeTaBin", true); // *.ta.bin isn't used
        execute_delayed_action_on_main_thread(make_delayed_action([&]() { PictureManager::init(&picMgrCfg); }));
      }

      set_corrected_fps_limit();

      hdrrender::init_globals(*::dgs_get_settings()->getBlockByNameEx("video"));

      // Warning: this may reload settings blk (so all pointers to its sublocks become dangling!)
      g_entity_mgr->broadcastEventImmediate(VromfsUpdateOnStartEvent{});

      bool drawFps = ::dgs_get_settings()->getBool("draw_fps", false);
      if (drawFps)
      {
        ::dgs_draw_fps = true;
      }

      const DataBlock *debugBlock = ::dgs_get_settings()->getBlockByNameEx("debug");

#if _TARGET_PC_WIN && DAGOR_DBGLEVEL > 0 && defined(_M_IX86_FP) && _M_IX86_FP == 0
      constexpr bool defFloatExceptions = true;
#else
      constexpr bool defFloatExceptions = false;
#endif
      if (debugBlock->getBool("floatExceptions", defFloatExceptions))
      {
        enable_float_exceptions(true);
        debug("FP exceptions enabled");
      }

      ::dgs_trace_inpdev_line = debugBlock->getBool("traceInpDev", false);
      debug("dgs_trace_inpdev_line=%d", (int)dgs_trace_inpdev_line);

      HumanInput::stg_pnt.touchScreen = debugBlock->getBool("touchScreen", false);
      debug("HumanInput::stg_pnt.touchScreen=%d", HumanInput::stg_pnt.touchScreen);

      HumanInput::stg_pnt.emuTouchScreenWithMouse = debugBlock->getBool("touchScreenEmulate", false);
      DEBUG_CTX("HumanInput::stg_pnt.emuTouchScreenWithMouse=%d", HumanInput::stg_pnt.emuTouchScreenWithMouse);

      set_window_title(is_true_net_server() ? "Host" : nullptr);
      init_webui();

#if _TARGET_PC
      streaming::set_streaming_now(::dgs_get_settings()->getBlockByNameEx("streaming")->getBool("enabled", false));
#endif

      sqfrp::throw_frp_script_errors = false;

      init_device_reset();

      local_storage::init();
      app_start();

      interlocked_release_store(terminating, 1);
    }

    void execute() override
    {
      TIME_PROFILE_AUTO_THREAD();
      das::daScriptEnvironment::bound = bound;
      ScopeSetWatchdogCurrentThreadDump wctd;

      execThread();
    }
  };

  auto initialLoadingThread = eastl::make_unique<InitialLoadingThread>();
#if _TARGET_IOS
  auto &runLoop = runIOSRunloop;
#else
  auto runLoop = [](auto cb) {
    do
      cb();
    while (!quit_reason || is_level_loading()); // Quitting during level load is not supported
  };
#endif
  runLoop([&initialLoadingThread]() {
    if (DAGOR_UNLIKELY(initialLoadingThread))
    {
      if (!dedicated::is_dedicated())
      {
        G_VERIFY(initialLoadingThread->start());
        initialLoadingThread->waitWithSplashAnimation();
      }
      else // Dedicated is single threaded
        initialLoadingThread->execThread();
      initialLoadingThread.reset();
      add_delayed_callback(
        [](void *) {
          initial_loading_complete = true;
          ::dagor_reset_spent_work_time();
        },
        nullptr); // DA to execute this after actual scene load
    }

#if !_TARGET_IOS
    FrameSleeper _sleep;
#endif
    ::dagor_work_cycle();
    update_webui();
    cpujobs::release_done_jobs();
    reset_framemem();
    watchdog_kick();
  });
  /* Don't add shutdown code here - it will not be executed, use post_shutdown_handler() instead */
  quit_game(0, /*bRestart*/ false);
  G_ASSERT(0); // unreachable, actual shutdown performed in post_shutdown_handler()
  return 0;
}

REGISTER_IMGUI_FUNCTION_EX("App", "Exit", "Alt+F4", 999, [] { exit_game("exit_from_imgui_menu_bar"); });
