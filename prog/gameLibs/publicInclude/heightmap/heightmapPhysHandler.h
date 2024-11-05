//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <vecmath/dag_vecMath.h>
#include <math/dag_Point2.h>
#include <math/dag_Point3.h>
#include <math/dag_bounds3.h>
#include <math/dag_bounds2.h>
#include <math/integer/dag_IBBox2.h>
#include <generic/dag_smallTab.h>
#include <heightMapLand/dag_compressedHeightmap.h>


enum class HmapVersion : int
{
  HMAP_BASE_VER = 0,
  HMAP_DELTA_COMPRESSION_VER = 1,
  HMAP_CBLOCK_DELTAC_VER = 2,
};

static constexpr int HMAP_WIDTH_BITS = 24;
static constexpr int HMAP_WIDTH_MASK = (1 << HMAP_WIDTH_BITS) - 1; // we encode version into heightmap width, since it is unlikely to
                                                                   // be more, than 16mln pixels


class IGenLoad;
class GlobalSharedMemStorage;

struct HeightmapPhysHandlerInfo // -V730
{
  bbox3f vecbox;
  vec4f invElemSize, v_worldOfsxzxz;
  DECL_ALIGN16(BBox2, worldBox2);
  BBox3 worldBox;
  Point2 worldPosOfs = {0, 0};
  Point2 worldSize = {1, 1};
  float hmapCellSize = 1, hScale = 1, hMin = 0;
  float hScaleRaw = 1;
  IPoint2 hmapWidth = {1, 1};
  IBBox2 excludeBounding = {{0, 0}, {0, 0}};
};

class HeightmapPhysHandler : public HeightmapPhysHandlerInfo
{
public:
  static constexpr int HMAP_BSIZE = 32;
  static constexpr int BASE_HMAP_LOD_COUNT = 8;
  enum class UpdateHtResult : int
  {
    UNCHANGED = 0,
    SAMPLE_CHANGED = 1,
    BLOCK_CHANGED = 2,
  };

  ~HeightmapPhysHandler() { close(); }
  HeightmapPhysHandler() // -V730
  {}

  void close();
  bool loadDump(IGenLoad &loadCb, GlobalSharedMemStorage *sharedMem, int skip_mips);
  bool checkOrAllocateOwnData();

  int traceDownMultiRay(bbox3f_cref rayBox, vec4f *ray_pos_mint, vec4f *v_out_norm, int cnt) const;
  bool getHeightBelow(const Point3 &p, float &ht, Point3 *normal) const;
  bool getHeight(const Point2 &p, float &ht, Point3 *normal) const;
  bool getHeightMax(const Point2 &p0, const Point2 &p1, float hmin, float &hmax) const;
  bool traceray(const Point3 &p, const Point3 &dir, real &t, Point3 *normal, bool cull = true) const;
  bool rayhitNormalized(const Point3 &p, const Point3 &normDir, real t) const;
  bool rayUnderHeightmapNormalized(const Point3 &p, const Point3 &normDir, real t) const;

  IBBox2 getExcludeBounding() const { return excludeBounding; }
  BBox3 getWorldBox() const { return worldBox; }

