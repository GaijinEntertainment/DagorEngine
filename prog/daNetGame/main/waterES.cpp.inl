// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ecs/core/entityManager.h>
#include <daECS/core/coreEvents.h>
#include <ecs/core/attributeEx.h>
#include <fftWater/fftWater.h>
#include <perfMon/dag_statDrv.h>
#include <EASTL/unique_ptr.h>

#include "net/time.h"
#include "net/dedicated.h"
#include "main/level.h"
#include "main/water.h"
#include "render/renderer.h"
#include <gamePhys/collision/collisionLib.h>
#include <gamePhys/phys/rendinstFloating.h>
#include <daECS/core/updateStage.h>
#include <startup/dag_globalSettings.h>
#include <ioSys/dag_dataBlock.h>

static FFTWater *dng_create_water()
{
  const int resBits = dgs_get_settings()->getBlockByNameEx("graphics")->getInt("fftResBits", 7);
  const float cascadeSize = dgs_get_settings()->getBlockByNameEx("graphics")->getReal("fftCascadeSize", 170.f);
  FFTWater *water =
    fft_water::create_water(fft_water::DONT_RENDER, clamp(cascadeSize, 32.f, 1024.f), clamp(resBits, (int)6, (int)8), true, true);
  dacoll::set_water_source(water);
  fft_water::create_flowmap(water);
  return water;
}
static void dng_destroy_water(FFTWater *water)
{
  if (!water)
    return;
  if (IRenderWorld *wr = get_world_renderer())
    wr->setWater(nullptr);
  fft_water::remove_flowmap(water);
  dacoll::set_water_source(nullptr);
  fft_water::delete_water(water);
}

class WaterCTM final : public ecs::ComponentTypeManager
{
public:
  typedef FFTWater component_type;

  WaterCTM() { fft_water::init(); }
  ~WaterCTM() { fft_water::close(); }
  void create(void *d, ecs::EntityManager &, ecs::EntityId, const ecs::ComponentsMap &, ecs::component_index_t) override
  {
    *(ecs::PtrComponentType<FFTWater>::ptr_type(d)) = dng_create_water();
  }

  void destroy(void *d) override { dng_destroy_water(*(ecs::PtrComponentType<FFTWater>::ptr_type(d))); }
};

ECS_REGISTER_MANAGED_TYPE(FFTWater, nullptr, WaterCTM);
ECS_AUTO_REGISTER_COMPONENT(FFTWater, "water", nullptr, 0);

static void water_es(const ecs::UpdateStageInfoAct &, FFTWater &water)
{
  TIME_D3D_PROFILE(water_simulate);
  float curTime = get_sync_time();
  if (curTime >= 0.f) // client doesn't have sync time until it received from server
    fft_water::simulate(&water, curTime);
}

ECS_TAG(gameClient)
ECS_BEFORE(water_strength_es)
ECS_ON_EVENT(on_appear, EventLevelLoaded)
static void set_water_on_level_loaded_es(const ecs::Event &, FFTWater &water)
{
  IRenderWorld *wr = get_world_renderer();
  if (!wr)
    return;

  if (wr->getWater())
    return;

  wr->setWater(&water);
}

ECS_ON_EVENT(on_appear)
ECS_AFTER(set_water_on_level_loaded_es)
ECS_TRACK(water__level)
static void water_level_es_event_handler(const ecs::Event &, FFTWater &water, float water__level)
{
  fft_water::set_level(&water, water__level);
}

ECS_ON_EVENT(on_appear)
ECS_TRACK(water__fft_resolution)
static void water_fft_resolution_es_event_handler(const ecs::Event &, FFTWater &water, int water__fft_resolution)
{
  // limit the resolution by 8 (which is 256x256) for perfomance reasons
  // and values greater than 8 is used mostly for big oceans in the ship simulators
  fft_water::set_fft_resolution(&water, min(water__fft_resolution, 8));
}

