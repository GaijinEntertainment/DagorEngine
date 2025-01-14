// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "renderer.h"
#include <drv/3d/dag_renderStates.h>
#include <drv/3d/dag_viewScissor.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_matricesAndPerspective.h>
#include <drv/3d/dag_texture.h>
#include <drv/3d/dag_driver.h>
#include <3d/dag_render.h>
#include <3d/dag_picMgr.h>
#include <3d/dag_texPackMgr2.h>
#include <render/dag_cur_view.h>
#include <gui/dag_stdGuiRender.h>
#include <perfMon/dag_cpuFreq.h>
#include <perfMon/dag_statDrv.h>
#include <ecs/core/attributeEx.h>
#include <ecs/scripts/scripts.h>
#include <ecs/delayedAct/actInThread.h>
#include <ecs/render/updateStageRender.h>
#include <ecs/camera/getActiveCameraSetup.h>
#include <gui/dag_visualLog.h>
#include <startup/dag_globalSettings.h>
#include <util/dag_convar.h>
#include <gui/dag_imgui.h>
#include <daEditorE/editorCommon/inGameEditor.h>
#include <debug/dag_debug3d.h>
#include <debug/dag_textMarks.h>
#include <render/debug3dSolidBuffered.h>
#include <osApiWrappers/dag_miscApi.h>
#include <workCycle/dag_workCycle.h>
#include <gamePhys/collision/collisionLib.h>
#include <webui/shaderEditors.h>
#include <dasModules/aotEcs.h> // bind_dascript::enable_thread_safe_das_ctx_region
#include <shaders/dag_dynSceneRes.h>
#include <drv/dag_vr.h>
#include <image/dag_texPixel.h>

#include "screencap.h"
#include "video360/video360.h"
#include "capture360.h"
#include "captureGbuffer.h"

#include "main/app.h"
#include "main/level.h"
#include "sound/dngSound.h"
#include "net/netStat.h"
#include "net/time.h"
#include "render/fx/fx.h"
#include "render/renderEvent.h"
#include "render/hdrRender.h"
#include "main/console.h"
#include "main/gameLoad.h"
#include "render/skies.h"
#include "render/world/cameraParams.h"

#include <drv/3d/dag_commands.h>
#include <drv/3d/dag_resetDevice.h>
#include <render/tdrGpu.h>
#include "ui/userUi.h"
#include "ui/uiShared.h"
#include "ui/uiRender.h"
#include "ui/loadingUi.h"

#include <util/dag_threadPool.h>
#include <daRg/dag_guiScene.h>

#include "animatedSplashScreen.h"
#include <debug/dag_memReport.h>
#include <render/deviceResetTelemetry/deviceResetTelemetry.h>
#include <render/world/wrDispatcher.h>

CONSOLE_INT_VAL("app", sleep_msec_val, 0, 0, 1000);
CONSOLE_BOOL_VAL("app", screenshot_hide_debug, true);
CONSOLE_BOOL_VAL("app", hide_gui, false);
CONSOLE_BOOL_VAL("app", enable_low_resolution_screensots, true);

CONSOLE_BOOL_VAL("mem", show_memreport, false);
CONSOLE_FLOAT_VAL("mem", memreport_panel_x, 0.f);
CONSOLE_FLOAT_VAL("mem", memreport_panel_y, 90.f);
extern ConVarT<bool, false> memreport_sys;
extern ConVarT<bool, false> memreport_gpu;

extern void set_screen_shot_comments(const TMatrix &itm);
bool should_hide_debug() { return screenshot_hide_debug.get() && screencap::should_hide_debug(); }
void toggle_hide_gui() { hide_gui.set(!hide_gui.get()); }
static bool should_hide_gui() { return screencap::should_hide_gui() || hide_gui.get() || WRDispatcher::shouldHideGui(); }

namespace game_scene
{
extern bool parallel_logic_mode;
extern void updateInput(float rtDt, float dt, float cur_time);
} // namespace game_scene

static int gui_screen_sizeVarId = -1;

static int last_realtime_elapsed_usec = 0;
float last_gametime_elapsed_sec = 0.f;

extern void free_reserved_tp_worker();

