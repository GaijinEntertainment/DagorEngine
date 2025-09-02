// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "render/wind/ambientWind.h"

#include <shaders/dag_shaders.h>
#include <drv/3d/dag_driver.h>
#include <math/dag_mathUtils.h>
#include <gameRes/dag_gameResources.h>
#include <3d/dag_lockTexture.h>

#include <nodeBasedShaderManager/nodeBasedShaderManager.h>

static constexpr float AMBIENT_WIND_PERIOD_SECONDS = 2000.0f;

#define GLOBAL_VARS_AMBIENT_WIND_LIST                                  \
  VAR(ambient_wind__speed__current_time__previous_time)                \
  VAR(ambient_wind_noise__speed__scale__perpendicular__strength)       \
  VAR(ambient_wind_grass_noise__speed__scale__perpendicular__strength) \
  VAR(ambient_wind_map_scale__offset)                                  \
  VAR(ambient_wind_tex)                                                \
  VAR(ambient_wind_tex_samplerstate)                                   \
  VAR(wind_perlin_tex)                                                 \
  VAR(wind_perlin_tex_samplerstate)

#define VAR(a) static int a##VarId = -1;
GLOBAL_VARS_AMBIENT_WIND_LIST
#undef VAR

float AmbientWind::beaufort_to_meter_per_second(float beaufort)
{
  if (abs(beaufort) > REAL_EPS)
    return powf(beaufort, 1.5f) * 0.836f;
  else
    return 0;
}

float AmbientWind::meter_per_second_to_beaufort(float mps)
{
  if (abs(mps) > REAL_EPS)
    return powf(mps * (1 / 0.836f), 2.0f / 3.0f);
  else
    return 0;
}

void AmbientWind::setWindParametersToShader(float wind_noise_strength_multiplier, float wind_noise_speed_beaufort,
  float wind_noise_scale, float wind_noise_perpendicular, const Point4 &wind_map_borders)
{
  float windNoiseSpeed = wind_noise_speed_beaufort;
  noiseStrength = wind_noise_strength_multiplier;
  float noisePerpendicular = noiseStrength * wind_noise_perpendicular;

  float noiseScale;
  if (wind_noise_scale > REAL_EPS)
  {
    noiseScale = 1.0 / wind_noise_scale;
  }
  else
  {
    noiseScale = 0;
  }

  ShaderGlobal::set_color4(ambient_wind_noise__speed__scale__perpendicular__strengthVarId,
    Color4(-windNoiseSpeed, noiseScale, noisePerpendicular, noiseStrength));
  ShaderGlobal::set_color4(ambient_wind_map_scale__offsetVarId,
    Color4(1.0 / (wind_map_borders.z - wind_map_borders.x), 1.0 / (wind_map_borders.w - wind_map_borders.y),
      -wind_map_borders.x / (wind_map_borders.z - wind_map_borders.x),
      -wind_map_borders.y / (wind_map_borders.w - wind_map_borders.y)));

  AmbientWind::update();
}

void AmbientWind::setWindParametersToShader(const AmbientWindParameters &params)
{
  float windGrassNoiseSpeed = params.windGrassNoiseSpeed;
  float grassNoiseStrength = params.windGrassNoiseStrength;
  float grassNoisePerpendicular = grassNoiseStrength * params.windGrassNoisePerpendicular;
  float grassNoiseScale;
  if (params.windGrassNoiseScale > REAL_EPS)
  {
    grassNoiseScale = 1.0 / params.windGrassNoiseScale;
  }
  else
  {
    grassNoiseScale = 0;
  }
  ShaderGlobal::set_color4(ambient_wind_grass_noise__speed__scale__perpendicular__strengthVarId,
    Color4(-windGrassNoiseSpeed, grassNoiseScale, grassNoisePerpendicular, grassNoiseStrength));

  setWindParametersToShader(params.windNoiseStrength, params.windNoiseSpeed, params.windNoiseScale, params.windNoisePerpendicular,
    params.windMapBorders);
}

