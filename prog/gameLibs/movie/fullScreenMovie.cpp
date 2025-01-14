// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <movie/fullScreenMovie.h>
#include <videoPlayer/dag_videoPlayer.h>
#include <sound/dag_genAudio.h>
#include <workCycle/dag_workCycle.h>
#include <workCycle/dag_delayedAction.h>
#include <workCycle/dag_gameSettings.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_matricesAndPerspective.h>
#include <drv/3d/dag_lock.h>
#include <drv/3d/dag_resetDevice.h>
#include <drv/3d/dag_platform.h>
#include <drv/hid/dag_hiGlobals.h>
#include <drv/hid/dag_hiJoystick.h>
#include <drv/hid/dag_hiJoyData.h>
#include <startup/dag_inpDevClsDrv.h>
#include <ioSys/dag_dataBlock.h>
#include <osApiWrappers/dag_files.h>
#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_miscApi.h>
#include <osApiWrappers/dag_wndProcComponent.h>
#include <osApiWrappers/dag_localConv.h>
#include <startup/dag_globalSettings.h>
#include <perfMon/dag_cpuFreq.h>
#include "vibroPlayer.h"
#include <movie/subtitleplayer.h>
#include "inputDev.h"
#include "yuvFrame.h"
#include "playerCreators.h"
#include <debug/dag_debug.h>
#include <render/hdrRender.h>
#include <drv/dag_vr.h>
#include <vr/vrGuiSurface.h>

#if _TARGET_PC_WIN
#include <windows.h>
#endif

bool fullScreenMovieSkipped = false;

static IGenVideoPlayer *stub_theora(const char *fn)
{
  DAG_FATAL("cannot show %s\ntheora player not inited, call init_player_for_ogg_theora()\n", fn);
  RETURN_X_AFTER_FATAL(NULL);
}
static IGenVideoPlayer *stub_native(const char *fn, int, int, float)
{
  DAG_FATAL("cannot show %s\nnative player not inited, call init_player_for_native_video()\n", fn);
  RETURN_X_AFTER_FATAL(NULL);
}
static IGenVideoPlayer *stub_dagui(const char *fn)
{
  DAG_FATAL("cannot show %s\ndagui player not inited, call init_player_for_dagui()\n", fn);
  RETURN_X_AFTER_FATAL(NULL);
}

IGenVideoPlayer *(*create_theora_movie_player)(const char *) = &stub_theora;
IGenVideoPlayer *(*create_dagui_movie_player)(const char *) = &stub_dagui;
IGenVideoPlayer *(*create_native_movie_player)(const char *, int, int, float) = &stub_native;

static bool movie_skippable_by_all_devices = true;

using VRFrameCallback = void (*)();
static VRFrameCallback vr_frame_cancel_callback = nullptr;
static VRFrameCallback vr_frame_resume_callback = nullptr;

static bool vr_gui_surface_inited = false;
static int vr_gui_surface_width = 16;
static int vr_gui_surface_height = 9;
static vrgui::SurfaceCurvature vr_gui_surface_curvature = vrgui::SurfaceCurvature::VRGUI_SURFACE_PLANE;
static void prepare_vr_gui_render()
{
  vr_gui_surface_inited = vrgui::is_inited();
  if (vr_gui_surface_inited)
    vrgui::get_ui_surface_params(vr_gui_surface_width, vr_gui_surface_height, vr_gui_surface_curvature);

  if (vr_gui_surface_inited)
    vrgui::destroy_surface();

  vrgui::init_surface(vr_gui_surface_width, vr_gui_surface_height, vrgui::SurfaceCurvature::VRGUI_SURFACE_PLANE);
}

static void reset_vr_gui_render()
{
  vrgui::destroy_surface();

  if (vr_gui_surface_inited)
    vrgui::init_surface(vr_gui_surface_width, vr_gui_surface_height, vr_gui_surface_curvature);
}

