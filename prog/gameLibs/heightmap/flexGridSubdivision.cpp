// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <heightmap/flexGridSubdivision.h>
#include <heightmap/flexGridSubdivisionUtils.h>
#include <heightmap/flexGridConsts.hlsli>

#include <heightmap/heightmapHandler.h>
#include <math/dag_bounds3.h>
#include <math/dag_Point2.h>
#include <util/dag_convar.h>
#include <scene/dag_occlusion.h>

namespace convar
{
CONSOLE_BOOL_VAL("render", terrain_flex_grid_occlusion_culling, true);
CONSOLE_FLOAT_VAL("flexgrid", metrics_water_min_lod, 7.f);
CONSOLE_FLOAT_VAL("flexgrid", metrics_water_height_mod, 0.2f);
} // namespace convar

namespace HeightmapUtils
{
uint16_t getHeight(uint32_t x, uint32_t y, uint32_t hmap_size, const HeightmapHandler &hmap_handler)
{
  G_ASSERT(x <= hmap_size && y <= hmap_size);

  return hmap_handler.getHeightmapHeightUnsafe(IPoint2(x, y));
}

uint16_t getHeightSafe(uint32_t x, uint32_t y, uint32_t hmap_size, const HeightmapHandler &hmap_handler)
{
  const uint32_t xClamped = eastl::min(x, hmap_size - 1);
  const uint32_t yClamped = eastl::min(y, hmap_size - 1);

  return hmap_handler.getHeightmapHeightUnsafe(IPoint2(xClamped, yClamped));
}

float getHeightAtPointUnpack(uint32_t x, uint32_t y, uint32_t hmap_size, const HeightmapHandler &hmap_handler)
{
  return hmap_handler.unpackHeightmapHeight(getHeightSafe(x, y, hmap_size, hmap_handler));
}

Point3 getPointInBbox(int32_t x, int32_t y, uint32_t hmap_size, const BBox3 &bbox, const HeightmapHandler &hmap_handler)
{
  Point3 result;

  const float bboxSizeX = bbox[1].x - bbox[0].x;
  const float bboxSizeZ = bbox[1].z - bbox[0].z;

  if (bboxSizeX < flexGrid::EPSILON || bboxSizeZ < flexGrid::EPSILON)
  {
    G_ASSERT(false);
    return result;
  }

  const float k = 1.f / float(hmap_size);

  result.x = bbox[0].x + x * k * bboxSizeX;
  result.y = getHeightAtPointUnpack((x > 0) ? uint32_t(x) : 0u, (y > 0) ? uint32_t(y) : 0u, hmap_size, hmap_handler);
  result.z = bbox[0].z + y * k * bboxSizeZ;

  return result;
}
} // namespace HeightmapUtils
// FlexGridSubdivisionAsset
namespace flexGridDetails
{
static float interpolate_quad(float x_factor, float y_factor, float x0y0, float x1y0, float x0y1, float x1y1)
{
  return x0y0 + x_factor * (x1y0 - x0y0) + y_factor * (x0y1 - x0y0) + x_factor * y_factor * (x0y0 + x1y1 - x1y0 - x0y1);
}
} // namespace flexGridDetails

FlexGridSubdivisionAsset::FlexGridSubdivisionAsset()
{
  patchSizeQuads = FLEXGRID_PATCH_QUADS;

  subdivBBox[0] = Point3(-1, 0, -1);
  subdivBBox[1] = Point3(1, 0, 1);

  heightRangeChainLevels = 0;
  heightErrorChainLevels = 0;
}

uint32_t FlexGridSubdivisionAsset::getHeightfieldMaxLevel() const { return heightRangeChainLevels; }

const BBox3 &FlexGridSubdivisionAsset::getBBox() const { return subdivBBox; }

