// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "global_vars.h"
#include <perfMon/dag_statDrv.h>
#include <shaders/dag_shaderBlock.h>
#include <drv/3d/dag_matricesAndPerspective.h>
#include <webui/editVarPlugin.h>
#include <fftWater/fftWater.h>
#include <heightmap/heightmapCulling.h>
#include <render/world/wrDispatcher.h>
#include "landMeshToHeightmapRenderer.h"
#include "shoreRenderer.h"

ShoreRenderer::ShoreRenderer() {}

void ShoreRenderer::buildUI()
{
  const GuiControlDescWebUi landDesc[] = {
    DECLARE_BOOL_BUTTON(land_panel, generate_shore, false),
  };
  de3_webui_build(landDesc);
}

void ShoreRenderer::setupShore(bool enabled, int texture_size, float hmap_size, float rivers_width, float significant_wave_threshold)
{
  if (shoreEnabled == enabled && shoreTextureSize == texture_size && shoreHmapSize == hmap_size && shoreRiversWidth == rivers_width &&
      shoreSignificantWaveThreshold == significant_wave_threshold)
    return;

  land_panel.generate_shore = true;
  shoreEnabled = enabled;
  shoreTextureSize = texture_size;
  shoreHmapSize = hmap_size;
  shoreRiversWidth = rivers_width;
  shoreSignificantWaveThreshold = significant_wave_threshold;
}

void ShoreRenderer::setupShoreSurf(float wave_height_to_amplitude,
  float amplitude_to_length,
  float parallelism_to_wind,
  float width_k,
  const Point4 &waves_dist,
  float depth_min,
  float depth_fade_interval,
  float gerstner_speed,
  float rivers_wave_multiplier)
{
  ShaderGlobal::set_real(shore_wave_height_to_amplitudeVarId, wave_height_to_amplitude);
  ShaderGlobal::set_real(shore_amplitude_to_lengthVarId, amplitude_to_length);
  ShaderGlobal::set_real(shore_parallelism_to_windVarId, parallelism_to_wind);
  ShaderGlobal::set_real(shore_width_kVarId, width_k);
  ShaderGlobal::set_color4(shore__waves_distVarId, Color4::xyzw(waves_dist));
  ShaderGlobal::set_real(shore__waves_depth_minVarId, depth_min);
  ShaderGlobal::set_real(shore__waves_depth_fade_intervalVarId, depth_fade_interval);
  ShaderGlobal::set_real(shore_gerstner_speedVarId, gerstner_speed);
  ShaderGlobal::set_real(shore_rivers_wave_multiplierVarId, rivers_wave_multiplier);
}

void ShoreRenderer::buildShore(const FFTWater *water)
{
  const float HMAP_TEX_SIZE_SHORE_MUL = 1.0f;

  int textureSize = shoreTextureSize;
  int hmapTextureSize = shoreTextureSize * HMAP_TEX_SIZE_SHORE_MUL;

  shoreHeightmapTex.close();
  shoreHeightmapTex = dag::create_tex(nullptr, hmapTextureSize, hmapTextureSize, TEXCF_RTARGET | TEXFMT_DEPTH16, 1, "heightmap_tex");
  {
    d3d::SamplerInfo smpInfo;
    smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Border;
    smpInfo.border_color = d3d::BorderColor::Color::TransparentBlack;
    d3d::SamplerHandle sampler = d3d::request_sampler(smpInfo);
    ShaderGlobal::set_sampler(get_shader_variable_id("heightmap_tex_samplerstate"), sampler);
    ShaderGlobal::set_sampler(get_shader_variable_id("flowmap_heightmap_tex_samplerstate"), sampler);
  }

  if (!shoreHeightmapTex)
  {
    logerr("can't create heightmap texture");
    return;
  }

  float waterLevel = fft_water::get_level(water);
  float dimensions = 50;
  Color4 flowmapHeightmapVar = Color4(1.0f / dimensions, 0.5f, dimensions, -0.5f * dimensions);
  const fft_water::WaterHeightmap *whm = fft_water::get_heightmap(water);
  float waterLevelMin = waterLevel;
  float waterLevelMax = waterLevel;
  if (whm)
  {
    waterLevelMin = min(waterLevelMin, whm->heightOffset);
    waterLevelMax = max(waterLevelMax, whm->heightOffset + whm->heightScale);
  }
  float heightMin = waterLevelMin - 0.5f * dimensions;
  float heightMax = waterLevelMax + 0.5f * dimensions;
  float heightScale = heightMax - heightMin;
  float heightOffset = heightMin;
  Color4 shoreHeightmapVar = Color4(1.0f / heightScale, -heightOffset / heightScale, heightScale, heightOffset);
  ShaderGlobal::set_color4(get_shader_variable_id("heightmap_min_max", true), shoreHeightmapVar);

  Point3 origin(0, 0, 0);
  Point4 world_to_heightmap;
  const float water_level = water ? fft_water::get_level(water) : HeightmapHeightCulling::NO_WATER_ON_LEVEL;
  render_landmesh_to_heightmap(shoreHeightmapTex.getTex2D(), shoreHmapSize, Point2::xz(origin), NULL, water_level, world_to_heightmap);
  ShaderGlobal::set_color4(get_shader_variable_id("world_to_heightmap", true), world_to_heightmap);

  // TextureInfo tinfo;
  // heightmap.getTex2D()->getinfo(tinfo, 0);
  float rivers_width = shoreRiversWidth;
  ShaderGlobal::set_color4(get_shader_variable_id("shore_heightmap_min_max", true), shoreHeightmapVar);

  // ShaderGlobal::set_color4( get_shader_variable_id("shore_heightmap_min_max"),
  //   1/hmap.getHeightScale(), -hmap.getHeightMin()/hmap.getHeightScale(),0,0);
  // Point3 worldPosOfs = hmap.getHeightmapOffset();
  // Point2 worldSize = hmap.getWorldSize();
  // ShaderGlobal::set_color4( get_shader_variable_id("world_to_heightmap"),
  //   Color4(1.0f/worldSize.x, 1.0f/worldSize.y, -worldPosOfs.x/worldSize.x, -worldPosOfs.z/worldSize.y));
  d3d::resource_barrier({shoreHeightmapTex.getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});

  debug("Building shore: textureSize=%d, hmapTextureSize=%d", textureSize, hmapTextureSize);

  fft_water::build_distance_field(shoreDistanceField, textureSize, hmapTextureSize, rivers_width, NULL);

  ShaderGlobal::set_texture(get_shader_variable_id("flowmap_heightmap_tex", true), shoreHeightmapTex);
  ShaderGlobal::set_int(get_shader_variable_id("flowmap_heightmap_texture_size"), hmapTextureSize);
  ShaderGlobal::set_color4(get_shader_variable_id("flowmap_heightmap_min_max", true), flowmapHeightmapVar);
  ShaderGlobal::set_color4(get_shader_variable_id("world_to_flowmap_heightmap", true), world_to_heightmap);
}

