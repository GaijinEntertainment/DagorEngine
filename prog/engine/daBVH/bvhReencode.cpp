// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daBVH/dag_bvhReencode.h>
#include <daBVH/swBLASLeafDefs.hlsli>
#include "shaders/swBVHDefine.hlsli"
#include <vecmath/dag_vecMath.h>
#include <string.h>

#include "swCommon.h"

namespace build_bvh
{

// Re-encode a single box node from uint16 [0,65535] pairs to FP16 [-1,1] pairs.
// Conservative: min rounds down, max rounds up.
static void reencodeBoxNodeToFP16(uint8_t *nodeData)
{
  alignas(16) uint32_t mm[4];
  memcpy(mm, nodeData, 16);

  vec4i raw = v_ldi((const int *)mm);
  vec4i loMask = v_splatsi(0xFFFF);
  vec4i mins_i = v_andi(raw, loMask);
  vec4i maxs_i = v_srli(raw, 16);

  const vec4f toNorm = v_splats(1.0f / 32767.5f);
  const vec4f bias = v_splats(-1.0f);
  vec4f fMins = v_madd(v_cvt_vec4f(mins_i), toNorm, bias);
  vec4f fMaxs = v_madd(v_cvt_vec4f(maxs_i), toNorm, bias);

  vec4i hMins = v_float_to_half_down(fMins);
  vec4i hMaxs = v_float_to_half_up(fMaxs);

  write_pair_halves(mm, hMins, hMaxs);
  memcpy(nodeData, mm, 12); // -V1086 preserve skip word
}

// Re-encode float3 vertices (12 bytes each) in-place from [0,65535] to [-1,1].
static void reencodeVertsToNorm(uint8_t *vertData, int numVerts)
{
  const float toNorm = 1.0f / 32767.5f;
  for (int v = 0; v < numVerts; v++)
  {
    float f[3];
    memcpy(f, vertData + v * 12, 12);
    f[0] = f[0] * toNorm - 1.0f;
    f[1] = f[1] * toNorm - 1.0f;
    f[2] = f[2] * toNorm - 1.0f;
    memcpy(vertData + v * 12, f, 12);
  }
}

void reencodeQuadBlasToFP16(uint8_t *data, int blasStartOffset, int blasSize, int vertCount, int leafSize)
{
  int ofs = blasStartOffset;
  const int endOfs = blasStartOffset + blasSize;

  // Walk tree nodes linearly
  for (; ofs < endOfs;)
  {
    uint32_t skip;
    memcpy(&skip, data + ofs + 12, sizeof(uint32_t));

    reencodeBoxNodeToFP16(data + ofs);

    ofs += (skip & QUAD_LEAF_FLAG) ? leafSize : BVH_BLAS_NODE_SIZE;
  }

  reencodeVertsToNorm(data + ofs, vertCount);
}

} // namespace build_bvh