static bool prepare_vr_frame(VRDevice::FrameData &frameData)
{
  bool vrActive = false;
  if (auto vrDevice = VRDevice::getInstance())
  {
    vrDevice->tick(nullptr);

    vrActive = vrDevice->prepareFrame(frameData, 1, 100);
    if (vrActive)
    {
      vrDevice->beginFrame(frameData);

      for (int eye : {0, 1})
      {
        d3d::set_render_target(0, frameData.frameTargets[eye].getTex2D(), 0, 0);
        d3d::clearview(CLEAR_TARGET, E3DCOLOR(0, 0, 0), 0, 0);
      }
    }
  }

  return vrActive;
}

static void render_vr_views(VRDevice::FrameData &frameData, TEXTUREID videoTexture)
{
  for (int eye : {0, 1})
  {
    d3d::set_render_target(0, frameData.frameTargets[eye].getTex2D(), 0, 0);

    TMatrix world = TMatrix::IDENT;
    Matrix44 view = Matrix44(frameData.views[eye].viewTransform);
    world.rotyTM(-PI / 2);
    d3d::settm(TM_WORLD, world);
    d3d::settm(TM_VIEW, &view);
    d3d::setpersp(frameData.views[eye].projection);

    vrgui::render_to_surface(videoTexture);
  }

  BaseTexture *sa[] = {frameData.frameTargets[0].getBaseTex(), frameData.frameTargets[1].getBaseTex(),
    frameData.frameDepths[0].getBaseTex(), frameData.frameDepths[1].getBaseTex()};
  d3d::driver_command(Drv3dCommand::PREPARE_TEXTURES_FOR_VR_CONSUMPTION, sa, (void *)4);
}

class GameInactivateDetect : public IWndProcComponent
{
public:
  GameInactivateDetect(IGenMusic *s, float v) : snd(s), vol(v) { add_wnd_proc_component(this); }
  ~GameInactivateDetect() { del_wnd_proc_component(this); }
  virtual RetCode process(void *, unsigned msg, uintptr_t wParam, intptr_t, intptr_t &)
  {
#if _TARGET_PC_WIN
    if (msg == WM_ACTIVATE && snd && vol > 0)
      dagor_audiosys->setMusicVolume(snd, LOWORD(wParam) == WA_INACTIVE ? 1e-6f : vol); // not zero to avoid channel become virtual
#else
    (void)msg;
    (void)wParam;
#endif
    return PROCEED_OTHER_COMPONENTS;
  }
  IGenMusic *snd;
  float vol;
};


static void check_and_process_reset_device(IGenVideoPlayer *player)
{
  bool deviceLost = d3d::device_lost(NULL);
  if (deviceLost && player)
    player->beforeResetDevice();

  check_and_restore_3d_device();

  if (deviceLost && !d3d::device_lost(NULL))
    player->afterResetDevice();
}


