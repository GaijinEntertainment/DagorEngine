// Copyright (C) Gaijin Games KFT.  All rights reserved.

#define INSIDE_RENDERER 1
#include <render/cinematicMode.h>
#include "world/private_worldRenderer.h"
#include <shaders/dag_shaders.h>
#include <render/resolution.h>
#include <util/dag_console.h>
#include <stdio.h>
#include <workCycle/dag_workCycle.h>
#include <drv/3d/dag_info.h>
#include <render/priorityManagedShadervar.h>

#define CINEMATIC_MODE_SHADERVARS  \
  VAR(cinematic_mode_on)           \
  VAR(chromatic_aberration_params) \
  VAR(film_grain_params)           \
  VAR(vignette_strength)           \
  VAR(flare_halo_space_mul)        \
  VAR(flare_ghosts_space_mul)      \
  VAR(enable_post_bloom_effects)

#define VAR(a) static int a##VarId = -1;
CINEMATIC_MODE_SHADERVARS
#undef VAR

CinematicMode::CinematicMode()
{
#define VAR(a) a##VarId = get_shader_glob_var_id(#a);
  CINEMATIC_MODE_SHADERVARS
#undef VAR

  ShaderGlobal::set_int(cinematic_mode_onVarId, 1);
  setOrigActRate(dagor_get_game_act_rate());
}

CinematicMode::~CinematicMode()
{
  dagor_set_game_act_rate(getOrigActRate());
  ShaderGlobal::set_int(cinematic_mode_onVarId, 0);
  PriorityShadervar::clear(chromatic_aberration_paramsVarId, CHROMATIC_ABERRATION_PRIORITY);
  PriorityShadervar::clear(film_grain_paramsVarId, FILM_GRAIN_PRIORITY);
  PriorityShadervar::clear(vignette_strengthVarId, VIGNETTE_PRIORITY);
  reset_default_color_grading();
  videoEncoder.stop();
  videoEncoder.shutdown();
  flare.close();
}

const CinematicMode::Settings *CinematicMode::getSettings() const { return &settings; }

void CinematicMode::setVignetteStrength(float strength)
{
  PriorityShadervar::set_real(vignette_strengthVarId, VIGNETTE_PRIORITY, strength);
}

void CinematicMode::setChromaticAberration(Point3 chromatic_aberration)
{
  PriorityShadervar::set_color4(chromatic_aberration_paramsVarId, CHROMATIC_ABERRATION_PRIORITY, Point4::xyz0(chromatic_aberration));
}

void CinematicMode::setFilmGrain(Point3 film_grain)
{
  PriorityShadervar::set_color4(film_grain_paramsVarId, FILM_GRAIN_PRIORITY, Point4::xyz0(film_grain));
}

void CinematicMode::setFps(int fps) { settings.videoSettings.fps = fps; }

void CinematicMode::setSubPixels(int sub_pixels) { settings.sub_pixels = sub_pixels; }

void CinematicMode::setSuperPixels(int super_pixels) { settings.super_pixels = super_pixels; }

void CinematicMode::setLenseFlareIntesity(float intensity)
{
  ShaderGlobal::set_real(flare_halo_space_mulVarId, intensity);
  ShaderGlobal::set_real(flare_ghosts_space_mulVarId, intensity);
}

void CinematicMode::toggleLenseFlareCovering(bool use_covering) { flare.toggleCovering(use_covering); }

void CinematicMode::initLenseFlare(const char *covering_tex_name, const char *readial_tex_name)
{
  flare.close();
  int scrw = 128, scrh = 128;
  get_rendering_resolution(scrw, scrh);
  flare.init(Point2(scrw / 4.0f, scrh / 4.0f), covering_tex_name, readial_tex_name);
}

void CinematicMode::renderLenseFlare(ManagedTexView prev_frame_tex)
{
  if (!prev_frame_tex)
  {
    logerr("Lens flare was enabled, but prev_frame_tex was null!");
    return;
  }
  flare.apply(prev_frame_tex.getTex2D(), prev_frame_tex.getTexId());
}

void CinematicMode::setOrigActRate(int rate) { settings.origActRate = rate; }

int CinematicMode::getOrigActRate() const { return settings.origActRate; }

void CinematicMode::setFName(const char *fname) { settings.videoSettings.fname = fname; }

void CinematicMode::setAudioDataFName(const eastl::string &fname) { settings.videoSettings.audioDataFName = fname; }

void CinematicMode::setBitrate(int bitrate) { settings.videoSettings.bitrate = bitrate; }

void CinematicMode::enablePostBloomEffects(bool enable_post_bloom)
{
  ShaderGlobal::set_int(enable_post_bloom_effectsVarId, enable_post_bloom);
}

