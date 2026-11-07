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
#include <render/lights/lightsResources.h>
#include <render/lights/renderLights.hlsli>
#include <render/primitiveObjects.h>
#include <shaders/dag_computeShaders.h>
#include <shaders/dag_overrideStates.h>
#include <shaders/dag_shaders.h>
#include <shaders/dag_shaderVar.h>
#include <shaders/dag_shaderVariableInfo.h>
#include <startup/dag_globalSettings.h>
#include <ioSys/dag_dataBlock.h>
#include <drv/3d/dag_driverDesc.h>
#include <render/lights/tiledLights.h>
#include <render/lights/tiledLights.hlsli>
#include <render/lights/tiled_light_consts.hlsli>

#define TILED_LIGHTS_GLOBAL_VARS_LIST \
  VAR(z_binning_lookup)               \
  VAR(z_binning_begins_ends_buf)      \
  VAR(max_lights_distance)            \
  VAR(zbinning_view_pos)              \
  VAR(zbinning_view_dir)              \
  VAR(spot_cones_vertex_slices)       \
  VAR(spot_cones_vertex_stacks)       \
  VAR(has_conservative_rasterization)

#define VAR(a) static ShaderVariableInfo a##VarId(#a, true);
TILED_LIGHTS_GLOBAL_VARS_LIST
#undef VAR
#undef TILED_LIGHTS_GLOBAL_VARS_LIST

static_assert(TILED_LIGHTS_DISPATCH_ARGS_SIZE == DISPATCH_INDIRECT_BUFFER_SIZE);
static_assert(TILED_LIGHTS_DRAW_ARGS_SIZE == DRAW_INDIRECT_BUFFER_SIZE);

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
  uint32_t spotsVertexCount = 0;
  calc_cylinder_vertex_face_count(spotsSlices, spotsStacks, spotsVertexCount, spotsFaceCount);

  ShaderGlobal::set_int(spot_cones_vertex_slicesVarId, spotsSlices);
  ShaderGlobal::set_int(spot_cones_vertex_stacksVarId, spotsStacks + 1); // vertices are not merged

  uint32_t ibSize = spotsFaceCount * sizeof(uint16_t) * 3;

  del_d3dres(spotsIndexBuf);
  spotsIndexBuf = d3d::create_ib(ibSize, 0, "tiled_spot_lights_ib", RESTAG_LIGHTS);
  fillSpotLightIb();
}

D3DRESID TiledLights::getLUTBufId() const { return zbinningLUT.getBufId(); }

D3DRESID TiledLights::getLightsListBufId() const { return lightsListBuf.getBufId(); }

const Tab<uint32_t> &TiledLights::getZBinningData() const
{
  G_ASSERTF(!isGpu(), "getZBinningData is only valid for the CPU binning path");
  return zBinningData;
}

TiledLights::TiledLights(float max_lights_dist, const LightsResourcesManager *lights_res_mgr) :
  lightsResMgr(lights_res_mgr), maxLightsDist(max_lights_dist)
{
  useTilesRT = d3d::get_driver_desc().issues.hasBrokenUAVOnlyPasses;
  resizeTilesRT = dgs_get_settings()->getBlockByNameEx("graphics")->getBool("tiledLightsResizeTilesRT", true);

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
  ShaderGlobal::set_int(has_conservative_rasterizationVarId, hasConservativeRasterization ? 1 : 0);

  if (hasConservativeRasterization)
  {
    shaders::OverrideState state;
    state.set(shaders::OverrideState::CONSERVATIVE);
    conservativeOverrideId = shaders::overrides::create(state);
  }
}