static void play_movie(const char *fname, const char *audio_fname, const char *subtitle_prefix, bool loop, int v_chan, int a_chan,
  bool /*leave_bgm*/, bool show_subtitles, bool do_force_fx, const char *sublangtiming, float volume, int skip_delay_msec)
{
  IGenVideoPlayer *player = NULL;
  IGenMusic *movie_sound = NULL;
  MovieSubPlayer subPlayer;
  MovieVibroPlayer vibroPlayer;
  bool done = false, have_frame = false;
  float audio_vol = volume;
  int lastPos = 0;

#if _TARGET_PC | _TARGET_ANDROID | _TARGET_IOS | _TARGET_TVOS | _TARGET_C3
  GameInactivateDetect game_inactivate_detect(movie_sound, audio_vol);

  player = create_native_movie_player(fname, v_chan, a_chan, volume);

#elif _TARGET_C1 | _TARGET_C2 | _TARGET_XBOX
  player = create_native_movie_player(fname, v_chan, a_chan, volume);
#endif

  if (!player || !yuvframerender::init(player->getFrameSize(), true))
  {
    debug(!player ? "cannot create movie player" : "YUV shader not found!");
    return;
  }

  movie_sound = dagor_audiosys->createMusic(audio_fname, loop);
  player->setAutoRewind(loop);

  if (subtitle_prefix && (show_subtitles || do_force_fx))
  {
    if (show_subtitles)
      subPlayer.init(String(0, "%s.sub.blk", subtitle_prefix), sublangtiming);
    if (do_force_fx)
      vibroPlayer.init(String(0, "%s.ff.blk", subtitle_prefix));
  }


  player->start(10);

  d3d::driver_command(Drv3dCommand::ACQUIRE_OWNERSHIP);
  d3d::set_render_target();
#if _TARGET_PC | _TARGET_C1 | _TARGET_C2 | _TARGET_XBOX
  for (int i = 0; i < 3; ++i)
  {
    d3d::clearview(CLEAR_TARGET, E3DCOLOR(0, 0, 0, 0), 1, 0);
    d3d::update_screen();
    dagor_idle_cycle();
  }
  bool old_vsync = d3d::get_vsync_enabled();

#endif
  UniqueTex videoTexture;
  if (VRDevice::getInstance())
  {
    Texture *backBufferTex = d3d::get_backbuffer_tex();
    TextureInfo ti;

    if (backBufferTex)
      backBufferTex->getinfo(ti);
    else
    {
      int sw, sh;
      d3d::get_screen_size(sw, sh);
      ti.w = sw;
      ti.h = sh;
      // no need to set ti.cflg, default inited to TEXFMT_DEFAULT
    }

    videoTexture = dag::create_tex(nullptr, ti.w, ti.h, (ti.cflg & TEXFMT_MASK) | TEXCF_RTARGET, 1, "video_texture");

    prepare_vr_gui_render();
  }

  d3d::driver_command(Drv3dCommand::RELEASE_OWNERSHIP);

  int startTime = ::get_time_msec();

  while (!player->isFinished() && !done)
  {
    check_and_process_reset_device(player);
    TEXTUREID texIdY, texIdU, texIdV;
    d3d::SamplerHandle smp;
    if (player->getFrame(texIdY, texIdU, texIdV, smp))
      if (!have_frame)
      {
        if (movie_sound)
          dagor_audiosys->playMusic(movie_sound, -1, ::dgs_app_active ? audio_vol : 0);

        have_frame = true;
      }

    if (player->getPosition() < lastPos && movie_sound)
    {
      dagor_audiosys->stopMusic(movie_sound);
      dagor_audiosys->playMusic(movie_sound, -1, ::dgs_app_active ? audio_vol : 0);
    }
    lastPos = player->getPosition();

    d3d::driver_command(Drv3dCommand::ACQUIRE_OWNERSHIP);

    VRDevice::FrameData frameData;
    bool vrActive = prepare_vr_frame(frameData);

    d3d::enable_vsync(!vrActive);

    if (vrActive)
      d3d::set_render_target(0, videoTexture.getTex2D(), 0, 0);
    else
      d3d::set_render_target();
    d3d::clearview(CLEAR_TARGET, E3DCOLOR(0, 0, 0, 0), 1, 0);
    if (have_frame)
    {
      yuvframerender::startRender();
      yuvframerender::render(texIdY, texIdU, texIdV, smp);

      if (show_subtitles)
        subPlayer.render(player->getPosition());
      if (do_force_fx)
        vibroPlayer.update(player->getPosition());
      StdGuiRender::flush_data();
      yuvframerender::endRender();
      StdGuiRender::update_internals_per_act();
      player->onCurrentFrameDispatched();

      if (vrActive)
      {
        d3d::stretch_rect(videoTexture.getTex2D(), d3d::get_backbuffer_tex());

        render_vr_views(frameData, videoTexture.getTexId());
      }
    }
    d3d::driver_command(Drv3dCommand::RELEASE_OWNERSHIP);

    player->advance(6000);
    player->advance(7000);

    d3d::driver_command(Drv3dCommand::ACQUIRE_OWNERSHIP);
    d3d::update_screen();
    d3d::driver_command(Drv3dCommand::RELEASE_OWNERSHIP);

    StdGuiRender::reset_per_frame_dynamic_buffer_pos();
    dagor_idle_cycle();
    if (movieinput::is_movie_interrupted(movie_skippable_by_all_devices))
    {
      if (::get_time_msec() - startTime >= skip_delay_msec)
        done = true;
    }
  }

  d3d::driver_command(Drv3dCommand::ACQUIRE_OWNERSHIP);

  videoTexture.close();
  reset_vr_gui_render();

  StdGuiRender::reset_draw_str_attr();
  yuvframerender::term();

#if _TARGET_PC | _TARGET_C1 | _TARGET_C2 | _TARGET_XBOX
  d3d::enable_vsync(old_vsync);
#endif

  d3d::driver_command(Drv3dCommand::RELEASE_OWNERSHIP);

  if (movie_sound)
  {
    dagor_audiosys->stopMusic(movie_sound);
    dagor_audiosys->releaseMusic(movie_sound);
  }

  player->destroy();
}

