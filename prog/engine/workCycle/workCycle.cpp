// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <workCycle/dag_workCycle.h>
#include <workCycle/dag_genGuiMgr.h>
#include <workCycle/dag_gameSettings.h>
#include <workCycle/dag_delayedAction.h>
#include <workCycle/dag_gameScene.h>
#include <workCycle/dag_gameSceneRenderer.h>
#include <workCycle/dag_workCyclePerf.h>
#include <workCycle/dag_wcHooks.h>
#include <workCycle/wcPrivHooks.h>
#include <drv/hid/dag_hiGlobals.h>
#include <drv/hid/dag_hiJoystick.h>
#include <drv/hid/dag_hiPointing.h>
#if _TARGET_C1 | _TARGET_C2

#endif
#include <startup/dag_inpDevClsDrv.h>
#include <startup/dag_restart.h>
#include <startup/dag_globalSettings.h>
#include <perfMon/dag_cpuFreq.h>
#include <drv/3d/dag_info.h>
#include <drv/3d/dag_lock.h>
#include <drv/3d/dag_renderStates.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_resetDevice.h>
#include <drv/3d/dag_resourceChecker.h>
#include <drv/3d/dag_viewScissor.h>
#include <3d/dag_lowLatency.h>
#include <3d/dag_nvFeatures.h>
#include <3d/dag_render.h>
#include <3d/tql.h>
#include <osApiWrappers/dag_critSec.h>
#include <debug/dag_debug.h>
#include <perfMon/dag_statDrv.h>
#include <perfMon/dag_cachesim.h>
#include <shaders/dag_shaders.h>
#include <util/dag_convar.h>
#include "workCyclePriv.h"

using workcycle_internal::game_scene;
using workcycle_internal::game_scene_renderer;
using workcycle_internal::gametimeElapsed;
using workcycle_internal::last_draw_time;
using workcycle_internal::last_wc_time;
using workcycle_internal::secondary_game_scene;

bool dagor_game_act_time_limited = true;
bool suppress_d3d_update = false;

#if DAGOR_DBGLEVEL > 0
bool workcycleperf::debug_on = false;
int64_t workcycleperf::ref_frame_start = 0;
static bool perf_debug_req_on = false;

void workcycleperf::enable_debug(bool on) { perf_debug_req_on = on; }

#if _TARGET_XBOX || _TARGET_PC_WIN || _TARGET_C1
#include <perfMon/dag_pix.h>
#include <util/dag_console.h>
CONSOLE_INT_VAL("render", pix_capture_n_frames, 0, 0, 50);
#endif
#endif

static bool updatescr_done = true;
static real min_fps = 0, max_fps = 0, last_fps = 0;
static bool occluded_window = false;

unsigned int dagor_workcycle_depth = 0;

struct AutoDepthCounter
{
#if _TARGET_IOS
  AutoDepthCounter() { ::dagor_workcycle_depth++; }
  ~AutoDepthCounter() { ::dagor_workcycle_depth--; }
#endif
};

static constexpr int MAX_TIME_STEP = 250000;

static void act();
static void draw(bool enable_stereo, int elapsed_usec, float gametime_elapsed, bool call_before_render = true, bool draw_gui = true);
static bool present(bool updateScreenNeeded);

static bool is_minimized_fullscreen() { return !::dgs_app_active && dgs_get_window_mode() == WindowMode::FULLSCREEN_EXCLUSIVE; }

bool dagor_work_cycle_is_need_to_draw()
{
#if _TARGET_XBOX
  return true;
#else
  return !(is_minimized_fullscreen() || occluded_window);
#endif
}