void TiledLights::init(DynLightsFeatures requested_features)
{
  bool useGPUBinning = static_cast<bool>(requested_features & DynLightsFeatures::GpuTiledLightsZBinning);

  if (useGPUBinning)
  {
    if (d3d::get_driver_desc().shaderModel < 5.0_sm)
    {
      useGPUBinning = false;
      logwarn("TiledLights: can not use GPU binning because of shader model: %d", d3d::get_driver_desc().shaderModel);
    }
  }

  if (useGPUBinning)
  {
    zBinningElem = ComputeShader("tiled_lights_zbinning_cs", true);
    zBinningPackElem = ComputeShader("tiled_lights_zbinning_pack_cs", true);
    zBinningClearElem = ComputeShader("tiled_lights_zbinning_clear_cs", true);

    zbinningLUT.close();
    zBinBeginsEndsBuf.close();
    zbinningLUT = dag::buffers::create_ua_sr_structured(sizeof(uint32_t), 2 * Z_BINS_COUNT,
      lightsResMgr->getResName("z_binning_lookup"), d3d::buffers::Init::No, RESTAG_LIGHTS);
    zBinBeginsEndsBuf = dag::buffers::create_ua_sr_structured(2 * sizeof(uint32_t), 2 * Z_BINS_COUNT,
      lightsResMgr->getResName("z_binning_begins_ends_buf"), d3d::buffers::Init::No, RESTAG_LIGHTS);
  }

  useGPUBinning = isGpu();

  if (!useGPUBinning)
  {
    zBinBeginsEndsBuf.close();
    zbinningLUT.close();
    zbinningLUT = dag::buffers::create_one_frame_sr_structured(sizeof(uint32_t), 2 * Z_BINS_COUNT,
      lightsResMgr->getResName("z_binning_lookup"), RESTAG_LIGHTS);
  }

  logdbg("TiledLights - use gpu binning: %d", useGPUBinning);

  if (!useGPUBinning)
    zBinningData.resize(Z_BINS_COUNT * 2);
}

DynLightsFeatures TiledLights::getRuntimeFeatures() const
{
  bool isGpuZBinning = static_cast<bool>(zBinningElem) && static_cast<bool>(zBinningPackElem) &&
                       static_cast<bool>(zBinningClearElem) && zbinningLUT && zBinBeginsEndsBuf;

  return isGpuZBinning ? DynLightsFeatures::GpuTiledLightsZBinning : DynLightsFeatures::CpuOnly;
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
      lightsResMgr->getResName("tiled_lights_rt"), RESTAG_LIGHTS);

  lightsListBuf.close();
  lightsListBuf = dag::buffers::create_ua_sr_structured(sizeof(uint32_t), tilesGridSize.x * tilesGridSize.y * DWORDS_PER_TILE,
    lightsResMgr->getResName("lights_list"), d3d::buffers::Init::No, RESTAG_LIGHTS);
}

void TiledLights::changeResolution(uint32_t width, uint32_t height)
{
  resizeTilesGrid(width, height);
  if (useTilesRT && resizeTilesRT)
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
  if (isGpu())
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
}

void TiledLights::computeTiledLigths(Sbuffer *indirect_args, uint32_t indirect_args_offset, const bool clear_lights)
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
      d3d::set_render_target({}, DepthAccess::RW, {{tilesRT.getBaseTex(), 0, 0}});
    else
      d3d::set_render_target({nullptr, 0, 0}, {}, {{nullptr, 0, 0}});
    d3d::setview(0, 0, tilesGridSize.x, tilesGridSize.y, 0.0f, 1.0f);

    STATE_GUARD_NULLPTR(d3d::set_rwbuffer(STAGE_PS, 1, VALUE), lightsListBuf.getBuf());
    shaders::overrides::set(conservativeOverrideId);

    if (omniLightsElem)
    {
      TIME_D3D_PROFILE(prepare_tile_omni_lights);
      omniLightsElem->setStates(0, true);
      d3d::draw_indirect(PRIM_TRILIST, indirect_args, indirect_args_offset + TILED_LIGHTS_INDIRECT_ARGS_OMNI_DRAW_OFFSET);
    }

    if (omniLightsElem && spotLightsElem)
      d3d::resource_barrier({lightsListBuf.getBuf(), RB_NONE});

    if (spotLightsElem && spotsIndexBuf)
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
        d3d::draw_indexed_indirect(PRIM_TRILIST, indirect_args, indirect_args_offset + TILED_LIGHTS_INDIRECT_ARGS_SPOT_DRAW_OFFSET);
        d3d::setind(nullptr);
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
  fillzBins(MAX_CLUSTERED_OMNI_LIGHTS, 0, omni_ligth_bounds, cur_view_pos, cur_view_dir);
  fillzBins(MAX_CLUSTERED_SPOT_LIGHTS, Z_BINS_COUNT, spot_light_bounds, cur_view_pos, cur_view_dir);
}

