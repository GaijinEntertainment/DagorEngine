// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daBVH/dag_bvhReencode.h>
#include <daBVH/swBLASLeafDefs.hlsli>
#include <daBVH/swBVHDefine.hlsli>
#include <vecmath/dag_vecMath.h>
#include <string.h>

#include <daBVH/swCommon.h>

namespace build_bvh
{

// Re-encode a single box node from uint16 [0,65535] pairs to FP16 [-1,1] pairs.
// Conservative: min rounds down, max rounds up.
void reencodeBoxNodeToFP16(uint8_t *nodeData)
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

// Map source float3 verts ([0,65535]) to [-1,1] and write them back in the requested GPU format.
// Source stride is always 12; the destination stride is blasGpuVertStride(fp16) (8 when fp16). When
// the destination is smaller the loop compacts in place forward (dst[v] starts before src[v] and each
// src vert is read into a register before its dst write, so reads always run ahead of writes).
static void reencodeVertsToNorm(uint8_t *vertData, int numVerts, bool fp16)
{
  const vec4f toNorm = v_splats(1.0f / 32767.5f);
  const vec4f bias = v_splats(-1.0f);
  const int dstStride = blasGpuVertStride(fp16);
  for (int v = 0; v < numVerts; v++)
  {
    vec4f n = v_madd(v_ldu_p3_safe((const float *)(vertData + v * 12)), toNorm, bias);
    writeGpuBlasVert(vertData + v * dstStride, n, fp16);
  }
}

void reencodeQuadBlasToFP16(uint8_t *data, int blasStartOffset, int blasSize, int vertCount, int leafSize, bool fp16_verts)
{
  const int vertRegionStart = blasStartOffset + blasSize;
  const int dstStride = blasGpuVertStride(fp16_verts);
  int ofs = blasStartOffset;

  // Walk tree nodes linearly
  for (; ofs < vertRegionStart;)
  {
    uint32_t skip;
    memcpy(&skip, data + ofs + 12, sizeof(uint32_t));

    reencodeBoxNodeToFP16(data + ofs);

    const bool isLeaf = (skip & QUAD_LEAF_FLAG) != 0;
    if (fp16_verts && isLeaf)
    {
      // The vertex pool shrinks 12 -> 8 B/vert, so the leaf's quad A base (W1[0:23], byte offset from
      // leaf+16, >> 2) must be re-pointed to the new stride. Vertex-index deltas (o*, deltaB) are
      // stride-independent and ride through untouched. Mirrors collapseQuadBLAS::emit.
      const int w1Ofs = ofs + BVH_BLAS_NODE_SIZE;
      uint32_t w1;
      memcpy(&w1, data + w1Ofs, sizeof(uint32_t));
      const int relBase = (int)((w1 & QUAD_BASE_MASK) << QUAD_BASE_ALIGN_SHIFT);
      const int vertIdx = (w1Ofs + relBase - vertRegionStart) / 12;
      const uint32_t baseNew = (uint32_t)((vertRegionStart + vertIdx * dstStride - w1Ofs) >> QUAD_BASE_ALIGN_SHIFT) & QUAD_BASE_MASK;
      w1 = baseNew | (w1 & ~QUAD_BASE_MASK);
      memcpy(data + w1Ofs, &w1, sizeof(uint32_t));
    }

    ofs += isLeaf ? leafSize : BVH_BLAS_NODE_SIZE;
  }

  reencodeVertsToNorm(data + vertRegionStart, vertCount, fp16_verts);
}

} // namespace build_bvh
