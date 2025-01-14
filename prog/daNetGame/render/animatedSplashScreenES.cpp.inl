// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "animatedSplashScreen.h"

#include <osApiWrappers/dag_threads.h>
#include <osApiWrappers/dag_miscApi.h>
#include <ioSys/dag_dataBlock.h>
#include <startup/dag_globalSettings.h>
#include <perfMon/dag_cpuFreq.h>
#include <math/random/dag_random.h>
#include <util/dag_console.h>
#include <util/dag_convar.h>
#include <drv/3d/dag_renderStates.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_draw.h>
#include <drv/3d/dag_vertexIndexBuffer.h>
#include <drv/3d/dag_shaderConstants.h>
#include <drv/3d/dag_texture.h>
#include <drv/3d/dag_lock.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_info.h>
#include <drv/3d/dag_commands.h>
#include <3d/dag_textureIDHolder.h>
#include <3d/dag_texPackMgr2.h>
#include <shaders/dag_shaders.h>
#include <shaders/dag_postFxRenderer.h>
#include <shaders/dag_shaderBlock.h>
#include <shaders/dag_overrideStates.h>
#include <EASTL/unique_ptr.h>
#include <util/dag_watchdog.h>
#include <osApiWrappers/dag_atomic.h>
#include <daECS/core/entitySystem.h>
#include <daECS/net/netEvents.h>
#include <render/noiseTex.h>
#include <render/hdrRender.h>
#include <3d/dag_createTex.h>
#include <osApiWrappers/dag_direct.h>
#include <drv/3d/dag_variableRateShading.h>

#include "renderer.h"

CONSOLE_INT_VAL("app", splashId, -1, -1, 1000);
CONSOLE_BOOL_VAL("render", forceLoadingHalfRes, true);
CONSOLE_BOOL_VAL("app", useTitleLogoForSplash, false);

extern bool grs_draw_wire;

static uint32_t splash_start_time = 0;
static int8_t splash_id = -1;
static bool splash_started = false;
static PostFxRenderer loadingSplash;
static Texture *halfTargetTex = nullptr;
static BaseTexture *titleLogo = nullptr;
static int last_splash_draw_msec = 0;
static int paper_white_nits = 200;
static bool is_encoding = true;

static void (*splash_render_func)(int w, int h, void *arg) = nullptr;
static void *splash_render_func_arg = nullptr;
static bool splash_render_func_exclusive = false;

static volatile int allow_watchdog_kick = 0;
void animated_splash_screen_allow_watchdog_kick(bool allow) { interlocked_release_store(allow_watchdog_kick, allow ? 1 : 0); }

void animated_splash_screen_register_render(void (*render_func)(int w, int h, void *arg), void *arg, bool exclusive)
{
  splash_render_func = render_func;
  splash_render_func_arg = arg;
  splash_render_func_exclusive = exclusive;
}

void load_title_logo()
{
  if (!useTitleLogoForSplash.get() || titleLogo)
    return;

  int w = 0, h = 0;
  d3d::get_render_target_size(w, h, nullptr, 0);
  if (w <= 0 || h <= 0)
  {
    w = 1280;
    h = 720;
  }

#if _TARGET_ANDROID
  const char *path = "asset://title_logo.svg";
#else
  const char *path = "ui/title_logo.svg";
#endif
  eastl::string fpath;
  // use half size always
  fpath.sprintf("%s:%u:%u:K", path, w / 2, h / 2);

  if (!dd_file_exists(path))
    // silently wait while engine systems initialize
    return;

  titleLogo = ::create_texture(fpath.c_str(), TEXCF_RGB | TEXCF_ABEST, 0, false, nullptr);
  if (!titleLogo)
    // silently wait while engine systems initialize
    return;
}