  inline Point2 getWorldSize() const { return worldSize; }
  inline Point3 getHeightmapOffset() const { return Point3::xVy(worldPosOfs, 0); }
  inline float getHeightmapCellSize() const { return hmapCellSize; }
  inline float getHeightScaleRaw() const { return hScaleRaw; } // hScale/65535
  inline float getHeightScale() const { return hScale; }       // hScale, if height is in [0..1]
  inline float getHeightMin() const { return hMin; }           // hScale, if height is in [0..1]
  inline int getHeightmapSizeX() const { return hmapWidth.x; }
  inline int getHeightmapSizeY() const { return hmapWidth.y; }
  __forceinline vec4f decodeInsideBlock4HeightsRaw(CompressedHeightmap::BlockInfo block, uint8_t block_shift, uint32_t blockId,
    uint32_t bi, uint32_t bj) const
  {
    if (block.delta == 0)
      return v_splats(block.mn);
    auto vi = compressed.getBlockVariance(blockId) + bi + (bj << block_shift);
    int xi = 1, yi = 1 << block_shift;
    vec4i hts = v_make_vec4i(vi[0], vi[xi], vi[yi], vi[yi + xi]);
    vec4f htF = v_cvt_vec4f(v_muli(hts, v_splatsi(block.delta)));
    htF = v_madd(htF, v_splats(1.f / 255), V_C_HALF);
    return v_cvt_vec4f(v_addi(v_cvt_vec4i(htF), v_splatsi(block.mn)));
  }
  inline vec4f getHeightmapCell5PtUnsafe(const IPoint2 &cell) const
  {
    G_ASSERT(uint32_t(cell.x) < hmapWidth.x && uint32_t(cell.y) < hmapWidth.y);
    const uint8_t block_mask = compressed.block_width_mask, block_shift = compressed.block_width_shift;
    const uint32_t bi = cell.x & block_mask, bj = cell.y & block_mask;
    vec4f htF;

    const uint32_t bx = cell.x >> block_shift, by = cell.y >> block_shift;
    const uint32_t blockId = bx + by * compressed.bw;
    if (bi < block_mask && bj < block_mask)
    {
      // fast path
      htF = decodeInsideBlock4HeightsRaw(compressed.getBlockInfo(blockId), block_shift, blockId, bi, bj);
    }
    else
    {
      const int xinside = min(cell.x + 1, hmapWidth.x - 1);
      const int yinside = min(cell.y + 1, hmapWidth.y - 1);
      htF = v_make_vec4f(compressed.decodePixelUnsafe(cell.x, cell.y), compressed.decodePixelUnsafe(xinside, cell.y),
        compressed.decodePixelUnsafe(cell.x, yinside), compressed.decodePixelUnsafe(xinside, yinside));
    }
    return v_madd(htF, v_splats(hScaleRaw), v_splats(hMin));
  }

  inline bool getHeightmapCell5PtUnsafeMinMax(vec4f &htRet, const IPoint2 &cell, float mnY, float mxY) const
  {
    G_ASSERT(uint32_t(cell.x) < hmapWidth.x && uint32_t(cell.y) < hmapWidth.y);
    const uint8_t block_mask = compressed.block_width_mask, block_shift = compressed.block_width_shift;
    const uint32_t bi = cell.x & block_mask, bj = cell.y & block_mask;
    vec4f htF;

    const uint32_t bx = cell.x >> block_shift, by = cell.y >> block_shift;
    const uint32_t blockId = bx + by * compressed.bw;
    if (bi < block_mask && bj < block_mask)
    {
      // fast path
      const auto block = compressed.getBlockInfo(blockId);
      if (block.getMin() > mxY || block.getMax() < mnY)
        return false;
      htF = decodeInsideBlock4HeightsRaw(compressed.getBlockInfo(blockId), block_shift, blockId, bi, bj);
    }
    else if (DAGOR_LIKELY(by < compressed.bh - 1 && bx < compressed.bw - 1))
    {
      const uint32_t br = (bi == block_mask) ? 1 : 0, bd = (bj == block_mask) ? compressed.bw : 0;
      const auto block0 = compressed.getBlockInfo(blockId);
      const auto block1 = compressed.getBlockInfo(blockId + br);
      const auto block2 = compressed.getBlockInfo(blockId + bd);
      const auto block3 = compressed.getBlockInfo(blockId + br + bd);
      if (min(min(block0.getMin(), block1.getMin()), min(block2.getMin(), block3.getMin())) > mxY ||
          max(max(block0.getMax(), block1.getMax()), max(block2.getMax(), block3.getMax())) < mnY)
        return false;
      auto vi0 = compressed.getBlockVariance(blockId), vi1 = compressed.getBlockVariance(blockId + br),
           vi2 = compressed.getBlockVariance(blockId + bd), vi3 = compressed.getBlockVariance(blockId + br + bd);
      uint32_t bi1 = ((bi + 1) & block_mask), bj1 = ((bj + 1) & block_mask);

      const uint32_t block0Index = bi + (bj << block_shift), block1Index = bi1 + (bj << block_shift),
                     block2Index = bi + (bj1 << block_shift), block3Index = bi1 + (bj1 << block_shift);

      const uint16_t h0 = block0.mn + uint32_t(uint32_t(vi0[block0Index]) * block0.delta + 127) / 255,
                     h1 = block1.mn + uint32_t(uint32_t(vi1[block1Index]) * block1.delta + 127) / 255,
                     h2 = block2.mn + uint32_t(uint32_t(vi2[block2Index]) * block2.delta + 127) / 255,
                     h3 = block3.mn + uint32_t(uint32_t(vi3[block3Index]) * block3.delta + 127) / 255;

      htF = v_make_vec4f(h0, h1, h2, h3);
    }
    else
    {
      const int xinside = min(cell.x + 1, hmapWidth.x - 1);
      const int yinside = min(cell.y + 1, hmapWidth.y - 1);
      htF = v_make_vec4f(compressed.decodePixelUnsafe(cell.x, cell.y), compressed.decodePixelUnsafe(xinside, cell.y),
        compressed.decodePixelUnsafe(cell.x, yinside), compressed.decodePixelUnsafe(xinside, yinside));
    }
    vec4f mn = v_min(htF, v_rot_2(htF));
    mn = v_min(mn, v_splat_y(mn));
    vec4f mx = v_max(htF, v_rot_2(htF));
    mx = v_max(mx, v_splat_y(mx));
    if (mxY < v_extract_x(mn) || mnY > v_extract_x(mx))
      return false;
    htF = v_madd(htF, v_splats(hScaleRaw), v_splats(hMin));
    htRet = htF;
    return true;
  }

