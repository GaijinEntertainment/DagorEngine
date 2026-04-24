//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <render/frustumClusters.h>
#include <render/spotLightsManager.h>
#include <generic/dag_tab.h>
#include <EASTL/array.h>
#include <EASTL/string.h>
#include <3d/dag_resPtr.h>

class Occlusion;

struct ClusteredLightsGrid
{
  ClusteredLightsGrid() = default;
  ClusteredLightsGrid(FrustumClusters &clusters, const char *name_suffix, int initial_light_density);
  void reset();

  // CPU path: fills bitmask arrays; if no lights found, shrinks corresponding array to 0.
  void cull(mat44f_cref view, mat44f_cref proj, float znear, float close_dist, float max_dist, dag::ConstSpan<vec4f> omni_bounds,
    dag::ConstSpan<SpotLightsManager::RawLight> spot_lights, dag::ConstSpan<vec4f> spot_bounds, Occlusion *occlusion);

  void fill();
  void advanceFrameState();

  bool hasOmni() const { return !clustersOmniGrid.empty(); }
  bool hasSpot() const { return !clustersSpotGrid.empty(); }
  // Current frame: result of the most recent cull.
  bool newFrameHasLights() const { return hasOmni() || hasSpot(); }
  // Previous frame: true if fill() was dispatched last frame (or NOT_INITED — forces first-frame update).
  bool lastFrameHasLights() const { return frameState != FrameState::NO_CLUSTERED_LIGHTS; }
  uint32_t getOmniWords() const { return clustersOmniGrid.size() / CLUSTERS_PER_GRID; }
  uint32_t getSpotWords() const { return clustersSpotGrid.size() / CLUSTERS_PER_GRID; }

private:
  static constexpr int CLUSTERS_PER_GRID = CLUSTERS_W * CLUSTERS_H * (CLUSTERS_D + 1);
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
  eastl::array<UniqueBuf, MAX_FRAMES> lightsFullGridCB;
  uint32_t lightsGridFrame = 0, allocatedWords = 0;

  void validateDensity(uint32_t words);
  void clusteredCullLights(mat44f_cref view, mat44f_cref proj, float znear, float min_dist, float max_dist,
    dag::ConstSpan<vec4f> omni_bounds, dag::ConstSpan<SpotLightsManager::RawLight> spot_lights, dag::ConstSpan<vec4f> spot_bounds,
    Occlusion *occlusion, bool &has_omni, bool &has_spot, uint32_t *omni_mask, uint32_t omni_words, uint32_t *spot_mask,
    uint32_t spot_words);
  bool fillClusteredCB(uint32_t *omni, uint32_t omni_words, uint32_t *spot, uint32_t spot_words);
};