void animated_splash_screen_start(bool do_encode)
{
  if (dgs_get_settings()->getBool("skipSplashScreenAnimation", false))
    return;

  debug("[splash] animated_splash_screen_start: splash_started=%d", splash_started);
  ddsx::set_streaming_mode(ddsx::MultiDecoders);
  if (splash_started)
    return;

  useTitleLogoForSplash.set(dgs_get_settings()->getBool("useTitleLogoForSplash", false));

  init_and_get_l8_64_noise();
  init_and_get_hash_128_noise();
  load_title_logo();
  if (!loadingSplash.getMat())
  {
    is_encoding = do_encode;
    if (do_encode)
    {
      HdrOutputMode outputMode = hdrrender::get_hdr_output_mode();
      eastl::string shaderName;
      shaderName.sprintf("loading_splash_%d", static_cast<int>(outputMode));
      loadingSplash.init(shaderName.c_str());
    }
    else
    {
      loadingSplash.init("loading_splash_4");
    }
  }

  const uint32_t lastDraw = (uint32_t)interlocked_relaxed_load(last_splash_draw_msec);
  if (!lastDraw || get_time_msec() > lastDraw + 2000)
  {
    splash_id = grnd();
    int maxIndex1 = dgs_get_settings()->getInt("splashMaxIndex", 3);
    int minIndex1 = dgs_get_settings()->getInt("splashMinIndex", 0);
    int maxIndex = max(minIndex1, maxIndex1), minIndex = min(minIndex1, maxIndex1);
    splash_id = dgs_get_settings()->getInt("splashId", minIndex + grnd() % max((int)1, maxIndex - minIndex + 1));
    splash_start_time = get_time_msec();
  }
  splash_started = true;

  paper_white_nits = dgs_get_settings()->getBlockByName("video")->getInt("paperWhiteNits", paper_white_nits);

  if (!get_world_renderer())
    splash_id = 0; // clouds

  ShaderElement::invalidate_cached_state_block();
}

bool is_animated_splash_screen_encoding() { return is_encoding; }

void animated_splash_screen_stop()
{
  debug("[splash] animated_splash_screen_stop: splash_started=%d", splash_started);
  if (!splash_started)
    return;
  if (is_animated_splash_screen_in_thread())
    return stop_animated_splash_screen_in_thread();

  loadingSplash.clear();
  del_d3dres(halfTargetTex);
  del_d3dres(titleLogo);
  release_l8_64_noise();
  release_hash_128_noise();
  splash_started = false;
  ddsx::set_streaming_mode(ddsx::BackgroundSerial);
}

void animated_splash_screen_draw(Texture *target_frame)
{
  interlocked_release_store(last_splash_draw_msec, (int)get_time_msec());
  if (::grs_draw_wire)
    d3d::setwire(0);

  float splash_time = (get_time_msec() - splash_start_time) * 1e-3;

  if (!loadingSplash.getElem())
  {
    d3d::clearview(CLEAR_TARGET, 0, 0, 0);
    return;
  }

  {
    int w = 0, h = 0;
    d3d::get_target_size(w, h);

    bool useVrs = d3d::get_driver_desc().caps.hasVariableRateShading;
    bool loadingHalfRes = forceLoadingHalfRes.get() || w > 1920;
#if !_TARGET_PC
    loadingHalfRes = true;
#endif
    load_title_logo();
    if (splash_render_func_exclusive || useVrs || hdrrender::get_hdr_output_mode() == HdrOutputMode::HDR10_AND_SDR)
      loadingHalfRes = false;

    Texture *rt = nullptr;
    if (loadingHalfRes)
    {
      if (auto wr = get_world_renderer(); wr && !hdrrender::is_hdr_enabled())
      {
        target_frame = wr->getFinalTargetTex2D();
      }
      rt = halfTargetTex;
      if (!rt)
      {
        uint32_t rtFmt = TEXFMT_R11G11B10F;
        if (!(d3d::get_texformat_usage(rtFmt) & d3d::USAGE_RTARGET) || hdrrender::is_hdr_enabled())
          rtFmt = TEXFMT_A16B16G16R16F;
        halfTargetTex = d3d::create_tex(NULL, w / 2, h / 2, rtFmt | TEXCF_RTARGET, 1, "splash_half_target");
        if (halfTargetTex)
        {
          halfTargetTex->disableSampler();
          debug("[splash] created splash_half_target on demand");
          rt = halfTargetTex;
        }
        else
          logerr("Failed to create RT <splash_half_target>");
      }
      if (!rt)
        d3d::set_render_target();
      else
        d3d::set_render_target(rt, 0);
    }

    d3d::get_target_size(w, h);
    float splashParams[4] = {splash_time, static_cast<float>(w), static_cast<float>(h), static_cast<float>(paper_white_nits)};
    d3d::set_ps_const(0, splashParams, 1);
    const SharedTexHolder &noise64 = init_and_get_l8_64_noise();
    const SharedTexHolder &noise128 = init_and_get_hash_128_noise();
    d3d::set_tex(STAGE_PS, 0, noise64.getTex2D());
    d3d::set_tex(STAGE_PS, 1, noise128.getTex2D());
    if (titleLogo)
      d3d::set_tex(STAGE_PS, 2, titleLogo);
    if (!splash_render_func_exclusive)
    {
      if (useVrs)
        d3d::set_variable_rate_shading(2, 2);

      loadingSplash.getElem()->setProgram(0);
      d3d::setvsrc(0, 0, 0);
      d3d::draw(PRIM_TRILIST, 0, 1);

      if (useVrs)
        d3d::set_variable_rate_shading(1, 1);
    }

    if (loadingHalfRes)
    {
      if (target_frame == d3d::get_backbuffer_tex())
        d3d::set_render_target();
      else
        d3d::set_render_target(target_frame, 0);
      if (rt)
      {
        rt->texmiplevel(0, 0);
        d3d::stretch_rect(rt, target_frame);
        rt->texmiplevel(-1, -1);
      }
    }
    if (splash_render_func)
    {
      d3d::get_target_size(w, h);
      splash_render_func(w, h, splash_render_func_arg);
    }

    release_l8_64_noise();
    release_hash_128_noise();
  }
}