  inline bool getHeightmapCell5Pt(const IPoint2 &cell, real &h0, real &hx, real &hy, real &hxy, real &hmid) const
  {
    if (uint32_t(cell.x) >= hmapWidth.x || uint32_t(cell.y) >= hmapWidth.y)
      return false;
    vec4f htF = getHeightmapCell5PtUnsafe(cell);

    alignas(16) float h[4];
    v_st(h, htF);
    h0 = h[0];
    hx = h[1];
    hy = h[2];
    hxy = h[3];
    hmid = (h0 + hx + hy + hxy) * 0.25f;
    return true;
  }
  inline bool getHeightmapCell5PtMinMax(const IPoint2 &cell, real &h0, real &hx, real &hy, real &hxy, real &hmid, real mny,
    real mxy) const
  {
    if (uint32_t(cell.x) >= hmapWidth.x || uint32_t(cell.y) >= hmapWidth.y)
      return false;
    vec4f htF;
    if (!getHeightmapCell5PtUnsafeMinMax(htF, cell, (mny - hMin) / hScaleRaw, (mxy - hMin) / hScaleRaw))
      return false;

    alignas(16) float h[4];
    v_st(h, htF);
    h0 = h[0];
    hx = h[1];
    hy = h[2];
    hxy = h[3];
    hmid = (h0 + hx + hy + hxy) * 0.25f;
    return true;
  }
  inline bool getHeightmapCellsMaxHeight(const IPoint2 &cell0, const IPoint2 &cell1, float hmin, real &hmax) const
  {
    const IPoint2 cell0Lim(clamp(cell0.x, 0, hmapWidth.x - 1), clamp(cell0.y, 0, hmapWidth.y - 1));
    const IPoint2 cell1Lim(clamp(cell1.x, 0, hmapWidth.x - 1), clamp(cell1.y, 0, hmapWidth.y - 1));

    uint16_t maxHtRaw = 0;
    for (int j = cell0Lim.y >> compressed.block_width, je = cell1Lim.y >> compressed.block_width; j <= je; ++j)
      for (int i = cell0Lim.x >> compressed.block_width, ie = cell1Lim.x >> compressed.block_width; i <= ie; ++i)
        maxHtRaw = max(compressed.getBlockInfo(i, j).getMin(), maxHtRaw);

    if (maxHtRaw * hScaleRaw + hMin <= hmin)
      return false;
#if 0
    // find accurate data
    // it is not really needed, as we use this only for "safe" flying, so max is good enough
    maxHtRaw = 0;
    for (int j = cell0Lim.y; j <= cell1Lim.y; ++j)
      for (int i = cell0Lim.x; i <= cell1Lim.x; ++i)
        maxHtRaw = max(compressed.decodePixelUnsafe(i, j), maxHtRaw);
#endif
    float hMax = maxHtRaw * hScaleRaw + hMin;

    if (hMax > hmin)
    {
      hmax = hMax;
      return true;
    }
    else
      return false;
  }
  bool getHeightmapHeightMinMaxInChunk(const Point2 &pos, const real &chunkSize, real &hmin, real &hmax) const;