static void play_movie_ogg(const char *fname, const char *audio_fname, const char *subtitle_prefix, bool loop, bool /*leave_bgm*/,
  bool show_subtitles, bool do_force_fx, const char *sublangtiming, float volume, int skip_delay_msec)
{
  IGenVideoPlayer *player = NULL;
  IGenMusic *movie_sound = NULL;
  MovieSubPlayer subPlayer;
  MovieVibroPlayer vibroPlayer;
  bool done = false, have_frame = false;
  float audio_vol = volume;
  int lastPos = 0;

#if _TARGET_PC | _TARGET_ANDROID | _TARGET_IOS | _TARGET_TVOS | _TARGET_C3
  GameInactivateDetect game_inactivate_detect(movie_sound, audio_vol);

  player = create_theora_movie_player(fname);

#elif _TARGET_C1 | _TARGET_C2 | _TARGET_XBOX
  player = create_theora_movie_player(fname);
#endif

  if (!player || !yuvframerender::init(player->getFrameSize(), false))
  {
    debug(!player ? "cannot create movie player" : "YUV shader not found!");
    return;
  }


  movie_sound = dagor_audiosys->createMusic(audio_fname, loop);
  player->setAutoRewind(loop);

  if (subtitle_prefix && (show_subtitles || do_force_fx))
  {
    if (show_subtitles)
    {
      String fsrt;
      if (sublangtiming)
        fsrt.printf(0, "%s.%s.srt", subtitle_prefix, sublangtiming);
      else
        fsrt.printf(0, "%s.srt", subtitle_prefix);
      fsrt = fsrt.toLower();
      if (::dd_file_exist(fsrt))
        subPlayer.initSrt(fsrt);
      else
      {
        fsrt.printf(0, "%s.sub.blk", subtitle_prefix);
        subPlayer.init(fsrt, sublangtiming);
      }
    }
    if (do_force_fx)
      vibroPlayer.init(String(0, "%s.ff.blk", subtitle_prefix));
  }


  player->start(10);

  d3d::driver_command(Drv3dCommand::ACQUIRE_OWNERSHIP);
  d3d::set_render_target();

#if _TARGET_PC | _TARGET_C1 | _TARGET_C2 | _TARGET_XBOX
  for (int i = 0; i < 3; ++i)
  {
    d3d::clearview(CLEAR_ZBUFFER | CLEAR_STENCIL | CLEAR_TARGET, E3DCOLOR(0, 0, 0, 0), 1, 0);
    d3d::update_screen();
    dagor_idle_cycle();
  }
  bool old_vsync = d3d::get_vsync_enabled();
  d3d::enable_vsync(true);

#endif
  d3d::driver_command(Drv3dCommand::RELEASE_OWNERSHIP);

  int startTime = ::get_time_msec();

  while (!player->isFinished() && !done)
  {
    check_and_process_reset_device(player);
    TEXTUREID texIdY, texIdU, texIdV;
    d3d::SamplerHandle smp;
    if (player->getFrame(texIdY, texIdU, texIdV, smp))
      if (!have_frame)
      {
        if (movie_sound)
          dagor_audiosys->playMusic(movie_sound, -1, ::dgs_app_active ? audio_vol : 0);

        have_frame = true;
      }

    if (player->getPosition() < lastPos && movie_sound)
    {
      dagor_audiosys->stopMusic(movie_sound);
      dagor_audiosys->playMusic(movie_sound, -1, ::dgs_app_active ? audio_vol : 0);
    }
    lastPos = player->getPosition();

    d3d::driver_command(Drv3dCommand::ACQUIRE_OWNERSHIP);
    hdrrender::set_render_target();
    d3d::clearview(CLEAR_TARGET, E3DCOLOR(0, 0, 0, 0), 1, 0);
    if (have_frame)
    {
      yuvframerender::startRender();
      yuvframerender::render(texIdY, texIdU, texIdV, smp);

      if (show_subtitles)
        subPlayer.render(player->getPosition());
      if (do_force_fx)
        vibroPlayer.update(player->getPosition());
      StdGuiRender::flush_data();
      yuvframerender::endRender();
      StdGuiRender::update_internals_per_act();
      player->onCurrentFrameDispatched();
    }
    d3d::set_render_target();
    hdrrender::encode();
    d3d::driver_command(Drv3dCommand::RELEASE_OWNERSHIP);

    player->advance(6000);
    player->advance(7000);
    dagor_idle_cycle();

    d3d::driver_command(Drv3dCommand::ACQUIRE_OWNERSHIP);
    d3d::update_screen();
    d3d::driver_command(Drv3dCommand::RELEASE_OWNERSHIP);

    if (movieinput::is_movie_interrupted(movie_skippable_by_all_devices))
    {
      if (::get_time_msec() - startTime >= skip_delay_msec)
        done = true;
    }
  }

  d3d::driver_command(Drv3dCommand::ACQUIRE_OWNERSHIP);

  StdGuiRender::reset_draw_str_attr();
  yuvframerender::term();

#if _TARGET_PC | _TARGET_C1 | _TARGET_C2 | _TARGET_XBOX
  d3d::enable_vsync(old_vsync);
#endif

  d3d::driver_command(Drv3dCommand::RELEASE_OWNERSHIP);

  if (movie_sound)
  {
    dagor_audiosys->stopMusic(movie_sound);
    dagor_audiosys->releaseMusic(movie_sound);
  }

  player->destroy();
}

