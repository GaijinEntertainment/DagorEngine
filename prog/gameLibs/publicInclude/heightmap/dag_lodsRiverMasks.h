//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <dag/dag_vector.h>
#include <stdint.h>

// LOD patch bitmask mipchain for river/water critical path.
// All mip levels packed into a single flat bit array.
//
// LOD numbering: LOD 0 = 1x1 (coarsest, whole map), LOD L = (1<<L) x (1<<L) patches.
// Higher LOD number = finer detail. LOD 0 is always stored.

struct LodsRiverMasks
{
  int8_t finestLod = -1; // finestLod < 0 means empty (no masks, always returns false)
  dag::Vector<uint32_t> masks;

  static uint32_t get_level_ofs(uint32_t level) { return uint32_t((1u << (level << 1u)) - 1u) / 3u; }

  void clear()
  {
    finestLod = -1;
    masks.clear();
  }

  bool isRiverCriticalPath(int px, int py, uint8_t lod) const
  {
    if (finestLod < 0 || (int)lod > (int)finestLod)
      return false;
    uint32_t mask = (1u << lod) - 1;
    uint32_t bit = get_level_ofs(lod) + ((py & mask) << lod) + (px & mask);
    return (masks[bit >> 5] >> (bit & 31)) & 1;
  }
};