void dagor_work_cycle()
{
  workcycleperf::mark_cpu_only_cycle_start();
  lowlatency::start_frame();
  [[maybe_unused]] AutoDepthCounter acntr;
  TIME_PROFILER_TICK();
  ScopedCacheSim cachesim;

  // perform delayed actions
  if (dwc_alloc_perform_delayed_actions)
    perform_delayed_actions();

  // check and perform restart
  if (int rflg = is_restart_required(RESTART_ALL | RESTART_GO); DAGOR_UNLIKELY(rflg & RESTART_GO))
  {
    rflg &= ~RESTART_GO;

    debug("\nRestarting...\n");
    volatile int64_t t_start = ref_time_ticks_qpc();
    shutdown_game(rflg);
    startup_game(rflg);
    debug("\n\nRestarted in %d ms\n", get_time_usec_qpc(t_start));

    ::dagor_reset_spent_work_time();
  }

  if (dgs_on_dagor_cycle_start)
  {
    TIME_PROFILE(dgs_on_dagor_cycle_start);
    dgs_on_dagor_cycle_start();
  }

  lowlatency::sleep();
  lowlatency::ScopedLatencyMarker actLatencyMarker(lowlatency::get_current_frame(), lowlatency::LatencyMarkerType::SIMULATION_START,
    lowlatency::LatencyMarkerType::SIMULATION_END);

  ::dagor_idle_cycle();

  if (DAGOR_UNLIKELY(is_minimized_fullscreen() && ::dgs_dont_use_cpu_in_background))
  {
    ResourceChecker::report();
    if (tql::on_frame_finished)
      tql::on_frame_finished();
    sleep_msec(200);
    ::dagor_reset_spent_work_time();
    return;
  }

  int time_step = dagor_game_time_step_usec;

  if (!workcycle_internal::minVariableRateActTimeUsec)
  {
    int elapsedUsec = get_time_usec_qpc(last_wc_time);
    while (elapsedUsec >= time_step)
    {
      // perform scene acting
      bool reset_time = false;
      if (elapsedUsec > MAX_TIME_STEP)
      {
        if (::dagor_game_act_rate > 4)
        {
          reset_time = true;
          elapsedUsec = MAX_TIME_STEP;
        }
        else if (elapsedUsec * ::dagor_game_act_rate > 2000000)
        {
          reset_time = true;
          elapsedUsec = 2000000 / ::dagor_game_act_rate;
        }
      }

      for (; elapsedUsec >= time_step; elapsedUsec -= time_step)
      {
        last_wc_time = rel_ref_time_ticks(last_wc_time, time_step);

        if (workcycle_internal::fixedActPerfFrame && workcycle_internal::curFrameActs >= workcycle_internal::fixedActPerfFrame)
          continue;

        gametimeElapsed += ::dagor_game_act_time * ::dagor_game_time_scale;

        int act_start_t = workcycleperf::get_frame_timepos_usec();

        act();

        if (workcycleperf::debug_on)
        {
          int fdt = workcycleperf::get_frame_timepos_usec();
          workcycleperf::log("== act:  endt=%6d us, dur=%6d us, elaps=%d", fdt, fdt - act_start_t, get_time_usec_qpc(last_wc_time));
        }

        workcycle_internal::curFrameActs++;
      }

      if (reset_time)
        ::dagor_reset_spent_work_time();

      if (present(false))
        break;
      elapsedUsec = get_time_usec_qpc(last_wc_time);
    }
  }
  else
  {
    const int64_t next_wc_time = ref_time_ticks_qpc();
    int lastFrameTime = ref_time_delta_to_usec(next_wc_time - last_wc_time);
    last_wc_time = next_wc_time;

    if (::dagor_game_act_time_limited && lastFrameTime > 100000)
      lastFrameTime = 100000;
    if (lastFrameTime < 100) // windows sleep mode can result in -1 (10000 FPS ought to be enough for anybody)
      lastFrameTime = 100;

    ::dagor_game_time_step_usec = lastFrameTime;
    ::dagor_game_act_time = lastFrameTime / 1000000.0f;
    G_ASSERTF(::dagor_game_act_time >= 0, "::dagor_game_act_time=%g", ::dagor_game_act_time);
    ::dagor_game_act_rate = int(1.0f / ::dagor_game_act_time);
    interlocked_release_store(workcycle_internal::lastFrameTime, 0);

    gametimeElapsed += ::dagor_game_act_time * ::dagor_game_time_scale;

    int act_start_t = workcycleperf::get_frame_timepos_usec();

    act();

    int fdt = workcycleperf::get_frame_timepos_usec();
    workcycleperf::log("== act:  endt=%6d us, dur=%6d us", fdt, fdt - act_start_t);

    time_step = workcycle_internal::minVariableRateActTimeUsec;
  }

  actLatencyMarker.close();

  int drawTimeElapsedUsec = max(get_time_usec_qpc(last_draw_time), 0);
  if (DAGOR_UNLIKELY(dgs_limit_fps && drawTimeElapsedUsec < time_step / 2))
  {
    sleep_msec((time_step / 2 - drawTimeElapsedUsec) / 1000);
    if (!workcycle_internal::minVariableRateActTimeUsec)
      return;
  }

  if (dwc_can_draw_next_frame)
  {
    int time_to_act = time_step - get_time_usec_qpc(last_wc_time);
    if (!dwc_can_draw_next_frame(dagor_frame_no(), time_to_act))
    {
      time_to_act = time_step - get_time_usec_qpc(last_wc_time);
      if (time_to_act < 2000)
        return;

      workcycleperf::log("== syncwait: %6d us", workcycleperf::get_frame_timepos_usec());

      sleep_msec(1);
      if (!dwc_can_draw_next_frame(dagor_frame_no(), time_step - get_time_usec_qpc(last_wc_time)))
        return;
    }
  }

  // no sense in drawing something that can never be drawn and seen (unless it's XB1 in which case this assumption is not true)
#if !_TARGET_XBOX
  if (DAGOR_UNLIKELY(!dagor_work_cycle_is_need_to_draw()))
  {
    d3d::GpuAutoLock gpuLock;
    d3d::driver_command(Drv3dCommand::PROCESS_APP_INACTIVE_UPDATE, &occluded_window);
    interlocked_release_store(workcycle_internal::lastFrameTime, 1); // tell act to proceed working
    ResourceChecker::report();
    if (tql::on_frame_finished)
      tql::on_frame_finished();
    return;
  }
#endif

  if (workcycle_internal::fixedActPerfFrame && workcycle_internal::curFrameActs < workcycle_internal::fixedActPerfFrame)
    return;

  workcycle_internal::curFrameActs = 0;

#if DAGOR_DBGLEVEL > 0
  if (workcycleperf::debug_on)
  {
    int fdt = workcycleperf::get_frame_timepos_usec();
    workcycleperf::log("frame end: %6d us, %.1ffps", fdt, 1000000.0 / fdt);
  }
  workcycleperf::debug_on = perf_debug_req_on;
  if (workcycleperf::debug_on)
  {
    workcycleperf::ref_frame_start = ref_time_ticks();
    workcycleperf::log("frame start #%u, at %d ms", dagor_frame_no(), get_time_msec());
  }
#endif

  // perform scene rendering
  {
    TIME_PROFILE(wait_for_async_present);
    d3d::wait_for_async_present();
  }
  lowlatency::start_render();
  drawTimeElapsedUsec = max(get_time_usec_qpc(last_draw_time), 0);
  last_draw_time = ref_time_ticks_qpc();
  inplace_max(gametimeElapsed, 0.0f);
  draw(true, drawTimeElapsedUsec, gametimeElapsed);
  gametimeElapsed = 0.0f;

  int fdt = workcycleperf::get_frame_timepos_usec();
  workcycleperf::log("== draw: endt=%6d us, dur=%6d us", fdt, fdt);

  workcycleperf::mark_cpu_only_cycle_end();

  if (game_scene && game_scene->canPresentAndReset())
  {
    updatescr_done = false;
    present(true);
  }

  occluded_window = (da_profiler::get_active_mode() == 0) && d3d::is_window_occluded();

  workcycleperf::log("== p(0): endt=%6d us", workcycleperf::get_frame_timepos_usec());
}

