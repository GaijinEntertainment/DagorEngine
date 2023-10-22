//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <debug/dag_assert.h>
#include <stdint.h>
class IGenLoad;

// implements ATI1N/BC5-style block compression
// minmax (in form of minimum plus delta) are stored independently from 'indices' (variance interpolation)
// all data is stored as min,delta + block_width^2 byte deltas
// the source data itself is supposed to be 16-bit
// with a very minor difference, we can make source data floats (or N bits)!
// with storing delta as half of fixed point I.F) (and limiting delta to I bits, we can store MORE detailed information in most
// interesing case of small variety per block) with fixed point there is no downside in performance with half float difference is very,
// very small (decoding half is simple and simd friendly)

// not owning structure. i.e. destination is to be provided to 'compress' function
struct CompressedHeightmap
{
  template <typename T>
  static inline constexpr T min(const T val1, const T val2)
  {
    return (val1 < val2 ? val1 : val2);
  }
  template <typename T>
  static inline constexpr T max(const T val1, const T val2)
  {
    return (val1 > val2 ? val1 : val2);
  }

  static size_t calc_data_size_needed(uint32_t w, uint32_t h, uint8_t &block_shift, uint32_t hrb_subsz);
  size_t dataSizeCurrent() const
  {
    return ((bh * bw * (sizeof(BlockInfo) + (1 << block_size_shift)) + 0xF) & ~0xF) +
           sHierGridOffsets[htRangeBlocksLevels] * sizeof(HeightRangeBlock);
  }

  static CompressedHeightmap compress(uint8_t *dest, uint32_t dest_size, const uint16_t *data, uint32_t w, uint32_t h,
    uint8_t block_shift, uint32_t hrb_subsz);

  static CompressedHeightmap init(uint8_t *data, uint32_t data_size, uint32_t w, uint32_t h, uint8_t block_shift, uint32_t hrb_subsz);
  static bool loadData(CompressedHeightmap &hmap, IGenLoad &crd, unsigned chunk_sz);
  static CompressedHeightmap downsample(uint8_t *data, uint32_t data_size, uint32_t w, uint32_t h, const CompressedHeightmap &orig);

  struct BlockInfo
  {
    uint16_t mn, delta;
    const uint16_t getMin() const { return mn; }
    const uint16_t getMax() const { return mn + delta; }
    const uint16_t decodeVariance(uint8_t v) const { return mn + uint32_t(uint32_t(v) * delta + 127) / 255; }
    const uint8_t encodeVarianceUnsafe(uint16_t h) const // not checking if delta == 0 or  mn <= h <= mn + delta
    {
      return uint32_t(uint32_t(h - mn) * 255 + (delta >> 1)) / delta;
    }
    const uint8_t encodeVariance(uint16_t h) const
    {
      if (!delta)
        return 0;
      int hMn = h - mn;
      hMn = hMn < 0 ? 0 : (hMn > delta ? delta : hMn);
      return uint32_t(hMn * 255 + (delta >> 1)) / delta;
    }
  };
  /// A block of 2x2 ranges used to form a hierarchical grid, ordered left top, right top, left bottom, right bottom
  struct alignas(16) HeightRangeBlock
  {
    uint16_t hMin[4], hMax[4];
  };
  static const unsigned sHierGridOffsets[15]; // grid offset for each level in flat layout

  operator bool() const { return fullData != nullptr; }
  uint8_t *fullData = nullptr;
  uint8_t *blockVariance = nullptr; // fullData + blocks*sizeof(BlockInfo);
  uint8_t block_width_shift = 0, block_width = 0, block_width_mask = 0, block_size_shift = 0;
  uint16_t bw = 0, bh = 0;

  HeightRangeBlock *htRangeBlocks = nullptr; // align(16) (blockVariance + getW() * getH());
  uint8_t htRangeBlocksLevels = 0, _resv1[3] = {0, 0, 0};
#if _TARGET_64BIT
  uint32_t _resv2 = 0;
#endif

  void decompress(uint16_t *data, size_t data_size) const;
  bool updateHierHeightRangeBlocksForPoint(unsigned x, unsigned y, uint16_t new_ht);
  bool recomputeHierHeightRangeBlocksForRect(unsigned x, unsigned y, unsigned w, unsigned h);
  void recomputeHierHeightRangeBlocks();

  unsigned getW() const { return bw << block_width_shift; }
  unsigned getH() const { return bh << block_width_shift; }

  const HeightRangeBlock *getHtRangeBlocksLevData(unsigned lev) const
  {
    return lev < htRangeBlocksLevels ? htRangeBlocks + sHierGridOffsets[lev] : nullptr;
  }
  unsigned getHtRangeBlocksLevStride(unsigned lev) const { return lev < htRangeBlocksLevels ? 1 << lev : 0; }
  unsigned getBestHtRangeBlocksResolution() const { return htRangeBlocksLevels ? 1 << htRangeBlocksLevels : 1; }