BBox3 FlexGridSubdivisionAsset::calculatePatchBBox(const HeightmapHandler &hmap_handler, uint32_t level, uint32_t x, uint32_t y) const
{
  BBox3 patchBBox;

  uint32_t levelSize = FlexGridSubdivisionUtils::get_level_size(level);

  G_ASSERT_RETURN(levelSize != 0, patchBBox);

  const float levelSizeF = float(levelSize);
  const float k = 1.f / levelSizeF;
  Point3 subdivBBoxSize = subdivBBox.width();

  patchBBox[0].x = subdivBBox[0].x + float(x) * k * subdivBBoxSize.x;
  patchBBox[1].x = subdivBBox[0].x + float(x + 1) * k * subdivBBoxSize.x;

  patchBBox[0].z = subdivBBox[0].z + float(y) * k * subdivBBoxSize.z;
  patchBBox[1].z = subdivBBox[0].z + float(y + 1) * k * subdivBBoxSize.z;

  const uint32_t hmapSize = hmap_handler.getHeightmapSizeX();

  uint16_t hMin, hMax;
  if (level >= heightRangeChainLevels)
  {
    uint32_t dlevel = level - heightRangeChainLevels + 1;

    // heights of heightfield quad corners
    uint32_t hfx = x >> dlevel;
    uint32_t hfy = y >> dlevel;

    float height_00 = float(HeightmapUtils::getHeight(hfx, hfy, hmapSize, hmap_handler));
    float height_10 = float(HeightmapUtils::getHeightSafe(hfx + 1, hfy, hmapSize, hmap_handler));
    float height_01 = float(HeightmapUtils::getHeightSafe(hfx, hfy + 1, hmapSize, hmap_handler));
    float height_11 = float(HeightmapUtils::getHeightSafe(hfx + 1, hfy + 1, hmapSize, hmap_handler));

    // origin of heightfield quad in subdivision space
    uint32_t origin_x = (x >> dlevel) << dlevel;
    uint32_t origin_y = (y >> dlevel) << dlevel;

    // coordinates in quad space
    float dlevelFactor = 1.f / FlexGridSubdivisionUtils::get_level_size(dlevel);
    float x0 = (x - origin_x) * dlevelFactor;
    float y0 = (y - origin_y) * dlevelFactor;
    float x1 = x0 + dlevelFactor;
    float y1 = y0 + dlevelFactor;

    auto minMaxPair = eastl::minmax({
      flexGridDetails::interpolate_quad(x0, y0, height_00, height_10, height_01, height_11),
      flexGridDetails::interpolate_quad(x0, y1, height_00, height_10, height_01, height_11),
      flexGridDetails::interpolate_quad(x1, y0, height_00, height_10, height_01, height_11),
      flexGridDetails::interpolate_quad(x1, y1, height_00, height_10, height_01, height_11),
    });

    hMin = uint16_t(minMaxPair.first);
    hMax = uint16_t(minMaxPair.second);
  }
  else
  {
    const HeightRange &range = getHeightRange(FlexGridSubdivisionUtils::get_chain_index(level, x, y));
    hMin = range.min;
    hMax = range.max;
  }

  patchBBox[0].y = hmap_handler.unpackHeightmapHeight(hMin);
  patchBBox[1].y = hmap_handler.unpackHeightmapHeight(hMax);

  return patchBBox;
}

const FlexGridSubdivisionAsset::HeightRange &FlexGridSubdivisionAsset::getHeightRange(uint32_t chain_index) const
{
  static const HeightRange zero_range = {0u, 0u};
  return (size_t(chain_index) < heightRangeChain.size()) ? heightRangeChain[chain_index] : zero_range;
}

const FlexGridSubdivisionAsset::HeightError &FlexGridSubdivisionAsset::getHeightError(uint32_t chain_index) const
{
  static const HeightError zero_error = {Point3{}, 0.f};
  return (size_t(chain_index) < heightErrorChain.size()) ? heightErrorChain[chain_index] : zero_error;
}

const FlexGridSubdivisionAsset::EdgeDirection &FlexGridSubdivisionAsset::getEdgeDirection(uint32_t chain_index) const
{
  static const EdgeDirection zero_edge_dir = EdgeDirection{0, 0};
  return (size_t(chain_index) < edgeDirectionChain.size()) ? edgeDirectionChain[chain_index] : zero_edge_dir;
}

