//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <stdint.h>

namespace build_bvh
{

// Re-encode a single quad BLAS from CPU uint16 [0,65535] space to GPU FP16 [-1,1] space.
// Walks the BVH tree starting at blasStartOffset and re-encodes box nodes and float3 vertices.
void reencodeQuadBlasToFP16(uint8_t *data, int blasStartOffset, int blasSize, int vertCount, int leafSize);

} // namespace build_bvh