void AmbientWind::fillFlowmapTexFallback(const Point2 &wind_dir)
{
  if (auto lockedTex = lock_texture<E3DCOLOR>(flowmapTexFallback.getTex2D(), 0, TEXLOCK_WRITE))
  {
    lockedTex.at(0, 0) = E3DCOLOR_MAKE(real2uchar(0.5f * wind_dir.x + 0.5f), real2uchar(0.5f * wind_dir.y + 0.5f), 0, 255);
  }
}

void AmbientWind::init()
{
#define VAR(a) a##VarId = get_shader_variable_id(#a, true);
  GLOBAL_VARS_AMBIENT_WIND_LIST
#undef VAR
}

void AmbientWind::setWindParameters(const DataBlock &blk)
{
  float azimuth = blk.getReal("wind__dir", 40);
  setWindParameters(blk.getPoint4("wind__left_top_right_bottom", Point4{-2048, -2048, 2048, 2048}),
    Point2(cosf(DegToRad(azimuth)), sinf(DegToRad(azimuth))), blk.getReal("wind__strength", 0), blk.getReal("wind__noiseStrength", 2),
    blk.getReal("wind__noiseSpeed", 1), blk.getReal("wind__noiseScale", 70), blk.getReal("wind__noisePerpendicular", 0.5));
}

void AmbientWind::setWindTextures(const DataBlock &blk)
{
  eastl::string wind_flowmap_name = blk.getStr("wind__flowMap", "");
  eastl::string wind_noisemap_name = blk.getStr("wind__noiseMap", "");
  setWindTextures(wind_flowmap_name.c_str(), wind_noisemap_name.c_str());
}


void AmbientWind::setWindParameters(const Point4 &wind_map_borders, const Point2 &wind_dir, float wind_strength_beaufort,
  float wind_noise_strength_multiplier, float wind_noise_speed_beaufort, float wind_noise_scale, float wind_noise_perpendicular)
{
  windDir = wind_dir;
  windSpeed = beaufort_to_meter_per_second(wind_strength_beaufort);
  windStrength = wind_strength_beaufort;

  setWindParametersToShader(wind_noise_strength_multiplier, wind_noise_speed_beaufort, wind_noise_scale, wind_noise_perpendicular,
    wind_map_borders);
  parametersInitialized = true;
  enable();
}

void AmbientWind::setWindParametersWithGrass(const AmbientWindParameters &params)
{
  windDir = params.windDir;
  windSpeed = beaufort_to_meter_per_second(params.windStrength);
  windStrength = params.windStrength;

  setWindParametersToShader(params);
  parametersInitialized = true;
  enable();
}

void AmbientWind::setWindTextures(const char *wind_flowmap_name, const char *wind_noisemap_name)
{
  bool needReset = noisemapName != wind_noisemap_name;
  bool hasNoiseMap = wind_noisemap_name;
  if (needReset)
  {
    noisemapName = wind_noisemap_name;
    NodeBasedShaderManager::clearAllCachedResources();

    if (hasNoiseMap)
    {
      noisemapGameresTex = dag::get_tex_gameres(wind_noisemap_name);
      if (!noisemapGameresTex)
        logerr("Could not load noise wind texture '%s'", wind_noisemap_name);
      prefetch_managed_texture(noisemapGameresTex.getTexId());
    }

    ShaderGlobal::set_texture(wind_perlin_texVarId, hasNoiseMap ? noisemapGameresTex.getTexId() : BAD_TEXTUREID);
    ShaderGlobal::set_sampler(wind_perlin_tex_samplerstateVarId,
      hasNoiseMap ? get_texture_separate_sampler(noisemapGameresTex.getTexId()) : d3d::INVALID_SAMPLER_HANDLE);
  }

  needReset = flowmapName != wind_flowmap_name || !flowmapTexFallback;
  bool hasFlowmap = wind_flowmap_name && wind_flowmap_name[0];
  if (needReset)
  {
    flowmapName = wind_flowmap_name;
    NodeBasedShaderManager::clearAllCachedResources();

    if (!flowmapTexFallback)
      flowmapTexFallback = dag::create_tex(nullptr, 1, 1, TEXFMT_A8R8G8B8, 1, "ambient_wind_tex_fallback");

    if (hasFlowmap)
    {
      flowmapGameresTex = dag::get_tex_gameres(wind_flowmap_name);
      G_ASSERTF(flowmapGameresTex, "Could not load ambient wind texture '%s'", wind_flowmap_name);
      prefetch_managed_texture(flowmapGameresTex.getTexId());
    }
    else
    {
      G_ASSERTF(flowmapTexFallback, "Could not load ambient wind texture (default)"); // justincase
    }

    ShaderGlobal::set_texture(ambient_wind_texVarId, hasFlowmap ? flowmapGameresTex.getTexId() : flowmapTexFallback.getTexId());
    ShaderGlobal::set_sampler(ambient_wind_tex_samplerstateVarId, d3d::request_sampler({}));
  }

  if (!hasFlowmap)
  {
    if (!parametersInitialized)
      logwarn("Setting flowmap fallback with default wind dir parameter - Point2(%.2f, %.2f)", windDir.x, windDir.y);
    fillFlowmapTexFallback(windDir);
  }
}

