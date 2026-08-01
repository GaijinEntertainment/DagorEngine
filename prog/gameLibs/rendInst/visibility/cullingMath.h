// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <vecmath/dag_vecMath.h>
#include <math/dag_frustum.h>


__forceinline int test_box_planes(vec4f bmin, vec4f bmax, const Frustum &curFrustum, vec4f *__restrict planes, int &nplanes)
{
  vec4f center = v_add(bmax, bmin);
  vec4f extent = v_sub(bmax, bmin);

  if (!curFrustum.testBoxExtentB(center, extent))
    return Frustum::OUTSIDE;

  vec4f signmask = v_cast_vec4f(V_CI_SIGN_MASK);
  center = v_mul(center, V_C_HALF);
  extent = v_mul(extent, V_C_HALF);

  // n-vertex distance to every plane; a plane the box straddles (n-vertex outside) joins the
  // reduced set. Planes 0-3 are already transposed on the frustum, so do them as one vertical
  // madd chain (4 at once) and planes 4-5 scalar - a crossing bit-mask instead of a 6x loop.
  vec4f cx = v_splat_x(center), cy = v_splat_y(center), cz = v_splat_z(center);
  vec4f ex = v_splat_x(extent), ey = v_splat_y(extent), ez = v_splat_z(extent);
  vec4f res03 = v_madd(v_sub(cx, v_xor(ex, v_and(curFrustum.plane03X, signmask))), curFrustum.plane03X, curFrustum.plane03W);
  res03 = v_madd(v_sub(cy, v_xor(ey, v_and(curFrustum.plane03Y, signmask))), curFrustum.plane03Y, res03);
  res03 = v_madd(v_sub(cz, v_xor(ez, v_and(curFrustum.plane03Z, signmask))), curFrustum.plane03Z, res03);
  int mask = v_truemask(v_cmp_lt(res03, v_zero())) & 0b1111;

  vec4f res4 = v_add(v_dot3(v_sub(center, v_xor(extent, v_and(curFrustum.camPlanes[4], signmask))), curFrustum.camPlanes[4]),
    v_splat_w(curFrustum.camPlanes[4]));
  vec4f res5 = v_add(v_dot3(v_sub(center, v_xor(extent, v_and(curFrustum.camPlanes[5], signmask))), curFrustum.camPlanes[5]),
    v_splat_w(curFrustum.camPlanes[5]));
  if (v_test_vec_x_lt_0(res4))
    mask |= 1 << 4;
  if (v_test_vec_x_lt_0(res5))
    mask |= 1 << 5;

  // collect the crossing planes (with doubled .w) into the reduced set; the branch skips the
  // perm+store for non-crossers, which measured faster than branchless always-store compaction
  nplanes = 0;
  for (int i = 0; i < 6; i++)
    if (mask & (1 << i))
      planes[nplanes++] = v_perm_xyzd(curFrustum.camPlanes[i], v_add(curFrustum.camPlanes[i], curFrustum.camPlanes[i]));
  return mask ? Frustum::INTERSECT : Frustum::INSIDE;
}

// Test 4 contiguous boxes against the reduced crossing-plane set at once; returns a 4-bit mask
// (bit i set = box i inside all planes). The p-vertex distance to a plane is the farthest box
// corner along the plane normal; a box is inside when no plane's p-vertex distance is negative.
// The 4 boxes go SoA (transpose) and each plane is one vertical madd chain over all four.
__forceinline int test_box_planesb_4(const bbox3f *__restrict box4, const vec4f *__restrict planes, int nplanes)
{
  vec4f signmask = v_cast_vec4f(V_CI_SIGN_MASK);
  vec4f cx = v_add(box4[0].bmax, box4[0].bmin), cy = v_add(box4[1].bmax, box4[1].bmin);
  vec4f cz = v_add(box4[2].bmax, box4[2].bmin), cw = v_add(box4[3].bmax, box4[3].bmin);
  vec4f ex = v_sub(box4[0].bmax, box4[0].bmin), ey = v_sub(box4[1].bmax, box4[1].bmin);
  vec4f ez = v_sub(box4[2].bmax, box4[2].bmin), ew = v_sub(box4[3].bmax, box4[3].bmin);
  v_mat44_transpose(cx, cy, cz, cw); // cx = the 4 boxes' center.x*2, cy their .y*2, cz their .z*2
  v_mat44_transpose(ex, ey, ez, ew);
  vec4f res = v_zero();
  for (int i = 0; i < nplanes; i++)
  {
    vec4f px = v_splat_x(planes[i]), py = v_splat_y(planes[i]), pz = v_splat_z(planes[i]), pw = v_splat_w(planes[i]);
    vec4f r = v_madd(v_add(cx, v_xor(ex, v_and(px, signmask))), px, pw);
    r = v_madd(v_add(cy, v_xor(ey, v_and(py, signmask))), py, r);
    r = v_madd(v_add(cz, v_xor(ez, v_and(pz, signmask))), pz, r);
    res = v_or(res, r);
  }
  return (~v_signmask(res)) & 0xF; // inside where no plane's p-vertex distance is negative
}