void dagor_work_cycle_flush_pending_frame() { present(true); }

void dagor_draw_scene_and_gui(bool call_before_render, bool draw_gui)
{
  int drawTimeElapsedUsec = max(get_time_usec_qpc(last_draw_time), 0);
  last_draw_time = ref_time_ticks_qpc();
  draw(false, drawTimeElapsedUsec, 0, call_before_render, draw_gui);
}

void dagor_enable_idle_priority(bool enable)
{
  workcycle_internal::set_priority(!enable ? true : ::dgs_app_active);
  interlocked_release_store(workcycle_internal::enable_idle_priority, enable);
}

void dagor_suppress_d3d_update(bool enable) { suppress_d3d_update = enable; }

//
// internal helpers
//
static void act()
{
  TIME_PROFILE(act);

  {
    TIME_PROFILE(update_input_devices);
    WinAutoLock lock(global_cls_drv_update_cs);
    if (::global_cls_drv_joy)
      ::global_cls_drv_joy->updateDevices();
#if _TARGET_C1 | _TARGET_C2


#endif
    if (::global_cls_drv_pnt)
      ::global_cls_drv_pnt->updateDevices();
  }

  if (game_scene)
  {
    TIME_PROFILE(actScene);
    game_scene->actScene();

    if (secondary_game_scene)
      secondary_game_scene->actScene();
  }
}

