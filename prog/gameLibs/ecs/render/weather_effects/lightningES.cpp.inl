// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <math/random/dag_random.h>
#include <ecs/anim/anim.h>
#include <daECS/core/coreEvents.h>
#include <ecs/core/entityManager.h>
#include <perfMon/dag_cpuFreq.h>
#include <perfMon/dag_statDrv.h>
#include <math/dag_mathBase.h>
#include <math/dag_noise.h>
#include <util/dag_convar.h>
#include <daSkies2/daSkies.h>

CONSOLE_INT_VAL("lightning", high_quality_frames, 12, 0, 30);
CONSOLE_INT_VAL("lightning", steps_per_sequence, 4, 1, 16);
CONSOLE_FLOAT_VAL_MINMAX("lightning", min_new_frame_weight, 0.7, 0, 1);

CONSOLE_BOOL_VAL("lightning", debug_mode, false);
CONSOLE_FLOAT_VAL("lightning", debug_strike_time, 1.0f);
CONSOLE_FLOAT_VAL("lightning", debug_sleep_time, 2.0f);
CONSOLE_FLOAT_VAL("lightning", debug_azimuth, -1.0f);
CONSOLE_FLOAT_VAL("lightning", debug_distance, 6000.0f);

#define GLOBAL_VARS_LIST                                     \
  VAR(lightning_point_light_pos)                             \
  VAR(lightning_point_light_color)                           \
  VAR(lightning_render_additional_point_light)               \
  VAR(lightning_additional_point_light_radius)               \
  VAR(lightning_additional_point_light_strength)             \
  VAR(lightning_additional_point_light_extinction_threshold) \
  VAR(lightning_additional_point_light_natural_fade)         \
  VAR(lightning_in_clouds)                                   \
  VAR(lightning_current_bottom)                              \
  VAR(lightning_emissive_multiplier)                         \
  VAR(lightning_vert_noise_scale)                            \
  VAR(lightning_vert_noise_strength)                         \
  VAR(lightning_vert_noise_time_multiplier)                  \
  VAR(lightning_vert_noise_speed)                            \
  VAR(lightning_step_size)                                   \
  VAR(lightning_inverse_height_difference)                   \
  VAR(lightning_top_height)                                  \
  VAR(lightning_scene_illumination)                          \
  VAR(lightning_scene_illumination_color)                    \
  VAR(lightning_scene_illumination_dir)                      \
  VAR(clouds_steps_per_sequence)                             \
  VAR(clouds_taa_min_new_frame_weight)


#define VAR(a) static int a##VarId = -1;
GLOBAL_VARS_LIST
#undef VAR

#define VAR(a) a##VarId = get_shader_variable_id(#a, true);
static void init_shader_vars() { GLOBAL_VARS_LIST; }
#undef VAR

DaSkies *lightning_get_skies();
int lightning_get_session_id();

static void get_debug_parameters(Point2 &azimuth_interval, Point2 &sleep_interval, Point2 &strike_interval, Point2 &distance_interval,
  float &series_probability, float &bolt_probability)
{
  if (debug_azimuth < 0.0f && lightning_get_skies())
  {
    Point3 sunDir = lightning_get_skies()->getSunDir();
    Point2 sunDir2D = Point2(sunDir.x, sunDir.z);
    sunDir2D.normalize();
    float azimuth = atan2f(sunDir2D.y, sunDir2D.x) / TWOPI;
    azimuth_interval = Point2(azimuth, azimuth);
  }
  else
    azimuth_interval = Point2(debug_azimuth, debug_azimuth);
  distance_interval = Point2(debug_distance, debug_distance);
  sleep_interval = Point2(debug_sleep_time, debug_sleep_time);
  strike_interval = Point2(debug_strike_time, debug_strike_time);
  series_probability = 0.0;
  bolt_probability = 1.0;
}

void setShaderEnabled(bool f) { ShaderGlobal::set_int(lightning_in_cloudsVarId, f); }

void setPointLightColor(Point3 p) { ShaderGlobal::set_color4(lightning_point_light_colorVarId, Point4::xyzV(p, 0)); }

void setPointLightPos(Point3 p) { ShaderGlobal::set_color4(lightning_point_light_posVarId, Point4::xyzV(p, 0)); }