TiledLights::DrawCounts TiledLights::getDrawCounts() const
{
  return {.omniVertexCount = OMNI_TILES_CUBE_VERTICES, .spotIndexCount = 3 * spotsFaceCount};
}

void TiledLights::applyBinning(Sbuffer *indirect_args, uint32_t indirect_args_offset)
{
  G_ASSERTF(is_main_thread(), "TiledLights::applyBinning must be called from the main thread");

  if (isGpu())
  {
    G_ASSERTF(Z_BINNING_GROUP_SIZE >= MAX_CLUSTERED_OMNI_LIGHTS,
      "TiledLights::applyBinning - thread group has not enough capacity for omni lights");
    G_ASSERTF(Z_BINNING_GROUP_SIZE >= MAX_CLUSTERED_SPOT_LIGHTS,
      "TiledLights::applyBinning - thread group has not enough capacity for spot lights");

    TIME_D3D_PROFILE(tiled_light_execute_gpu_z_binning);

    // set parameters for zBinning (omni_lights, spot_lights already set)
    ShaderGlobal::set_float(max_lights_distanceVarId, maxLightsDist);
    ShaderGlobal::set_float4(zbinning_view_dirVarId, binningViewDir);
    ShaderGlobal::set_float4(zbinning_view_posVarId, binningViewPos);

    {
      STATE_GUARD_NULLPTR(z_binning_begins_ends_bufVarId.set_buffer(VALUE), zBinBeginsEndsBuf.getBuf());
      STATE_GUARD(z_binning_lookupVarId.set_buffer(VALUE), zbinningLUT.getBufId(), z_binning_lookupVarId.get_buffer());

      {
        // init begin/ends
        zBinningClearElem.dispatchGroups(dag::divide_align_up(2 * Z_BINS_COUNT, Z_BINNING_GROUP_SIZE), 1, 1);
        d3d::resource_barrier({zBinBeginsEndsBuf.getBuf(), RB_FLUSH_UAV | RB_STAGE_COMPUTE | RB_SOURCE_STAGE_COMPUTE});
      }

      {
        // calculate binning for omni and spot lights in one dispatch
        zBinningElem.dispatchIndirect(indirect_args, indirect_args_offset + TILED_LIGHTS_INDIRECT_ARGS_ZBINNING_DISPATCH_OFFSET);
      }

      {
        // pack begin/ends to LUT
        d3d::resource_barrier({zBinBeginsEndsBuf.getBuf(), RB_FLUSH_UAV | RB_STAGE_COMPUTE | RB_SOURCE_STAGE_COMPUTE});
        zBinningPackElem.dispatchGroups(dag::divide_align_up(2 * Z_BINS_COUNT, Z_BINNING_GROUP_SIZE), 1, 1);
      }
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

void TiledLights::clearZBinningLUT()
{
  if (!isGpu())
    return;

  {
    STATE_GUARD_NULLPTR(z_binning_begins_ends_bufVarId.set_buffer(VALUE), zBinBeginsEndsBuf.getBuf());
    STATE_GUARD(z_binning_lookupVarId.set_buffer(VALUE), zbinningLUT.getBufId(), z_binning_lookupVarId.get_buffer());
    zBinningClearElem.dispatchGroups(dag::divide_align_up(2 * Z_BINS_COUNT, Z_BINNING_GROUP_SIZE), 1, 1);
    d3d::resource_barrier({zBinBeginsEndsBuf.getBuf(), RB_FLUSH_UAV | RB_STAGE_COMPUTE | RB_SOURCE_STAGE_COMPUTE});
    zBinningPackElem.dispatchGroups(dag::divide_align_up(2 * Z_BINS_COUNT, Z_BINNING_GROUP_SIZE), 1, 1);
  }
  d3d::resource_barrier({zbinningLUT.getBuf(), RB_RO_SRV | RB_STAGE_PIXEL | RB_STAGE_COMPUTE});
}

void TiledLights::setMaxLightsDist(const float max_lights_dist) { maxLightsDist = max_lights_dist; }

void TiledLights::beforeResetDevice() { spotFilled = false; }

void TiledLights::afterResetDevice()
{
  spotFilled = false;
  clearZBinningLUT();
}