static class AdditionalGameJob final : public cpujobs::IJob
{
  float dt = 0.;
  float rtdt = 0.;
  float curTime = 0.;

public:
  AdditionalGameJob *prepare(float _rtdt, float time_speed, float _curTime)
  {
    rtdt = _rtdt;
    dt = _rtdt * time_speed;
    curTime = _curTime;
    bind_dascript::enable_thread_safe_das_ctx_region(true);
    return this;
  }
  void doJob() override
  {
    static bool gpuLatencyWait = dgs_get_settings()->getBlockByName("video")->getBool("pufdGpuLatencyWait", true);
    if (gpuLatencyWait)
    {
      TIME_PROFILE(gpu_latency_wait);
      d3d::gpu_latency_wait();
    }
    TIME_PROFILE(ParallelUpdateFrameDelayed);
    FRAMEMEM_REGION;

    if (game_scene::parallel_logic_mode) // we already update input in games without parallel logic mode
    {
      dainput::set_control_thread_id(get_current_thread_id());
      game_scene::updateInput(rtdt, dt, curTime); // update input
      dainput::set_control_thread_id(get_main_thread_id());
    }

    uirender::start_ui_before_render_job();
    dacoll::phys_world_set_control_thread_id(get_current_thread_id());
    g_entity_mgr->broadcastEventImmediate(ParallelUpdateFrameDelayed(dt, curTime));
    dacoll::phys_world_set_control_thread_id(get_main_thread_id());

    dacoll::phys_world_set_invalid_fetch_sim_res_thread(-1); // set to invalid

    free_reserved_tp_worker(); // Allow Jolt to use all threadpool workers
  }
} additional_game_job;

static inline void wait_additional_game_job_done()
{
  if (!interlocked_acquire_load(additional_game_job.done))
  {
    TIME_PROFILE(wait_additional_game_job_done);
    threadpool::wait(&additional_game_job);
  }
  bind_dascript::enable_thread_safe_das_ctx_region(false);
}

void render_scene_debug(BaseTexture *target, BaseTexture *depth, const CameraParams &camera)
{
  wait_additional_game_job_done(); // some systems on UpdateStageInfoRenderDebug can conflict with this game job
  if (should_hide_debug() || !get_world_renderer())
    return;

  d3d::settm(TM_VIEW, camera.viewTm);
  if (target)
    d3d::set_render_target(target, 0);

  // some debug features can rely on depth test, but on some consoles we can't use that because of different resolutions with RTs
  static constexpr bool canDrawWithoutDepth = true; // TODO: fix it correctly

  if (depth)
    d3d::set_depth(depth, DepthAccess::RW);
  if (depth || canDrawWithoutDepth)
  {
    d3d::setpersp(camera.jitterPersp);
    TIME_D3D_PROFILE(debug_visualization_jittered);
    g_entity_mgr->broadcastEventImmediate(RenderDebugWithJitter());
    flush_buffered_debug_lines(get_timespeed() == 0.f);
    flush_buffered_debug_meshes(get_timespeed() == 0.f);
  }

  {
    d3d::setpersp(camera.noJitterPersp);

    TIME_D3D_PROFILE(debug_visualization_nojitter);
    if (depth)
      d3d::set_depth(depth, DepthAccess::RW);
    if (depth || canDrawWithoutDepth)
    {
      g_entity_mgr->update(UpdateStageInfoRenderDebug(reinterpret_cast<mat44f_cref>(camera.noJitterGlobtm), camera.viewItm));
      flush_buffered_debug_lines(get_timespeed() == 0.f);
    }

    d3d::set_depth(nullptr, DepthAccess::RW);
    acesfx::draw_debug_opt(camera.noJitterGlobtm);
  }
}