  const BlockInfo *blocksInfo() const { return (const BlockInfo *)fullData; }
  const BlockInfo &getBlockInfo(uint32_t b) const { return blocksInfo()[b]; }
  const uint8_t *getBlockVariance(uint32_t b) const { return blockVariance + (b << block_size_shift); }
  const BlockInfo &getBlockInfo(uint32_t bx, uint32_t by) const { return getBlockInfo(bx + by * bw); }
  const uint8_t *getBlockVariance(uint32_t bx, uint32_t by) const { return getBlockVariance(bx + by * bw); }
  const uint16_t decodeBlockPixelUnsafe(uint32_t blockId, uint32_t in_block_coord) const
  {
    const BlockInfo block = getBlockInfo(blockId);
    auto vi = getBlockVariance(blockId);
    if (block.delta == 0)
      return block.mn;
    return block.decodeVariance(vi[in_block_coord]);
  }
  const uint16_t decodePixelUnsafe(uint32_t x, uint32_t y) const
  {
    const uint32_t bx = x >> block_width_shift, by = y >> block_width_shift, bi = x & block_width_mask, bj = y & block_width_mask;
    return decodeBlockPixelUnsafe(bx + by * bw, bi + (bj << block_width_shift));
  }
  template <typename CB>
  void iterateBlocks(uint32_t bx, uint32_t by, uint32_t bx_w, uint32_t by_w, const CB &cb) const
  {
    G_ASSERT_RETURN(bx + bx_w <= bw && by + by_w <= bh, );
    const uint32_t bi = by * bw + bx;
    const BlockInfo *__restrict bInfo = blocksInfo() + bi;
    uint32_t widthLeft = bw - bx_w;
    for (int bj = by; bj < by + by_w; ++bj, bInfo += widthLeft)
      for (int bi = bx; bi < bx + bx_w; ++bi, ++bInfo)
        cb(bi, bj, bInfo->getMin(), bInfo->getMax());
  }

  template <typename CB>
  void iterateBlocksPixels(uint32_t bx, uint32_t by, uint32_t bx_w, uint32_t by_w, const CB &cb) const
  {
    G_ASSERT_RETURN(bx + bx_w <= bw && by + by_w <= bh, );
    const uint32_t blockId = by * bw + bx;
    const BlockInfo *__restrict bInfo = &getBlockInfo(blockId);
    const uint8_t *__restrict vi = getBlockVariance(blockId);
    uint32_t widthLeft = bw - bx_w;
    for (int bj = by, bje = by + by_w; bj < bje; ++bj, bInfo += widthLeft, vi += (widthLeft << block_size_shift))
      for (int bi = bx, bie = bx + bx_w; bi < bie; ++bi, ++bInfo)
      {
        const uint16_t mn = bInfo->mn, delta = bInfo->delta;

        uint32_t x = bi << block_width_shift, y = (bj << block_width_shift);
        if (delta == 0)
        {
          for (int j = 0, je = block_width; j < je; ++j)
            for (int i = 0; i < je; ++i, ++vi)
              cb(x + i, y + j, mn);
        }
        else
          for (int j = 0, je = block_width; j < je; ++j)
            for (int i = 0; i < je; ++i, ++vi)
            {
              const uint16_t decodedVal = mn + uint32_t(uint32_t(*vi) * delta + 127) / 255;
              cb(x + i, y + j, decodedVal);
            }
      }
  }
  template <typename CB>
  void iteratePixels(uint32_t x, uint32_t y, uint32_t w, uint32_t h, const CB &cb) const
  {
    G_ASSERT_RETURN(((x + w - 1) >> block_width_shift) < bw && ((y + h - 1) >> block_width_shift) < bh, );
    const uint32_t bAlignedSX = (x + block_width_mask) >> block_width_shift, bAlignedSY = (y + block_width_mask) >> block_width_shift;
    const uint32_t bAlignedEX = (x + w) >> block_width_shift, bAlignedEY = (y + h) >> block_width_shift;

    if (bAlignedEX > bAlignedSX && bAlignedEY > bAlignedSY)
      iterateBlocksPixels(bAlignedSX, bAlignedSY, bAlignedEX - bAlignedSX, bAlignedEY - bAlignedSY, cb);
    // not optimal - we can iterate by blocks also. first row with with unaligned pixels, than first column, than
    for (int yi = y, ye = min(y + h, bAlignedSY << block_width_shift); yi < ye; ++yi)
    {
      for (int xi = x, xe = x + w; xi < xe; ++xi)
        cb(xi, yi, decodePixelUnsafe(xi, yi));
    }
    if (bAlignedSY <= bAlignedEY)
    {
      for (int yi = bAlignedEY << block_width_shift, ye = y + h; yi < ye; ++yi)
      {
        for (int xi = x, xe = x + w; xi < xe; ++xi)
          cb(xi, yi, decodePixelUnsafe(xi, yi));
      }
    }
    for (int yi = (bAlignedSY << block_width_shift), ye = (bAlignedEY << block_width_shift); yi < ye; ++yi)
    {
      for (int xi = x, xe = min(x + w, bAlignedSX << block_width_shift); xi < xe; ++xi)
        cb(xi, yi, decodePixelUnsafe(xi, yi));
      if (bAlignedSX <= bAlignedEX)
      {
        for (int xi = (bAlignedEX << block_width_shift), xe = x + w; xi < xe; ++xi)
          cb(xi, yi, decodePixelUnsafe(xi, yi));
      }
    }
  }

private:
  HeightRangeBlock *getHtRangeBlocksLevData(unsigned lev)
  {
    return lev < htRangeBlocksLevels ? htRangeBlocks + sHierGridOffsets[lev] : nullptr;
  }
};
