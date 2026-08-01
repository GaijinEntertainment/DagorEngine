//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <3d/dag_resPtr.h>
#include <3d/dag_lockTexture.h>
#include <math/dag_Point2.h>
#include <math/dag_Point3.h>
#include <math/integer/dag_IBBox2.h>
#include "simpleHeightmapRenderer.h"
#include "heightmapCulling.h"
#include "heightmapPhysHandler.h"
#include "lodGrid.h"
#include <EASTL/span.h>
#include <EASTL/unique_ptr.h>
#include <ska_hash_map/flat_hash_map2.hpp>

class IGenLoad;
class Occlusion;
class SimpleHeightmapRenderer;
struct MetricsErrors;
struct Frustum;

struct HeightmapFrustumCullingInfo;

class IHeightmapHandler
{
public:
  virtual float getMaxUpwardDisplacement() const = 0;
  virtual float getMaxDownwardDisplacement() const = 0;
  virtual IPoint2 getHeightmapSize() const = 0;
  virtual Point3 getHeightmapOffset() const = 0;
  virtual float getHeightmapCellSize() const = 0;
  virtual bool getHeightmapHeightMinMaxInChunk(const Point2 &pos, const real &chunkSize, real &hmin, real &hmax) const = 0;

protected:
  ~IHeightmapHandler() = default;
};

class HeightmapHandler : public IHeightmapHandler, public HeightmapPhysHandler
{

protected:
  struct HeightmapRenderData
  {
    uint32_t texFMT = 0;
    UniqueTex heightmap;
    d3d::SamplerHandle heightmapSampler = d3d::SamplerHandle::Invalid;
  };

public:
  static constexpr int HMAP_BSIZE = 32;
  static constexpr int BASE_HMAP_LOD_COUNT = 8;

  ~HeightmapHandler() { close(); }
  HeightmapHandler() : hmapDimBits(0) {}

  bool init(int dim_bits = 0);
  void afterDeviceReset();
  void close();
  bool loadDump(IGenLoad &loadCb, bool load_render_data, float water_level = HeightmapHeightCulling::NO_WATER_ON_LEVEL,
    float shore_error = 2.0f);
  void fillHmapTextures();
  bool fillHmapRegion(int region_index, bool NVworkaround_applyOnNextFrame = false);

  bool isEnabledMipsUpdating() const { return enabledMipsUpdating; }
  void setEnableMipsUpdating(bool enable) { enabledMipsUpdating = enable; }

  // Pure distance check, no side effects: true when world_pos is close enough to render the
  // tessellated heightmap. hmap_distance_mul scales the switch distance; it is the caller's
  // (per-level) config, not handler state, so every caller of one level must pass the same value.
  bool shouldRenderTessellatedHmap(const Point3 &world_pos, float hmap_distance_mul) const;
  // Binds the heightmap shader vars (setVars) and returns shouldRenderTessellatedHmap();
  // call before rendering terrain. const = mutates no member state; run makeBookKeeping() once per
  // frame for the per-frame upkeep.
  bool prepare(const Point3 &world_pos, float hmap_distance_mul) const;
  // Both const: pure functions of (metrics, fi) -> caller-owned cull data / draw calls. metrics and
  // metricsRenderer only change inside makeBookKeeping()/initRender(), never during cull or render,
  // so any number of culls may run concurrently between bookkeeping points.
  void frustumCulling(LodGridCullData &data, const HeightmapFrustumCullingInfo &fi) const;
  void renderCulled(const LodGridCullData &) const;

  void renderOnePatch(const Frustum &frustum); // no tesselation, render whole area; culled against the caller's frustum
  void invalidateCulling(const IBBox2 &);
  void setMaxUpwardDisplacement(float v);
  void setMaxDownwardDisplacement(float v);
  float getMaxUpwardDisplacement() const override { return maxUpwardDisplacement; }
  float getMaxDownwardDisplacement() const override { return maxDownwardDisplacement; }
  IPoint2 getHeightmapSize() const override { return {getHeightmapSizeX(), getHeightmapSizeY()}; }
  bool getHeightmapHeightMinMaxInChunk(const Point2 &pos, const real &chunkSize, real &hmin, real &hmax) const override
  {
    return HeightmapPhysHandler::getHeightmapHeightMinMaxInChunk(pos, chunkSize, hmin, hmax);
  }
  float getHeightmapCellSize() const override { return HeightmapPhysHandler::getHeightmapCellSize(); }
  Point3 getHeightmapOffset() const override { return HeightmapPhysHandler::getHeightmapOffset(); }
  // works only on a mip level 0 for simplicity
  bool setHeightmapHeightUnsafeVisual(const IPoint2 &cell, uint16_t ht)
  {
    G_ASSERT(cell.x >= 0 && cell.y >= 0 && cell.x < hmapWidth.x && cell.y < hmapWidth.y);
    int index = cell.x + cell.y * hmapWidth.x;
    return visualHeights.emplace(index, ht).second;
  }
  void clearHeightmapHeightsVisual() { visualHeights.clear(); }
  void changedHeightmapCellUnsafe(const IPoint2 &cell)
  {
    G_ASSERT(cell.x >= 0 && cell.y >= 0 && cell.x < hmapWidth.x && cell.y < hmapWidth.y);
    int heightsStride = hmapWidth.x / HMAP_BSIZE;
    int changesIndex = cell.x / HMAP_BSIZE + cell.y / HMAP_BSIZE * heightsStride;
    heightChangesIndex.insert(changesIndex);
  }

  // incremented when terrain changes
  int getTerrainStateVersion() const { return terrainStateVersion; }

  eastl::unique_ptr<HeightmapHeightCulling> heightmapHeightCulling;
  void initRender(bool clamp = true, float water_level = HeightmapHeightCulling::NO_WATER_ON_LEVEL, float shore_error = 2.0f);
  const UniqueTex *getTexture() const { return renderData ? &renderData->heightmap : nullptr; }
  void setTexture(UniqueTex &&);
  void setSampler(d3d::SamplerHandle &&);
  void setVars() const;
  void makeBookKeeping();
  bool isMirror() const { return mirror; }
  const MetricsErrors *getMetricsRaw() const { return metrics; }
  void initMetrics(float water_level, float shore_error_meters = 2.0f);

protected:
  // applies one pending visual-height-modified region per call; only makeBookKeeping() should call this
  void prepareHmapModificaton();
  void fillHmapRegionDetailed(IPoint2 region_pivot, IPoint2 region_width, bool updateMips, BaseTexture *upload_tex,
    LockedImageRawBytes upload_texlock, eastl::span<uint16_t> temp_mem, bool NVworkaround_applyOnNextFrame = false);
  SimpleHeightmapRenderer renderer;
  eastl::unique_ptr<HeightmapRenderData> renderData;
  bool fillHmapTexturesNeeded = false;
  int hmapDimBits;
  // These hardcoded displacements are required to account for additional GPU displacements which can be both up and downwards
  float maxUpwardDisplacement = 0.5f;
  float maxDownwardDisplacement = 0.5f;
  ska::flat_hash_set<int> heightChangesIndex;
  ska::flat_hash_map<uint32_t, uint16_t> visualHeights;
  UniqueTex hmapUploadTex;
  int lastRegionUpdated_NVworkaround = -1;
  float shoreErrorMeters = 2.0f;
  bool enabledMipsUpdating = true;
  int terrainStateVersion = 0;
  // Stable between bookkeeping points: written by initRender()/makeBookKeeping() only, read-only
  // for frustumCulling()/renderCulled().
  MetricsErrors *metrics = nullptr;
  SimpleHeightmapRenderer *metricsRenderer = nullptr;
};
