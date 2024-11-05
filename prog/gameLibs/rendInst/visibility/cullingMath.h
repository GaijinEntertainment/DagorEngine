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
  int result = Frustum::INSIDE;
  nplanes = 0;
  for (int i = 0; i < 6; i += 2)
  {
    vec4f res1, res2;
    res1 = v_add(v_dot3(v_sub(center, v_xor(extent, v_and(curFrustum.camPlanes[i], signmask))), curFrustum.camPlanes[i]),
      v_splat_w(curFrustum.camPlanes[i]));
    res2 = v_add(v_dot3(v_sub(center, v_xor(extent, v_and(curFrustum.camPlanes[i + 1], signmask))), curFrustum.camPlanes[i + 1]),
      v_splat_w(curFrustum.camPlanes[i + 1]));
    if (v_test_vec_x_lt_0(res1))
    {
      planes[nplanes] = v_perm_xyzd(curFrustum.camPlanes[i], v_add(curFrustum.camPlanes[i], curFrustum.camPlanes[i]));
      nplanes++;
      result = Frustum::INTERSECT;
    }

    if (v_test_vec_x_lt_0(res2))
    {
      planes[nplanes] = v_perm_xyzd(curFrustum.camPlanes[i + 1], v_add(curFrustum.camPlanes[i + 1], curFrustum.camPlanes[i + 1]));
      nplanes++;
      result = Frustum::INTERSECT;
    }
  }
  return result;
}

__forceinline int test_box_planesb(vec4f bmin, vec4f bmax, const vec4f *__restrict planes, int nplanes)
{
  vec4f signmask = v_cast_vec4f(V_CI_SIGN_MASK);
  vec4f center = v_add(bmax, bmin);
  vec4f extent = v_sub(bmax, bmin);
  vec4f res = v_zero();
  if (nplanes & 1)
  {
    res = v_add_x(v_dot3_x(v_add(center, v_xor(extent, v_and(planes[0], signmask))), planes[0]), v_splat_w(planes[0]));
    planes++;
    nplanes--;
  }

  for (int i = 0; i < nplanes; i += 2, planes += 2)
  {
    res = v_or(res, v_add_x(v_dot3_x(v_add(center, v_xor(extent, v_and(planes[0], signmask))), planes[0]), v_splat_w(planes[0])));
    res = v_or(res, v_add_x(v_dot3_x(v_add(center, v_xor(extent, v_and(planes[1], signmask))), planes[1]), v_splat_w(planes[1])));
  }
#if _TARGET_SIMD_SSE
  return (~_mm_movemask_ps(res)) & 1;
#else
  return !v_test_vec_x_lt_0(res);
#endif
}