void before_draw_scene(
  int realtime_elapsed_usec, float gametime_elapsed_sec, float time_speed, ecs::EntityId cur_cam, TMatrix &view_itm)
{
  last_realtime_elapsed_usec = realtime_elapsed_usec;
  last_gametime_elapsed_sec = gametime_elapsed_sec;

  // this has to happen before setting constrained mt mode, because it calls tick between ECS entity deletion and creation
  update_delayed_weather_selection();

  if (imgui_get_state() != ImGuiState::OFF)
    imgui_perform_registered();

  G_ASSERT(!g_entity_mgr->isConstrainedMTMode());
  g_entity_mgr->setConstrainedMTMode(true);

  before_render_daskies();
  PictureManager::per_frame_update();

  float rtDt = (screencap::fixed_act_rate() < 0.f ? ::dagor_game_act_time : screencap::fixed_act_rate());
  float scaledDt = rtDt * time_speed;

  if (!sceneload::is_scene_switch_in_progress())
  {
    if (bool(cur_cam))
      if (const TMatrix *tm = get_da_editor4().getCameraForcedItm())
        g_entity_mgr->set(cur_cam, ECS_HASH("transform"), *tm);
  }

  const CameraSetup cam = !is_level_loading() ? get_active_camera_setup() : CameraSetup{};

  int w, h;
  d3d::get_screen_size(w, h);
  TMatrix camTransform = cam.transform;
  DPoint3 camPosition = cam.accuratePos;
  Driver3dPerspective curPersp = calc_camera_perspective(cam, w, h);

  view_itm = camTransform;

  if (capture360::is_360_capturing_in_progress())
  {
    if (eastl::optional<CameraSetupPerspPair> camera360 = screencap::get_camera())
    {
      camTransform = camera360->camera.transform;
      camPosition = camera360->camera.accuratePos;
      curPersp = camera360->persp;
    }
  }

  TMatrix viewTransform = calc_camera_view_tm(camTransform);
  apply_camera_setup(camTransform, viewTransform, curPersp, w, h);
  ::grs_cur_view.itm = camTransform;
  ::grs_cur_view.tm = viewTransform;
  ::grs_cur_view.pos = ::grs_cur_view.itm.getcol(3);

  if (auto wr = get_world_renderer(); wr && is_level_loaded_not_empty())
  {
    wr->beforeRender(scaledDt, rtDt, realtime_elapsed_usec * 1e-6, get_sync_time(), camTransform, camPosition, curPersp);
  }

  user_ui::before_render();
  uirender::before_render(realtime_elapsed_usec * 1e-6, camTransform, viewTransform);

  if (has_in_game_editor())
  {
    TMatrix4 projTm;
    d3d::gettm(TM_PROJ, &projTm);
    get_da_editor4().beforeRender(::grs_cur_view.tm, ::grs_cur_view.itm, projTm, ::grs_cur_view.pos);
  }
}

static void final_blit()
{
  // this stretch rect is be optimized away, if D3D API will return BaseTexture for backbuffer
  if (get_world_renderer()->getFinalTargetTex().getTex2D() != d3d::get_backbuffer_tex())
  {
    TIME_D3D_PROFILE(final_blit);
    d3d::set_srgb_backbuffer_write(false);
    d3d::stretch_rect(get_world_renderer()->getFinalTargetTex().getTex2D(), NULL);
  }
}

enum
{
  AGT_NONE = 0,
  AGT_ADDITIONAL = 1,
  AGT_UI = 2,
  AGT_DAFX = 4,
  AGT_ALL = 7
};
static uint8_t async_game_tasks_started = AGT_NONE;
extern void acefx_start_update_prepared();

void start_async_game_tasks(int agt = AGT_ALL, bool wake = true)
{
  if ((async_game_tasks_started & agt) == agt)
    return;

  G_ASSERT(g_entity_mgr->isConstrainedMTMode());

  // use PRIO_LOW for relatively long jobs to avoid accidently executing them on active waits in main thread
  if ((agt & AGT_ADDITIONAL) && !(async_game_tasks_started & AGT_ADDITIONAL))
  {
    if (is_level_loaded())
      threadpool::add(additional_game_job.prepare(last_gametime_elapsed_sec, get_timespeed(), get_sync_time()), threadpool::PRIO_LOW,
        wake);
    else // we must start before render ui job when level loading in progress
      uirender::start_ui_before_render_job();
  }

  if ((agt & AGT_DAFX) && !(async_game_tasks_started & AGT_DAFX))
    acefx_start_update_prepared();

  if ((agt & AGT_UI) && !(async_game_tasks_started & AGT_UI))
  {
    if (!should_hide_gui() && uirender::multithreaded)
      uirender::start_ui_render_job(wake);
    else
      uirender::skip_ui_render_job();
  }

  async_game_tasks_started |= agt;
}

