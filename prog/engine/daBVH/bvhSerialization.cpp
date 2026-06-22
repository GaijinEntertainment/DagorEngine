// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daBVH/dag_bvhSerialization.h>
#include <vecmath/dag_vecMath.h>
#include <math/dag_bits.h>
#include "swCommon.h"

namespace build_bvh
{

template <typename IdxT>
bool checkIfIsBox(const IdxT *indices, int index_count, const vec4f *vertices, int vertex_count, const bbox3f &box)
{
  if (index_count != 12 * 3 || vertex_count != 8)
    return false;
  vec4f threshold = v_max(v_mul(v_splats(1e-6f), v_bbox3_size(box)), v_splats(1e-6f));
  for (int i = 0; i < vertex_count; ++i)
  {
    vec4f minMaskV = v_cmp_lt(v_abs(v_sub(vertices[i], box.bmin)), threshold);
    vec4f maxMaskV = v_cmp_lt(v_abs(v_sub(vertices[i], box.bmax)), threshold);
    vec4f mask = v_or(minMaskV, maxMaskV);
    if (!v_check_xyz_all_true(mask))
      return false;
  }
  for (int i = 0; i < index_count; i += 3)
  {
    vec4f v0 = vertices[indices[i]];
    const vec4f n = v_cross3(v_sub(vertices[indices[i + 1]], v0), v_sub(vertices[indices[i + 2]], v0));
    const vec4f threshold = v_mul(v_length3(n), v_splats(1e-6f));
    const vec4f nA = v_abs(n);
    if (__popcount(7 & v_signmask(v_cmp_gt(nA, threshold))) != 1)
      return false;
  }
  return true;
}

template bool checkIfIsBox<uint16_t>(const uint16_t *, int, const vec4f *, int, const bbox3f &);
template bool checkIfIsBox<uint32_t>(const uint32_t *, int, const vec4f *, int, const bbox3f &);

void packVert21(uint8_t *dst, vec4f quantized_xyz)
{
  // Simple 21-bit-per-component: x[20:0] | y[41:21] | z[62:42]
  // Store round(value * 32), max value = 65535 * 32 = 2097120 < 2^21.
  // v_cvt_vec4i truncates toward zero, so bias by +0.5 (V_C_HALF) first to round-to-nearest,
  // then clamp to [0, 0x1FFFFF] in SIMD. Only the final scalar bit packing stays scalar.
  vec4i v = v_cvt_vec4i(v_madd(quantized_xyz, v_splats(32.f), V_C_HALF));
  v = v_mini(v_maxi(v, v_zeroi()), v_splatsi(0x1FFFFF));
  alignas(16) int s[4];
  v_sti(s, v);
  uint64_t packed = uint64_t(unsigned(s[0])) | (uint64_t(unsigned(s[1])) << 21) | (uint64_t(unsigned(s[2])) << 42);
  memcpy(dst, &packed, 8);
}

}; // namespace build_bvh
