// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <vecmath/dag_vecMath.h>

VECMATH_FINLINE vec3f closestPointOnTriangleBarycentric(vec3f p, vec3f A, vec3f B, vec3f C)
{
  vec3f BA = v_sub(B, A), PA = v_sub(p, A);
  vec3f CB = v_sub(C, B), PB = v_sub(p, B);
  vec3f AC = v_sub(A, C), PC = v_sub(p, C);

  vec3f N = v_cross3(BA, AC);
  vec3f q = v_cross3(N, PA);
  vec3f d = v_rcp_x(v_length3_sq_x(N));
  vec3f u = v_mul_x(d, v_dot3_x(q, AC));
  vec3f v = v_mul_x(d, v_dot3_x(q, BA));
  vec3f w = v_sub_x(V_C_ONE, v_add_x(u, v));

  vec3f zero = v_zero();
  if (v_test_vec_x_lt(u, zero))
  {
    w = v_clamp(v_div_x(v_dot3_x(PC, AC), v_length3_sq_x(AC)), zero, V_C_ONE);
    u = zero;
    v = v_sub_x(V_C_ONE, w);
  }
  else if (v_test_vec_x_lt(v, zero))
  {
    u = v_clamp(v_div_x(v_dot3_x(PA, BA), v_length3_sq_x(BA)), zero, V_C_ONE);
    v = zero;
    w = v_sub_x(V_C_ONE, u);
  }
  else if (v_test_vec_x_lt(w, zero))
  {
    v = v_clamp(v_div_x(v_dot3_x(PB, CB), v_length3_sq_x(CB)), zero, V_C_ONE);
    w = zero;
    u = v_sub_x(V_C_ONE, v);
  }
  return v_madd(v_splat_x(u), B, v_madd(v_splat_x(v), C, v_mul(v_splat_x(w), A)));
}

VECMATH_FINLINE vec3f distToTriangleSq(vec3f p, vec3f A, vec3f B, vec3f C)
{
  vec3f tp = closestPointOnTriangleBarycentric(p, A, B, C);
  tp = v_sub(tp, p);

  return v_length3_sq_x(tp);
}
