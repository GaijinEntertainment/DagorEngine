//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <stdint.h>
#include <dag/dag_vector.h>

// Generic SDF (Signed Distance Field) from a 1-bit mask.
// Uses EDT (Exact Euclidean Distance Transform) - Felzenszwalb-Huttenlocher algorithm.

namespace sdf
{

// 1D squared distance transform (Felzenszwalb-Huttenlocher).
// f[i] = 0 for surface, large for non-surface. On output, f[i] = squared distance to nearest surface.
// tmp_d: n floats, tmp_v: n ints, tmp_z: (n+1) floats -- caller-provided temporaries.
void dt_1d(float *f, int n, float *tmp_d, int *tmp_v, float *tmp_z);

// w*w float distance field stored as w separate per-row allocations instead of one
// contiguous block, so it needs no single large allocation (a contiguous w*w block
// is impractical on memory-constrained platforms). The EDT works row by row, so each
// row() is a contiguous float buffer; operator[] takes a flat y*w+x index.
struct DistField
{
  dag::Vector<dag::Vector<float>> rows;
  int w = 0;
  void resize(int side)
  {
    rows.resize(side);
    for (auto &r : rows)
      r.resize_noinit(side);
    w = side; // set last: operator[] must not divide by w until every row exists
  }
  float &operator[](int i) { return rows[i / w][i % w]; }
  const float &operator[](int i) const { return rows[i / w][i % w]; }
  float *row(int y) { return rows[y].data(); }
  const float *row(int y) const { return rows[y].data(); }
};

// Build Euclidean distance field from a 1-bit mask.
// mask     - packed bitmask, bit=1 for set pixels, ceil(w*w/32) words
// w        - grid width (square grid)
// out_sdf  - output field, resized to side w by this call (see DistField)
void build_edt(const uint32_t *mask, int w, DistField &out_sdf);

} // namespace sdf