void setPointLightEnable(bool render) { ShaderGlobal::set_int(lightning_render_additional_point_lightVarId, render); }


void setPointLightNaturalFade(bool render) { ShaderGlobal::set_int(lightning_additional_point_light_natural_fadeVarId, render); }

void setPointLightRadius(float r) { ShaderGlobal::set_real(lightning_additional_point_light_radiusVarId, r); }

void setPointLightStrength(float r) { ShaderGlobal::set_real(lightning_additional_point_light_strengthVarId, r); }

void setPointLightExtinctionThreshold(float r)
{
  ShaderGlobal::set_real(lightning_additional_point_light_extinction_thresholdVarId, r);
}

float getCloudsAlt()
{
  auto skies = lightning_get_skies();
  if (skies)
  {
    auto clouds_form = lightning_get_skies()->getCloudsForm();
    // mimics getCloudsStartAlt() from daskies, but with thickness term, since it also effects start alt
    return (clouds_form.layers[0].thickness / 10.0f + clouds_form.layers[0].startAt) * 1000.0f;
  }
  else
    return 0.0f;
}

struct LightningFX
{
  LightningFX()
  {
    // align mesh so that lightning origin is at the top.
    TMatrix rotyTM;
    rotyTM.identity();
    rotyTM.rotyTM((-90) * DEG_TO_RAD);
    TMatrix rotzTM;
    rotzTM.identity();
    rotzTM.rotzTM(-90.0f * DEG_TO_RAD);
    defaultRotTM = rotzTM * rotyTM;
    defaultStepsPerSequence = ShaderGlobal::get_real(clouds_steps_per_sequenceVarId);
  };
  LightningFX(LightningFX &&) = default;

  ~LightningFX()
  {
    ShaderGlobal::set_real(clouds_taa_min_new_frame_weightVarId, 0);
    ShaderGlobal::set_real(clouds_steps_per_sequenceVarId, defaultStepsPerSequence);
  };

  void updateTM(TMatrix &transform, const float azimuth, const float dist, const float meshOffset)
  {
    float cloudsHeight = getCloudsAlt();
    Point3 pos = Point3(cos(azimuth) * dist, cloudsHeight + meshOffset, sin(azimuth) * dist);
    TMatrix scaleTM;
    scaleTM.identity();
    scaleTM *= cloudsHeight + meshOffset;
    transform = getRotationMatrix(azimuth) * scaleTM;
    transform.setcol(3, pos);
  }

  void updateState(Point2 sleepInterval, Point2 strikeInterval, Point2 azimuthInterval, Point2 distanceInterval, Point2 radiusInterval,
    Point2 strengthInterval)
  {
    strikeStartTime = strikeEndTime + _rnd_float(rngSeed, sleepInterval.x, sleepInterval.y);
    strikeEndTime = strikeStartTime + _rnd_float(rngSeed, strikeInterval.x, strikeInterval.y);
    azimuthCurrent = _rnd_float(rngSeed, azimuthInterval.x, azimuthInterval.y);
    distanceCurrent = _rnd_float(rngSeed, distanceInterval.x, distanceInterval.y);
    radiusMultiplierCurrent = _rnd_float(rngSeed, radiusInterval.x, radiusInterval.y);
    strengthMultiplierCurrentSeries = _rnd_float(rngSeed, strengthInterval.x, strengthInterval.y);
  }

  void startLowQualityFrames()
  {
    ShaderGlobal::set_real(clouds_taa_min_new_frame_weightVarId, 0);
    ShaderGlobal::set_real(clouds_steps_per_sequenceVarId, defaultStepsPerSequence);
  }

  void startHighQualityFrames()
  {
    ShaderGlobal::set_real(clouds_taa_min_new_frame_weightVarId, min_new_frame_weight);
    ShaderGlobal::set_real(clouds_steps_per_sequenceVarId, steps_per_sequence);
  }

  float strengthCurrent = 0.0f;
  float azimuthCurrent = 0.0f;
  float distanceCurrent = 0.0f;
  float radiusMultiplierCurrent = 1.0f;
  float strengthMultiplierCurrentSeries = 1.0f;
  int seriesSizeCurrent = 1;

  float strikeStartTime = 0.0f;
  float strikeEndTime = 0.0f;
  float strikeFadeOutStartTime = 0.0f;

