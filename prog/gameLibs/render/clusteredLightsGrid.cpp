// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/clusteredLightsGrid.h>
#include <render/frustumClusters.h>
#include <drv/3d/dag_buffers.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_rwResource.h>
#include <shaders/dag_shaders.h>
#include <shaders/dag_shaderVar.h>
#include <perfMon/dag_statDrv.h>
#include <memory/dag_framemem.h>
#include <EASTL/unique_ptr.h>
#include <3d/dag_lockSbuffer.h>
#include <util/dag_string.h>
#include <startup/dag_globalSettings.h>
#include <ioSys/dag_dataBlock.h>
#include <render/renderLights.hlsli>
#include <render/clusteredLightsGrid.hlsli>
#include <generic/dag_carray.h>
#include <generic/dag_align.h>

#define CLG_GLOBAL_VARS_OPT_LIST                \
  VAR(cull_lights_view_tm)                      \
  VAR(cull_lights_proj_tm)                      \
  VAR(cull_lights_proj_params)                  \
  VAR(use_clustered_occlusion)                  \
  VAR(lights_full_grid)                         \
  VAR(clustered_grid_lights_precalculated_data) \
  VAR(clustered_frustum_points_buf)             \
  VAR(clusteredLightsShrinkSpheres)             \
  VAR(clustered_occlusion_z_slice)

#define VAR(a) static ShaderVariableInfo a##VarId(#a, true);
CLG_GLOBAL_VARS_OPT_LIST
#undef VAR
#undef CLG_GLOBAL_VARS_OPT_LIST

bool ClusteredLightsGrid::isGPU() const
{
  return static_cast<bool>(precalculate_clustered_frustum_data_cs) && static_cast<bool>(precalculate_grid_lights_data_cs) &&
         static_cast<bool>(fill_items_spheres_cs) && static_cast<bool>(cull_spots_cs);
}
bool ClusteredLightsGrid::hasOmni() const { return storedOmniCount != 0; }
bool ClusteredLightsGrid::hasSpot() const { return storedSpotCount != 0; }
bool ClusteredLightsGrid::newFrameHasLights() const { return hasOmni() || hasSpot(); }
bool ClusteredLightsGrid::lastFrameHasLights() const { return frameState != FrameState::NO_CLUSTERED_LIGHTS; }
uint32_t ClusteredLightsGrid::getOmniWords() const { return dag::divide_align_up(storedOmniCount, 32); }
uint32_t ClusteredLightsGrid::getSpotWords() const { return dag::divide_align_up(storedSpotCount, 32); }


ClusteredLightsGrid::ClusteredLightsGrid(FrustumClusters &clusters_, const char *name_suffix, int initial_light_density) :
  clusters(&clusters_), nameSuffix(name_suffix)
{
  bool useGpuLights = dgs_get_settings()->getBlockByNameEx("graphics")->getBool("gpuLights", true);
  if (useGpuLights)
  {
    fill_items_spheres_cs = ComputeShader("clustered_lights_fill_items_spheres_cs", true);
    cull_spots_cs = ComputeShader("clustered_lights_cull_spots_cs", true);
  }

  ShaderGlobal::set_int(clusteredLightsShrinkSpheresVarId, SHRINK_SPHERE);

  if (fill_items_spheres_cs)
  {
    precalculate_clustered_frustum_data_cs = ComputeShader("precalculate_clustered_frustum_data", true);
    precalculate_grid_lights_data_cs = ComputeShader("precalculate_grid_lights_data_cs", true);
    precalculate_occlusion_z_slice_cs = ComputeShader("precalculate_occlusion_z_slice_cs", true);

    //'precalculate_occlusion_z_slice_cs' produces 'clustered_occlusion_z_slice'.
    // it's an optional optimization for 'fill_items_spheres_cs' and can be skipped
    if (precalculate_occlusion_z_slice_cs)
    {
      String occlusionName(128, "clustered_occlusion_z_slice%s", nameSuffix.c_str());
      lightsOcclusionZSlice = dag::create_sbuffer(sizeof(uint32_t), CLUSTERS_W * CLUSTERS_H,
        SBCF_BIND_SHADER_RES | SBCF_BIND_UNORDERED | SBCF_MISC_STRUCTURED, 0, occlusionName, RESTAG_LIGHTS);
    }

    String precalcName(128, "clustered_grid_lights_precalculated_data%s", nameSuffix.c_str());
    lightsGridPrecalculatedData = dag::create_sbuffer(sizeof(CLG_LightData), MAX_OMNI_LIGHTS + MAX_SPOT_LIGHTS,
      SBCF_BIND_SHADER_RES | SBCF_BIND_UNORDERED | SBCF_MISC_STRUCTURED, 0, precalcName, RESTAG_LIGHTS);

    String frustumPtsName(128, "clustered_frustum_points%s", nameSuffix.c_str());
    lightsGridFrustumPoints = dag::create_sbuffer(sizeof(Point4), (CLUSTERS_W + 1) * (CLUSTERS_H + 1) * (CLUSTERS_D + 1),
      SBCF_BIND_SHADER_RES | SBCF_BIND_UNORDERED | SBCF_MISC_STRUCTURED, 0, frustumPtsName, RESTAG_LIGHTS);
  }

  const uint32_t words = clamp((initial_light_density + 31) / 32, (int)2, (int)(MAX_SPOT_LIGHTS + MAX_OMNI_LIGHTS + 31) / 32);
  validateDensity(words);
}

