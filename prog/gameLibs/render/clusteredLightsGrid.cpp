// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/clusteredLightsGrid.h>
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

static int lights_full_gridVarId = -1;

ClusteredLightsGrid::ClusteredLightsGrid(FrustumClusters &clusters_, const char *name_suffix, int initial_light_density) :
  clusters(&clusters_), nameSuffix(name_suffix)
{
  lights_full_gridVarId = ::get_shader_variable_id("lights_full_grid", true);

  const uint32_t words = clamp((initial_light_density + 31) / 32, (int)2, (int)(MAX_SPOT_LIGHTS + MAX_OMNI_LIGHTS + 31) / 32);
  validateDensity(words);
}

void ClusteredLightsGrid::reset()
{
  for (auto &buf : lightsFullGridCB)
    buf.close();
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
  const uint32_t bufFlags = (SBCF_DYNAMIC | SBCF_CPU_ACCESS_WRITE | SBCF_BIND_SHADER_RES | SBCF_MISC_STRUCTURED);
  for (int i = 0; i < (int)lightsFullGridCB.size(); ++i)
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

void ClusteredLightsGrid::cull(mat44f_cref view, mat44f_cref proj, float znear, float close_dist, float max_dist,
  dag::ConstSpan<vec4f> omni_bounds, dag::ConstSpan<SpotLightsManager::RawLight> spot_lights, dag::ConstSpan<vec4f> spot_bounds,
  Occlusion *occlusion)
{
  const uint32_t omniCount = omni_bounds.size();
  const uint32_t spotCount = spot_lights.size();
  const uint32_t omniWords = (omniCount + 31) / 32;
  const uint32_t spotWords = (spotCount + 31) / 32;
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
      clustersOmniGrid.resize(0);
    if (!hasSpotLights)
      clustersSpotGrid.resize(0);
  }
}

void ClusteredLightsGrid::fill()
{
  const uint32_t omniWords = getOmniWords();
  const uint32_t spotWords = getSpotWords();

  TIME_D3D_PROFILE(clusteredFill_CPU);
  fillClusteredCB(clustersOmniGrid.data(), omniWords, clustersSpotGrid.data(), spotWords);
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