  int strikeNum = 0;

  int rngSeed = 0;

  bool started = false;
  bool isFadingOut = false;
  bool isNextSeries = false;
  bool finished = false;
  int finishedFrames = -1;

  uint32_t currentAnimcharId = -1;
  uint32_t animcharNum = 0;

  Point3 sunDir;
  bool doFlickering = false;

private:
  TMatrix defaultRotTM;

  int highQualityFramesRemaining = 0;
  float defaultStepsPerSequence = 2;

  TMatrix getRotationMatrix(float azimuth)
  {
    TMatrix azimuthTM2;
    azimuthTM2.identity();
    azimuthTM2.rotyTM(azimuth);
    return azimuthTM2 * defaultRotTM;
  }
};

ECS_DECLARE_RELOCATABLE_TYPE(LightningFX);
ECS_REGISTER_RELOCATABLE_TYPE(LightningFX, nullptr);

template <typename Callable>
static void enable_lightning_animchar_ecs_query(ecs::EntityId eid, Callable c);
template <typename Callable>
static void disable_lightning_animchar_ecs_query(ecs::EntityId eid, Callable c);
template <typename Callable>
static void get_animchar_eids_ecs_query(Callable c);
template <typename Callable>
static void get_lightnings_ecs_query(Callable c);

ECS_TAG(render)
ECS_ON_EVENT(on_disappear)
static void lightning_manager_destroy_es(const ecs::Event &, ecs::EidList &lightning__animchars_eids_base, ecs::EntityManager &manager)
{
  setShaderEnabled(false);
  auto skies = lightning_get_skies();

  ShaderGlobal::set_int(lightning_scene_illuminationVarId, 0);

  if (skies)
    skies->enablePanoramaDownsampledDepth(false);
  for (auto eid : lightning__animchars_eids_base)
    manager.destroyEntity(eid);
}

ECS_TAG(render)
ECS_ON_EVENT(on_appear)
static void lightning_manager_created_es(const ecs::Event &, const ecs::StringList &lightning__animchars,
  ecs::EidList &lightning__animchars_eids_base, ecs::EntityManager &manager)
{
  init_shader_vars();
  auto skies = lightning_get_skies();
  if (skies)
    skies->enablePanoramaDownsampledDepth(true);

  setShaderEnabled(true);

  for (auto eid : lightning__animchars_eids_base)
    manager.destroyEntity(eid);
  lightning__animchars_eids_base.clear();
  lightning__animchars_eids_base.reserve(lightning__animchars.size());

  ShaderGlobal::set_int(lightning_scene_illuminationVarId, 0);

  for (auto res : lightning__animchars)
  {
    ecs::ComponentsInitializer attrs;
    ECS_INIT(attrs, animchar__res, res);
    ECS_INIT(attrs, animchar_render__enabled, false);
    lightning__animchars_eids_base.push_back(manager.createEntityAsync("lightning_animchar", eastl::move(attrs)));
  }

  // find lightnings replicated before manager
  get_lightnings_ecs_query(
    [&](ecs::EidList &lightning__animchars_eids) { lightning__animchars_eids = lightning__animchars_eids_base; });
}

ECS_TAG(render)
ECS_ON_EVENT(on_appear)
static void lightning_created_es(const ecs::Event &, LightningFX &lightning, ecs::EidList &lightning__animchars_eids)
{
  lightning__animchars_eids.clear();
  get_animchar_eids_ecs_query(
    [&](ecs::EidList &lightning__animchars_eids_base) { lightning__animchars_eids = lightning__animchars_eids_base; });

  lightning.animcharNum = lightning__animchars_eids.size();
  lightning.currentAnimcharId = 0;
  lightning.rngSeed = lightning_get_session_id();
}

