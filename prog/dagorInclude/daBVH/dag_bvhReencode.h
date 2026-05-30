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

// Re-encode just one node's box (16 B node header: 3x uint16 [0,65535] min/max pairs -> FP16 [-1,1]),
// preserving the skip/encoding word at +12. For callers that already walk the tree node-by-node and
// want to fold the box re-encode into that pass instead of re-traversing via reencodeQuadBlasToFP16.
void reencodeBoxNodeToFP16(uint8_t *nodeData);

} // namespace build_bvh
