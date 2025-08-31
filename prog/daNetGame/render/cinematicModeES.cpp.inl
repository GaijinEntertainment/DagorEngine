// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ecs/core/entityManager.h>
#include <daECS/core/coreEvents.h>
#include <daECS/core/sharedComponent.h>

#include <util/dag_console.h>
#include <ecs/weather/skiesSettings.h>
#include <render/cinematicMode.h>
#include <render/renderEvent.h>
#include <render/resolution.h>
#include <drv/3d/dag_info.h>

#include "main/level.h"
#include <render/skies.h>
#include <main/weatherPreset.h>
#include <workCycle/dag_workCycle.h>

wchar_t *utf8_to_wcs(const char *utf8_str, wchar_t *wcs_buf, int wcs_buf_len);

ECS_REGISTER_RELOCATABLE_TYPE(CinematicMode, nullptr);

template <typename Callable>
static void get_cinematic_mode_manager_ecs_query(Callable);


CinematicMode *get_cinematic_mode()
{
  CinematicMode *cinematicMode = nullptr;
  get_cinematic_mode_manager_ecs_query(
    [&cinematicMode](CinematicMode &cinematic_mode__manager) { cinematicMode = &cinematic_mode__manager; });
  return cinematicMode;
}

template <typename Callable>
static void set_video_recording_ecs_query(Callable c);

void process_video_encoder(BaseTexture *final_render_target)
{
  if (auto *cinematicMode = get_cinematic_mode())
  {
    TIME_D3D_PROFILE(videoEncoder_process);
    if (!cinematicMode->getVideoEncoder().process(final_render_target))
    {
      logerr("Can't write video sample. Stopping video record.");
      set_video_recording_ecs_query([&](bool &cinematic_mode__recording) { cinematic_mode__recording = false; });
    }
  }
}

ECS_TRACK(cinematic_mode__weatherPreset)
static void cinematic_mode_weather_changed_es_event_handler(const ecs::Event &, const ecs::string &cinematic_mode__weatherPreset)
{
  select_weather_preset_delayed(cinematic_mode__weatherPreset.c_str());
}

ECS_TRACK(cinematic_mode__rain)
static void cinematic_mode_toggle_rain_es_event_handler(
  const ecs::Event &, const bool cinematic_mode__rain, const bool cinematic_mode__hasRain)
{
  if (!cinematic_mode__hasRain)
    return;

  if (cinematic_mode__rain)
    create_rain_entities();
  else
    delete_rain_entities();
}

ECS_TRACK(cinematic_mode__snow)
static void cinematic_mode_toggle_snow_es_event_handler(
  const ecs::Event &, const bool cinematic_mode__snow, const bool cinematic_mode__hasSnow)
{
  if (!cinematic_mode__hasSnow)
    return;

  if (cinematic_mode__snow)
    create_snow_entities();
  else
    delete_snow_entities();
}

ECS_TRACK(cinematic_mode__lightning)
static void cinematic_mode_toggle_lightning_es_event_handler(
  const ecs::Event &, const bool cinematic_mode__lightning, const bool cinematic_mode__hasLightning)
{
  if (!cinematic_mode__hasLightning)
    return;

  if (cinematic_mode__lightning)
    create_lightning_entities();
  else
    delete_lightning_entities();
}

ECS_ON_EVENT(on_appear)
ECS_TRACK(cinematic_mode__recording)
static void cinematic_mode_record_es_event_handler(const ecs::Event &,
  CinematicMode &cinematic_mode__manager,
  const bool cinematic_mode__recording,
  const bool cinematic_mode__loadAudio)
{
  if (cinematic_mode__recording == is_recording())
    return;

  CinematicMode &cm = cinematic_mode__manager;
  cm.setOrigActRate(dagor_get_game_act_rate());
  auto &ve = cm.getVideoEncoder();
  if (cinematic_mode__recording)
  {
    dagor_set_game_act_rate(cm.getVideoSettings().fps);
    // TODO:: temporarily ignore resource hog settings in order to reduce audio out-of-sync/glitching issues
    IPoint2 s = get_final_render_target_resolution();
    cm.setResolution(s);
    ve.init(d3d::get_device());
    if (cinematic_mode__loadAudio)
      cm.loadAudioData();
    if (!cm.getAudioBuffer().empty())
      ve.setExternalAudioBuffer(eastl::move(cm.getAudioBuffer()));
    ve.start(cm.getVideoSettings());
  }
  else
  {
    dagor_set_game_act_rate(cm.getOrigActRate());
    ve.stop();
    ve.shutdown();
    if (cinematic_mode__loadAudio)
      remove(cm.getVideoSettings().audioDataFName.c_str());
  }
}