void ClusteredLightsGrid::reset()
{
  for (auto &buf : lightsFullGridCB)
    buf.close();
  lightsGridFrustumPoints.close();
  lightsGridPrecalculatedData.close();
  lightsOcclusionZSlice.close();
  clustersOmniGrid.clear();
  clustersSpotGrid.clear();
  clusters = nullptr;
  allocatedWords = 0;
  lightsGridFrame = 0;
  frameState = FrameState::NOT_INITED;
}

void ClusteredLightsGrid::validateDensity(uint32_t words)
{
  if (words <= allocatedWords)
    return;
  allocatedWords = words;
  // GPU path: UAV + SRV (no CPU write access). CPU path: dynamic CPU-writable SRV.
  const uint32_t bufFlags = isGPU() ? (SBCF_BIND_SHADER_RES | SBCF_BIND_UNORDERED | SBCF_MISC_STRUCTURED)
                                    : (SBCF_DYNAMIC | SBCF_CPU_ACCESS_WRITE | SBCF_BIND_SHADER_RES | SBCF_MISC_STRUCTURED);
  const int numBuffers = isGPU() ? 1 : (int)lightsFullGridCB.size();
  for (int i = 0; i < numBuffers; ++i)
  {
    String name(128, "lights_full_grid_%d%s", i, nameSuffix.c_str());
    lightsFullGridCB[i].close();
    lightsFullGridCB[i] = dag::create_sbuffer(sizeof(uint32_t), CLUSTERS_PER_GRID * allocatedWords, bufFlags, 0, name, RESTAG_LIGHTS);
  }
}

void ClusteredLightsGrid::advanceFrameState()
{
  frameState = newFrameHasLights() ? FrameState::HAS_CLUSTERED_LIGHTS : FrameState::NO_CLUSTERED_LIGHTS;
}

void ClusteredLightsGrid::prepareGPUCulling(mat44f_cref view, mat44f_cref proj, float znear, float zfar, float close_dist,
  float max_dist, uint32_t omni_bounds_size, uint32_t spot_bounds_size)
{
  G_ASSERT(isGPU());
  storedOmniCount = omni_bounds_size;
  storedSpotCount = spot_bounds_size;

  v_stu(&cullLightsViewTm.row[0].x, view.col0);
  v_stu(&cullLightsViewTm.row[1].x, view.col1);
  v_stu(&cullLightsViewTm.row[2].x, view.col2);
  v_stu(&cullLightsViewTm.row[3].x, view.col3);

  v_stu(&cullLightsProjTm.row[0].x, proj.col0);
  v_stu(&cullLightsProjTm.row[1].x, proj.col1);
  v_stu(&cullLightsProjTm.row[2].x, proj.col2);
  v_stu(&cullLightsProjTm.row[3].x, proj.col3);

  cullLightsProjParams = Point4(znear, zfar, max_dist, 0);
  clusters->updateFrustumVariables(view, proj, znear, close_dist, max_dist);
}