uint32_t FlexGridSubdivisionAsset::getEdgeDirectionMaxLevel() const { return edgeDirectionChainLevels; }

namespace flexGridDetails
{
static bool is_sniper_fov(float projection_scale_x, const FlexGridConfig &cfg)
{
  return projection_scale_x > cfg.fovBased.projScaleRelaxStart;
}

static float get_fov_based_term(float projection_scale_x, const FlexGridConfig &cfg)
{
  return clamp((projection_scale_x - cfg.fovBased.projScaleRelaxStart) * cfg.fovBased.projScaleRelaxTerm, 0.0f, 1.0f);
}

static float get_additional_subdivide_min_distance(float projection_scale_x, const FlexGridConfig &cfg)
{
  return cfg.additionalSubdivMinDistance + cfg.fovBased.minDistTerm * get_fov_based_term(projection_scale_x, cfg);
}

static float get_additional_subdivide_max_distance(float projection_scale_x, const FlexGridConfig &cfg)
{
  return cfg.additionalSubdivMaxDistance + cfg.fovBased.maxDistTerm * get_fov_based_term(projection_scale_x, cfg);
}

static float get_fov_based_dot_product_threshold(float projection_scale_x, const FlexGridConfig &cfg)
{
  if (!is_sniper_fov(projection_scale_x, cfg))
  {
    return 0.0f; // always enabled
  }

  return cfg.fovBased.dotProductThresholdConst + cfg.fovBased.dotProductThresholdTerm * get_fov_based_term(projection_scale_x, cfg);
}

static float calculate_max_radius_error(float radius_error, float projection_scale_x, float patch_distance, const FlexGridConfig &cfg)
{
  const float relaxTerm = get_fov_based_term(projection_scale_x, cfg);
  return (1.f + relaxTerm * cfg.fovBased.tesselationRelaxScale) * radius_error;
}

static float calculate_max_radius_error_close_detailed(float radius_error, float projection_scale_x, float patch_distance,
  float fov_based_term, float pd_near, float pd_far, float close_patch_radius_modifier, const FlexGridConfig &cfg)
{
  float closeSubdivTerm = 1.0f;
  if (patch_distance < pd_far)
  {
    const float dotProductThreshold = get_fov_based_dot_product_threshold(projection_scale_x, cfg);

    float t1 = 1.0f - eastl::max(0.0f, (patch_distance - pd_far * 0.5f) / pd_far * 0.5f);
    float t2 = eastl::min(1.0f, patch_distance / pd_near);
    float t3 = (fov_based_term < dotProductThreshold) ? 0.0f : 1.0f;

    float t = t1 * t1 * t2 * t3;

    closeSubdivTerm = (1.0f - t) + t * close_patch_radius_modifier;
  }
  const float relaxTerm = get_fov_based_term(projection_scale_x, cfg);
  return (1.f + relaxTerm * cfg.fovBased.tesselationRelaxScale) * radius_error * closeSubdivTerm;
}

static float calculate_interjacent_value(const FlexGridSubdivision::Patch *patch, const FlexGridConfig::Metrics &metrics,
  float projection_scale_x, float fovBasedTerm, float pd_near, float pd_far, float max_height_error, const FlexGridConfig &cfg)
{
  const FlexGridSubdivision::Patch *parent = patch->parent;

  const float maxPatchRadiusError = flexGridDetails::calculate_max_radius_error_close_detailed(metrics.maxPatchRadiusError,
    projection_scale_x, patch->patchDistance, fovBasedTerm, pd_near, pd_far, metrics.closePatchRadiusModifier, cfg);
  float radiusErrorP = (parent != nullptr) ? parent->radiusError : maxPatchRadiusError;
  float radiusErrorRel = eastl::min(patch->radiusError, maxPatchRadiusError) / maxPatchRadiusError;
  float radiusErrorPRel = eastl::max(radiusErrorP, maxPatchRadiusError) / maxPatchRadiusError;

  float heightErrorP = (parent != nullptr) ? parent->heightError : max_height_error;
  float heightErrorRel = eastl::min(patch->heightError, max_height_error) / max_height_error;
  float heightErrorPRel = eastl::max(heightErrorP, max_height_error) / max_height_error;

  float errorRel = 1.f - eastl::max(radiusErrorRel, heightErrorRel);
  float errorPRel = eastl::max(radiusErrorPRel, heightErrorPRel) - 1.f;
  float interjacentValue = (errorRel > 0.f) ? (1.f - errorRel / (errorPRel + errorRel)) : 1.f;

  return clamp(interjacentValue, 0.0f, 1.0f);
}

static uint32_t calc_max_subdiv_level(uint32_t world_size_meters, uint32_t max_quads_per_meter_exp)
{
  uint32_t totalQuads = world_size_meters * (1 << max_quads_per_meter_exp);
  uint32_t tilesPerSide = totalQuads / FLEXGRID_PATCH_QUADS;

  uint32_t level = 0;
  while (tilesPerSide > 1)
  {
    tilesPerSide /= 2;
    ++level;
  }

  return eastl::min(level, flexGrid::MAX_REASONABLE_SUBDIV_LEVEL);
}
} // namespace flexGridDetails