static void handle_programmatic_pix_capture()
{
#if DAGOR_DBGLEVEL > 0 && (_TARGET_XBOX || _TARGET_PC_WIN || _TARGET_C1)
  if (pix_capture_n_frames.get() == 0)
    return;

  static int framesToCapture = 0;
  static int framesToWaitBeforeCapture = 0;
  static uint32_t modeBackup;

  if (framesToCapture && framesToWaitBeforeCapture == 0)
  {
    framesToCapture--;
    if (framesToCapture == 0)
    {
      da_profiler::set_mode(modeBackup);
      pix_capture_n_frames.set(0);
    }
  }
  else if (framesToWaitBeforeCapture)
  {
    --framesToWaitBeforeCapture;
    if (framesToWaitBeforeCapture == 0)
    {
      DagorDateTime time;
      ::get_local_time(&time);

      static wchar_t capture_name[1024];

#if _TARGET_C1

#else
#define WCHAR_BUF(x) x
#endif

      swprintf(WCHAR_BUF(capture_name), L"gpu_capture_%04d.%02d.%02d_%02d.%02d.%02d", time.year, time.month, time.day, time.hour,
        time.minute, time.second);

      PIX_GPU_CAPTURE_NEXT_FRAMES(0, capture_name, framesToCapture);
      console::print_d("Capturing %d frames", framesToCapture);
      framesToCapture += 2; // Add a little buffer before turning off GPU mode
    }
  }
  else
  {
    framesToWaitBeforeCapture = 2;
    framesToCapture = pix_capture_n_frames.get();

    modeBackup = da_profiler::get_current_mode();
    da_profiler::add_mode(da_profiler::EVENTS | da_profiler::UNIQUE_EVENTS | da_profiler::GPU | da_profiler::TAGS);
  }

#endif
}

static void draw(bool enable_stereo, int elapsed_usec, float gametime_elapsed, bool call_before_render, bool draw_gui)
{
  G_ASSERTF_RETURN(d3d::is_inited(), , "Drawing with uninitialized d3d");

  handle_programmatic_pix_capture();

  int present_start_t = workcycleperf::get_frame_timepos_usec();
  eastl::optional<d3d::GpuAutoLock> gpuLock{eastl::in_place}; // acquire the GPU
  ShaderElement::invalidate_cached_state_block();

  auto gameScene = game_scene;
  auto secondaryGameScene = secondary_game_scene;

  if (call_before_render && gameScene)
  {
    TIME_D3D_PROFILE(beforeDraw);
    gameScene->beforeDrawScene(elapsed_usec, gametime_elapsed);

    if (secondaryGameScene)
      secondaryGameScene->beforeDrawScene(elapsed_usec, gametimeElapsed);
  }

  workcycleperf::log("==== br:   dur=%6d us", workcycleperf::get_frame_timepos_usec() - present_start_t);

  present_start_t = workcycleperf::get_frame_timepos_usec();
  if (::dagor_gui_manager && call_before_render && draw_gui)
  {
    TIME_D3D_PROFILE(beforeDrawGui);
    gpuLock.reset(); // release the GPU
    ::dagor_gui_manager->beforeRender(elapsed_usec);
    gpuLock.emplace(); // re-acquire the GPU
  }

  workcycleperf::log("==== brUI: dur=%6d us", workcycleperf::get_frame_timepos_usec() - present_start_t);

  present_start_t = workcycleperf::get_frame_timepos_usec();

  {
    TIME_D3D_PROFILE(drawScene);
    if (DAGOR_LIKELY(gameScene))
    {
      if (::grs_draw_wire)
        d3d::setwire(1);

      int g_start_t = workcycleperf::get_frame_timepos_usec();

      gameScene->enableStereo(enable_stereo);
      game_scene_renderer->render(*gameScene);
      gameScene->enableStereo(false);

      if (secondaryGameScene)
      {
        secondaryGameScene->enableStereo(enable_stereo);
        game_scene_renderer->render(*secondaryGameScene);
        secondaryGameScene->enableStereo(false);
      }

      workcycleperf::log("====== scene:  dur=%6d us", workcycleperf::get_frame_timepos_usec() - g_start_t);

      if (::grs_draw_wire)
        d3d::setwire(0);
    }
    else if (int w = 1, h = 1; d3d::get_target_size(w, h)) // clear scene
    {
      d3d::setview(0, 0, w, h, 0, 1);
      d3d::clearview(CLEAR_TARGET | CLEAR_ZBUFFER | CLEAR_STENCIL, 0, 1, 0);
    }

    if (::dagor_gui_manager && ::dgs_draw_fps)
      ::dagor_gui_manager->drawFps(min_fps, max_fps, last_fps);
  }

  gpuLock.reset(); // release the GPU

  workcycleperf::log("==== draw: dur=%6d us", workcycleperf::get_frame_timepos_usec() - present_start_t);
}