ECS_ON_EVENT(on_appear)
ECS_TRACK(water__flowmap,
  water__flowmap_tex,
  water__flowmap_area,
  water__wind_strength,
  water__flowmap_range,
  water__flowmap_fading,
  water__flowmap_damping,
  water__flowmap_prebaked_speed,
  water__flowmap_prebaked_foam_scale,
  water__flowmap_prebaked_foam_power,
  water__flowmap_prebaked_foamfx,
  water__flowmap_min_distance,
  water__flowmap_max_distance,
  water__flowmap_simulated_speed,
  water__flowmap_simulated_foam_scale)
static void water_flowmap_es_event_handler(const ecs::Event &,
  FFTWater &water,
  bool water__flowmap,
  ecs::string &water__flowmap_tex,
  const Point4 &water__flowmap_area,
  float water__wind_strength,
  float water__flowmap_range,
  float water__flowmap_fading,
  float water__flowmap_damping,
  float water__flowmap_prebaked_speed,
  float water__flowmap_prebaked_foam_scale,
  float water__flowmap_prebaked_foam_power,
  float water__flowmap_prebaked_foamfx,
  float water__flowmap_min_distance,
  float water__flowmap_max_distance,
  float water__flowmap_simulated_speed,
  float water__flowmap_simulated_foam_scale)
{
  fft_water::WaterFlowmap *waterFlowmap = fft_water::get_flowmap(&water);
  if (waterFlowmap)
  {
    String tex = String(water__flowmap_tex.c_str());
    Point4 area = water__flowmap_area;
    area.z -= area.x;
    area.w -= area.y;
    G_ASSERT(area.z && area.w);
    if (area.z && area.w)
      area = Point4(1.0f / area.z, 1.0f / area.w, -area.x / area.z, -area.y / area.w);
    Point4 flowmap_strength(water__flowmap_prebaked_speed, water__flowmap_prebaked_foam_scale, water__flowmap_prebaked_foam_power,
      water__flowmap_prebaked_foamfx);
    Point4 flowmap_strength_add(water__flowmap_min_distance, water__flowmap_max_distance,
      water__flowmap_simulated_speed * (1 - water__flowmap_damping), water__flowmap_simulated_foam_scale);

    waterFlowmap->enabled = water__flowmap;
    waterFlowmap->texName = tex;
    waterFlowmap->texArea = area;
    waterFlowmap->windStrength = water__wind_strength;
    waterFlowmap->flowmapRange = water__flowmap_range;
    waterFlowmap->flowmapFading = water__flowmap_fading;
    waterFlowmap->flowmapDamping = water__flowmap_damping;
    waterFlowmap->flowmapStrength = flowmap_strength;
    waterFlowmap->flowmapStrengthAdd = flowmap_strength_add;

    fft_water::set_flowmap_tex(&water);
    fft_water::set_flowmap_params(&water);
  }
}

ECS_ON_EVENT(on_appear)
ECS_TRACK(water__flowmap_foam_power,
  water__flowmap_foam_scale,
  water__flowmap_foam_threshold,
  water__flowmap_foam_reflectivity,
  water__flowmap_foam_reflectivity_min,
  water__flowmap_foam_color,
  water__flowmap_foam_tiling,
  water__flowmap_speed_depth_scale,
  water__flowmap_foam_speed_scale,
  water__flowmap_speed_depth_max,
  water__flowmap_foam_depth_max,
  water__flowmap_slope,
  water__has_slopes,
  water__flowmap_detail)