// FlexGridSubdivision
const FlexGridSubdivision::SubdivisionInstance &FlexGridSubdivision::prepare(FlexGridSubdivisionAsset *subdiv_asset,
  int flex_grid_subdiv_uid, const Point3 &subdiv_camera_pos, const TMatrix4 &subdiv_camera_view, const TMatrix4 &subdiv_camera_proj,
  const Frustum &viewFrustum, const HeightmapHandler &hmap_handler, const Occlusion *occlusion, const FlexGridConfig::Metrics &metrics,
  bool limit_level_by_heightfield, uint32_t min_subdiv_level, const FlexGridConfig &cfg)
{
  G_ASSERTF(flex_grid_subdiv_uid < subdivisionCache.size(), "FlexGridSubdivision::prepare: out of bounds (subdivisionCache)");

  SubdivisionInstance &subdivInstance = subdivisionCache.at(flex_grid_subdiv_uid);

  subdivInstance.frustum = viewFrustum;
  subdivInstance.viewDirXZ.set_xyz(subdiv_camera_view.row[2]);
  subdivInstance.viewDirXZ.x = -subdivInstance.viewDirXZ.x;

  if (abs(subdivInstance.viewDirXZ.x) > flexGrid::EPSILON || abs(subdivInstance.viewDirXZ.z) > flexGrid::EPSILON)
    subdivInstance.viewDirXZ.y = 0.0f;

  subdivInstance.viewDirXZ.normalize();

  subdivInstance.localViewPosition = subdiv_camera_pos;
  subdivInstance.projectionScaleX = subdiv_camera_proj._11;
  subdivInstance.projectionScaleY = subdiv_camera_proj._22;
  subdivInstance.projectionZDiv = subdiv_camera_proj._34;
  subdivInstance.projectionW = subdiv_camera_proj._44;
  subdivInstance.terminatedPatchesCount = 0;
  subdivInstance.metrics = metrics;
  subdivInstance.subdivAsset = subdiv_asset;
  subdivInstance.minSubdivisionLevel = min_subdiv_level;
  subdivInstance.maxSubdivisionLevel =
    flexGridDetails::calc_max_subdiv_level(subdiv_asset->getBBox().width().x, metrics.maxQuadsPerMeterExp);
  subdivInstance.limitLevelByHeightfield = limit_level_by_heightfield;

  subdivInstance.occlusion = convar::terrain_flex_grid_occlusion_culling.get() ? occlusion : nullptr;

  const float waterLevel = hmap_handler.getPreparedWaterLevel();

  subdivideRecursive(subdivInstance.root, nullptr, 0, 0, 0, subdivInstance, hmap_handler, 0x3f, cfg, waterLevel, true, nullptr);

  return subdivInstance;
}

