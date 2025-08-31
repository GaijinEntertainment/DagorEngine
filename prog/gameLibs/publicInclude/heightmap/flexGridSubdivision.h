//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <math/dag_Point3.h>
#include <math/dag_math3d.h>
#include <math/dag_frustum.h>
#include <math/dag_TMatrix4.h>
#include <memory/dag_fixedBlockAllocator.h>
#include <dag/dag_vector.h>
#include <heightmap/flexGridDefines.h>

class HeightmapHandler;
class Occlusion;

namespace HeightmapUtils
{
// packed uint16_t [0, uint16_t::Max]
uint16_t getHeight(uint32_t x, uint32_t y, uint32_t hmap_size, const HeightmapHandler &hmap_handler);
uint16_t getHeightSafe(uint32_t x, uint32_t y, uint32_t hmap_size, const HeightmapHandler &hmap_handler);

// unpacked [hMin, hMax]
float getHeightAtPointUnpack(uint32_t x, uint32_t y, uint32_t hmap_size, const HeightmapHandler &hmap_handler);
Point3 getPointInBbox(int32_t x, int32_t y, uint32_t hmap_size, const BBox3 &bbox, const HeightmapHandler &hmap_handler);
} // namespace HeightmapUtils

class FlexGridSubdivisionAsset
{
public:
  FlexGridSubdivisionAsset();

  uint32_t getHeightfieldMaxLevel() const;
  const BBox3 &getBBox() const;

  BBox3 calculatePatchBBox(const HeightmapHandler &hmap_handler, uint32_t level, uint32_t x, uint32_t y) const;

  struct HeightRange
  {
    uint16_t max;
    uint16_t min;
  };

  struct HeightError
  {
    Point3 maxErrorPosition;
    float maxError;
  };

  struct EdgeDirection
  {
    uint32_t p1 = 0; // packed top 8x4 quads
    uint32_t p2 = 0; // packed bottom 8x4 quads

    // patch data
    uint32_t level = 0;
    uint32_t x = 0;
    uint32_t y = 0;
  };

  const HeightRange &getHeightRange(uint32_t chain_index) const;
  const HeightError &getHeightError(uint32_t chain_index) const;
  const EdgeDirection &getEdgeDirection(uint32_t chain_index) const;
  uint32_t getEdgeDirectionMaxLevel() const;

private:
  friend class FlexGridRenderer;

  BBox3 subdivBBox;
  uint32_t patchSizeQuads = 0u;

  dag::Vector<HeightRange> heightRangeChain;
  dag::Vector<HeightError> heightErrorChain;
  dag::Vector<EdgeDirection> edgeDirectionChain;
  uint32_t heightRangeChainLevels = 0;
  uint32_t heightErrorChainLevels = 0;
  uint32_t edgeDirectionChainLevels = 0;

  bool isInitialized = false;
};

class FlexGridSubdivision
{
public:
  struct Patch
  {
    Patch *parent = nullptr;
    Patch *child[4] = {};

    BBox3 bbox;
    float radius = 0.f;

    uint32_t level = 0;
    uint32_t x = 0;
    uint32_t y = 0;

    float interjacentValue = 0.f;
    float patchDistance = 0.f;

    float heightError = 0.f;
    float radiusError = 0.f;
    float absoluteError = 0.f;
    Point3 maxErrorPosition;

    uint32_t edgeDirPacked1 = 0;
    uint32_t edgeDirPacked2 = 0;
    uint32_t parentEdgeDataOffset = 0;
    uint32_t parentEdgeDataScale = 1;

    uint8_t classifyPlaneId = 0;
    bool isTerminated = false;
  };

  struct SubdivisionInstance
  {
    Patch *root = nullptr;
    FlexGridSubdivisionAsset *subdivAsset = nullptr;
    const Occlusion *occlusion = nullptr;
    FlexGridConfig::Metrics metrics = {};
    Frustum frustum;
    Point3 localViewPosition;
    Point3 viewDirXZ;
    float projectionScaleX = 0.f;
    float projectionScaleY = 0.f;
    float projectionZDiv = 1.f;
    float projectionW = 0.f;
    uint32_t terminatedPatchesCount = 0u;
    uint32_t minSubdivisionLevel = 0u;
    uint32_t maxSubdivisionLevel = 0u;
    bool limitLevelByHeightfield = false;

    uint16_t patchAllocatorKey = 0;
  };

  FlexGridSubdivision();
  ~FlexGridSubdivision();

  const FlexGridSubdivision::SubdivisionInstance &prepare(FlexGridSubdivisionAsset *subdiv_asset, int flex_grid_subdiv_uid,
    const Point3 &subdiv_camera_pos, const TMatrix4 &subdiv_camera_view, const TMatrix4 &subdiv_camera_proj,
    const Frustum &view_frustum, const HeightmapHandler &hmap_handler, const Occlusion *occlusion,
    const FlexGridConfig::Metrics &metrics, bool limit_level_by_heightfield, uint32_t min_subdiv_level, const FlexGridConfig &cfg);

  const SubdivisionInstance &getPreparedSubdivision(int flex_grid_subdiv_uid);
  void clear();

private:
  void safeDeletePatchRecursive(Patch *&patch, FixedBlockAllocator *patch_allocator);

  void subdivideRecursive(Patch *&patch, Patch *parent, uint32_t level, uint32_t x, uint32_t y, SubdivisionInstance &subdiv_instance,
    const HeightmapHandler &hmap_handler, uint8_t frustum_plane_mask, const FlexGridConfig &cfg, float waterLevel, bool isWaterZone,
    const FlexGridSubdivisionAsset::EdgeDirection *parentEdgeDir);

  dag::Vector<FlexGridSubdivision::SubdivisionInstance> subdivisionCache;
  dag::Vector<FixedBlockAllocator> patchAllocators;
};