void finish_rendering_ui()
{
  wait_additional_game_job_done();

  CameraSetup cameraSetup = get_active_camera_setup();
  TMatrix viewTm;
  TMatrix4 globTm;
  TMatrix4 projTm;
  Driver3dPerspective persp;
  int view_w, view_h;
  calc_camera_values(cameraSetup, viewTm, persp, view_w, view_h);
  d3d::calcproj(persp, projTm);
  globTm = TMatrix4(viewTm) * projTm;
  prepare_debug_text_marks(globTm, view_w, view_h);
  bool isPersp = d3d::validatepersp(persp);

  auto uiScenes = uirender::get_all_scenes();

  bool willResetGuiBufferPosInThread = !should_hide_gui() && uirender::multithreaded && !uiScenes.empty();
  if (!willResetGuiBufferPosInThread)
    StdGuiRender::reset_per_frame_dynamic_buffer_pos();

  if (!should_hide_gui())
  {
    TIME_D3D_PROFILE(gui);

    SCOPE_RENDER_TARGET;

    d3d::set_depth(nullptr, DepthAccess::RW);

    {
      int sw, sh;
      d3d::get_screen_size(sw, sh);
      if (gui_screen_sizeVarId == -1)
        gui_screen_sizeVarId = get_shader_variable_id("gui_screen_size", true);
      ShaderGlobal::set_color4(gui_screen_sizeVarId, sw, sh, 1.0f / sw, 1.0f / sh);

      if (!uiScenes.empty())
      {
        if (DAGOR_LIKELY(uirender::multithreaded))
        {
          uirender::wait_ui_render_job_done();
        }
        else
        {
          TIME_PROFILE(darg_scene_build_render);
          for (darg::IGuiScene *scn : uirender::get_all_scenes())
          {
            scn->renderThreadBeforeRender();
            scn->buildRender();
          }
        }
      }

      // draw debug text marks before the UI
      // a lot of marks overlapping Editor UI and mouse cursor is quite annoying
      // and seem to have no benefits
      if (!should_hide_debug())
        render_debug_text_marks();

      if (isPersp)
      {
        TIME_D3D_PROFILE(render_ui);
        StdGuiRender::ScopeStarterOptional strt;
        StdGuiRender::set_font(0);
        StdGuiRender::set_color(255, 0, 255, 255);
        StdGuiRender::set_ablend(true);
        StdGuiRender::flush_data();

        mat44f ssGlobTm;
        v_mat44_make_from_44cu(ssGlobTm, &globTm._11);
        g_entity_mgr->broadcastEventImmediate(RenderEventUI(viewTm, cameraSetup.transform, ssGlobTm, persp));
      }

      if (!uiScenes.empty())
      {
        TIME_D3D_PROFILE(flush_render);
        int w, h;
        d3d::get_target_size(w, h);
        d3d::setview(0, 0, w, h, 0, 1);
        for (darg::IGuiScene *scn : uiScenes)
          scn->flushRender();
      }

      if (isPersp)
      {
        TIME_D3D_PROFILE_DEV(render_after_ui);
        StdGuiRender::ScopeStarterOptional strt;
        StdGuiRender::set_font(0);
        StdGuiRender::set_color(255, 0, 255, 255);
        StdGuiRender::set_ablend(true);
        StdGuiRender::flush_data();

        mat44f ssGlobTm;
        v_mat44_make_from_44cu(ssGlobTm, &globTm._11);
        g_entity_mgr->broadcastEventImmediate(RenderEventAfterUI(viewTm, cameraSetup.transform, ssGlobTm, persp));
      }

      netstat::render();

      if (isPersp)
        get_da_editor4().renderUi();
    }
  }

  if (!screencap::should_hide_debug())
  {
    {
      TIME_D3D_PROFILE(debug_world);
      if (get_world_renderer())
        get_world_renderer()->debugDraw();
    }
    {
      TIME_D3D_PROFILE(dngsound);
      dngsound::debug_draw();
    }
    g_entity_mgr->broadcastEventImmediate(RenderEventDebugGUI());
    visuallog::draw();
    TIME_D3D_PROFILE(console);
    console::update();
    console::render();
  }
  if (imgui_get_state() != ImGuiState::OFF)
  {
    TIME_D3D_PROFILE(imgui);
    if (screencap::should_hide_debug())
      imgui_endframe();
    else
      imgui_render();
  }

  if (::grs_draw_wire)
    d3d::setwire(::grs_draw_wire);

  if (show_memreport.get())
  {
    StdGuiRender::ScopeStarter strt;
    StdGuiRender::set_font(0);
    memreport::on_screen_memory_usage_report(memreport_panel_x.get(), memreport_panel_y.get(), memreport_sys.get(),
      memreport_gpu.get());
  }
}

