//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <3d/dag_resPtr.h>
#include <3d/dag_lockTexture.h>
#include <vecmath/dag_vecMath.h>
#include <math/dag_Point2.h>
#include <math/dag_Point3.h>
#include <math/dag_bounds3.h>
#include <math/dag_bounds2.h>
#include <math/integer/dag_IBBox2.h>
#include "heightmapRenderer.h"
#include "heightmapCulling.h"
#include "heightmapTesselationData.h"
#include "heightmapPhysHandler.h"
#include "lodGrid.h"
#include <EASTL/span.h>
#include <ska_hash_map/flat_hash_map2.hpp>

class IGenLoad;
class GlobalSharedMemStorage;
class Occlusion;
struct Frustum;

class HeightmapHandler : public HeightmapPhysHandler
{
  friend HMapTesselationData;

protected:
  struct HeightmapRenderData
  {
    uint32_t texFMT = 0;
    UniqueTex heightmap;
    bool diamondSubDiv = true;
  };

public:
  static constexpr int HMAP_BSIZE = 32;
  static constexpr int BASE_HMAP_LOD_COUNT = 8;

  ~HeightmapHandler() { close(); }
  HeightmapHandler() :
    preparedOriginPos(0.f, 0.f, 0.f),
    preparedCameraHeight(0.f),
    preparedWaterLevel(HeightmapHeightCulling::NO_WATER_ON_LEVEL),
    lodDistance(1),
    lodCount(BASE_HMAP_LOD_COUNT),
    hmapDimBits(0),
    hmapTData(*this)
  {}

  bool init();
  void afterDeviceReset();
  void close();
  bool loadDump(IGenLoad &loadCb, bool load_render_data, GlobalSharedMemStorage *sharedMem);
  void fillHmapTextures();
  bool fillHmapRegion(int region_index, bool NVworkaround_applyOnNextFrame = false);

  bool isEnabledMipsUpdating() const { return enabledMipsUpdating; }
  void setEnableMipsUpdating(bool enable) { enabledMipsUpdating = enable; }

  // Returns true if we should render it
  bool prepare(const Point3 &world_pos, float camera_height, float water_level = HeightmapHeightCulling::NO_WATER_ON_LEVEL);
  void prepareHmapModificaton(); // called from prepare, unless pushHmapModificationOnPrepare is off
  void render(int min_tank_lod); // Uses parameters from prepare call.

  void frustumCulling( // Independent from prepare for multithreading.
    LodGridCullData &data, const Point3 &world_pos, float camera_height, float water_level, const Frustum &frustum, int min_tank_lod,
    const Occlusion *occlusion, int lod0subdiv = 0, float lod0scale = 1.0f);
  void renderCulled(const LodGridCullData &);

  void renderOnePatch(); // no tesselation, render whole area
  void invalidateCulling(const IBBox2 &);
  void setMaxUpwardDisplacement(float v);
  void setMaxDownwardDisplacement(float v);
  float getMaxUpwardDisplacement() const { return maxUpwardDisplacement; }
  float getMaxDownwardDisplacement() const { return maxDownwardDisplacement; }
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
  void tesselatePatch(const IPoint2 &cell, bool enable) { hmapTData.tesselatePatch(cell, enable); }
  void addTessSphere(const Point2 &pos, float radius) { hmapTData.addTessSphere(pos, radius); }
  void addTessRect(const TMatrix &transform, bool project_volume) { hmapTData.addTessRect(transform, project_volume); }

  int getLodDistance() const { return lodDistance; }
  void setLodDistance(int lodD) { lodDistance = lodD; }
  void setLodCount(int lods) { lodCount = lods; }
  void setHmapDistanceMul(float hmap_distance_mul) { hmapDistanceMul = hmap_distance_mul; }
  Point3 getPreparedOriginPos() const { return preparedOriginPos; }
  float getPreparedCameraHeight() const { return preparedCameraHeight; }
  float getPreparedWaterLevel() const { return preparedWaterLevel; }
  // incremented when terrain changes
  int getTerrainStateVersion() const { return terrainStateVersion; }

  eastl::unique_ptr<HeightmapHeightCulling> heightmapHeightCulling;
  bool pushHmapModificationOnPrepare = true;

protected:
  int calcLod(int min_lod, const Point3 &origin_pos, float camera_height) const;
  void fillHmapRegionDetailed(IPoint2 region_pivot, IPoint2 region_width, bool updateMips, BaseTexture *upload_tex,
    LockedImage2D upload_texlock, eastl::span<uint16_t> temp_mem, bool NVworkaround_applyOnNextFrame = false);
  HeightmapRenderer renderer;
  Point3 preparedOriginPos;
  float preparedCameraHeight;
  float preparedWaterLevel;
  eastl::unique_ptr<HeightmapRenderData> renderData;
  bool fillHmapTexturesNeeded = false;
  int hmapDimBits;
  int lodDistance;
  int lodCount;
  float hmapDistanceMul = 1.0f;
  float maxUpwardDisplacement = 0.5f;
  float maxDownwardDisplacement = 0.0f;
  ska::flat_hash_set<int> heightChangesIndex;
  ska::flat_hash_map<uint32_t, uint16_t> visualHeights;
  HMapTesselationData hmapTData;
  UniqueTex hmapUploadTex;
  int lastRegionUpdated_NVworkaround = -1;
  bool needUpdate = true;
  bool enabledMipsUpdating = true;
  bool forceCsRegionCopy = false;
  int terrainStateVersion = 0;
};