void FlexGridSubdivision::subdivideRecursive(Patch *&patch, Patch *parent, uint32_t level, uint32_t x, uint32_t y,
  SubdivisionInstance &subdiv_instance, const HeightmapHandler &hmap_handler, uint8_t frustum_plane_mask, const FlexGridConfig &cfg,
  float waterLevel, bool isWaterZone, const FlexGridSubdivisionAsset::EdgeDirection *parentEdgeDir)
{
  BBox3 patchBBox = (patch != nullptr && !patch->bbox.isempty())
                      ? patch->bbox
                      : subdiv_instance.subdivAsset->calculatePatchBBox(hmap_handler, level, x, y);
  uint8_t classifyPlaneId = (patch != nullptr) ? patch->classifyPlaneId : 0u;
  FixedBlockAllocator *patchAllocator = &patchAllocators.at(subdiv_instance.patchAllocatorKey);

  if (frustum_plane_mask != 0u)
  {
    DECL_ALIGN16(BBox3, clippingPatchBBox);
    clippingPatchBBox.lim[0] = patchBBox.lim[0];
    clippingPatchBBox.lim[1] = patchBBox.lim[1];
    clippingPatchBBox.lim[0].y += cfg.tessellationPatchExpandCullingDown;
    clippingPatchBBox.lim[1].y += cfg.tessellationPatchExpandCullingUp;
    int frustumCull = subdiv_instance.frustum.testBox(clippingPatchBBox);

    if (frustumCull == Frustum::OUTSIDE)
    {
      safeDeletePatchRecursive(patch, patchAllocator);
      return;
    }
    else if (frustumCull == Frustum::INSIDE)
      frustum_plane_mask = 0u;

    if (subdiv_instance.occlusion && level <= flexGrid::OCCLUSION_MAX_PATCH_LEVEL && level >= flexGrid::OCCLUSION_MIN_PATCH_LEVEL)
    {
      vec4f bmin = v_ld(&clippingPatchBBox[0].x);
      vec4f bmax = v_ldu(&clippingPatchBBox[1].x);
      vec4f center2 = v_add(bmax, bmin);
      vec4f extent2 = v_sub(bmax, bmin);
      if (subdiv_instance.occlusion->isOccludedBoxExtent2(center2, extent2))
      {
        safeDeletePatchRecursive(patch, patchAllocator);
        return;
      }
    }
  }

  if (patch == nullptr)
  {
    patch = new (patchAllocator->allocateOneBlock()) Patch();
    patch->parent = parent;
    patch->level = level;
    patch->x = x;
    patch->y = y;
  }
  if (patch->bbox.isempty())
  {
    patch->bbox = patchBBox;
    patch->radius = (patchBBox.center() - patchBBox[1]).length();
  }
  patch->classifyPlaneId = classifyPlaneId;
  const Point3 &bboxCenter = patchBBox.center();

  G_ASSERTF(patch->parent == parent, "FlexGridSubdivision::subdivideRecursive - wrong parent");

  float patchDistance = (subdiv_instance.localViewPosition - bboxCenter).length();
  float radiusError = abs((patch->radius * subdiv_instance.projectionScaleX) /
                          (patchDistance * subdiv_instance.projectionZDiv + subdiv_instance.projectionW));

  float absoluteHeightError = 0.f;
  float heightError = 0.f;

  const uint32_t chainIndex = FlexGridSubdivisionUtils::get_chain_index(level, x, y);
  const uint32_t hmapMaxLevel = subdiv_instance.subdivAsset->getHeightfieldMaxLevel();

  if (level < hmapMaxLevel)
  {
    const FlexGridSubdivisionAsset::HeightError &h_error = subdiv_instance.subdivAsset->getHeightError(chainIndex);

    absoluteHeightError = h_error.maxError;
    float maxHeightErrorDistance = (subdiv_instance.localViewPosition - h_error.maxErrorPosition).length();
    float maxHeightErrorTerm = max(0.01f, maxHeightErrorDistance * subdiv_instance.projectionZDiv + subdiv_instance.projectionW);
    heightError = abs((absoluteHeightError * subdiv_instance.projectionScaleY) / maxHeightErrorTerm);
    patch->maxErrorPosition = h_error.maxErrorPosition;
  }
  if (level < subdiv_instance.subdivAsset->getEdgeDirectionMaxLevel() - 4) // todo: calc and store maxReasonableEdgeLevel in metrics
  {
    const FlexGridSubdivisionAsset::EdgeDirection &edgeDir = subdiv_instance.subdivAsset->getEdgeDirection(chainIndex);
    patch->edgeDirPacked1 = edgeDir.p1;
    patch->edgeDirPacked2 = edgeDir.p2;
    patch->parentEdgeDataOffset = 0; // need these values only if using other patch's data
    patch->parentEdgeDataScale = 1;  // need these values only if using other patch's data

    parentEdgeDir = &edgeDir;
  }
  else
  {
    patch->edgeDirPacked1 = parentEdgeDir->p1;
    patch->edgeDirPacked2 = parentEdgeDir->p2;

    uint32_t levelDiff = level - parentEdgeDir->level;
    uint32_t scale = 1u << levelDiff;
    uint16_t quadStartX = uint16_t(x % scale) * (1 << (3 - levelDiff));
    uint16_t quadStartY = uint16_t(y % scale) * (1 << (3 - levelDiff));

    patch->parentEdgeDataOffset = (static_cast<uint32_t>(quadStartX) << 16) | static_cast<uint32_t>(quadStartY);
    patch->parentEdgeDataScale = scale;
  }

  patch->heightError = heightError;
  patch->radiusError = radiusError;
  patch->absoluteError = absoluteHeightError;
  patch->patchDistance = patchDistance;

  uint16_t maxLevel = subdiv_instance.maxSubdivisionLevel;
  if (subdiv_instance.limitLevelByHeightfield)
    maxLevel = eastl::min(maxLevel, uint16_t(subdiv_instance.subdivAsset->getHeightfieldMaxLevel() - 1u));

  float heightErr = subdiv_instance.metrics.maxHeightError;
  const float heightAbsErr = subdiv_instance.metrics.maxAbsoluteHeightError;

  const float pdFar = flexGridDetails::get_additional_subdivide_max_distance(subdiv_instance.projectionScaleX, cfg);
  const float pdNear = flexGridDetails::get_additional_subdivide_min_distance(subdiv_instance.projectionScaleX, cfg);

  float fovBasedTerm = 1.0f;
  if (patchDistance < pdFar && level >= maxLevel - 2)
  {
    Point3 toPatchCenter = bboxCenter - subdiv_instance.localViewPosition;
    if (abs(toPatchCenter.x) > flexGrid::EPSILON || abs(toPatchCenter.z) > flexGrid::EPSILON)
    {
      toPatchCenter.y = 0.0f;
      toPatchCenter.normalize();
      fovBasedTerm = dot(toPatchCenter, subdiv_instance.viewDirXZ);
    }
  }

  patch->interjacentValue = flexGridDetails::calculate_interjacent_value(patch, subdiv_instance.metrics,
    subdiv_instance.projectionScaleX, fovBasedTerm, pdNear, pdFar, heightErr, cfg);
  const float maxPatchRadiusError = flexGridDetails::calculate_max_radius_error_close_detailed(
    subdiv_instance.metrics.maxPatchRadiusError, subdiv_instance.projectionScaleX, patch->patchDistance, fovBasedTerm, pdNear, pdFar,
    subdiv_instance.metrics.closePatchRadiusModifier, cfg);

  if (isWaterZone)
  {
    isWaterZone = (patch->bbox.lim[0].y < waterLevel) && (patch->bbox.lim[1].y > waterLevel);
  }
  bool waterSubdivCriteria = false;
  if (isWaterZone && level < hmapMaxLevel)
  {
    if (level < convar::metrics_water_min_lod.get())
    {
      waterSubdivCriteria = true;
    }
    else
    {
      heightErr *= convar::metrics_water_height_mod.get();
    }
  }

  const bool metricsCriteria = (maxPatchRadiusError <= radiusError) || (heightErr <= heightError) ||
                               (heightAbsErr < abs(absoluteHeightError)) || waterSubdivCriteria;

  if (subdiv_instance.minSubdivisionLevel > level || (level < maxLevel && metricsCriteria))
  {
    patch->isTerminated = false;

    ++level;
    x <<= 1;
    y <<= 1;

    subdivideRecursive(patch->child[0], patch, level, x + 0, y + 0, subdiv_instance, hmap_handler, frustum_plane_mask, cfg, waterLevel,
      isWaterZone, parentEdgeDir);
    subdivideRecursive(patch->child[1], patch, level, x + 1, y + 0, subdiv_instance, hmap_handler, frustum_plane_mask, cfg, waterLevel,
      isWaterZone, parentEdgeDir);
    subdivideRecursive(patch->child[2], patch, level, x + 0, y + 1, subdiv_instance, hmap_handler, frustum_plane_mask, cfg, waterLevel,
      isWaterZone, parentEdgeDir);
    subdivideRecursive(patch->child[3], patch, level, x + 1, y + 1, subdiv_instance, hmap_handler, frustum_plane_mask, cfg, waterLevel,
      isWaterZone, parentEdgeDir);
  }
  else
  {
    patch->isTerminated = true;

    ++subdiv_instance.terminatedPatchesCount;

    for (size_t childIndex = 0; childIndex < eastl::size(patch->child); ++childIndex)
      safeDeletePatchRecursive(patch->child[childIndex], patchAllocator);
  }
}