void debug_animated_splash_screen()
{
#if DAGOR_DBGLEVEL > 0
  if (splashId.get() >= 0)
  {
    int8_t prev_splash_id = splash_id;
    splash_id = splashId.get();
    animated_splash_screen_draw(hdrrender::is_hdr_enabled() ? hdrrender::get_render_target_tex() : d3d::get_backbuffer_tex());
    splash_id = prev_splash_id;
  }
  else
    splash_start_time = get_time_msec();
#endif
}

bool is_animated_splash_screen_started() { return splash_started; }

static void splash_render()
{
  d3d::GpuAutoLock gpuLock;
  d3d::set_render_target();

  d3d::clearview(CLEAR_TARGET, 0, 0, 0);
  animated_splash_screen_draw(d3d::get_backbuffer_tex());

  d3d::update_screen();
}

class SplashThread;

static eastl::unique_ptr<SplashThread> splash_thread = nullptr;
d3d::BeforeWindowDestroyedCookie *splash_cookie = nullptr;

class SplashThread final : public DaThread
{
  void execute() override
  {
    while (!interlocked_acquire_load(this->terminating))
    {
      const uint32_t lastDraw = (uint32_t)interlocked_relaxed_load(last_splash_draw_msec);
      if (get_time_msec() > lastDraw + 33)
      {
#if _TARGET_PC_WIN
        if (dgs_get_window_mode() != WindowMode::FULLSCREEN_EXCLUSIVE)
#endif
          splash_render();
        if (interlocked_acquire_load(allow_watchdog_kick))
          watchdog_kick();
      }
      sleep_msec(33 - clamp(get_time_msec() - lastDraw, 0u, 33u));
    }
  }

public:
  SplashThread() :
    DaThread("SplashRenderThread",
#if _TARGET_PC_WIN // For Fraps and other 3rd parties that hook d3d present
      512 << 10,
#else
      256 << 10,
#endif
      // we increase priority by one step to match main thread priority
      cpujobs::DEFAULT_THREAD_PRIORITY - cpujobs::THREAD_PRIORITY_LOWER_STEP,
      WORKER_THREADS_AFFINITY_MASK)
  {
    animated_splash_screen_start();
  }
};

void start_animated_splash_screen_in_thread()
{
  if (dgs_get_settings()->getBool("skipSplashScreenAnimation", false))
    return;

#if _TARGET_PC_WIN
  if (dgs_get_window_mode() == WindowMode::FULLSCREEN_EXCLUSIVE)
  {
    if (is_main_thread())
    {
      // render single splash frame when called from main thread in full-screen
      bool was_started = is_animated_splash_screen_started();
      if (!was_started)
        animated_splash_screen_start();
      splash_render();
      if (!was_started)
        animated_splash_screen_stop();
    }
  }
#endif
  debug("[splash] start_animated_splash_screen_in_thread: splash_thread=%p splash_started=%d", splash_thread.get(), splash_started);
  if (!splash_thread)
  {
    // It's enough to register only once and don't unregister, because the stop_animated_splash_screen_in_thread can be called anyway
    if (!splash_cookie)
      splash_cookie = d3d::register_before_window_destroyed_callback(stop_animated_splash_screen_in_thread);
    splash_thread.reset(new SplashThread());
    splash_thread->start();
  }
}

void stop_animated_splash_screen_in_thread()
{
  debug("[splash] stop_animated_splash_screen_in_thread: splash_thread=%p splash_started=%d", splash_thread.get(), splash_started);
  if (splash_thread)
  {
    splash_thread->terminate(true, -1);
    splash_thread.reset();
    animated_splash_screen_stop();
  }
}

bool is_animated_splash_screen_in_thread() { return splash_thread.get() != nullptr; }

static void stop_splash_on_disconnect_es_event_handler(const EventOnDisconnectedFromServer &)
{
  stop_animated_splash_screen_in_thread();
}