void ClusteredLightsGrid::cullCPU(mat44f_cref view, mat44f_cref proj, float znear, float close_dist, float max_dist,
  dag::ConstSpan<vec4f> omni_bounds, dag::ConstSpan<SpotLightsManager::RawLight> spot_lights, dag::ConstSpan<vec4f> spot_bounds,
  Occlusion *occlusion)
{
  G_ASSERT(!isGPU());
  storedOmniCount = omni_bounds.size();
  storedSpotCount = spot_lights.size();
  const uint32_t omniWords = getOmniWords();
  const uint32_t spotWords = getSpotWords();
  clustersOmniGrid.resize(CLUSTERS_PER_GRID * omniWords);
  clustersSpotGrid.resize(CLUSTERS_PER_GRID * spotWords);

  if (clustersOmniGrid.size() || clustersSpotGrid.size())
  {
    bool hasOmniLights = !clustersOmniGrid.empty();
    bool hasSpotLights = !clustersSpotGrid.empty();
    mem_set_0(clustersOmniGrid);
    mem_set_0(clustersSpotGrid);
    clusteredCullLights(view, proj, znear, close_dist, max_dist, omni_bounds, spot_lights, spot_bounds, occlusion, hasOmniLights,
      hasSpotLights, clustersOmniGrid.data(), omniWords, clustersSpotGrid.data(), spotWords);
    if (!hasOmniLights)
    {
      clustersOmniGrid.resize(0);
      storedOmniCount = 0;
    }
    if (!hasSpotLights)
    {
      clustersSpotGrid.resize(0);
      storedSpotCount = 0;
    }
  }
}

void ClusteredLightsGrid::fill()
{
  const uint32_t omniWords = getOmniWords();
  const uint32_t spotWords = getSpotWords();

  if (!isGPU())
    fillClusteredCB(clustersOmniGrid.data(), omniWords, clustersSpotGrid.data(), spotWords);
  else
    executeGPUFillingPipeline();
}

void ClusteredLightsGrid::executeGPUFillingPipeline()
{
  G_ASSERT(isGPU());
  TIME_D3D_PROFILE(clusteredFill_GPU);
  validateDensity(getSpotWords() + getOmniWords());

  ShaderGlobal::set_buffer(lights_full_gridVarId, lightsFullGridCB[0]);
  ShaderGlobal::set_buffer(clustered_grid_lights_precalculated_dataVarId, lightsGridPrecalculatedData);

  ShaderGlobal::set_float4x4(cull_lights_view_tmVarId, cullLightsViewTm);
  ShaderGlobal::set_float4x4(cull_lights_proj_tmVarId, cullLightsProjTm);
  ShaderGlobal::set_float4(cull_lights_proj_paramsVarId, cullLightsProjParams);

  ShaderGlobal::set_buffer(clustered_frustum_points_bufVarId, lightsGridFrustumPoints);
  precalculate_clustered_frustum_data_cs.dispatchThreads(CLUSTERS_W + 1, CLUSTERS_H + 1, CLUSTERS_D + 1);
  d3d::resource_barrier({lightsGridFrustumPoints.getBuf(), RB_FLUSH_UAV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_COMPUTE});

  const bool hasOcclusion = static_cast<bool>(precalculate_occlusion_z_slice_cs);
  ShaderGlobal::set_int(use_clustered_occlusionVarId, hasOcclusion ? 1 : 0);
  if (hasOcclusion)
  {
    ShaderGlobal::set_buffer(clustered_occlusion_z_sliceVarId, lightsOcclusionZSlice);
    precalculate_occlusion_z_slice_cs.dispatchThreads(CLUSTERS_W, CLUSTERS_H, 1);
    d3d::resource_barrier({lightsOcclusionZSlice.getBuf(), RB_FLUSH_UAV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_COMPUTE});
  }

  d3d::zero_rwbufi(lightsFullGridCB[0].getBuf());
  d3d::resource_barrier({lightsFullGridCB[0].getBuf(), RB_FLUSH_UAV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_COMPUTE});

  const uint32_t totalLights = storedOmniCount + storedSpotCount;
  if (totalLights > 0)
  {
    precalculate_grid_lights_data_cs.dispatchThreads(totalLights, 1, 1);
    d3d::resource_barrier({lightsGridPrecalculatedData.getBuf(), RB_FLUSH_UAV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_COMPUTE});
    fill_items_spheres_cs.dispatchThreads(CLUSTERS_W, CLUSTERS_H, CLUSTERS_D * totalLights);
    if (storedSpotCount > 0)
    {
      d3d::resource_barrier({lightsFullGridCB[0].getBuf(), RB_FLUSH_UAV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_COMPUTE});
      cull_spots_cs.dispatchThreads(CLUSTERS_W, CLUSTERS_H, CLUSTERS_D * storedSpotCount);
    }
  }

  d3d::resource_barrier({lightsFullGridCB[0].getBuf(), RB_FLUSH_UAV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_PIXEL});
}

