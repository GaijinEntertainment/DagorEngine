// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <workCycle/dag_startupModules.h>
#include <workCycle/dag_workCycle.h>
#include <workCycle/dag_gameSettings.h>
#include <workCycle/dag_gameScene.h>
#include <startup/dag_restart.h>
#include <startup/dag_globalSettings.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_info.h>
#include <drv/3d/dag_lock.h>
#include <3d/dag_render.h>
#include <drv/3d/dag_resetDevice.h>
#include <3d/dag_texMgr.h>
#include <osApiWrappers/dag_localConv.h>
#include <osApiWrappers/dag_progGlobals.h>
#include <osApiWrappers/setProgGlobals.h>
#include <ioSys/dag_dataBlock.h>
#include <debug/dag_fatal.h>
#include <debug/dag_debug.h>
#include <debug/dag_log.h>
#include <debug/dag_logSys.h>
#include <generic/dag_initOnDemand.h>
#include <util/dag_globDef.h>
#include <perfMon/dag_statDrv.h>
#include <drv/dag_vr.h>
#include "workCyclePriv.h"
#include <osApiWrappers/dag_messageBox.h>
#include <osApiWrappers/dag_miscApi.h>
#include <util/dag_localization.h>
#include <util/dag_watchdog.h>

#include <atomic>

#if _TARGET_PC_WIN
#include <workCycle/threadedWindow.h>
#include <startup/dag_winSplashScreen.inc.cpp>
#endif

using workcycle_internal::game_scene;

class MyD3dInitCB : public Driver3dInitCallback
{
public:
  void verifyResolutionSettings(int &ref_scr_wdt, int &ref_scr_hgt, int base_scr_wdt, int base_scr_hgt,
    bool window_mode) const override
  {
    if ((ref_scr_wdt > base_scr_wdt || ref_scr_hgt > base_scr_hgt) && window_mode && !dgs_execute_quiet)
    {
      if (allowResolutionOverlarge == -1)
      {
        ScopeSetWatchdogTimeout _wd(WATCHDOG_DISABLE);
        allowResolutionOverlarge = os_message_box(get_localized_text("video/resolution_overlarge"),
                                     get_localized_text("video/resolution_overlarge_hdr"), GUI_MB_YES_NO) == GUI_MB_BUTTON_1
                                     ? 0
                                     : 1;
      }
    }
    else
      allowResolutionOverlarge = -1;

    if (allowResolutionOverlarge == 1)
    {
      ref_scr_wdt = base_scr_wdt;
      ref_scr_hgt = base_scr_hgt;
    }
  }

  int validateDesc(Driver3dDesc &) const override { return 1; }

  int compareDesc(Driver3dDesc &, Driver3dDesc &) const override { return 0; }

  bool desiredStereoRender() const override
  {
    if (auto vrDevice = VRDevice::getInstance())
      return vrDevice->hasActiveSession();

    return false;
  }

  int64_t desiredAdapter() const override { return VRDevice::getInstance() ? VRDevice::getInstance()->getAdapterId() : 0; }

  RenderSize desiredRendererSize() const override
  {
    RenderSize s = {0, 0};
    if (auto vrDevice = VRDevice::getInstance())
      vrDevice->getViewResolution(s.width, s.height);
    return s;
  }

  const char *desiredRendererDeviceExtensions() const override
  {
    return VRDevice::getInstance() ? VRDevice::getInstance()->getRequiredDeviceExtensions() : nullptr;
  }

  const char *desiredRendererInstanceExtensions() const override
  {
    return VRDevice::getInstance() ? VRDevice::getInstance()->getRequiredInstanceExtensions() : nullptr;
  }

  VersionRange desiredRendererVersionRange() const override
  {
    VersionRange v = {0, 0};
    if (auto vrDevice = VRDevice::getInstance())
    {
      v.minVersion = vrDevice->getRequiredGraphicsAPIRange().first;
      v.maxVersion = vrDevice->getRequiredGraphicsAPIRange().second;
    }
    return v;
  }

  mutable int allowResolutionOverlarge = -1;
};

static MyD3dInitCB cb;

class VideoRestartProc : public SRestartProc
{
public:
  const char *wcName = nullptr;
  const char *title = nullptr;
  const char *gameName = nullptr;
  int ncmd = 0;
  void *hicon = nullptr;
  uint32_t gameVersion = 0;

