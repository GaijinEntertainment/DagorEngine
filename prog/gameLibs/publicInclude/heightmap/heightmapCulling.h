//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <math/dag_Point2.h>
#include <math/dag_frustum.h>
#include <generic/dag_carray.h>
#include <generic/dag_smallTab.h>
#include <heightmap/lodGrid.h>
#include <3d/dag_resPtr.h>
#include <EASTL/optional.h>


class HeightmapHandler;

struct LodGridVertexData
{
  Ibuffer *ib = nullptr;
  Ibuffer *quadsIb = nullptr;
  int patchDim = 0;
  int faceCnt = 0;
  int quadsCnt = 0;
  int verticesCnt = 0, indicesCnt = 0, quadsIndicesCnt = 0;
  volatile int refCnt = 0;
  bool recreateBuffers = false;
  void close();
  bool init(int dim);
  bool createBuffers();
  void beforeResetDevice();
  void afterResetDevice();
};

class HeightmapHeightCulling
{
public:
  static constexpr float NO_WATER_ON_LEVEL = -10000.0f;

  bool init(HeightmapHandler *handler);
  void updateMinMaxHeights(HeightmapHandler *handler, const IBBox2 &ib);
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

struct LodGridCullData
{
  Tab<Point4> patches;
  int startFlipped = 1000000;      // starting from startFlipped, patches triangulation is flipped
  Point2 originPos = Point2::ZERO; // just in case
  float scaleX = 1;
  uint32_t lod0PatchesCount = 0;
  LodGrid lodGrid;
  Point4 worldToLod0 = Point4::ZERO;
  bool useHWTesselation = true;
  eastl::optional<Frustum> frustum;

  LodGridCullData(IMemAlloc *mem = midmem) : patches(mem) {}
  int getCount() const { return patches.size(); }
  void eraseAll() { patches.clear(); }
};

class Occlusion;
class HMapTesselationData;

void cull_lod_grid(const LodGrid &lodGrid, int max_lod, float originPosX, float originPosY, float scaleX, float scaleY, float alignX,
  float alignY, float hMin, float hMax, const Frustum *frustum, const BBox2 *clip, LodGridCullData &cull_data,
  const Occlusion *occlusion, float &out_lod0_area_radius, int dim = default_patch_dim, bool fight_t_junctions = true,
  const HeightmapHeightCulling *handler = NULL, const HMapTesselationData *hmap_tdata = NULL, BBox2 *lodsRegion = nullptr,
  float waterLevel = HeightmapHeightCulling::NO_WATER_ON_LEVEL, const Point3 *viewPos = nullptr);
