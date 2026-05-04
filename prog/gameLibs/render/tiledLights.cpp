// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <drv/3d/dag_rwResource.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_draw.h>
#include <drv/3d/dag_buffers.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_driverDesc.h>
#include <drv/3d/dag_viewScissor.h>
#include <generic/dag_tab.h>
#include <generic/dag_align.h>
#include <math/dag_hlsl_floatx.h>
#include <math/dag_vecMathCompatibility.h>
#include <memory/dag_framemem.h>
#include <perfMon/dag_statDrv.h>
#include <render/renderLights.hlsli>
#include <render/primitiveObjects.h>
#include <render/spheres_consts.hlsli>
#include <shaders/dag_computeShaders.h>
#include <shaders/dag_overrideStates.h>
#include <shaders/dag_shaders.h>
#include <shaders/dag_shaderVar.h>
#include <drv/3d/dag_driverDesc.h>

#include <render/tiledLights.h>
#include <render/tiled_light_consts.hlsli>
#include <startup/dag_globalSettings.h>
#include <ioSys/dag_dataBlock.h>

static bool hasConservativeRasterization = false;

void TiledLights::fillSpotLightIb()
{
  uint32_t ibSize = spotsFaceCount * sizeof(uint16_t) * 3;
  uint8_t *vertices = NULL;
  uint8_t *indices = NULL;
  if (!spotsIndexBuf->lock(0, 0, (void **)&indices, VBLOCK_WRITEONLY))
  {
    logerr("can't lock spotlights buffer");
    return;
  }

  create_cylinder_mesh(dag::Span<uint8_t>(vertices, 0), dag::Span<uint8_t>(indices, ibSize), 1, 1, spotsSlices, spotsStacks, 0, false,
    false, false);

  spotsIndexBuf->unlock();
  spotFilled = true;
}

void TiledLights::createSpotLightIb()
{
  calc_cylinder_vertex_face_count(spotsSlices, spotsStacks, spotsVertexCount, spotsFaceCount);

  ShaderGlobal::set_int(::get_shader_variable_id("spot_cones_vertex_slices", true), spotsSlices);
  ShaderGlobal::set_int(::get_shader_variable_id("spot_cones_vertex_stacks", true), spotsStacks + 1); // vertices are not merged

  uint32_t ibSize = spotsFaceCount * sizeof(uint16_t) * 3;

  del_d3dres(spotsIndexBuf);
  spotsIndexBuf = d3d::create_ib(ibSize, 0, "tiled_spot_lights_ib", RESTAG_LIGHTS);
  fillSpotLightIb();
}

bool TiledLights::isGPUZBinning() const
{
  return static_cast<bool>(zBinningOmniElem) && static_cast<bool>(zBinningSpotElem) && static_cast<bool>(zBinningPackElem) &&
         static_cast<bool>(zBinningClearElem);
}

