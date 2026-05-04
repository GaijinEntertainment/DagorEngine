//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <math/dag_Point2.h>
#include <math/dag_frustum.h>
#include <generic/dag_carray.h>
#include <generic/dag_smallTab.h>
#include <heightmap/lodGrid.h>
#include <3d/dag_resPtr.h>
#include <EASTL/optional.h>
#include <heightmap/heightmapCullingQuality.h>


class IHeightmapHandler;

struct LodGridVertexData
{
  Ibuffer *ib = nullptr;
  Ibuffer *quadsIb = nullptr;
  int patchDim = 0;
  volatile int refCnt = 0;
  bool recreateBuffers = false;
  void close();
  bool init(int dim_start);
  bool createBuffers();
  void beforeResetDevice();
  void afterResetDevice();
};

class HeightmapHeightCulling
{
public:
  static constexpr float NO_WATER_ON_LEVEL = -10000.0f;

  bool init(IHeightmapHandler *handler);
  void updateMinMaxHeights(IHeightmapHandler *handler, const IBBox2 &ib);
  void getMinMax(int lod, const Point2 &patch_corner, const real &patch_size, real &hMin, real &hMax) const;
  void getMinMaxInterpolated(int lod, const Point2 &at, real &hMin, real &hMax) const;
  float getLod(float patch_size) const;
  float getChunkSize() const { return chunkSize; }
  HeightmapHeightCulling()
  {
    mem_set_0(lodChunkSizes);
    mem_set_0(lodOffsets);
  }
  ~HeightmapHeightCulling();
  void setUpDisplacement(float v) { displacementUpwardMaxOffset = v; };
  void setDownDisplacement(float v) { displacementDownwardMaxOffset = v; };
  float getDisplacementRange() const { return displacementUpwardMaxOffset + displacementDownwardMaxOffset; };

protected:
  static constexpr int HEIGHT_CULLING_BUFFER_SIZE = 128;
  static constexpr int HEIGHT_CULLING_LOD_COUNT = 8;

  Point2 generateLodPoint(int offset, int i, int j, const int lod_size);

  carray<float, HEIGHT_CULLING_LOD_COUNT> lodChunkSizes;
  carray<int, HEIGHT_CULLING_LOD_COUNT> lodOffsets;
  SmallTab<Point2, TmpmemAlloc> minMaxHeights;
  float displacementUpwardMaxOffset = 0, displacementDownwardMaxOffset = 0;
  Point2 origin = Point2::ZERO;
  int arraySize = 0;
  float hmapSize = 0;
  float chunkSize = 0;
  int chunkSizeInTexels = 1;
  real absMin = MIN_REAL;
  real absMax = MAX_REAL;
};

union LodGridPatchParams
{
  struct
  {
    float size;
    uint32_t tess;
    float originX, originY;
  };
  Point4 params;

  LodGridPatchParams() = default;
  LodGridPatchParams(float _size, uint32_t _tess, float origin_x, float origin_y)
  {
    size = _size;
    tess = _tess;
    originX = origin_x;
    originY = origin_y;
  }
  LodGridPatchParams(const Point4 &p) : params(p) {}
};

struct LodGridCullData
{
  Tab<LodGridPatchParams> patches;
  enum
  {
    DIAMOND_FIRST_LTRB,
    REGULAR_RTLB,
    REGULAR_LTRB,
    ADD_TRICNT
  };
  carray<Tab<LodGridPatchParams>, ADD_TRICNT> additionalTriPatches;
  Tab<uint8_t> edgesData;
  Tab<uint32_t> morphData;
  bool hasAdditionalTriPatches() const
  {
    return !additionalTriPatches[0].empty() || !additionalTriPatches[1].empty() || !additionalTriPatches[2].empty();
  }
  bool hasPatches() const { return !patches.empty() || hasAdditionalTriPatches(); }
  uint32_t morph_at = 0, morph_no_edges_at = 0, edges_at = 0;
  int startFlipped = 1000000;      // starting from startFlipped, patches triangulation is flipped
  Point2 originPos = Point2::ZERO; // just in case
  float scaleX = 1;
  uint32_t lod0PatchesCount = 0;
  LodGrid lodGrid;
  Point4 worldToLod0 = Point4::ZERO;
  float maxDisplacementErrAtMaxLod = 0.f;
  bool useHWTesselation = true;
  bool exact_edges = false;
  eastl::optional<Frustum> frustum;

  LodGridCullData(IMemAlloc *mem = midmem) : patches(mem), edgesData(mem), morphData(mem) {}
  int getCount() const { return patches.size(); }
  void eraseAll()
  {
    morph_at = morph_no_edges_at = edges_at = 0;
    edgesData.clear();
    morphData.clear();
    patches.clear();
    for (int i = 0; i < additionalTriPatches.size(); ++i)
      additionalTriPatches[i].clear();
    startFlipped = 1000000;
  }
};

class Occlusion;
class HMapTesselationData;

static inline float proj_to_distance_scale(const TMatrix4 &proj) { return proj._34 > 0 ? max(fabsf(proj._11), fabsf(proj._22)) : 0; }

struct HeightmapFrustumCullingInfo
{
  Point3 world_pos = {0, 0, 0};
  float camera_height = 0;
  float water_level = HeightmapHeightCulling::NO_WATER_ON_LEVEL;
  Frustum frustum;
  const Occlusion *occlusion = nullptr;
  const vec4f *world_bbox_xzs = nullptr;
  int min_tank_lod = 0;
  int lod0subdiv = 0;
  float lod0scale = 1.0f;
  HeightmapMetricsQuality metrics;
};


void cull_lod_grid(const LodGrid &lodGrid, int max_lod, float originPosX, float originPosY, float scaleX, float scaleY, float alignX,
  float alignY, float hMin, float hMax, const Frustum *frustum, const BBox2 *clip, LodGridCullData &cull_data,
  const Occlusion *occlusion, float &out_lod0_area_radius, int hmap_tess_factorVarId = -1, int dim = default_patch_dim,
  bool fight_t_junctions = true, const HeightmapHeightCulling *handler = NULL, BBox2 *lodsRegion = nullptr,
  float waterLevel = HeightmapHeightCulling::NO_WATER_ON_LEVEL, const Point3 *viewPos = nullptr,
  eastl::function<bool(const Point3_vec4 &pos, const Point3_vec4 &posRB)> cullCb = {});

struct MetricsErrors;
void cull_lod_grid3(const MetricsErrors &errors, LodGridCullData &cull_data, const Frustum &frustum, const Occlusion *use_occlusion,
  float hMin, float hMax, const BBox2 &clip, float maxUpwardDisplacement, float maxDownwardDisplacement, bool mirror, vec3f vp,
  const vec4f *bbox2, const HeightmapMetricsQuality &q);