ECS_ON_EVENT(on_appear)
ECS_TRACK(cinematic_mode__audio_recording)
static void cinematic_mode_audio_record_es_event_handler(const ecs::Event &,
  CinematicMode &cinematic_mode__manager,
  const bool cinematic_mode__audio_recording,
  const bool cinematic_mode__saveAudio)
{
  if (cinematic_mode__audio_recording == is_audio_recording())
    return;
  auto &audioRecorder = cinematic_mode__manager.getVideoEncoder().audioRecorder;
  if (cinematic_mode__audio_recording)
  {
    cinematic_mode__manager.getAudioBuffer().clear();
    audioRecorder.init();
    audioRecorder.start();
  }
  else
  {
    audioRecorder.stop(eastl::move(cinematic_mode__manager.getAudioBuffer()));
    audioRecorder.shutdown();
    if (cinematic_mode__saveAudio)
      cinematic_mode__manager.saveAudioData();
  }
}

ECS_ON_EVENT(on_appear)
static void cinematic_mode_get_weathers_es_event_handler(const ecs::Event &, ecs::StringList &cinematic_mode__weatherPresetList)
{
  get_weather_preset_list(cinematic_mode__weatherPresetList);
}

ECS_TRACK(cinematic_mode__dayTime)
static void cinematic_mode_change_time_es_event_handler(const ecs::Event &, const float cinematic_mode__dayTime)
{
  if (abs(cinematic_mode__dayTime - get_daskies_time()) > 0.001f)
  {
    set_daskies_time(cinematic_mode__dayTime);
  }
}

ECS_ON_EVENT(on_appear, EventSkiesLoaded)
static void cinematic_mode_set_weather_es_event_handler(const ecs::Event &,
  bool &cinematic_mode__rain,
  bool &cinematic_mode__snow,
  bool &cinematic_mode__lightning,
  bool &cinematic_mode__hasRain,
  bool &cinematic_mode__hasSnow,
  bool &cinematic_mode__hasLightning,
  float &cinematic_mode__dayTime,
  ecs::string &cinematic_mode__weatherPreset)
{
  bool hasRain = has_any_rain_entity();
  bool hasSnow = has_any_snow_entity();
  bool hasLightning = has_any_lightning_entity();

  cinematic_mode__rain = hasRain;
  cinematic_mode__snow = hasSnow;
  cinematic_mode__lightning = hasLightning;

  cinematic_mode__hasRain = hasRain;
  cinematic_mode__hasSnow = hasSnow;
  cinematic_mode__hasLightning = hasLightning;

  cinematic_mode__dayTime = get_daskies_time();

  cinematic_mode__weatherPreset = get_current_weather_preset_name();
}

ECS_ON_EVENT(on_appear)
ECS_TRACK(cinematic_mode__vignetteStrength,
  cinematic_mode__lenseFlareIntensity,
  cinematic_mode__lenseDust,
  cinematic_mode__chromaticAberration,
  cinematic_mode__filmGrain,
  cinematic_mode__fps,
  cinematic_mode__subPixels,
  cinematic_mode__fname,
  cinematic_mode__bitrate,
  cinematic_mode__audioDataFName,
  cinematic_mode__superPixels,
  cinematic_mode__enablePostBloom)
static void cinematic_mode_settings_changed_es_event_handler(const ecs::Event &,
  CinematicMode &cinematic_mode__manager,
  const float cinematic_mode__vignetteStrength,
  const float cinematic_mode__lenseFlareIntensity,
  ecs::string &cinematic_mode__lenseCoveringTex,
  ecs::string &cinematic_mode__lenseRadialTex,
  const bool cinematic_mode__lenseDust,
  const Point3 cinematic_mode__chromaticAberration,
  const Point3 cinematic_mode__filmGrain,
  const int cinematic_mode__fps,
  const int cinematic_mode__subPixels,
  const int cinematic_mode__superPixels,
  const ecs::string &cinematic_mode__fname,
  const int cinematic_mode__bitrate,
  const ecs::string &cinematic_mode__audioDataFName,
  const bool cinematic_mode__enablePostBloom)
{
  CinematicMode &cm = cinematic_mode__manager;

  cm.setVignetteStrength(cinematic_mode__vignetteStrength);
  cm.setChromaticAberration(cinematic_mode__chromaticAberration);
  cm.setFilmGrain(cinematic_mode__filmGrain);
  cm.setFps(cinematic_mode__fps);
  cm.setSubPixels(cinematic_mode__subPixels);
  cm.setSuperPixels(cinematic_mode__superPixels);
  cm.setLenseFlareIntesity(cinematic_mode__lenseFlareIntensity);
  cm.initLenseFlare(cinematic_mode__lenseCoveringTex.c_str(), cinematic_mode__lenseRadialTex.c_str());
  cm.toggleLenseFlareCovering(cinematic_mode__lenseDust);
  cm.setFName(cinematic_mode__fname.c_str());
  cm.setBitrate(cinematic_mode__bitrate);
  cm.setAudioDataFName(cinematic_mode__audioDataFName.c_str());
  cm.enablePostBloomEffects(cinematic_mode__enablePostBloom);
}