static void play_movie_gui(const char *fname, const char *audio_fname, const char *subtitle_prefix, bool /*leave_bgm*/,
  bool do_force_fx, float volume, int skip_delay_msec)
{
  IGenVideoPlayer *player = NULL;
  IGenMusic *movie_sound = NULL;
  MovieVibroPlayer vibroPlayer;
  bool done = false;
  float audio_vol = volume;
  int lastPos = 0;
  bool have_frame = false;

#if _TARGET_PC | _TARGET_ANDROID | _TARGET_IOS | _TARGET_TVOS | _TARGET_C3
  GameInactivateDetect game_inactivate_detect(movie_sound, audio_vol);

  player = create_dagui_movie_player(fname);

#elif _TARGET_C1 | _TARGET_C2 | _TARGET_XBOX
  player = create_dagui_movie_player(fname);

#endif

  if (!player)
  {
    debug("cannot create movie player");
    return;
  }

  movie_sound = dagor_audiosys->createMusic(audio_fname, false);
  player->setAutoRewind(false);

  if (subtitle_prefix && do_force_fx)
    vibroPlayer.init(String(0, "%s.ff.blk", subtitle_prefix));


  player->start(10);

  d3d::driver_command(Drv3dCommand::ACQUIRE_OWNERSHIP);
  d3d::set_render_target();
#if _TARGET_PC | _TARGET_C1 | _TARGET_C2 | _TARGET_XBOX
  for (int i = 0; i < 3; ++i)
  {
    d3d::clearview(CLEAR_TARGET, E3DCOLOR(0, 0, 0, 0), 1, 0);
    d3d::update_screen();
    dagor_idle_cycle();
  }
  bool old_vsync = d3d::get_vsync_enabled();
  d3d::enable_vsync(true);

#endif

  d3d::driver_command(Drv3dCommand::RELEASE_OWNERSHIP);

  int startTime = ::get_time_msec();

  while (!player->isFinished() && !done)
  {
    check_and_process_reset_device(player);

    if (!have_frame)
    {
      if (movie_sound)
        dagor_audiosys->playMusic(movie_sound, -1, ::dgs_app_active ? audio_vol : 0);

      have_frame = true;
    }

    if (player->getPosition() < lastPos && movie_sound)
    {
      dagor_audiosys->stopMusic(movie_sound);
      dagor_audiosys->playMusic(movie_sound, -1, ::dgs_app_active ? audio_vol : 0);
    }

    lastPos = player->getPosition();

    d3d::driver_command(Drv3dCommand::ACQUIRE_OWNERSHIP);
    d3d::clearview(CLEAR_TARGET, E3DCOLOR(0, 0, 0, 0), 1, 0);
    player->renderFrame(NULL, NULL);
    if (do_force_fx)
      vibroPlayer.update(player->getPosition());
    d3d::driver_command(Drv3dCommand::RELEASE_OWNERSHIP);

    player->advance(6000);
    player->advance(7000);
    dagor_idle_cycle();

    d3d::driver_command(Drv3dCommand::ACQUIRE_OWNERSHIP);
    d3d::update_screen();
    d3d::driver_command(Drv3dCommand::RELEASE_OWNERSHIP);

    if (movieinput::is_movie_interrupted(movie_skippable_by_all_devices))
    {
      if (::get_time_msec() - startTime >= skip_delay_msec)
        done = true;
    }
  }

  d3d::driver_command(Drv3dCommand::ACQUIRE_OWNERSHIP);

#if _TARGET_PC | _TARGET_C1 | _TARGET_C2 | _TARGET_XBOX
  d3d::enable_vsync(old_vsync);
#endif

  d3d::driver_command(Drv3dCommand::RELEASE_OWNERSHIP);

  if (movie_sound)
  {
    dagor_audiosys->stopMusic(movie_sound);
    dagor_audiosys->releaseMusic(movie_sound);
  }

  player->destroy();
}