ECS_TAG(render)
ECS_NO_ORDER
static void lightning_update_es(const ecs::UpdateStageInfoAct &evt, LightningFX &lightning, TMatrix &transform,
  bool lightning__is_volumetric, Point2 lightning__base_strike_time_interval, Point2 lightning__base_sleep_time_interval,
  Point2 lightning__base_distance_interval, Point2 lightning__base_azimuth_interval, float lightning__base_offset,
  Point2 lightning__series_radius_interval, Point2 lightning__series_strength_interval, Point2 lightning__series_size_interval,
  Point2 lightning__series_sleep_time_interval, Point2 lightning__series_strike_time_interval,
  float lightning__series_distance_deviation, float lightning__series_azimuth_deviation, float lightning__series_fadeout_time,
  float lightning__series_probability, bool lightning__series_create_bolt, float lightning__bolt_probability,
  float lightning__bolt_strike_time, int lightning__bolt_step_size, float lightning__emissive_multiplier,
  float lightning__emissive_fadein_time, float lightning__emissive_fadeout_time, float lightning__vert_noise_scale,
  float lightning__vert_noise_strength, float lightning__vert_noise_time, float lightning__vert_noise_speed,
  float lightning__point_light_fadeout_time, float lightning__point_light_flickering_probability,
  float lightning__point_light_flickering_speed, float lightning__point_light_offset, float lightning__point_light_radius,
  Point2 lightning__point_light_strength_interval, float lightning__point_light_extinction_threshold,
  bool lightning__point_light_natural_fade, Point3 lightning__point_light_color, float lightning__scene_illumination_multiplier,
  bool lightning__scene_illumination_enable_for_flash, float lightning__scene_illumination_near_sun_threshold,
  ecs::EidList &lightning__animchars_eids)
{
  auto skies = lightning_get_skies();
  if (!skies || !lightning.animcharNum)
    return;

  if (debug_mode)
    get_debug_parameters(lightning__base_azimuth_interval, lightning__base_sleep_time_interval, lightning__base_strike_time_interval,
      lightning__base_distance_interval, lightning__series_probability, lightning__bolt_probability);

  float currentTime = evt.curTime;
  lightning.sunDir = skies->getSunDir();

  // make sure clouds are fully update before disabling high quality frames
  // otherwise there could be lightspots.
  if (lightning.finishedFrames > high_quality_frames)
  {
    lightning.startLowQualityFrames();
    lightning.finished = false;
    lightning.finishedFrames = 0;
    return;
  }

  if (lightning.finished)
  {
    setPointLightEnable(false);
    lightning.finishedFrames++;
    return;
  }

  // restrict execution to only one of lightning entities dependin on current skies setting.
  bool earlyExitOnUnmatchedSkies = lightning__is_volumetric != !lightning_get_skies()->panoramaEnabled();
  if (earlyExitOnUnmatchedSkies)
  {
    // disable current animchar when swtiching skies settings
    if (lightning.started)
    {
      lightning.started = false;
      if (lightning__animchars_eids.empty())
        return;
      disable_lightning_animchar_ecs_query(lightning__animchars_eids[lightning.currentAnimcharId],
        [&](bool &animchar_render__enabled) { animchar_render__enabled = false; });
    }
    return;
  }
  bool isSeries = lightning.isNextSeries;

  // enable lightning effect if currently in between strike start time and end time
  if (lightning.strikeStartTime < currentTime && currentTime < lightning.strikeEndTime && !lightning.isFadingOut)
  {
    if (!lightning.started)
    {
      float timeDiff = lightning.strikeEndTime - lightning.strikeStartTime;
      lightning.strikeStartTime = currentTime;
      lightning.strikeEndTime = currentTime + timeDiff;

      lightning.startHighQualityFrames();
      lightning.started = true;
      lightning.updateTM(transform, lightning.azimuthCurrent * TWOPI, lightning.distanceCurrent, lightning__base_offset);
      bool doBolt = lightning__bolt_probability > _rnd_float(lightning.rngSeed, 0.0f, 1.0f);
      doBolt = (doBolt && lightning__series_create_bolt) || (doBolt && !isSeries);
      float cloudsHeight = getCloudsAlt();
      Point3 lightPos = transform.getcol(3);
      lightPos.y = cloudsHeight + lightning__point_light_offset;
      Point2 lightPos2D = Point2(lightPos.x, lightPos.z);
      Point2 sunDir2D = Point2(lightning.sunDir.x, lightning.sunDir.z);
      lightPos2D.normalize();
      sunDir2D.normalize();
      bool isNearSun = dot(lightPos2D, sunDir2D) > cos(lightning__scene_illumination_near_sun_threshold * DEG_TO_RAD);
      lightning.doFlickering = lightning__point_light_flickering_probability > _rnd_float(lightning.rngSeed, 0.0f, 1.0f);
      if ((doBolt || lightning__scene_illumination_enable_for_flash) && isNearSun)
      {
        Point3 towardsLightDir = lightPos;
        towardsLightDir.normalize();
        ShaderGlobal::set_int(lightning_scene_illuminationVarId, 1);
        ShaderGlobal::set_color4(lightning_scene_illumination_dirVarId, towardsLightDir);
      }
      if (lightning__animchars_eids.empty())
        return;

      ecs::EntityId eid = lightning__animchars_eids[lightning.currentAnimcharId];
      enable_lightning_animchar_ecs_query(eid,
        [&, &parentTransform = transform](bool &animchar_render__enabled, float &animchar_render__dist_sq, TMatrix &transform) {
          transform = parentTransform;

          float height = transform.getcol(3).y;
          float padding = 1000;

          if (doBolt)
          {
            animchar_render__dist_sq =
              (lightning__base_distance_interval.y * lightning__base_distance_interval.y) + (height * height) + (padding * padding);
            animchar_render__enabled = lightning.started;
          }

          // update shader params here so that flash and mesh would enable at the same time
          setPointLightPos(lightPos);
          setPointLightColor(lightning__point_light_color);
          setPointLightRadius(lightning__point_light_radius * lightning.radiusMultiplierCurrent);
          setPointLightStrength(lightning__point_light_strength_interval.x * lightning.strengthMultiplierCurrentSeries);
          setPointLightExtinctionThreshold(lightning__point_light_extinction_threshold);
          setPointLightNaturalFade(lightning__point_light_natural_fade);
          ShaderGlobal::set_real(lightning_current_bottomVarId, floor(cloudsHeight / lightning__bolt_step_size));
          ShaderGlobal::set_real(lightning_vert_noise_scaleVarId, lightning__vert_noise_scale);
          ShaderGlobal::set_real(lightning_vert_noise_strengthVarId, lightning__vert_noise_strength);
          ShaderGlobal::set_real(lightning_vert_noise_speedVarId, lightning__vert_noise_speed);

          float heightEps = 1.0f;
          ShaderGlobal::set_real(lightning_inverse_height_differenceVarId,
            1.0f / (eastl::max(transform.getcol(3).y - cloudsHeight, 0.0f) + heightEps));
          ShaderGlobal::set_real(lightning_top_heightVarId, transform.getcol(3).y);

          ShaderGlobal::set_real(lightning_step_sizeVarId, 1.0f / float(lightning__bolt_step_size));
          setPointLightEnable(true);

          lightning.strengthCurrent = lightning__point_light_strength_interval.x;
        });
    }

    float bottomLerpVal = (currentTime - lightning.strikeStartTime) / lightning__bolt_strike_time;
    float currentNoise = eastl::max(1 - (currentTime - lightning.strikeStartTime) / lightning__vert_noise_time, 0.0f);
    float currentBottom = eastl::max(lerp(getCloudsAlt(), 0.0f, bottomLerpVal), 0.0f);
    ShaderGlobal::set_real(lightning_current_bottomVarId, floor(currentBottom / lightning__bolt_step_size));
    ShaderGlobal::set_real(lightning_vert_noise_time_multiplierVarId, currentNoise);

    float emissiveLerpFadeInVal = clamp((currentTime - lightning.strikeStartTime) / lightning__emissive_fadein_time, 0.0f, 1.0f);
    float emissiveLerpFadeOutVal = clamp((lightning.strikeEndTime - currentTime) / lightning__emissive_fadeout_time, 0.0f, 1.0f);
    float emissiveLerpValue = clamp(emissiveLerpFadeInVal * emissiveLerpFadeOutVal, 0.0f, 1.0f);
    float emissiveCurrent = eastl::max(lerp(0.0f, lightning__emissive_multiplier, emissiveLerpValue), 0.0f);
    ShaderGlobal::set_real(lightning_emissive_multiplierVarId, emissiveCurrent);

    float strengthCurrent =
      lerp(lightning__point_light_strength_interval.x, lightning__point_light_strength_interval.y, emissiveLerpValue) *
      lightning.strengthMultiplierCurrentSeries;

    strengthCurrent *=
      lightning.doFlickering ? (perlin_noise::noise1(currentTime * lightning__point_light_flickering_speed) + 1.0f) / 2.0f : 1.0f;
    setPointLightStrength(strengthCurrent);

    Point3 currentIllumColor = lightning__point_light_color;
    float illumMulCurrent = lerp(0.0f, lightning__scene_illumination_multiplier, emissiveLerpValue);
    ShaderGlobal::set_color4(lightning_scene_illumination_colorVarId, currentIllumColor * strengthCurrent * illumMulCurrent);
    return;
  }

  // disable lightning effects after strike is finished
  if (lightning.started)
  {
    lightning.started = false;
    lightning.isFadingOut = true;
    lightning.strikeFadeOutStartTime = currentTime;
    ShaderGlobal::set_int(lightning_scene_illuminationVarId, 0);

    if (lightning__animchars_eids.empty())
      return;
    disable_lightning_animchar_ecs_query(lightning__animchars_eids[lightning.currentAnimcharId],
      [&](bool &animchar_render__enabled) { animchar_render__enabled = false; });
  }

  // player, who joined after some time, will catch up and will have lightning synchronized; catching up 2 hours takes ~3usec
  while (lightning.strikeEndTime < currentTime)
  {
    // Determine if next strike would be a part of series or not
    if (!isSeries)
    {
      lightning.isNextSeries = _rnd_float(lightning.rngSeed, 0.0f, 1.0f) < lightning__series_probability;
      // Reset series counter on start
      if (lightning.isNextSeries)
      {
        lightning.seriesSizeCurrent =
          _rnd_int(lightning.rngSeed, lightning__series_size_interval.x, lightning__series_size_interval.y);
        lightning.strikeNum = 1;
      }
    }
    else
      lightning.isNextSeries = lightning.seriesSizeCurrent > ++lightning.strikeNum;

    Point2 sleepInterval, strikeInterval, azimuthInterval, distanceInterval, radiusInterval, strengthInterval;

    if (isSeries)
    {
      sleepInterval = lightning__series_sleep_time_interval;
      azimuthInterval = Point2(lightning.azimuthCurrent - lightning__series_azimuth_deviation,
        lightning.azimuthCurrent + lightning__series_azimuth_deviation);
      distanceInterval = Point2(lightning.distanceCurrent - lightning__series_distance_deviation,
        lightning.distanceCurrent + lightning__series_distance_deviation);
    }
    else
    {
      sleepInterval = lightning__base_sleep_time_interval;
      azimuthInterval = lightning__base_azimuth_interval;
      distanceInterval = lightning__base_distance_interval;
    }

    if (lightning.isNextSeries)
    {
      strikeInterval = lightning__series_strike_time_interval;
      radiusInterval = lightning__series_radius_interval;
      strengthInterval = lightning__series_strength_interval;
    }
    else
    {
      strikeInterval = lightning__base_strike_time_interval;
      radiusInterval = Point2(1.0f, 1.0f);
      strengthInterval = Point2(1.0f, 1.0f);
    }
    lightning.updateState(sleepInterval, strikeInterval, azimuthInterval, distanceInterval, radiusInterval, strengthInterval);

    lightning.currentAnimcharId =
      (lightning.currentAnimcharId + _rnd_int(lightning.rngSeed, 1, lightning.animcharNum - 1)) % (lightning.animcharNum);
  }

  // Update point light radius and strength for fadeout effect
  if (!lightning.isFadingOut)
    return;

  float strengthInitial = lightning__point_light_strength_interval.x;
  float fadeoutTime = isSeries ? lightning__series_fadeout_time : lightning__point_light_fadeout_time;
  float strengthLerpVal = (lightning.strikeFadeOutStartTime + fadeoutTime - currentTime) / lightning__point_light_fadeout_time;
  lightning.strengthCurrent = eastl::max(lerp(0.0f, strengthInitial, strengthLerpVal), 0.0f);
  setPointLightStrength(lightning.strengthCurrent);
  constexpr float stopStrength = 0.01;
  if (lightning.strengthCurrent < stopStrength)
  {
    setPointLightStrength(0.0f);
    lightning.isFadingOut = false;
    lightning.finished = true;
    lightning.finishedFrames = 0;
  }
}