template <typename Callable>
static void get_bloom_threshold_ecs_query(Callable);

template <typename Callable>
static void set_bloom_threshold_ecs_query(Callable);

ECS_ON_EVENT(on_appear)
static void cinematic_mode_save_bloom_threshold_es(
  const ecs::Event &, float &cinematic_mode__saved_bloom_threshold, float cinematic_mode__default_bloom_threshold = 1.0)
{
  get_bloom_threshold_ecs_query([&](float &bloom__threshold) {
    cinematic_mode__saved_bloom_threshold = bloom__threshold;
    bloom__threshold = cinematic_mode__default_bloom_threshold;
  });
}

ECS_ON_EVENT(on_disappear)
static void cinematic_mode_restore_bloom_threshold_es(const ecs::Event &, float cinematic_mode__saved_bloom_threshold)
{
  set_bloom_threshold_ecs_query([=](float &bloom__threshold) { bloom__threshold = cinematic_mode__saved_bloom_threshold; });
}

ECS_TAG(render)
ECS_NO_ORDER
static void flare_render_es(
  const RenderLateTransEvent &evt, CinematicMode &cinematic_mode__manager, const float &cinematic_mode__lenseFlareIntensity)
{
  CinematicMode &cm = cinematic_mode__manager;
  if (cinematic_mode__lenseFlareIntensity <= 0.0)
    return;
  cm.renderLenseFlare(evt.prevFrameTex);
}

template <typename Callable>
ECS_REQUIRE(ecs::Tag tonemapper)
static void delete_color_grading_ecs_query(Callable c);

static void select_color_grading(const ecs::string &selected, const ecs::StringList &options)
{
  if (selected.empty())
    return;

  bool valid = false;
  for (const ecs::string &option : options)
  {
    if (option == selected)
    {
      valid = true;
      break;
    }
  }
  if (!valid)
    return;

  bool hasSelected = false;
  delete_color_grading_ecs_query([&](ecs::EntityId eid) {
    if (selected != g_entity_mgr->getTemplateName(g_entity_mgr->getEntityTemplateId(eid)))
      g_entity_mgr->destroyEntity(eid);
    else
      hasSelected = true;
  });

  if (!hasSelected)
    g_entity_mgr->createEntityAsync(selected.c_str(), {});
}

ECS_ON_EVENT(on_appear)
ECS_TRACK(cinematic_mode__colorGradingSelected)
static void cinematic_mode_color_grading_es_event_handler(const ecs::Event &,
  const ecs::string &cinematic_mode__colorGradingSelected,
  const ecs::SharedComponent<ecs::StringList> &cinematic_mode__colorGradingOptions)
{
  select_color_grading(cinematic_mode__colorGradingSelected, *cinematic_mode__colorGradingOptions);
}

template <typename Callable>
static void get_default_color_grade_ecs_query(Callable c);

void reset_default_color_grading()
{
  const ecs::StringList *options = nullptr;
  get_default_color_grade_ecs_query([&](const ecs::SharedComponent<ecs::StringList> &cinematic_mode__colorGradingOptions) {
    options = cinematic_mode__colorGradingOptions.get();
  });
  // Do not reset the color grading when quitting the game
  if (options)
  {
    G_ASSERT_RETURN(!options->empty(), );
    const ecs::string *selected = options->begin();
    select_color_grading(*selected, *options);
  }
}

static bool cinematic_mode_console_handler(const char *argv[], int argc)
{
  int found = 0;
  CONSOLE_CHECK_NAME("cinematic_mode", "enable", 1, 1)
  {
    ecs::ComponentsInitializer init;
    g_entity_mgr->createEntityAsync("cinematic_mode", eastl::move(init));
  }
  CONSOLE_CHECK_NAME("cinematic_mode", "disable", 1, 1)
  {
    ecs::EntityId cmId = g_entity_mgr->getSingletonEntity(ECS_HASH("cinematic_mode"));
    if (cmId)
      g_entity_mgr->destroyEntity(cmId);
  }
  return found;
}

REGISTER_CONSOLE_HANDLER(cinematic_mode_console_handler);