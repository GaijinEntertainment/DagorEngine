// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ecs/weather/skiesSettings.h>

#include <ecs/core/entityManager.h>
#include <daECS/core/coreEvents.h>
#include <ecs/core/attributeEx.h>
#include <daSkies2/daSkies.h>
#include <daSkies2/daScattering.h>
#include <dag_noise/dag_uint_noise.h>
#include <math/random/dag_random.h>


ECS_REGISTER_EVENT(EventSkiesLoaded)
ECS_REGISTER_EVENT(CmdSkiesInvalidate)


// Based on get_point2lerp_or_real_noise (without the preset selection)
static float get_randomized_float_from_point2(const Point2 &v12, uint32_t seed, uint32_t at)
{
  unsigned int noiseValAt1 = uint_noise1D(at * 2 + 1, seed);
  float lerpVal = (noiseValAt1 & 0xFFFFFF) / float(1 << 24); // mantissa is 24 bit
  return lerp(v12.x, v12.y, lerpVal);
}
#define RANDOMIZE_PARAM(var, str) \
  get_randomized_float_from_point2(var, weatherSeed, eastl::integral_constant<uint32_t, str_hash_fnv1(str)>::value)
#define STRATA_CLOUDS(name)      RANDOMIZE_PARAM(strata_clouds__##name, #name)
#define CLOUDS_WEATHER_GEN(name) RANDOMIZE_PARAM(clouds_weather_gen__##name, #name)
#define CLOUDS_FORM(name)        RANDOMIZE_PARAM(clouds_form__##name, #name)
#define SKY_ATMOSPHERE(name)     RANDOMIZE_PARAM(sky_atmosphere__##name, #name)
#define LAYER(name)                                                                                   \
  get_randomized_float_from_point2(layer.getMemberOr<Point2>(ECS_HASH(#name), Point2()), weatherSeed, \
    str_hash_fnv1(String(0, "layers[%d].%s", i, #name))) // Runtime hash to allow getting values in for loop

#define INIT_SEED() uint32_t weatherSeed = get_daskies_seed();


ECS_REQUIRE(ecs::Tag skies_settings_tag)
ECS_ON_EVENT(on_appear)
ECS_BEFORE(strata_clouds_es, clouds_form_es, clouds_weather_gen_es, sky_atmosphere_es)
static void skies_settings_weather_seed_created_es_event_handler(const ecs::Event &, int &skies_settings__weatherSeed)
{
  if (skies_settings__weatherSeed < 0)
    skies_settings__weatherSeed = dagor_random::grnd();
  set_daskies_seed(skies_settings__weatherSeed);
}

ECS_REQUIRE(ecs::Tag skies_settings_tag)
ECS_TRACK(skies_settings__weatherSeed)
static void skies_settings_weather_seed_changed_es_event_handler(const ecs::Event &, int skies_settings__weatherSeed)
{
  set_daskies_seed(skies_settings__weatherSeed);
}

// This is to track other means of seed changing in the entity.
// This will be called if this entity changed the seed, but because the value is same before and after
// the change event shouldn't trigger again.
ECS_REQUIRE(ecs::Tag skies_settings_tag)
ECS_BEFORE(strata_clouds_es, clouds_form_es, clouds_weather_gen_es, sky_atmosphere_es)
static void skies_settings_skies_loaded_es_event_handler(const EventSkiesLoaded &, int &skies_settings__weatherSeed)
{
  skies_settings__weatherSeed = get_daskies_seed();
}


// clouds_rendering
ECS_REQUIRE(ecs::Tag skies_settings_tag)
ECS_TRACK(*)
ECS_ON_EVENT(on_appear, EventSkiesLoaded)
static __forceinline void clouds_rendering_es_event_handler(const ecs::Event &, float clouds_rendering__forward_eccentricity,
  float clouds_rendering__back_eccentricity, float clouds_rendering__forward_eccentricity_weight,
  float clouds_rendering__erosion_noise_size, float clouds_rendering__ambient_desaturation, float clouds_rendering__ms_contribution,
  float clouds_rendering__ms_attenuation, float clouds_rendering__ms_ecc_attenuation, float clouds_rendering__erosionWindSpeed)
{
  DaSkies *skies = get_daskies_impl();
  if (!skies)
    return;

  DaSkies::CloudsRendering cloudsRendering;
  cloudsRendering.forward_eccentricity = clouds_rendering__forward_eccentricity;
  cloudsRendering.back_eccentricity = clouds_rendering__back_eccentricity;
  cloudsRendering.forward_eccentricity_weight = clouds_rendering__forward_eccentricity_weight;
  cloudsRendering.erosion_noise_size = clouds_rendering__erosion_noise_size;
  cloudsRendering.ambient_desaturation = clouds_rendering__ambient_desaturation;
  cloudsRendering.ms_contribution = clouds_rendering__ms_contribution;
  cloudsRendering.ms_attenuation = clouds_rendering__ms_attenuation;
  cloudsRendering.ms_ecc_attenuation = clouds_rendering__ms_ecc_attenuation;
  cloudsRendering.erosionWindSpeed = clouds_rendering__erosionWindSpeed;
  skies->setCloudsRendering(cloudsRendering);
  g_entity_mgr->broadcastEventImmediate(CmdSkiesInvalidate());
}

// strata_clouds
ECS_REQUIRE(ecs::Tag skies_settings_tag)
ECS_TRACK(*)
ECS_ON_EVENT(on_appear, EventSkiesLoaded)
static __forceinline void strata_clouds_es_event_handler(const ecs::Event &, const Point2 &strata_clouds__amount,
  const Point2 &strata_clouds__altitude)
{
  DaSkies *skies = get_daskies_impl();
  if (!skies)
    return;
  INIT_SEED();

  DaSkies::StrataClouds strataClouds;
  strataClouds.amount = clamp(STRATA_CLOUDS(amount), 0.f, 1.f);
  strataClouds.altitude = STRATA_CLOUDS(altitude);
  skies->setStrataClouds(strataClouds);
  g_entity_mgr->broadcastEventImmediate(CmdSkiesInvalidate());
}

// strata_clouds_texture
ECS_REQUIRE(ecs::Tag skies_settings_tag)
ECS_TRACK(*)
ECS_ON_EVENT(on_appear, EventSkiesLoaded)
static void strata_clouds_tex_es(const ecs::Event &, const ecs::string &strata_clouds__tex)
{
  DaSkies *skies = get_daskies_impl();
  if (!skies)
    return;

  skies->setStrataCloudsTexture(strata_clouds__tex.c_str());
  g_entity_mgr->broadcastEventImmediate(CmdSkiesInvalidate());
};

void validate_volumetric_clouds_settings();

// clouds_form
ECS_REQUIRE(ecs::Tag skies_settings_tag)
ECS_TRACK(*)
ECS_ON_EVENT(on_appear, EventSkiesLoaded)
static __forceinline void clouds_form_es_event_handler(const ecs::Event &, const ecs::Array &clouds_form__layers,
  const Point2 &clouds_form__extinction, const Point2 &clouds_form__turbulenceStrength, int clouds_form__shapeNoiseScale,
  int clouds_form__cumulonimbusShapeScale, int clouds_form__turbulenceFreq)
{
  DaSkies *skies = get_daskies_impl();
  if (!skies)
    return;
  INIT_SEED();

  validate_volumetric_clouds_settings();

  DaSkies::CloudsForm cloudsForm;

  for (int i = 0; i < clouds_form__layers.size(); i++)
  {
    ecs::Object layer = clouds_form__layers[i].get<ecs::Object>();

    cloudsForm.layers[i].thickness = LAYER(thickness);
    cloudsForm.layers[i].startAt = LAYER(startAt);
    cloudsForm.layers[i].density = LAYER(density);
    cloudsForm.layers[i].clouds_type = LAYER(clouds_type);
    cloudsForm.layers[i].clouds_type_variance = LAYER(clouds_type_variance);
  }
  cloudsForm.extinction = CLOUDS_FORM(extinction);
  cloudsForm.turbulenceStrength = CLOUDS_FORM(turbulenceStrength);
  cloudsForm.shapeNoiseScale = clouds_form__shapeNoiseScale;
  cloudsForm.cumulonimbusShapeScale = clouds_form__cumulonimbusShapeScale;
  cloudsForm.turbulenceFreq = clouds_form__turbulenceFreq;
  skies->setCloudsForm(cloudsForm);
  g_entity_mgr->broadcastEventImmediate(CmdSkiesInvalidate());
}

// clouds_settings
ECS_REQUIRE(ecs::Tag skies_settings_tag)
ECS_TRACK(*)
ECS_ON_EVENT(on_appear, EventSkiesLoaded)
static __forceinline void clouds_settings_es_event_handler(const ecs::Event &, float clouds_settings__maximum_averaging_ratio,
  int clouds_settings__quality, int clouds_settings__target_quality, bool clouds_settings__competitive_advantage,
  bool clouds_settings__fastEvolution)
{
  DaSkies *skies = get_daskies_impl();
  if (!skies)
    return;

  DaSkies::CloudsSettingsParams cloudsSettings;
  cloudsSettings.maximum_averaging_ratio = clouds_settings__maximum_averaging_ratio;
  cloudsSettings.quality = clouds_settings__quality;
  cloudsSettings.target_quality = clouds_settings__target_quality;
  cloudsSettings.competitive_advantage = clouds_settings__competitive_advantage;
  cloudsSettings.fastEvolution = clouds_settings__fastEvolution;
  skies->setCloudsGameSettingsParams(cloudsSettings);
  g_entity_mgr->broadcastEventImmediate(CmdSkiesInvalidate());
}

// clouds_weather_gen
ECS_REQUIRE(ecs::Tag skies_settings_tag)
ECS_TRACK(*)
ECS_ON_EVENT(on_appear, EventSkiesLoaded)
static __forceinline void clouds_weather_gen_es_event_handler(const ecs::Event &, const ecs::Array &clouds_weather_gen__layers,
  float clouds_weather_gen__epicness, const Point2 &clouds_weather_gen__cumulonimbusCoverage,
  const Point2 &clouds_weather_gen__cumulonimbusSeed, float clouds_weather_gen__worldSize)
{
  DaSkies *skies = get_daskies_impl();
  if (!skies)
    return;
  INIT_SEED();

  DaSkies::CloudsWeatherGen cloudsWeatherGen;

  for (int i = 0; i < clouds_weather_gen__layers.size(); i++)
  {
    ecs::Object layer = clouds_weather_gen__layers[i].get<ecs::Object>();
    cloudsWeatherGen.layers[i].coverage = LAYER(coverage);
    cloudsWeatherGen.layers[i].freq = LAYER(freq);
    cloudsWeatherGen.layers[i].seed = LAYER(seed);
  }
  cloudsWeatherGen.epicness = clouds_weather_gen__epicness;
  cloudsWeatherGen.cumulonimbusCoverage = CLOUDS_WEATHER_GEN(cumulonimbusCoverage);
  cloudsWeatherGen.cumulonimbusSeed = CLOUDS_WEATHER_GEN(cumulonimbusSeed);
  cloudsWeatherGen.worldSize = clouds_weather_gen__worldSize;
  skies->setWeatherGen(cloudsWeatherGen);
  g_entity_mgr->broadcastEventImmediate(CmdSkiesInvalidate());
}

// sky_atmosphere
ECS_REQUIRE(ecs::Tag skies_settings_tag)
ECS_TRACK(*)
ECS_ON_EVENT(on_appear, EventSkiesLoaded)
static __forceinline void sky_atmosphere_es_event_handler(const ecs::Event &, float sky_atmosphere__average_ground_albedo,
  float sky_atmosphere__min_ground_offset, float sky_settings__haze_strength, float sky_settings__haze_min_angle,
  float sky_settings__haze_peak_hour_offset, const Point3 &sky_atmosphere__ground_color, const Point2 &sky_atmosphere__mie2_thickness,
  const Point2 &sky_atmosphere__mie2_altitude, const Point2 &sky_atmosphere__mie2_scale, const Point2 &sky_atmosphere__mie_height,
  const Point2 &sky_atmosphere__mie_scale, const Point2 &sky_atmosphere__mie_absorption_scale,
  const Point3 &sky_atmosphere__mie_scattering_color, const Point3 &sky_atmosphere__mie_absorption_color,
  const Point2 &sky_atmosphere__mie_assymetry, const Point2 &sky_atmosphere__mie_back_assymetry, float sky_atmosphere__planet_scale,
  float sky_atmosphere__atmosphere_scale, const Point2 &sky_atmosphere__rayleigh_scale,
  const Point2 &sky_atmosphere__rayleigh_alt_scale, const Point3 &sky_atmosphere__rayleigh_color,
  float sky_atmosphere__multiple_scattering_factor, float sky_atmosphere__ozone_alt_dist, float sky_atmosphere__ozone_max_alt,
  const Point2 &sky_atmosphere__ozone_scale, float sky_atmosphere__sun_brightness, float sky_atmosphere__moon_brightness,
  const Point3 &sky_atmosphere__moon_color, bool sky_disable_sky_influence = false)
{
  DaSkies *skies = get_daskies_impl();
  if (!skies)
    return;
  INIT_SEED();

  SkyAtmosphereParams skyAtmosphereParams;

  skyAtmosphereParams.average_ground_albedo = sky_atmosphere__average_ground_albedo;
  skyAtmosphereParams.min_ground_offset = sky_atmosphere__min_ground_offset;
  skyAtmosphereParams.ground_color = Color3::xyz(sky_atmosphere__ground_color);
  skyAtmosphereParams.mie2_thickness = SKY_ATMOSPHERE(mie2_thickness);
  skyAtmosphereParams.mie2_altitude = SKY_ATMOSPHERE(mie2_altitude);
  skyAtmosphereParams.mie2_scale = SKY_ATMOSPHERE(mie2_scale);
  skyAtmosphereParams.mie_height = SKY_ATMOSPHERE(mie_height);
  skyAtmosphereParams.mie_scale = SKY_ATMOSPHERE(mie_scale);
  skyAtmosphereParams.mie_absorption_scale = SKY_ATMOSPHERE(mie_absorption_scale);
  skyAtmosphereParams.mie_scattering_color = Color3::xyz(sky_atmosphere__mie_scattering_color);
  skyAtmosphereParams.mie_absorption_color = Color3::xyz(sky_atmosphere__mie_absorption_color);
  skyAtmosphereParams.mie_assymetry = SKY_ATMOSPHERE(mie_assymetry);
  skyAtmosphereParams.mie_back_assymetry = SKY_ATMOSPHERE(mie_back_assymetry);
  skyAtmosphereParams.planet_scale = sky_atmosphere__planet_scale;
  skyAtmosphereParams.atmosphere_scale = sky_atmosphere__atmosphere_scale;
  skyAtmosphereParams.rayleigh_scale = SKY_ATMOSPHERE(rayleigh_scale);
  skyAtmosphereParams.rayleigh_alt_scale = SKY_ATMOSPHERE(rayleigh_alt_scale);
  skyAtmosphereParams.rayleigh_color = Color3::xyz(sky_atmosphere__rayleigh_color);
  skyAtmosphereParams.multiple_scattering_factor = sky_atmosphere__multiple_scattering_factor;
  skyAtmosphereParams.ozone_alt_dist = sky_atmosphere__ozone_alt_dist;
  skyAtmosphereParams.ozone_max_alt = sky_atmosphere__ozone_max_alt;
  skyAtmosphereParams.ozone_scale = SKY_ATMOSPHERE(ozone_scale);
  skyAtmosphereParams.sun_brightness = sky_atmosphere__sun_brightness;
  skyAtmosphereParams.haze_strength = sky_settings__haze_strength;
  skyAtmosphereParams.haze_min_angle = sky_settings__haze_min_angle;
  skyAtmosphereParams.haze_peak_hour_offset = sky_settings__haze_peak_hour_offset;
  skyAtmosphereParams.moon_brightness = sky_atmosphere__moon_brightness;
  skyAtmosphereParams.moon_color = Color3::xyz(sky_atmosphere__moon_color);

  if (sky_disable_sky_influence)
  {
    skyAtmosphereParams.ground_color.zero();
    skyAtmosphereParams.rayleigh_color.zero();
    skyAtmosphereParams.sun_brightness = 0;
    skyAtmosphereParams.moon_brightness = 0;
    skyAtmosphereParams.moon_color.zero();
  }

  skies->setSkyParams(skyAtmosphereParams);
  g_entity_mgr->broadcastEventImmediate(CmdSkiesInvalidate());
}

ECS_REQUIRE(ecs::Tag skies_settings_tag)
ECS_TRACK(use_clouds_hole)
ECS_ON_EVENT(on_appear, EventSkiesLoaded)
static __forceinline void use_clouds_hole_es_event_handler(const ecs::Event &, bool use_clouds_hole)
{
  if (DaSkies *skies = get_daskies_impl(); skies)
    skies->setUseCloudsHole(use_clouds_hole);
}

template <typename Callable>
static void is_cloud_hole_enabled_for_preset_ecs_query(Callable c);

ECS_REQUIRE(ecs::Tag clouds_hole_tag)
ECS_ON_EVENT(on_appear, EventSkiesLoaded)
ECS_TRACK(transform, density)
static __forceinline void clouds_hole_es_event_handler(const ecs::Event &, const TMatrix &transform, const float &density)
{
  DaSkies *skies = get_daskies_impl();
  if (!skies)
    return;

  bool enabled = false;
  is_cloud_hole_enabled_for_preset_ecs_query([&](bool level__cloudsHoleEnabled) { enabled = level__cloudsHoleEnabled; });

  if (!enabled)
    skies->resetCloudsHole();
  else
    skies->setCloudsHole(transform.getcol(3), density);

  g_entity_mgr->broadcastEventImmediate(CmdSkiesInvalidate());
}

ECS_REQUIRE(ecs::Tag clouds_hole_tag)
ECS_ON_EVENT(on_disappear)
ECS_TAG(render)
static __forceinline void disappear_clouds_hole_es(const ecs::Event &)
{
  DaSkies *skies = get_daskies_impl();
  if (!skies)
    return;

  skies->resetCloudsHole();
  g_entity_mgr->broadcastEventImmediate(CmdSkiesInvalidate());
}

ECS_REQUIRE(ecs::Tag moon_params)
ECS_ON_EVENT(on_appear, EventSkiesLoaded)
static __forceinline void moon_params_es_event_handler(const ecs::Event &, const ecs::string &moon_res_name, float moon_size)
{
  DaSkies *skies = get_daskies_impl();
  if (!skies)
    return;
  skies->setMoonResourceName(moon_res_name.c_str());
  skies->setMoonSize(moon_size);
  g_entity_mgr->broadcastEventImmediate(CmdSkiesInvalidate());
}

ECS_TAG(dev)
ECS_TRACK(randomize_seed_button)
ECS_REQUIRE(ecs::Tag skies_settings_tag)
static void randomize_skies_setting_seed_es_event_handler(const ecs::Event &, bool randomize_seed_button,
  int &skies_settings__weatherSeed)
{
  G_UNUSED(randomize_seed_button); // Value doesn't matter, it's changing is just used to trigger this event.
  skies_settings__weatherSeed = dagor_random::grnd();
}

template <typename Callable>
ECS_REQUIRE(ecs::Tag skies_settings_tag)
static void is_panorama_forced_ecs_query(Callable c);

bool is_panorama_forced()
{
  bool forcePanorama = false;
  is_panorama_forced_ecs_query([&](bool clouds_settings__force_panorama) { forcePanorama = clouds_settings__force_panorama; });
  return forcePanorama;
}

template <typename Callable>
ECS_REQUIRE(ecs::Tag skies_settings_tag)
void validate_volumetric_clouds_settings_ecs_query(Callable c);

void validate_volumetric_clouds_settings()
{
  DaSkies *skies = get_daskies_impl();
  if (!skies)
    return;
  validate_volumetric_clouds_settings_ecs_query([&](const ecs::Array &clouds_form__layers, bool clouds_settings__force_panorama) {
    // thickness can affect volumetric clouds worse than panoramic, hiding wrong params, so we
    // only check if volumetric can be turned on in release, or is turned on through console commands
    if (!clouds_settings__force_panorama || !skies->panoramaEnabled())
    {
      for (int i = 0; i < clouds_form__layers.size(); i++)
      {
        ecs::Object layer = clouds_form__layers[i].get<ecs::Object>();

        const Point2 &thicknessRange = layer.getMemberOr<Point2>(ECS_HASH("thickness"), Point2());
        float minCloudThickness = skies->getMinCloudThickness();
        if (thicknessRange.x < minCloudThickness || thicknessRange.y < minCloudThickness)
        {
          eastl::string extraInfo = clouds_settings__force_panorama
                                      ? "This error message shouldn't happen on release builds, because "
                                        "clouds_settings__force_panorama is true. However volumetric clouds may be broken now."
                                      : "Either increase thickness, or enable clouds_settings__force_panorama for the preset!";
          logerr("Cloud layer thickness must be at least %f. It has a range of [%f,%f] on layer %d!\n%s", minCloudThickness,
            thicknessRange.x, thicknessRange.y, i, extraInfo.c_str());
        }
      }
    }
  });
}