void AmbientWind::close()
{
  ShaderGlobal::set_texture(ambient_wind_texVarId, BAD_TEXTUREID);
  ShaderGlobal::set_texture(wind_perlin_texVarId, BAD_TEXTUREID);
  flowmapGameresTex.close();
  noisemapGameresTex.close();
  flowmapTexFallback.close();
  noisemapName = "";
}

float AmbientWind::getWindStrength() const { return windStrength; }

const Point2 &AmbientWind::getWindDir() const { return windDir; }

float AmbientWind::getWindSpeed() const { return windSpeed; }

void AmbientWind::update()
{
  if (!enabled)
    return;
  float previousTime = windParams.g;
  windParams =
    Color4(windSpeed, get_shader_global_time_phase(AMBIENT_WIND_PERIOD_SECONDS, 0) * AMBIENT_WIND_PERIOD_SECONDS, previousTime, 0);
  ShaderGlobal::set_color4(ambient_wind__speed__current_time__previous_timeVarId, windParams);
}

void AmbientWind::enable() { enabled = true; }
void AmbientWind::disable()
{
  enabled = false;
  float time = get_shader_global_time_phase(AMBIENT_WIND_PERIOD_SECONDS, 0) * AMBIENT_WIND_PERIOD_SECONDS;
  Color4 params = Color4(0, time, time, 0);
  ShaderGlobal::set_color4(ambient_wind__speed__current_time__previous_timeVarId, params);
}


// This functions are for use in non-ECS projects only
static eastl::unique_ptr<AmbientWind> ambient_wind;

AmbientWind *get_ambient_wind() { return ambient_wind.get(); }
void init_ambient_wind(const DataBlock &blk)
{
  ambient_wind.reset(new AmbientWind());
  ambient_wind->setWindParameters(blk);
  ambient_wind->setWindTextures(blk);
}
void init_ambient_wind(const char *wind_flowmap_name, const Point4 &wind_map_borders, const Point2 &wind_dir,
  float wind_strength_beaufort, float wind_noise_strength_multiplier, float wind_noise_speed_beaufort, float wind_noise_scale,
  float wind_noise_perpendicular, const char *wind_noisemap_name)
{
  ambient_wind.reset(new AmbientWind());
  ambient_wind->setWindParameters(wind_map_borders, wind_dir, wind_strength_beaufort, wind_noise_strength_multiplier,
    wind_noise_speed_beaufort, wind_noise_scale, wind_noise_perpendicular);
  ambient_wind->setWindTextures(wind_flowmap_name, wind_noisemap_name);
}
void init_ambient_wind_with_grass(const AmbientWindParameters &params, const char *wind_flowmap_name, const char *wind_noisemap_name)
{
  ambient_wind.reset(new AmbientWind());
  ambient_wind->setWindParametersWithGrass(params);
  ambient_wind->setWindTextures(wind_flowmap_name, wind_noisemap_name);
}
void close_ambient_wind()
{
  if (ambient_wind)
    ambient_wind->close();
  ambient_wind.reset();
}
void update_ambient_wind()
{
  if (ambient_wind)
    ambient_wind->update();
}
void enable_ambient_wind()
{
  if (ambient_wind)
    ambient_wind->enable();
}
void disable_ambient_wind()
{
  if (ambient_wind)
    ambient_wind->disable();
}
