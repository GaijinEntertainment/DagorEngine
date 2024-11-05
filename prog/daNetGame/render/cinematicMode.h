// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <math/dag_Point3.h>
#include <videoEncoder/videoEncoder.h>
#include <daECS/core/componentTypes.h>
#include <render/fx/flare.h>

class CinematicMode
{
public:
  struct Settings
  {
    Settings() = default;

    VideoSettings videoSettings;
    int sub_pixels = 1;
    int super_pixels = 1;
    int origActRate = 0;
  };

  CinematicMode();
  CinematicMode &operator=(const CinematicMode &) = delete;
  ~CinematicMode();
  const Settings *getSettings() const;
  void setVignetteStrength(float strength);
  static constexpr int CHROMATIC_ABERRATION_PRIORITY = 1;
  static constexpr int VIGNETTE_PRIORITY = 1;
  static constexpr int FILM_GRAIN_PRIORITY = 1;
  void setChromaticAberration(Point3 chromatic_aberration);
  void setFilmGrain(Point3 film_grain);
  void setFps(int fps);
  void setSubPixels(int sub_pixels);
  void setSuperPixels(int super_pixels);
  void setLenseFlareIntesity(float intensity);
  void toggleLenseFlareCovering(bool use_covering);
  void initLenseFlare(const char *covering_tex_name, const char *readial_tex_name);
  void renderLenseFlare(ManagedTexView prev_frame_tex);
  void setFName(const char *fname);
  void setAudioDataFName(const eastl::string &fname);
  void setBitrate(int bitrate);
  void enablePostBloomEffects(bool enable_post_bloom);
  void setResolution(const IPoint2 &res);
  AudioBuffer &getAudioBuffer() { return audioBuffersPerSample; }
  const VideoSettings &getVideoSettings() const { return settings.videoSettings; }
  VideoEncoder &getVideoEncoder() { return videoEncoder; }
  bool loadAudioData();
  bool saveAudioData();
  void setOrigActRate(int rate);
  int getOrigActRate() const;

private:
  Settings settings;
  float defaultBloomThreshold = 0.8f;
  AudioBuffer audioBuffersPerSample;
  VideoEncoder videoEncoder;

  Flare flare;
};

ECS_DECLARE_RELOCATABLE_TYPE(CinematicMode);

// returns temporaty pointer, don't store it
CinematicMode *get_cinematic_mode();
void reset_default_color_grading();
const CinematicMode::Settings *get_cinematic_mode_settings();
bool is_cinematic_mode_enabled();
bool is_recording();
bool is_audio_recording();
void process_video_encoder(BaseTexture *final_render_target);