//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <dag/dag_vector.h>
#include <heightmap/dag_lodsRiverMasks.h>
#include <stdint.h>

typedef uint16_t hmap_err_t;
class HeightmapHandler;


struct HeightBounds
{
  uint16_t hMin, hMax;
  uint16_t getMin() const { return hMin; }
  uint16_t getMax() const { return hMax; }
};

enum
{
  TRI_DIAMOND_FIRST_RTLB,
  TRI_DIAMOND_FIRST_LTRB,
  TRI_REGULAR_RTLB,
  TRI_REGULAR_LTRB,
  TRI_COUNT
};

struct MetricsErrors
{
  dag::Vector<hmap_err_t> data;
  dag::Vector<uint8_t> order;
  dag::Vector<uint8_t> exactOrder;
  dag::Vector<HeightBounds> heightBounds;
  LodsRiverMasks lodsRiverMasks;
  uint8_t minLevel = 0;
  uint8_t maxBoundsLevel = 0;
  uint8_t maxErrLevel = 0;
  uint8_t maxOrderLevel = 0;
  uint8_t maxResShift = 0;
  uint16_t dim = 0;
  uint32_t minLevelOfs = 0;
  uint32_t szX = 0, szY = 0;
  float boundsHeightScale = 1, boundsHeightOffset = 0;
  float errorHeightScale = 1;

  hmap_err_t getErrorUnsafe(uint8_t level, uint32_t x, uint32_t y, hmap_err_t def = 0) const;
  hmap_err_t getErrorSafe(uint8_t level, uint32_t x, uint32_t y, hmap_err_t def = 0) const;
  bool getBoundsSafe(uint8_t level, uint32_t x, uint32_t y, HeightBounds &r) const;

  void recalc_lod_errors(const HeightmapHandler &h, uint16_t min_level, uint16_t max_err_level, uint16_t max_order_level, uint16_t dim,
    float water_level, float shore_error_at_best_lod);
  void updateHeightBounds(const HeightmapHandler &h, const IBBox2 &texel_box, bool reset_old);
  void calc_lod_errors(const HeightmapHandler &h, uint16_t min_level, uint16_t max_level, uint16_t dim, float water_level,
    float shore_error_at_best_lod = 2.0f);
  void clear();
};

// geometric sum of up to level*level
static inline uint32_t get_level_ofs(uint32_t level) { return uint32_t((1u << (level << 1u)) - 1u) / 3u; }

inline hmap_err_t MetricsErrors::getErrorUnsafe(uint8_t level, uint32_t x, uint32_t y, hmap_err_t def) const
{
  const uint32_t dataOfs = get_level_ofs(level) - minLevelOfs + (y << level) + x;
  return dataOfs < data.size() ? data[dataOfs] : def;
}
inline hmap_err_t MetricsErrors::getErrorSafe(uint8_t level, uint32_t x, uint32_t y, hmap_err_t def) const
{
  const uint8_t revLevel = maxErrLevel - level;
  if (x >= (szX >> revLevel) || y >= (szY >> revLevel))
    return def;
  return getErrorUnsafe(level, x, y, def);
}

inline bool MetricsErrors::getBoundsSafe(uint8_t level, uint32_t x, uint32_t y, HeightBounds &r) const
{
  if (level >= maxBoundsLevel)
    return false;

  r = heightBounds[get_level_ofs(level) + (y << level) + x];
  return true;
}