  virtual const char *procname() { return "video"; }
  VideoRestartProc() : SRestartProc(RESTART_VIDEO) {}

  virtual void startup()
  {
    workcycle_internal::window_initing = true;
    const DataBlock *pblk_video = ::dgs_get_settings()->getBlockByNameEx("video");
    const DataBlock *pblk_gr = ::dgs_get_settings()->getBlockByNameEx("graphics");
    const DataBlock &blk_wc = *::dgs_get_settings()->getBlockByNameEx("workcycle");
    bool fatal_on_init_failed = pblk_video->getBool("fatalOnInitFailure", true);
#define RETURN_FATAL(...)      \
  do                           \
  {                            \
    if (fatal_on_init_failed)  \
      DAG_FATAL(__VA_ARGS__);  \
    else                       \
      LOGERR_CTX(__VA_ARGS__); \
    return;                    \
  } while (0)

    if (blk_wc.getInt("act_rate", 0))
    {
      int act_rate = blk_wc.getInt("act_rate", 0);
      dagor_set_game_act_rate(act_rate);
      if (act_rate > 0)
        debug("workcycle setup: %d acts/sec", act_rate);
      else
        debug("workcycle setup: variable act rate, target %d frames/sec", -act_rate);
    }
    workcycle_internal::fixedActPerfFrame = blk_wc.getInt("fixed_act_per_frame", 0);
    if (workcycle_internal::fixedActPerfFrame)
    {
      if (workcycle_internal::minVariableRateActTimeUsec)
      {
        logerr("workcycle setup: fixed_act_per_frame is incompatible with variable act rate");
        workcycle_internal::fixedActPerfFrame = 1;
      }
      debug("workcycle setup: fixed %d acts/frame", workcycle_internal::fixedActPerfFrame);
      workcycle_internal::curFrameActs = 0;
    }

    ::dgs_limit_fps = pblk_gr->getBool("limitfps", false);

    if (!d3d::init_driver())
      RETURN_FATAL("Error initializing 3D driver:\n%s", d3d::get_last_error());

    workcycle_internal::is_window_in_thread = pblk_video->getBool("threadedWindow", !d3d::is_stub_driver());

    d3d::driver_command(Drv3dCommand::SET_APP_INFO, (void *)gameName, (void *)&gameVersion);

#if !(_TARGET_IOS | _TARGET_ANDROID)
    d3d::update_window_mode();
#endif

    if (!pblk_video->getBool("compatibilityMode", false) && VRDevice::shouldBeEnabled())
    {
      VRDevice::ApplicationData vrAppData;
      vrAppData.name = gameName;
      vrAppData.version = gameVersion;
      VRDevice::create(VRDevice::RenderingAPI::Default, vrAppData);
    }

    main_wnd_f *wndProc = workcycle_internal::main_window_proc;
    void *hwnd = nullptr;
#if _TARGET_PC_WIN
    if (workcycle_internal::is_window_in_thread)
    {
      wndProc = nullptr;
      hwnd = windows::create_threaded_window(win32_get_instance(), wcName, ncmd, hicon, title, &cb);
    }
#endif

    ResourceChecker::init();
    if (!d3d::init_video(win32_get_instance(), wndProc, wcName, ncmd, hwnd, hwnd, hicon, title, &cb))
    {
      // currently unsupported for Metal path
      if (pblk_video->getBool("msgBoxAndQuitOnInitVideoFail", false))
      {
        logerr("Error initializing video (%s):\n%s", d3d::get_driver_name(), d3d::get_last_error());
        ::flush_debug_file(); // extra flush so that error msg appears in the log with higher probability
        ::watchdog_set_option(WATCHDOG_OPTION_TRIG_THRESHOLD, WATCHDOG_DISABLE);
        std::atomic<bool> msgBoxClosed{false};
        ::execute_in_new_thread(
          [&msgBoxClosed](auto) {
            eastl::string msgBoxHeader{::get_localized_text("video/init_failed_hdr", "Error")};
            eastl::string msgBoxText{::get_localized_text("video/init_failed",
              "Failed to initialize graphics subsystem. Please ensure that your device has all available updates installed.")};
            ::os_message_box(msgBoxText.c_str(), msgBoxHeader.c_str(), GUI_MB_OK);
            msgBoxClosed = true;
          },
          "initVideoFailedMsgBox");
        while (!msgBoxClosed) // -V776
        {
          ::dagor_idle_cycle(false);
          ::sleep_msec(100);
        }
        {
          ::flush_debug_file();
          // intentionally using quit functions which do not destroy objects before terminating app
          // otherwise there will be crashes due to wrong deinit order of various entities
#if _TARGET_PC_WIN || _TARGET_XBOX
          ::terminate_process(1);
#else
          ::abort();
#endif
        }
      }
      else
      {
        RETURN_FATAL("Error initializing video (%s):\n%s%s", d3d::get_driver_name(), d3d::get_last_error(),
          d3d::get_driver_code().is(d3d::vulkan) ? "\n\nPlease ensure that Vulkan drivers are installed and GPU supports it\n" : "");
      }
    }

    if (VRDevice *vrDevice = VRDevice::getInstance())
    {
      if (!vrDevice->setRenderingDevice())
        RETURN_FATAL("Error initializing VR session!");
    }

    win32_set_main_wnd(hwnd);

#if !_TARGET_XBOX // Use system splash screen
    // first scene render (usually for splash)
    if (game_scene)
      game_scene->beforeDrawScene(0, 0);
    check_and_restore_3d_device();
    if (game_scene)
      game_scene->drawScene();
    else
    {
      d3d::GpuAutoLock gpuLock;
#if DAGOR_DBGLEVEL > 0
      d3d::clearview(CLEAR_TARGET | CLEAR_ZBUFFER | CLEAR_STENCIL, E3DCOLOR(64, 64, 64, 0), 1, 0);
#else
      d3d::clearview(CLEAR_TARGET | CLEAR_ZBUFFER | CLEAR_STENCIL, E3DCOLOR(0, 0, 0, 0), 1, 0);
#endif
    }
#if _TARGET_PC_WIN
    win_hide_splash_screen();
#endif
#if _TARGET_C2

#endif
    {
      d3d::GpuAutoLock gpuLock;
      d3d::update_screen();
    }
    dagor_idle_cycle();
#endif

    pblk_gr = ::dgs_get_settings()->getBlockByNameEx("graphics");

    load_anisotropy_from_settings();

    const char *tex_q = pblk_gr->getStr("texquality", NULL);
    if (tex_q)
    {
#if DAGOR_DBGLEVEL < 1
      if (d3d::get_driver_desc().maxsimtex <= 2)
      {
        if (dd_stricmp(tex_q, "high") == 0)
          tex_q = "medium";
      }
#endif

      if (dd_stricmp(tex_q, "high") == 0)
        ::dgs_tex_quality = 0;
      else if (dd_stricmp(tex_q, "medium") == 0)
        ::dgs_tex_quality = 1;
      else if (dd_stricmp(tex_q, "low") == 0)
        ::dgs_tex_quality = 2;
      else if (dd_stricmp(tex_q, "ultralow") == 0)
        ::dgs_tex_quality = 3;
    }

    visibility_range_multiplier = pblk_gr->getReal("vis_range_mul", visibility_range_multiplier);

    workcycle_internal::window_initing = false;
#undef RETURN_FATAL
  }

  virtual void shutdown()
  {
    TIME_PROFILER_SHUTDOWN();
    d3d::release_driver();
#if _TARGET_PC_WIN
    windows::shutdown_threaded_window();
#endif
  }

  void init(const char *wc, int cmd, void *_hicon, const char *titl, const char *game_name, uint32_t game_version)
  {
    wcName = wc;
    ncmd = cmd;
    hicon = _hicon;
    title = titl;
    gameName = game_name;
    gameVersion = game_version;
  }
};
static InitOnDemand<VideoRestartProc> video_rproc;

void dagor_init_video(const char *wc_name, int ncmd, void *hicon, const char *title, const char *game_name, int game_version)
{
  video_rproc.demandInit();
  video_rproc->init(wc_name, ncmd, hicon, title, game_name, game_version);
  dgs_on_swap_callback = &workcycle_internal::default_on_swap_callback;

  add_restart_proc(video_rproc);
}