void CinematicMode::setResolution(const IPoint2 &res)
{
  settings.videoSettings.width = res.x;
  settings.videoSettings.height = res.y;
}

bool CinematicMode::loadAudioData()
{
  FILE *f = fopen(settings.videoSettings.audioDataFName.c_str(), "rb");
  if (!f)
    return false;
  audioBuffersPerSample.clear();
  while (!feof(f))
  {
    uint32_t size;
    size_t read = fread(&size, 4, 1, f);
    G_ASSERT(1 == read);
    if (1 != read)
    {
      fclose(f);
      return false;
    }
    eastl::vector<uint8_t> sample;
    sample.resize(size);
    read = fread(sample.data(), size, 1, f);
    G_ASSERT(1 == read);
    if (1 != read)
    {
      fclose(f);
      return false;
    }
    audioBuffersPerSample.emplace_back(eastl::move(sample));
  }
  fclose(f);
  G_ASSERT(!audioBuffersPerSample.empty());
  return !audioBuffersPerSample.empty();
}

bool CinematicMode::saveAudioData()
{
  FILE *f = fopen(settings.videoSettings.audioDataFName.c_str(), "wb");
  if (!f)
    return false;
  for (const auto &sample : audioBuffersPerSample)
  {
    size_t size = sample.size();
    if (!size)
      continue;
    size_t written = fwrite(&size, 4, 1, f);
    G_ASSERT(1 == written);
    if (1 != written)
    {
      fclose(f);
      return false;
    }
    written = fwrite(sample.data(), sample.size(), 1, f);
    G_ASSERT(1 == written);
    if (1 != written)
    {
      fclose(f);
      return false;
    }
  }
  fclose(f);
  return true;
}

const CinematicMode::Settings *get_cinematic_mode_settings()
{
  if (CinematicMode *cinematicMode = get_cinematic_mode())
    return cinematicMode->getSettings();
  else
    return nullptr;
}

bool is_cinematic_mode_enabled() { return get_cinematic_mode(); }

bool is_recording()
{
  if (CinematicMode *cinematicMode = get_cinematic_mode())
    return cinematicMode->getVideoEncoder().isRecording();
  return false;
}

bool is_audio_recording()
{
  if (CinematicMode *cinematicMode = get_cinematic_mode())
    return cinematicMode->getVideoEncoder().audioRecorder.isRecording();
  return false;
}


static bool encoder_console_handler(const char *argv[], int argc)
{
  int found = 0;
  CONSOLE_CHECK_NAME("video", "list_default", 1, 1) { list_default_device(); }
  CONSOLE_CHECK_NAME("video", "list", 1, 1) { list_devices(); }
  CONSOLE_CHECK_NAME("video", "start", 1, 1)
  {
    CinematicMode *cinematicMode = get_cinematic_mode();
    VideoSettings settings;
    settings.fname = "d:\\encode.mp4";
    // settings.audioDataFName = "d:\\audio.data";
    WorldRenderer *wr = (WorldRenderer *)get_world_renderer();
    TextureInfo info;
    wr->getFinalTargetTex()->getinfo(info);
    settings.width = info.w;
    settings.height = info.h;
    settings.fps = 30;
    cinematicMode->getVideoEncoder().init(d3d::get_device());
    // cinematicMode->loadAudioData();
    // cinematicMode->getVideoEncoder().setExternalAudioBuffer(eastl::move(cinematicMode->getAudioBuffer()));
    cinematicMode->getVideoEncoder().start(settings);
  }

  CONSOLE_CHECK_NAME("video", "stop", 1, 1)
  {
    CinematicMode *cinematicMode = get_cinematic_mode();
    cinematicMode->getVideoEncoder().stop();
    cinematicMode->getVideoEncoder().shutdown();
  }

  CONSOLE_CHECK_NAME("video", "startAudio", 1, 1)
  {
    CinematicMode *cinematicMode = get_cinematic_mode();
    cinematicMode->getVideoEncoder().audioRecorder.init();
    cinematicMode->getVideoEncoder().audioRecorder.start();
  }

  CONSOLE_CHECK_NAME("video", "stopAudio", 1, 1)
  {
    CinematicMode *cinematicMode = get_cinematic_mode();
    cinematicMode->getVideoEncoder().audioRecorder.stop(eastl::move(cinematicMode->getAudioBuffer()));
    cinematicMode->getVideoEncoder().audioRecorder.shutdown();
    cinematicMode->saveAudioData();
  }
  return found;
}

REGISTER_CONSOLE_HANDLER(encoder_console_handler);
