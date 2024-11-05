//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <math/dag_Point3.h>
#include <stdint.h>

struct SparseSDFMip
{
  Point3 toIndirMul = {0, 0, 0};
  Point3 toIndirAdd = {0, 0, 0};
  uint32_t streamOffset = 0; // from compressed data
  uint32_t compressedSize = 0;
  static constexpr uint32_t dim_bits = 10, dim_mask = (1 << dim_bits) - 1;
  uint32_t indirectionDim = 0;     // 10|10|10 encoding
  int32_t sdfBlocksCount = 0;      // total number of valid blocks
  uint16_t maxEncodedDistFP16 = 0; // negative half if mostlyTwoSided
  uint16_t padding = 0;
  void getIndirectionDim(int &x, int &y, int &z) const
  {
    x = indirectionDim & dim_mask, y = (indirectionDim >> dim_bits) & dim_mask, z = (indirectionDim >> (dim_bits * 2));
  }
  uint32_t getVolume() const
  {
    return (indirectionDim & dim_mask) * ((indirectionDim >> dim_bits) & dim_mask) * (indirectionDim >> (dim_bits * 2));
  }
  static uint32_t encode_dim(uint32_t x, uint32_t y, uint32_t z) { return x | (y << dim_bits) | (z << (2 * dim_bits)); }
};
