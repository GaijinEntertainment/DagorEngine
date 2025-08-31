//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <generic/dag_tab.h>
#include <generic/dag_carray.h>

static constexpr int default_patch_bits = 5;
static constexpr int default_patch_dim = 1 << default_patch_bits;

class BBox2;
struct Frustum;
struct LodGridVertex
{
  int16_t x, y;
};

class LodGrid
{
public:
  LodGrid() : lodsCount(8), lodStep(1), lastLodRad(1), lod0SubDiv(0), lastLodExtension(0.0f) {}
  LodGrid &operator=(const LodGrid &) = default;
  ~LodGrid() {}
  void init(int numLod, int lod0, int lod0_sub_div, int last_lod_rad, float last_lod_extension = 0.0f)
  {
    G_ASSERT(numLod < LodGrid::MAX_LODS);
    G_ASSERT(lod0 >= 1);
    G_ASSERT(last_lod_rad >= 1);
    G_ASSERT(lod0_sub_div >= 0);
    lodsCount = numLod;
    lodStep = lod0;
    lastLodRad = last_lod_rad;
    lod0SubDiv = lod0_sub_div;
    lastLodExtension = last_lod_extension;
  }

  static constexpr int MAX_LODS = 16;

  uint16_t lodsCount, lod0SubDiv, lodStep, lastLodRad;
  float lastLodExtension;
};

template <class It>
static inline int generate_patch_indices(int dim, It &&indices, int flip_order = 0, int diamond_mask = 1)
{
  size_t index = 0;
  for (int y = 0; y < dim; ++y)
    for (int x = 0; x < dim; ++x)
    {

      int topleft = y * (dim + 1) + x;
      int downleft = topleft + (dim + 1);
      int downright = topleft + (dim + 1) + 1;
      int topright = topleft + 1;
      indices[index + 0] = topleft;
      indices[index + 1] = downleft;
      indices[index + 2] = ((x + y) & diamond_mask) == flip_order ? downright : topright;
      index += 3;
      if (((x + y) & diamond_mask) == flip_order)
      {
        indices[index + 0] = topleft;
        indices[index + 1] = downright;
        indices[index + 2] = topright;
      }
      else
      {
        indices[index + 0] = topright;
        indices[index + 1] = downleft;
        indices[index + 2] = downright;
      }
      index += 3;
    }
  return index;
}