static void water_flowmap_foam_es_event_handler(const ecs::Event &,
  FFTWater &water,
  float water__flowmap_foam_power,
  float water__flowmap_foam_scale,
  float water__flowmap_foam_threshold,
  float water__flowmap_foam_reflectivity,
  float water__flowmap_foam_reflectivity_min,
  const Point3 &water__flowmap_foam_color,
  float water__flowmap_foam_tiling,
  float water__flowmap_speed_depth_scale,
  float water__flowmap_foam_speed_scale,
  float water__flowmap_speed_depth_max,
  float water__flowmap_foam_depth_max,
  float water__flowmap_slope,
  bool water__has_slopes,
  bool water__flowmap_detail)
{
  fft_water::WaterFlowmap *waterFlowmap = fft_water::get_flowmap(&water);
  if (waterFlowmap)
  {
    Point4 flowmap_foam(water__flowmap_foam_power, water__flowmap_foam_scale, water__flowmap_foam_threshold,
      water__flowmap_foam_reflectivity);
    Point4 flowmap_depth(water__flowmap_speed_depth_scale, water__flowmap_foam_speed_scale, water__flowmap_speed_depth_max,
      water__flowmap_foam_depth_max);

    waterFlowmap->flowmapFoam = flowmap_foam;
    waterFlowmap->flowmapFoamColor = water__flowmap_foam_color;
    waterFlowmap->flowmapFoamTiling = water__flowmap_foam_tiling;
    waterFlowmap->flowmapFoamReflectivityMin = water__flowmap_foam_reflectivity_min;
    waterFlowmap->flowmapDepth = flowmap_depth;
    waterFlowmap->flowmapSlope = water__flowmap_slope;
    waterFlowmap->flowmapDetail = water__flowmap_detail;
    waterFlowmap->hasSlopes = water__has_slopes;

    fft_water::set_flowmap_foam_params(&water);
  }
}

ECS_REQUIRE(const FFTWater &water)
ECS_ON_EVENT(on_appear, EventLevelLoaded)
ECS_TRACK(shore__texture_size, shore__enabled, shore__hmap_size, shore__rivers_width, shore__significant_wave_threshold, )
static void water_shore_setup_es_event_handler(const ecs::Event &,
  int shore__texture_size,
  bool shore__enabled,
  float shore__hmap_size,
  float shore__rivers_width,
  float shore__significant_wave_threshold)
{
  if (IRenderWorld *wr = get_world_renderer())
  {
    wr->setupShore(shore__enabled, shore__texture_size, shore__hmap_size, shore__rivers_width, shore__significant_wave_threshold);
  }
}

ECS_REQUIRE(const FFTWater &water)
ECS_ON_EVENT(on_appear, EventLevelLoaded)
ECS_TRACK(shore__wave_height_to_amplitude,
  shore__amplitude_to_length,
  shore__parallelism_to_wind,
  shore__width_k,
  shore__waves_dist,
  shore__waves_depth_min,
  shore__waves_depth_fade_interval,
  shore__wave_gspeed)
static void water_shore_surf_setup_es_event_handler(const ecs::Event &,
  float shore__wave_height_to_amplitude,
  float shore__amplitude_to_length,
  float shore__parallelism_to_wind,
  float shore__width_k,
  const Point4 &shore__waves_dist,
  float shore__waves_depth_min,
  float shore__waves_depth_fade_interval,
  float shore__wave_gspeed)
{
  if (IRenderWorld *wr = get_world_renderer())
    wr->setupShoreSurf(shore__wave_height_to_amplitude, shore__amplitude_to_length, shore__parallelism_to_wind, shore__width_k,
      shore__waves_dist, shore__waves_depth_min, shore__waves_depth_fade_interval, shore__wave_gspeed);
}

ECS_ON_EVENT(on_appear, EventLevelLoaded)
ECS_TRACK(water__wind_dir, water__strength, water__oneToFourFFTPeriodicDivisor)
static void water_strength_es_event_handler(const ecs::Event &,
  FFTWater &water,
  float water__wind_dir,
  float water__strength,
  int water__max_tessellation,
  float water__oneToFourFFTPeriodicDivisor = 1.0f)
{
  float windDirX = cosf(DegToRad(water__wind_dir)), windDirZ = sinf(DegToRad(water__wind_dir));
  fft_water::set_wind(&water, water__strength, Point2(windDirX, windDirZ));

  fft_water::apply_wave_preset(&water, water__strength, Point2(windDirX, windDirZ), fft_water::Spectrum::PHILLIPS);

  if (fft_water::one_to_four_render_enabled(&water))
    // scale FFT period down so 0 cascade have something, instead of being empty
    fft_water::set_period(&water, fft_water::get_period(&water) / water__oneToFourFFTPeriodicDivisor);

  if (IRenderWorld *wr = get_world_renderer())
    wr->setMaxWaterTessellation(water__max_tessellation);
}