void FlexGridSubdivision::safeDeletePatchRecursive(Patch *&patch, FixedBlockAllocator *patch_allocator)
{
  if (patch != nullptr)
  {
    for (size_t i = 0; i < eastl::size(patch->child); ++i)
      safeDeletePatchRecursive(patch->child[i], patch_allocator);

    patch_allocator->freeOneBlock(patch);
    patch = nullptr;
  }
}

FlexGridSubdivision::FlexGridSubdivision()
{
  subdivisionCache.resize(flexGrid::SUBDIV_CACHE_MAIN_THREAD_SIZE + flexGrid::SUBDIV_CACHE_ASYNC_SIZE);
  patchAllocators.emplace_back(sizeof(Patch), 512);

  for (size_t i = 0; i < flexGrid::SUBDIV_CACHE_MAIN_THREAD_SIZE; ++i)
  {
    subdivisionCache[i].patchAllocatorKey = 0;
  }
  for (size_t i = 0; i < flexGrid::SUBDIV_CACHE_ASYNC_SIZE; ++i)
  {
    patchAllocators.emplace_back(sizeof(Patch), 512);
    subdivisionCache[flexGrid::SUBDIV_CACHE_MAIN_THREAD_SIZE + i].patchAllocatorKey = uint16_t(patchAllocators.size() - 1);
  }
}

FlexGridSubdivision::~FlexGridSubdivision() { clear(); }

void FlexGridSubdivision::clear()
{
  for (auto &subdivInstance : subdivisionCache)
    safeDeletePatchRecursive(subdivInstance.root, &patchAllocators.at(subdivInstance.patchAllocatorKey));
  subdivisionCache.clear();
}

const FlexGridSubdivision::SubdivisionInstance &FlexGridSubdivision::getPreparedSubdivision(int flex_grid_subdiv_uid)
{
  G_ASSERTF(flex_grid_subdiv_uid < subdivisionCache.size(),
    "FlexGridSubdivision::getPreparedSubdivision out of bounds (subdivisionCache)");

  return subdivisionCache.at(flex_grid_subdiv_uid);
}