void draw_scene(const TMatrix &view_itm)
{
  if (DAGOR_UNLIKELY(sceneload::is_scene_switch_in_progress()))
  {
    uirender::skip_ui_render_job();
    g_entity_mgr->setConstrainedMTMode(false);
    return;
  }

#if _TARGET_C1 | _TARGET_C2

#endif

  screencap::start_pending_request();

  CameraSetup cameraSetup = get_active_camera_setup();
  TMatrix viewTm;
  TMatrix4 globTm;
  TMatrix4 projTm;
  Driver3dPerspective persp;
  int view_w, view_h;
  calc_camera_values(cameraSetup, viewTm, persp, view_w, view_h);
  d3d::calcproj(persp, projTm);
  globTm = TMatrix4(viewTm) * projTm;
  prepare_debug_text_marks(globTm, view_w, view_h);
  bool isPersp = d3d::validatepersp(persp);

  if (isPersp)
    uishared::save_world_3d_view(persp, viewTm, cameraSetup.transform);
  else
    uishared::invalidate_world_3d_view();

  if (sleep_msec_val.get())
    sleep_msec(sleep_msec_val.get());

  if (auto wr = get_world_renderer())
  {
    wr->updateFinalTargetFrame();
    d3d::set_render_target(wr->getFinalTargetTex().getTex2D(), 0);
  }
  else
    hdrrender::set_render_target();

  async_game_tasks_started = AGT_NONE;
  if (get_world_renderer())
  {
    if (is_level_loaded())
    {
      if (is_animated_splash_screen_started())
        animated_splash_screen_stop();
      else if (ddsx::get_streaming_mode() != ddsx::BackgroundSerial)
        ddsx::set_streaming_mode(ddsx::BackgroundSerial);

      if (is_level_loaded_not_empty())
      {
        TIME_D3D_PROFILE(draw_frame);
        get_world_renderer()->draw(last_realtime_elapsed_usec * 1e-6f);
      }

      debug_animated_splash_screen();
    }
    else
    {
      if (ddsx::get_streaming_mode() != ddsx::MultiDecoders)
        ddsx::set_streaming_mode(ddsx::MultiDecoders);
      if (loading_ui::is_fully_covering())
        d3d::clearview(CLEAR_TARGET, 0, 0, 0);
      else
      {
        if (is_animated_splash_screen_encoding() && hdrrender::is_hdr_enabled())
        {
          animated_splash_screen_stop();
          animated_splash_screen_start(false);
        }
        else if (!is_animated_splash_screen_started())
        {
          animated_splash_screen_start(!hdrrender::is_hdr_enabled());
        }

        animated_splash_screen_draw(hdrrender::is_hdr_enabled() ? hdrrender::get_render_target_tex() : d3d::get_backbuffer_tex());
      }
    }
  }

  // Preset is soon, this makes driver thread sleep less
  d3d::driver_command(Drv3dCommand::ENABLE_WORKER_LOW_LATENCY_MODE, (void *)(uintptr_t)1); //-V566

  start_async_game_tasks(); // Start the rest of tasks in case it wasnt started before

  // On Xbox this sets the SDR render target for GUI render
  d3d::set_render_target(1, d3d::get_secondary_backbuffer_tex(), 0);

  if (::grs_draw_wire)
    d3d::setwire(0);

  d3d::setpersp(persp);

  get_da_editor4().render3d(Frustum(globTm), Point3(cameraSetup.accuratePos));

  if (!is_level_loaded_not_empty() || (get_world_renderer() && !get_world_renderer()->needSeparatedUI()))
    finish_rendering_ui();
  else
    wait_additional_game_job_done();

  if (screencap::is_screenshot_scheduled())
    set_screen_shot_comments(view_itm);
  if (get_world_renderer())
  {
    if (capture360::is_360_capturing_in_progress())
      capture360::update_capture(get_world_renderer()->getFinalTargetTex());
    else if (screencap::is_screenshot_scheduled())
    {
      auto colorspace = hdrrender::is_hdr_enabled() ? screencap::ColorSpace::Linear : screencap::ColorSpace::sRGB;
      if (const auto &superResolutionScreenshot = get_world_renderer()->getSuperResolutionScreenshot())
      {
        String postfix;
        TextureInfo ti;
        DagorDateTime time;
        ::get_local_time(&time);
        superResolutionScreenshot->getinfo(ti, 0);
        postfix.printf(256, "_super_resolution_%dx%d", ti.w, ti.h);
        screencap::make_screenshot(superResolutionScreenshot, nullptr, false, colorspace, postfix.c_str());
      }
      if (enable_low_resolution_screensots.get())
        screencap::make_screenshot(get_world_renderer()->getFinalTargetTex(), nullptr, false, colorspace, nullptr);
      screencap::screenshots_saved();
    }
    d3d::set_render_target();
    if (!hdrrender::encode())
      final_blit();
  }
  else
  {
    if (screencap::is_screenshot_scheduled())
    {
      if (const ManagedTex &hdrTex = hdrrender::get_render_target())
        screencap::make_screenshot(hdrTex, nullptr, false, screencap::ColorSpace::Linear);
      else
        screencap::make_screenshot(dag::get_backbuffer());
      screencap::screenshots_saved();
    }
    d3d::set_render_target();
    hdrrender::encode();
  }

  g_entity_mgr->setConstrainedMTMode(false);
  G_ASSERT(!g_entity_mgr->isConstrainedMTMode());

#if _TARGET_C1 | _TARGET_C2

#endif
}