  bool getHeightmapHeight(const IPoint2 &cell, real &ht) const
  {
    if (uint32_t(cell.x) < hmapWidth.x && uint32_t(cell.y) < hmapWidth.y)
    {
      ht = compressed.decodePixelUnsafe(cell.x, cell.y) * hScaleRaw + hMin;
      return true;
    }
    return false;
  }
  uint16_t getHeightmapHeightUnsafe(const IPoint2 &cell) const
  {
    G_ASSERT(uint32_t(cell.x) < hmapWidth.x && uint32_t(cell.y) < hmapWidth.y);
    return compressed.decodePixelUnsafe(cell.x, cell.y);
  }
  UpdateHtResult setHeightmapHeightUnsafe(const IPoint2 &cell, uint16_t ht)
  {
    G_ASSERTF(hmap_data, "need own data for deformations, use checkOrAllocateOwnData");

    auto ret = UpdateHtResult::UNCHANGED;
    G_ASSERT(uint32_t(cell.x) < hmapWidth.x && uint32_t(cell.y) < hmapWidth.y);
    const uint8_t block_mask = compressed.block_width_mask, block_shift = compressed.block_width_shift;
    const uint32_t bi = cell.x & block_mask, bj = cell.y & block_mask;
    const uint32_t blockId = (cell.x >> block_shift) + (cell.y >> block_shift) * compressed.bw;

    auto block = compressed.getBlockInfo(blockId);
    if (block.getMin() > ht || block.getMax() < ht)
    {
      updateBlockMinMax(blockId, ht);
      ret = UpdateHtResult::BLOCK_CHANGED;
      block = compressed.getBlockInfo(blockId);
      G_ASSERT(ht >= block.getMin() && ht <= block.getMax());
    }
    if (block.delta == 0)
      return ret;
    const uint32_t encoded = uint32_t(uint32_t(ht - block.mn) * 255 + (block.delta >> 1)) / block.delta;
    const uint32_t blockIndex = (blockId << compressed.block_size_shift) + bi + (bj << block_shift);
    if (compressed.blockVariance[blockIndex] == encoded)
      return ret;
    auto &dest = compressed.blockVariance[blockIndex];
    if (ret == UpdateHtResult::UNCHANGED)
      ret = UpdateHtResult::SAMPLE_CHANGED;
    dest = encoded;
    compressed.updateHierHeightRangeBlocksForPoint(cell.x, cell.y, ht);
    return ret;
  }
  uint16_t packHeightmapHeight(float height) const { return (height - hMin) / hScaleRaw; }
  float unpackHeightmapHeight(uint16_t ht) const { return ht * hScaleRaw + hMin; }
  bool getHeightsGrid(bbox3f_cref box, float *heights, int cnt, Point2 &gridOfs, float &elemSize, int8_t &w, int8_t &h) const;
  bool getHeightsGrid(const Point4 &origin, float *heights, int w, int h, Point2 &gridOfs, float &elemSize) const;
  void fillHeightsInRegion(int regions[4], int cnt, float *heights, float &elemSize, Point2 &gridOfs) const;
  IPoint2 posToHmapXZ(const Point2 &pos) const { return IPoint2::xy((pos - worldPosOfs) / getHeightmapCellSize()); }
  // incremented when terrain changes

  int getMinMaxHtGridSize() const { return compressed.getBestHtRangeBlocksResolution(); }
  bool getMinMaxHtInGrid(const IPoint2 &grid_cell, float &min_ht, float &max_ht) const
  {
    if (compressed.htRangeBlocksLevels)
    {
      unsigned grid_res = compressed.getBestHtRangeBlocksResolution();
      if (unsigned(grid_cell.x) < grid_res && unsigned(grid_cell.y) < grid_res)
      {
        unsigned hrb_stride = grid_res / 2;
        const auto &hrb =
          compressed.getHtRangeBlocksLevData(compressed.htRangeBlocksLevels - 1)[(grid_cell.y / 2) * hrb_stride + (grid_cell.x / 2)];
        unsigned idx = (grid_cell.y & 1) * 2 + (grid_cell.x & 1);
        min_ht = min(min_ht, hrb.hMin[idx] * hScaleRaw + hMin);
        max_ht = max(max_ht, hrb.hMax[idx] * hScaleRaw + hMin);
        return true;
      }
    }
    return false;
  }

  const CompressedHeightmap &getCompressedData() const { return compressed; }

protected:
  void updateBlockMinMax(uint32_t blockId, uint16_t ht, float updateBlockMinMax = 1.f);
  Point3 getClippedOrigin(const Point3 &origin_pos) const;
  CompressedHeightmap compressed;
  void *hmap_data = nullptr;
};
