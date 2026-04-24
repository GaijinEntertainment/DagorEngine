//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <stdint.h>

// Generic SDF (Signed Distance Field) from a 1-bit mask.
// Uses EDT (Exact Euclidean Distance Transform) - Felzenszwalb-Huttenlocher algorithm.

namespace sdf
{

// 1D squared distance transform (Felzenszwalb-Huttenlocher).
// f[i] = 0 for surface, large for non-surface. On output, f[i] = squared distance to nearest surface.
// tmp_d: n floats, tmp_v: n ints, tmp_z: (n+1) floats -- caller-provided temporaries.
void dt_1d(float *f, int n, float *tmp_d, int *tmp_v, float *tmp_z);

// Build Euclidean distance field from a 1-bit mask.
// mask     - packed bitmask, bit=1 for set pixels, ceil(w*w/32) words
// w        - grid width (square grid)
// out_sdf  - output buffer, must be pre-allocated to w*w floats
void build_edt(const uint32_t *mask, int w, float *out_sdf);

} // namespace sdf