TiledLights::TiledLights(float max_lights_dist) : maxLightsDist(max_lights_dist), omniLightCount(0), spotLightCount(0)
{
  useTilesRT = d3d::get_driver_desc().issues.hasBrokenUAVOnlyPasses;
  bool useGPUBinning = dgs_get_settings()->getBlockByNameEx("graphics")->getBool("gpuLights", true);

  if (useGPUBinning)
  {
    if (d3d::get_driver_desc().issues.hasBrokenUAVOnlyPasses)
    {
      useGPUBinning = false;
      logwarn("TiledLights: can not use GPU binning because of brokenUAVOnlyPasses in driver");
    }

    if (d3d::get_driver_desc().shaderModel < 5.0_sm)
    {
      useGPUBinning = false;
      logwarn("TiledLights: can not use GPU binning because of shader model: %d", d3d::get_driver_desc().shaderModel);
    }
  }

  if (useGPUBinning)
  {
    zBinningOmniElem = ComputeShader("tiled_lights_zbinning_omni_cs", true);
    zBinningSpotElem = ComputeShader("tiled_lights_zbinning_spot_cs", true);
    zBinningPackElem = ComputeShader("tiled_lights_zbinning_pack_cs", true);
    zBinningClearElem = ComputeShader("tiled_lights_zbinning_clear_cs", true);
    useGPUBinning = isGPUZBinning();

    viewDirVarId = ::get_shader_variable_id("zbinning_view_dir", true);
    viewPosVarId = ::get_shader_variable_id("zbinning_view_pos", true);
  }

  maxDistanceVarId = ::get_shader_variable_id("max_lights_distance", true);

  if (useGPUBinning)
  {
    zbinningLUT = dag::buffers::create_ua_sr_structured(sizeof(uint32_t), 2 * Z_BINS_COUNT, "z_binning_lookup", d3d::buffers::Init::No,
      RESTAG_LIGHTS);
    zBinBeginsEndsBuf = dag::buffers::create_ua_sr_structured(2 * sizeof(uint32_t), 2 * Z_BINS_COUNT, "z_binning_begins_ends_buf",
      d3d::buffers::Init::No, RESTAG_LIGHTS);
  }
  else
  {
    zbinningLUT = dag::buffers::create_one_frame_sr_structured(sizeof(uint32_t), 2 * Z_BINS_COUNT, "z_binning_lookup", RESTAG_LIGHTS);
  }

  logdbg("TiledLights - use gpu binning: %d", isGPUZBinning());

  setMaxLightsDist(maxLightsDist);

  spotLightsMat = new_shader_material_by_name("spot_lights_tiles");
  if (spotLightsMat)
  {
    spotLightsMat->addRef();
    spotLightsElem = spotLightsMat->make_elem();

    createSpotLightIb();
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

  if (!useGPUBinning)
    zBinningData.resize(Z_BINS_COUNT * 2);
}

void TiledLights::resizeTilesGrid(uint32_t width, uint32_t height)
{
  tilesGridSize = IPoint2((width + TILE_EDGE - 1) / TILE_EDGE, (height + TILE_EDGE - 1) / TILE_EDGE);
}

void TiledLights::setResolution(uint32_t width, uint32_t height)
{
  resizeTilesGrid(width, height);

  tilesRT.close();
  if (useTilesRT)
    tilesRT = dag::create_tex(nullptr, tilesGridSize.x, tilesGridSize.y, TEXFMT_R8 | TEXCF_RTARGET | TEXCF_TRANSIENT, 1,
      "tiled_lights_rt", RESTAG_LIGHTS);

  lightsListBuf.close();
  lightsListBuf = dag::buffers::create_ua_sr_structured(sizeof(uint32_t), tilesGridSize.x * tilesGridSize.y * DWORDS_PER_TILE,
    "lights_list", d3d::buffers::Init::No, RESTAG_LIGHTS);
}

void TiledLights::changeResolution(uint32_t width, uint32_t height)
{
  resizeTilesGrid(width, height);
  if (useTilesRT)
    tilesRT.resize(tilesGridSize.x, tilesGridSize.y);
}

TiledLights::~TiledLights()
{
  del_it(spotLightsMat);
  del_it(omniLightsMat);
  del_d3dres(spotsIndexBuf);
}

void TiledLights::prepare(const Tab<vec4f> &omni_ligth_bounds, const Tab<vec4f> &spot_light_bounds, const vec4f &cur_view_pos,
  const vec4f &cur_view_dir)
{
  if (isGPUZBinning())
  {
    // save zBinningParams
    binningViewDir = Color4::xyzw(as_point4(&cur_view_dir));
    binningViewPos = Color4::xyzw(as_point4(&cur_view_pos));
  }
  else
  {
    // zBinning cpu fallback
    TIME_PROFILE(tiled_light_execute_cpu_z_binning);
    prepareZBinningLUT(omni_ligth_bounds, spot_light_bounds, cur_view_pos, cur_view_dir);
  }

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
    if (useTilesRT)
    {
      d3d::set_render_target({}, DepthAccess::RW, {{tilesRT.getBaseTex(), 0, 0}});
    }
    else
    {
      d3d::set_render_target({nullptr, 0, 0}, {}, {{nullptr, 0, 0}});
      d3d::setview(0, 0, tilesGridSize.x, tilesGridSize.y, 0.0f, 1.0f);
    }

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

    if (spotLightCount && spotLightsElem && spotsIndexBuf)
    {
      if (!spotFilled)
        createSpotLightIb();
      if (spotFilled)
      {
        G_ASSERT(spotsIndexBuf);
        TIME_D3D_PROFILE(prepare_tile_spot_lights);
        d3d::setind(spotsIndexBuf);
        d3d::setvsrc_ex(0, NULL, 0, 0);
        spotLightsElem->setStates(0, true);
        d3d::drawind_instanced(PRIM_TRILIST, 0, spotsFaceCount, 0, spotLightCount);
      }
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

void TiledLights::applyBinning()
{
  G_ASSERTF(is_main_thread(), "TiledLights::applyBinning must be called from the main thread");

  if (isGPUZBinning())
  {
    G_ASSERTF(Z_BINNING_GROUP_SIZE >= MAX_OMNI_LIGHTS,
      "TiledLights::applyBinning - thread group has not enough capacity for omni lights");
    G_ASSERTF(Z_BINNING_GROUP_SIZE >= MAX_SPOT_LIGHTS,
      "TiledLights::applyBinning - thread group has not enough capacity for spot lights");

    TIME_D3D_PROFILE(tiled_light_execute_gpu_z_binning);

    // set parameters for zBinning (omniLightsCount, spotLightsCount, omni_lights, spot_lights already set)
    ShaderGlobal::set_float(maxDistanceVarId, maxLightsDist);
    ShaderGlobal::set_float4(viewDirVarId, binningViewDir);
    ShaderGlobal::set_float4(viewPosVarId, binningViewPos);

    {
      // init begin/ends
      zBinningClearElem.dispatchGroups(dag::divide_align_up(2 * Z_BINS_COUNT, Z_BINNING_GROUP_SIZE), 1, 1);
      d3d::resource_barrier({zBinBeginsEndsBuf.getBuf(), RB_FLUSH_UAV | RB_STAGE_COMPUTE | RB_SOURCE_STAGE_COMPUTE});
    }

    {
      // calculate binning for omni lights
      uint lightBlocksCount = dag::divide_align_up(omniLightCount, Z_BINNING_LIGHTS_BLOCK_SIZE);
      if (lightBlocksCount > 0)
        zBinningOmniElem.dispatchGroups(dag::divide_align_up(Z_BINS_COUNT, Z_BINNING_GROUP_SIZE), lightBlocksCount, 1);
    }

    if (omniLightCount > 0 && spotLightCount > 0)
    {
      d3d::resource_barrier({zBinBeginsEndsBuf.getBuf(), RB_NONE});
    }

    {
      // calculate binning for spot lights
      uint lightBlocksCount = dag::divide_align_up(spotLightCount, Z_BINNING_LIGHTS_BLOCK_SIZE);
      if (lightBlocksCount > 0)
        zBinningSpotElem.dispatchGroups(dag::divide_align_up(Z_BINS_COUNT, Z_BINNING_GROUP_SIZE), lightBlocksCount, 1);
    }

    {
      // pack begin/ends to LUT
      d3d::resource_barrier({zBinBeginsEndsBuf.getBuf(), RB_FLUSH_UAV | RB_STAGE_COMPUTE | RB_SOURCE_STAGE_COMPUTE});
      zBinningPackElem.dispatchGroups(dag::divide_align_up(2 * Z_BINS_COUNT, Z_BINNING_GROUP_SIZE), 1, 1);
    }

    {
      d3d::resource_barrier({zbinningLUT.getBuf(), RB_RO_SRV | RB_STAGE_PIXEL | RB_STAGE_COMPUTE});
    }
  }
  else
  {
    zbinningLUT.getBuf()->updateData(0, data_size(zBinningData), zBinningData.data(), VBLOCK_WRITEONLY | VBLOCK_DISCARD);
  }
}

void TiledLights::setMaxLightsDist(const float max_lights_dist)
{
  maxLightsDist = max_lights_dist;
  ShaderGlobal::set_float(maxDistanceVarId, maxLightsDist);
}

void TiledLights::beforeResetDevice() { spotFilled = false; }

void TiledLights::afterResetDevice() { spotFilled = false; }