static bool present(bool updateScreenNeeded)
{
  if (!check_and_restore_3d_device())
    return false;

  if (updatescr_done)
    return true;

  if (suppress_d3d_update)
    return true;

  if (updateScreenNeeded)
  {
    TIME_D3D_PROFILE_NAME(present, ("blockedPresent"));
    d3d::GpuAutoLock gpuLock;
    updatescr_done = d3d::update_screen(dgs_app_active);
  }
  ResourceChecker::report();
  if (tql::on_frame_finished)
    tql::on_frame_finished();
  return updatescr_done;
}

void workcycle_internal::default_on_swap_callback()
{
  static volatile int64_t lastSwapTime = 0;
  static unsigned sumFrameTime = 0, minFrameTime = 100000000U, maxFrameTime = 0, fpsNum = 0;

  unsigned frameTime = get_time_usec(lastSwapTime);
  lastSwapTime = ref_time_ticks();

  nv::DLSS *dlss = nullptr;
  d3d::driver_command(Drv3dCommand::GET_DLSS, &dlss);
  dagor_frame_no_add(dlss ? dlss->getActualFramesPresented() : 1);
  if (interlocked_relaxed_load(workcycle_internal::lastFrameTime) >= 0 || frameTime < 1000000000U)
  {
    if (dwc_hook_after_frame)
      dwc_hook_after_frame();

    // debug("### frame %dms, #%d at %d", frameTime/1000, dagor_frame_no(), get_time_msec());
    sumFrameTime += frameTime;
    float frame_time_f = max((float)frameTime, 1e-6f);
    if (sumFrameTime > 1000000U)
    {
      last_fps = 1e6 * float(fpsNum) / float(sumFrameTime);

      fpsNum = 0;
      sumFrameTime = 0;
      maxFrameTime = minFrameTime = frameTime;
      max_fps = min_fps = 1e6 / frame_time_f;
    }
    else if (frameTime < minFrameTime)
    {
      minFrameTime = frameTime;
      max_fps = 1e6 / frame_time_f;
    }
    else if (frameTime > maxFrameTime)
    {
      minFrameTime = frameTime;
      min_fps = 1e6 / frame_time_f;
    }
    fpsNum++;
  }
  else
    frameTime = 0;
  interlocked_release_store(workcycle_internal::lastFrameTime, frameTime);

  if (dwc_hook_before_frame)
    dwc_hook_before_frame();
  if (dwc_hook_ts_before_frame)
    dwc_hook_ts_before_frame();
  if (dwc_hook_fps_log)
    dwc_hook_fps_log(frameTime);
  if (dwc_hook_memory_report)
    dwc_hook_memory_report();
}