bool ShoreRenderer::getNeedShore(const FFTWater *water) const
{
  if (!shoreEnabled)
    return false;

  auto lmeshMgr = WRDispatcher::getLandMeshManager();
  if (!lmeshMgr || !water)
    return false;

  fft_water::WaterFlowmap *waterFlowmap = fft_water::get_flowmap(water);
  if (waterFlowmap && waterFlowmap->hasSlopes)
    return true;

  if (isForcingWaterWaves)
    return true;

  return fft_water::get_significant_wave_height(water) >= fft_water::get_shore_wave_threshold(water);
}

void ShoreRenderer::updateShore(FFTWater *water, int32_t water_quality, const DPoint3 &cameraWorldPos)
{
  if (water)
  {
    bool needShore = getNeedShore(water);
    bool hasShore = fft_water::is_shore_enabled(water);
    G_ASSERT(hasShore == (shoreDistanceField.getTexId() != BAD_TEXTUREID));

    if (needShore && (!hasShore || land_panel.generate_shore))
    {
      debug("enabling shore, significantWave is %f", fft_water::get_significant_wave_height(water));
      TIME_D3D_PROFILE(generate_shore);
      fft_water::shore_enable(water, true);
      buildShore(water);
    }
    else if (!needShore && hasShore)
    {
      debug("disabling shore, significantWave is %f", fft_water::get_significant_wave_height(water));
      fft_water::shore_enable(water, false);
      closeShoreAndWaterFlowmap(water);
    }

    fft_water::set_shore_wave_threshold(water, shoreSignificantWaveThreshold);

    if (needShore && (hasShore || land_panel.generate_shore))
    {
      fft_water::WaterFlowmap *waterFlowmap = fft_water::get_flowmap(water);
      if (waterFlowmap && waterFlowmap->enabled && (waterFlowmap->flowmapWaveFade.y > fft_water::get_max_wave(water)))
      {
        if (waterFlowmap->hasSlopes || water_quality >= 1)
        {
          TIME_D3D_PROFILE(build_flowmap_1);
          fft_water::build_flowmap(water, shoreTextureSize / 2, shoreTextureSize, cameraWorldPos, 0, true);
        }
        if (water_quality >= 2)
        {
          TIME_D3D_PROFILE(build_flowmap_2);
          fft_water::build_flowmap(water, shoreTextureSize, shoreTextureSize, cameraWorldPos, 1, true);
        }
      }
    }
  }
  else
  {
    G_ASSERT(shoreDistanceField.getTexId() == BAD_TEXTUREID);
  }
  land_panel.generate_shore = false;
}

void ShoreRenderer::closeShoreAndWaterFlowmap(FFTWater *water)
{
  shoreHeightmapTex.close();
  shoreDistanceField.close();
  fft_water::close_flowmap(water);
}