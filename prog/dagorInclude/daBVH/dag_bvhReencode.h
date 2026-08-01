//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <stdint.h>
#include <string.h>
#include <vecmath/dag_vecMath.h>

namespace build_bvh
{

// GPU serialized vertex stride for the chosen format: 8 B (x float, y/z fp16) or 12 B float3.
// The format is a runtime choice per BLAS (it must match the consuming shader's BLAS_VERT_FP16),
// NOT a compile-time define -- one build serves shaders of both formats.
inline int blasGpuVertStride(bool fp16) { return fp16 ? 8 : 12; }

// Write one GPU BLAS vertex (already mapped to FP16 box space [-1,1]) in the requested format:
// fp16 ? 8 B = x float + y,z fp16 round-to-nearest : 12 B float3. Must match loadQuadVert in
// swBLAS.dshl, whose BLAS_VERT_FP16 define selects the same stride for the consuming shader.
inline void writeGpuBlasVert(uint8_t *dst, vec4f norm_xyz, bool fp16)
{
  if (fp16)
  {
    vec4i h = v_float_to_half_rtne(norm_xyz); // lane i low 16 bits = half(component i)
    alignas(16) int hi[4];
    v_sti(hi, h);
    float x = v_extract_x(norm_xyz);
    uint32_t yz = (uint32_t(hi[1]) & 0xffffu) | (uint32_t(hi[2]) << 16);
    memcpy(dst, &x, 4);      // -V1086
    memcpy(dst + 4, &yz, 4); // -V1086
  }
  else
  {
    alignas(16) float f[4];
    v_st(f, norm_xyz);
    memcpy(dst, f, 12); // -V1086
  }
}

// Re-encode a single quad BLAS from CPU uint16 [0,65535] space to GPU FP16 [-1,1] space. Walks the
// tree at blasStartOffset re-encoding box nodes; the trailing vertex pool is mapped to [-1,1] and
// written in the requested GPU format. When fp16_verts the pool is compacted in place to 8 B/vert and
// each leaf's quad A base is rewritten for the 8 B stride, so the pool then occupies
// vertCount * blasGpuVertStride(fp16_verts) bytes -- callers must trim the buffer.
void reencodeQuadBlasToFP16(uint8_t *data, int blasStartOffset, int blasSize, int vertCount, int leafSize, bool fp16_verts);

// Re-encode just one node's box (16 B node header: 3x uint16 [0,65535] min/max pairs -> FP16 [-1,1]),
// preserving the skip/encoding word at +12. For callers that already walk the tree node-by-node and
// want to fold the box re-encode into that pass instead of re-traversing via reencodeQuadBlasToFP16.
void reencodeBoxNodeToFP16(uint8_t *nodeData);

} // namespace build_bvh
