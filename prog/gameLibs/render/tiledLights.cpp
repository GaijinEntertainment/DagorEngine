// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <drv/3d/dag_rwResource.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_draw.h>
#include <drv/3d/dag_buffers.h>
#include <drv/3d/dag_texture.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_info.h>
#include <drv/3d/dag_viewScissor.h>
#include <generic/dag_tab.h>
#include <math/dag_hlsl_floatx.h>
#include <math/dag_vecMathCompatibility.h>
#include <memory/dag_framemem.h>
#include <perfMon/dag_statDrv.h>
#include <render/renderLights.hlsli>
#include <render/spheres_consts.hlsli>
#include <shaders/dag_overrideStates.h>
#include <shaders/dag_shaders.h>
#include <shaders/dag_shaderVar.h>

#include <render/tiledLights.h>
#include <render/tiled_light_consts.hlsli>

static bool hasConservativeRasterization = false;

TiledLights::TiledLights(float max_lights_dist) : maxLightsDist(max_lights_dist), omniLightCount(0), spotLightCount(0)
{
  zbinningLUT = dag::buffers::create_one_frame_sr_structured(sizeof(uint32_t), 2 * Z_BINS_COUNT, "z_binning_lookup");

  setMaxLightsDist(maxLightsDist);

  spotLightsMat = new_shader_material_by_name("spot_lights_tiles");
  if (spotLightsMat)
  {
    spotLightsMat->addRef();
    spotLightsElem = spotLightsMat->make_elem();
  }

  omniLightsMat = new_shader_material_by_name("omni_lights_tiles");
  if (omniLightsMat)
  {
    omniLightsMat->addRef();
    omniLightsElem = omniLightsMat->make_elem();
  }

  hasConservativeRasterization = d3d::get_driver_desc().caps.hasConservativeRassterization;
  ShaderGlobal::set_int(::get_shader_variable_id("has_conservative_rasterization", true), hasConservativeRasterization ? 1 : 0);

  if (hasConservativeRasterization)
  {
    shaders::OverrideState state;
    state.set(shaders::OverrideState::CONSERVATIVE);
    conservativeOverrideId = shaders::overrides::create(state);
  }

  zBinningData.resize(Z_BINS_COUNT * 2);
}

void TiledLights::resizeTilesGrid(uint32_t width, uint32_t height)
{
  tilesGridSize = IPoint2((width + TILE_EDGE - 1) / TILE_EDGE, (height + TILE_EDGE - 1) / TILE_EDGE);
}

void TiledLights::setResolution(uint32_t width, uint32_t height)
{
  resizeTilesGrid(width, height);

  lightsListBuf.close();
  lightsListBuf =
    dag::buffers::create_ua_sr_structured(sizeof(uint32_t), tilesGridSize.x * tilesGridSize.y * DWORDS_PER_TILE, "lights_list");
}

void TiledLights::changeResolution(uint32_t width, uint32_t height) { resizeTilesGrid(width, height); }

TiledLights::~TiledLights()
{
  del_it(spotLightsMat);
  del_it(omniLightsMat);
}

void TiledLights::prepare(const Tab<vec4f> &omni_ligth_bounds, const Tab<vec4f> &spot_light_bounds, const vec4f &cur_view_pos,
  const vec4f &cur_view_dir)
{
  TIME_PROFILE(prepare_tiled_lights);
  prepareZBinningLUT(omni_ligth_bounds, spot_light_bounds, cur_view_pos, cur_view_dir);
  omniLightCount = omni_ligth_bounds.size();
  spotLightCount = spot_light_bounds.size();
}