void ClusteredLightsGrid::clusteredCullLights(mat44f_cref view, mat44f_cref proj, float znear, float min_dist, float max_dist,
  dag::ConstSpan<vec4f> omni_bounds, dag::ConstSpan<SpotLightsManager::RawLight> spot_lights, dag::ConstSpan<vec4f> spot_bounds,
  Occlusion *occlusion, bool &has_omni_lights, bool &has_spot_lights, uint32_t *omni_mask, uint32_t omni_words, uint32_t *spot_mask,
  uint32_t spot_words)
{
  has_spot_lights = spot_lights.size() != 0;
  has_omni_lights = omni_bounds.size() != 0;
  if (!omni_bounds.size() && !spot_lights.size())
    return;
  TIME_D3D_PROFILE(clusteredCullLights_CPU);
  clusters->prepareFrustum(view, proj, znear, min_dist, max_dist, occlusion);
  eastl::unique_ptr<FrustumClusters::ClusterGridItemMasks, framememDeleter> tempOmniItemsPtr, tempSpotItemsPtr;

  tempOmniItemsPtr.reset(new (framemem_ptr()) FrustumClusters::ClusterGridItemMasks);

  TIME_PROFILE(clustered);
  uint32_t clusteredOmniLights = clusters->fillItemsSpheres(omni_bounds.data(), elem_size(omni_bounds) / sizeof(vec4f),
    omni_bounds.size(), *tempOmniItemsPtr, omni_mask, omni_words);

  tempOmniItemsPtr.reset();

  uint32_t clusteredSpotLights = 0;
  if (spot_lights.size())
  {
    tempSpotItemsPtr.reset(new (framemem_ptr()) FrustumClusters::ClusterGridItemMasks);
    clusteredSpotLights = clusters->fillItemsSpheres(spot_bounds.data(), elem_size(spot_bounds) / sizeof(vec4f), spot_lights.size(),
      *tempSpotItemsPtr, spot_mask, spot_words);
  }

  if (clusteredSpotLights)
  {
    TIME_PROFILE(cullSpots)
    clusteredSpotLights = clusters->cullSpots((const vec4f *)spot_lights.data(), elem_size(spot_lights) / sizeof(vec4f),
      (const vec4f *)&spot_lights[0].dir_tanHalfAngle, elem_size(spot_lights) / sizeof(vec4f), *tempSpotItemsPtr, spot_mask,
      spot_words);
  }
  has_spot_lights = clusteredSpotLights != 0;
  has_omni_lights = clusteredOmniLights != 0;
}

bool ClusteredLightsGrid::fillClusteredCB(uint32_t *source_omni, uint32_t omni_words, uint32_t *source_spot, uint32_t spot_words)
{
  G_ASSERT(!isGPU());
  TIME_D3D_PROFILE(clusteredFill_CPU);
  validateDensity(spot_words + omni_words);

  lightsGridFrame = (lightsGridFrame + 1) % lightsFullGridCB.size();
  ShaderGlobal::set_buffer(lights_full_gridVarId, lightsFullGridCB[lightsGridFrame]);

  LockedBuffer<uint32_t> masks =
    lock_sbuffer<uint32_t>(lightsFullGridCB[lightsGridFrame].getBuf(), 0, 0, VBLOCK_WRITEONLY | VBLOCK_DISCARD);
  if (!masks)
    return false;
  masks.updateDataRange(0, source_omni, omni_words * CLUSTERS_PER_GRID);
  masks.updateDataRange(CLUSTERS_PER_GRID * omni_words, source_spot, spot_words * CLUSTERS_PER_GRID);
  return true;
}
