//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
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

struct LodGridNode
{
  enum
  {
    INTERNAL = 0,
    LF = 1,    // left from this is low detailed
    RG = 2,    // right from this is low detailed
    TP = 3,    // top from this is low detailed
    LF_TP = 4, // left and top from this is low detailed
    RG_TP = 5, // right and top from this is low detailed
    BT = 6,
    LF_BT = 7,
    RG_BT = 8, // right and bottom from this is low detailed
    TOTAL
  };
  static inline bool is_left_border(uint8_t nodeType) { return nodeType % 3 == LodGridNode::LF; }

  static inline bool is_right_border(uint8_t nodeType) { return nodeType % 3 == LodGridNode::RG; }

  static inline bool is_top_border(uint8_t nodeType) { return nodeType / 3 == 1; }

  static inline bool is_bottom_border(uint8_t nodeType) { return nodeType / 3 == 2; }

  int16_t posX, posY;
  uint8_t nodeType;
  uint8_t level;
  LodGridNode(int16_t pos_x, int16_t pos_y, uint8_t clevel, uint8_t node_tp) :
    posX(pos_x), posY(pos_y), level(clevel), nodeType(node_tp)
  {}
};

class LodGrid
{
public:
  LodGrid() : lodsCount(8), lodStep(1), lastLodRad(1), lod0SubDiv(0), lastLodExtension(0.0f) {}
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
static inline int generate_patch_indices(int dim, It &&indices, int startInd, uint8_t nodeType, int flip_order = 0)
{
  size_t index = 0;
  for (int y = 0; y < dim; ++y)
    for (int x = 0; x < dim; ++x)
    {

      int topleft = y * (dim + 1) + x + startInd;
      int downleft = topleft + (dim + 1);
      int downright = topleft + (dim + 1) + 1;
      int topright = topleft + 1;
      bool skip_first = false, skip_second = false;
      if (nodeType)
      {
        if (x == 0 && LodGridNode::is_left_border(nodeType))
        {
          if ((y & 1) == 0)
            downleft += dim + 1;
          else
            skip_first = true;
        }
        if (x == dim - 1 && LodGridNode::is_right_border(nodeType))
        {
          if ((y & 1) == 0)
            downright += dim + 1;
          else
            skip_second = true;
        }
        if (y == 0 && LodGridNode::is_top_border(nodeType))
        {
          if ((x & 1) == 0)
            topright += 1;
          else
            skip_first = true;
        }
        if (y == dim - 1 && LodGridNode::is_bottom_border(nodeType))
        {
          if ((x & 1) == 0)
            downright += 1;
          else
            skip_first = true;
        }
      }
      if (!skip_first)
      {
        indices[index + 0] = topleft;
        indices[index + 1] = downleft;
        if (((x + y) & 1) == flip_order)
        {
          indices[index + 2] = downright;
        }
        else
        {
          indices[index + 2] = topright;
        }
        index += 3;
      }
      if (!skip_second)
      {
        if (((x + y) & 1) == flip_order)
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
    }
  return index;
}
