//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <render/frustumClusters.h>
#include <render/spotLightsManager.h>
#include <shaders/dag_computeShaders.h>
#include <generic/dag_tab.h>
#include <EASTL/array.h>
#include <EASTL/string.h>
#include <math/dag_TMatrix4.h>
#include <math/dag_Point4.h>
#include <3d/dag_resPtr.h>

class Occlusion;

struct ClusteredLightsGrid
{
  ClusteredLightsGrid() = default;
  ClusteredLightsGrid(FrustumClusters &clusters, const char *name_suffix, int initial_light_density);
  void reset();

  // GPU path: calls prepareFrustum and stores view/proj params for later dispatch.
  void prepareGPUCulling(mat44f_cref view, mat44f_cref proj, float znear, float zfar, float close_dist, float max_dist,
    uint32_t omni_bounds_size, uint32_t spot_bounds_size);

  // CPU path: fills bitmask arrays; if no lights found, shrinks corresponding array to 0.
  void cullCPU(mat44f_cref view, mat44f_cref proj, float znear, float close_dist, float max_dist, dag::ConstSpan<vec4f> omni_bounds,
    dag::ConstSpan<SpotLightsManager::RawLight> spot_lights, dag::ConstSpan<vec4f> spot_bounds, Occlusion *occlusion);

  void fill();
  void advanceFrameState();

  bool isGPU() const;
  bool hasOmni() const;
  bool hasSpot() const;
  bool newFrameHasLights() const;
  bool lastFrameHasLights() const;
  uint32_t getOmniWords() const;
  uint32_t getSpotWords() const;

private:
  static constexpr int MAX_FRAMES = 2;

  enum class FrameState : uint8_t
  {
    NO_CLUSTERED_LIGHTS,
    HAS_CLUSTERED_LIGHTS,
    NOT_INITED
  };
  FrameState frameState = FrameState::NOT_INITED;

  FrustumClusters *clusters = nullptr;
  eastl::string nameSuffix;
  Tab<uint32_t> clustersOmniGrid, clustersSpotGrid;
  ComputeShader precalculate_clustered_frustum_data_cs;
  ComputeShader precalculate_grid_lights_data_cs;
  ComputeShader precalculate_occlusion_z_slice_cs;
  ComputeShader fill_items_spheres_cs;
  ComputeShader cull_spots_cs;
  UniqueBuf lightsGridFrustumPoints;
  UniqueBuf lightsGridPrecalculatedData;
  UniqueBuf lightsOcclusionZSlice;
  TMatrix4 cullLightsViewTm;
  TMatrix4 cullLightsProjTm;
  Point4 cullLightsProjParams; // (znear, zFar, maxSliceDist, 0)
  uint32_t storedOmniCount = 0, storedSpotCount = 0;
  eastl::array<UniqueBuf, MAX_FRAMES> lightsFullGridCB;
  uint32_t lightsGridFrame = 0, allocatedWords = 0;

  void validateDensity(uint32_t words);
  void executeGPUFillingPipeline();
  void clusteredCullLights(mat44f_cref view, mat44f_cref proj, float znear, float min_dist, float max_dist,
    dag::ConstSpan<vec4f> omni_bounds, dag::ConstSpan<SpotLightsManager::RawLight> spot_lights, dag::ConstSpan<vec4f> spot_bounds,
    Occlusion *occlusion, bool &has_omni, bool &has_spot, uint32_t *omni_mask, uint32_t omni_words, uint32_t *spot_mask,
    uint32_t spot_words);
  bool fillClusteredCB(uint32_t *omni, uint32_t omni_words, uint32_t *spot, uint32_t spot_words);
};
