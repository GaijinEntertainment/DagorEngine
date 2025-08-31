//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

namespace flexGrid
{
constexpr int SUBDIV_CACHE_MAIN_THREAD_SIZE = 8; // entry per LandMeshRenderer::RENDER_TYPES_COUNT
constexpr int SUBDIV_CACHE_ASYNC_SIZE = 8;       // entry per LandMeshRenderer::RENDER_TYPES_COUNT
constexpr uint32_t OCCLUSION_MIN_PATCH_LEVEL = 3;
constexpr uint32_t OCCLUSION_MAX_PATCH_LEVEL = 8;
constexpr float CULLING_DEFAULT_EXPAND_UP = 0.5f;    // more than hmap_displacement_up
constexpr float CULLING_DEFAULT_EXPAND_DOWN = -0.5f; // less than hmap_displacement_down
constexpr uint32_t MAX_REASONABLE_SUBDIV_LEVEL = 16;
constexpr float EPSILON = 0.00001f;
} // namespace flexGrid

struct FlexGridConfig
{
  struct Metrics
  {
    float maxPatchRadiusError = 1.0f;
    float closePatchRadiusModifier = 1.0f;
    float maxHeightError = 0.1f;
    float maxAbsoluteHeightError = 32.0f;
    uint32_t maxQuadsPerMeterExp = 1; // 2^param quads per meter
  };

  // render
  uint32_t maxInstanceCount = 4096; // 4k is 2m triangles

  // subdiv
  Metrics forceLow{1.0f, 1.0f, 0.02f, 1000.0f, 1};
  Metrics low{0.5f, 0.77f, 0.007f, 800.0f, 3};
  Metrics medium{0.4f, 0.4f, 0.005f, 200.0f, 4};
  Metrics high{0.4f, 0.23f, 0.004f, 100.0f, 5};
  Metrics ultraHigh{0.3f, 0.25f, 0.003f, 150.0f, 6};

  uint32_t minSubdivLevel = 2;
  float tessellationPatchExpandCullingUp = flexGrid::CULLING_DEFAULT_EXPAND_UP;
  float tessellationPatchExpandCullingDown = flexGrid::CULLING_DEFAULT_EXPAND_DOWN;

  float additionalSubdivMinDistance = 2.0f;
  float additionalSubdivMaxDistance = 70.0f;

  struct FovBased
  {
    float projScaleRelaxStart = 1.732f; // 1/tan(30) == 1.732, relax if fov-x is less than 60 deg
    float projScaleRelaxTerm = 0.077f;
    float tesselationRelaxScale = 4.0f; // 0 - means no modification, bigger number - less polycount

    float minDistTerm = 10.0f;
    float maxDistTerm = 40.0f;
    float dotProductThresholdConst = 0.985f;
    float dotProductThresholdTerm = 0.01f;
  } fovBased;
};