void set_movie_skippable_by_all_devices(bool en) { movie_skippable_by_all_devices = en; }

void set_vr_frame_cancel_and_resume_callbacks(void (*cancel_callback)(), void (*resume_callback)())
{
  vr_frame_cancel_callback = cancel_callback;
  vr_frame_resume_callback = resume_callback;
}

void play_fullscreen_movie(const char *fname, const char *audio_fname, const char *subtitle_prefix, bool loop, int v_chan, int a_chan,
  bool leave_bgm, bool show_subtitles, bool do_force_fx, const char *sublangtiming, float volume, int skip_delay_msec)
{
  perform_delayed_actions();

  fullScreenMovieSkipped = false;

  if (vr_frame_cancel_callback)
  {
    d3d::GpuAutoLock gpuLock;
    vr_frame_cancel_callback();
  }

  movieinput::on_movie_start();
  dagor_work_cycle_flush_pending_frame();
  bool dont_use_cpu_in_background = dgs_dont_use_cpu_in_background;
  ::dgs_dont_use_cpu_in_background = false;

  const char *ext = dd_get_fname_ext(fname);
  if (ext && (dd_stricmp(ext, ".ogg") == 0 || dd_stricmp(ext, ".ogm") == 0 || dd_stricmp(ext, ".ogv") == 0))
    play_movie_ogg(fname, audio_fname, subtitle_prefix, loop, leave_bgm, show_subtitles, do_force_fx, sublangtiming, volume,
      skip_delay_msec);
  else if (ext && (dd_stricmp(ext, ".blk") == 0))
    play_movie_gui(fname, audio_fname, subtitle_prefix, leave_bgm, do_force_fx, volume, skip_delay_msec);
  else
    play_movie(fname, audio_fname, subtitle_prefix, loop, v_chan, a_chan, leave_bgm, show_subtitles, do_force_fx, sublangtiming,
      volume, skip_delay_msec);

  movieinput::on_movie_end();

  if (vr_frame_resume_callback)
  {
    d3d::GpuAutoLock gpuLock;
    vr_frame_resume_callback();
  }

  perform_delayed_actions();
  dagor_reset_spent_work_time();
  ::dgs_dont_use_cpu_in_background = dont_use_cpu_in_background;
}