#if _TARGET_PC_WIN

#include <util/dag_watchdog.h>
static bool watchdog_paused = false;
static int old_tt;

static struct DeviceLostCallback : public IDrv3DDeviceLostCB
{
  virtual void onDeviceLost()
  {
    G_ASSERT(is_main_thread());

    if (!watchdog_paused)
    {
      DEBUG_CTX("onDeviceLost");
      old_tt = watchdog_set_option(WATCHDOG_OPTION_TRIG_THRESHOLD, WATCHDOG_DISABLE);
      watchdog_paused = true;
      debug_flush(false);
    }
  }
} drv_3d_lost_device_cb;


static struct Reset3DCallback : public IDrv3DResetCB
{
  int resetCount = 0;
  int resetSeriesSize;
  int lastResetTime;
  enum
  {
    MAX_RESET_COUNT = 30,
    NO_RESET_TIME_MS = 10000
  };
  Reset3DCallback() : resetSeriesSize(0), lastResetTime(0) {}
  virtual void beforeReset(bool full_reset)
  {
    debug("Reset3DCallback::beforeReset(%s), resetCount=%d, time since last %d ms", full_reset ? "full" : "partial", resetSeriesSize,
      get_time_msec() - lastResetTime);
    debug_flush(false);

    resetCount++;
    if (get_time_msec() - lastResetTime >= NO_RESET_TIME_MS)
      resetSeriesSize = 0;
    if (++resetSeriesSize >= MAX_RESET_COUNT)
    {
      DAG_FATAL("Video driver stopped responding. Try decrease graphics settings");
      return;
    }
    lastResetTime = get_time_msec();

    report_device_reset();

    dump_gpu_temperature();
    dump_proc_memory();
    dump_gpu_memory();

    uishared::before_device_reset();
    DEBUG_CP();

    if (!watchdog_paused)
    {
      old_tt = watchdog_set_option(WATCHDOG_OPTION_TRIG_THRESHOLD, WATCHDOG_DISABLE);
      watchdog_paused = true;
    }

    if (get_world_renderer())
      get_world_renderer()->beforeDeviceReset(full_reset);

    debug_flush(false);
  }


  void windowResized() override
  {
    uishared::window_resized();
    if (get_world_renderer())
      get_world_renderer()->windowResized();
  }

  virtual void afterReset(bool full_reset)
  {
    debug("Reset3DCallback::afterReset(%s)", full_reset ? "full" : "partial");
    debug_flush(false);

    hdrrender::update_globals();

    StdGuiRender::after_device_reset();

    if (get_world_renderer())
      get_world_renderer()->afterDeviceReset(full_reset);

    uishared::after_device_reset();

    if (watchdog_paused)
    {
      watchdog_set_option(WATCHDOG_OPTION_TRIG_THRESHOLD, old_tt);
      watchdog_paused = false;
    }

    debug_flush(false);
  }
} drv_3d_reset_cb;


void init_device_reset()
{
  set_3d_device_reset_callback(&drv_3d_reset_cb);
  set_3d_device_lost_callback(&drv_3d_lost_device_cb);
}

#else

void init_device_reset() {}

#endif // _TARGET_PC_WIN