void TiledLights::computeTiledLigths(const bool clear_lights)
{
  if (clear_lights)
  {
    TIME_D3D_PROFILE(clear_tiled_lights);
    d3d::zero_rwbufi(lightsListBuf.getBuf());
    d3d::resource_barrier({lightsListBuf.getBuf(), RB_FLUSH_UAV | RB_STAGE_COMPUTE | RB_SOURCE_STAGE_COMPUTE});
  }

  {
    TIME_D3D_PROFILE(prepare_tile_lights);
    SCOPE_RENDER_TARGET;
    d3d::set_render_target({nullptr, 0, 0}, {}, {{nullptr, 0, 0}});
    d3d::setview(0, 0, tilesGridSize.x, tilesGridSize.y, 0.0f, 1.0f);
    STATE_GUARD_NULLPTR(d3d::set_rwbuffer(STAGE_PS, 1, VALUE), lightsListBuf.getBuf());
    shaders::overrides::set(conservativeOverrideId);

    if (omniLightCount && omniLightsElem)
    {
      TIME_D3D_PROFILE(prepare_tile_omni_lights);
      omniLightsElem->setStates(0, true);
      d3d::draw_instanced(PRIM_TRILIST, 0, SPHERES_INDICES_TO_DRAW, omniLightCount);
    }

    if (omniLightCount && omniLightsElem && spotLightCount && spotLightsElem)
      d3d::resource_barrier({lightsListBuf.getBuf(), RB_NONE});

    if (spotLightCount && spotLightsElem)
    {
      TIME_D3D_PROFILE(prepare_tile_spot_lights);
      spotLightsElem->setStates(0, true);
      d3d::draw_instanced(PRIM_TRILIST, 0, 8, spotLightCount);
    }
    d3d::resource_barrier({lightsListBuf.getBuf(), RB_RO_SRV | RB_STAGE_PIXEL | RB_STAGE_COMPUTE});

    shaders::overrides::reset();
  }
}

void TiledLights::fillzBins(uint32_t max_id, uint32_t offset, const Tab<vec4f> &ligth_bounds, const vec4f &cur_view_pos,
  const vec4f &cur_view_dir)
{
  zBinBegins.assign(Z_BINS_COUNT, max_id);
  zBinEnds.assign(Z_BINS_COUNT, 0);
  const float distScale = Z_BINS_COUNT / maxLightsDist;
  for (uint16_t i = 0; i < ligth_bounds.size(); ++i)
  {
    const float radius = v_extract_w(ligth_bounds[i]);
    float depth = v_extract_x(v_dot3_x(cur_view_dir, v_sub(ligth_bounds[i], cur_view_pos)));
    const float minDepth = depth - radius;
    const float maxDepth = depth + radius;
    const int minBin = clamp(static_cast<int>(minDepth * distScale), 0, Z_BINS_COUNT);
    const int maxBin = clamp(static_cast<int>(maxDepth * distScale), 0, Z_BINS_COUNT - 1);
    if (minBin >= Z_BINS_COUNT)
      continue;
    for (uint32_t bin = minBin; bin <= maxBin; ++bin)
    {
      zBinBegins[bin] = min(zBinBegins[bin], i);
      zBinEnds[bin] = max(zBinEnds[bin], i);
    }
  }
  for (uint32_t i = 0; i < Z_BINS_COUNT; ++i)
  {
    zBinningData[i + offset] = (zBinBegins[i] << 16) + zBinEnds[i];
  }
}

void TiledLights::prepareZBinningLUT(const Tab<vec4f> &omni_ligth_bounds, const Tab<vec4f> &spot_light_bounds,
  const vec4f &cur_view_pos, const vec4f &cur_view_dir)
{
  fillzBins(MAX_OMNI_LIGHTS, 0, omni_ligth_bounds, cur_view_pos, cur_view_dir);
  fillzBins(MAX_SPOT_LIGHTS, Z_BINS_COUNT, spot_light_bounds, cur_view_pos, cur_view_dir);
}

void TiledLights::fillBuffers()
{
  zbinningLUT.getBuf()->updateData(0, data_size(zBinningData), zBinningData.data(), VBLOCK_WRITEONLY | VBLOCK_DISCARD);
}

void TiledLights::setMaxLightsDist(const float max_lights_dist)
{
  maxLightsDist = max_lights_dist;
  ShaderGlobal::set_real(::get_shader_variable_id("max_lights_distance", true), maxLightsDist);
}
