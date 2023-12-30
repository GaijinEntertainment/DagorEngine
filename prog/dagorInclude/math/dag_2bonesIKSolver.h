//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <vecmath/dag_vecMath.h>
#include <math/dag_geomTree.h>

VECTORCALL VECMATH_FINLINE vec4f local_solve_2bones_ik(float A, float B, vec3f P, vec3f D)
{
  mat44f Minv;
  Minv.col0 = v_norm3(P);
  Minv.col1 = v_norm3(v_sub(D, v_mul(v_dot3(D, Minv.col0), Minv.col0)));
  Minv.col2 = v_cross3(Minv.col0, Minv.col1);
  Minv.col3 = v_zero();

  mat44f Mfwd;
  v_mat44_orthonormal_inverse43(Mfwd, Minv);

  vec3f R = v_mat44_mul_vec3p(Mfwd, P);
  float c = v_extract_x(v_length3_x(R));
  float d = clamp(A, 0.f, (c + (A * A - B * B) / c) / 2.f);
  float e = sqrtf(fabsf(A * A - d * d));

  vec4f S = v_make_vec4f(d, e, 0.f, 0.f);
  return v_mat44_mul_vec3p(Minv, S);
}

static void solve_2bones_ik(mat44f &n0_wtm, mat44f &n1_wtm, mat44f &n2_wtm, const mat44f &target_tm, float length01, float length12,
  vec3f flexion_point, float max_reach_scale = 1.0f)
{
  // Set target tm to hand.
  n2_wtm = target_tm;

  // Solve IK in local coordinates.
  vec3f ikNode2 = v_sub(n2_wtm.col3, n0_wtm.col3);

  float maxReachLen = max_reach_scale * (length01 + length12);
  float targetLenSq = v_extract_x(v_length3_sq_x(ikNode2));
  if (sqr(maxReachLen) < targetLenSq)
  {
    n2_wtm.col3 = v_add(n0_wtm.col3, v_mul(v_splats(maxReachLen / sqrtf(targetLenSq)), v_sub(n2_wtm.col3, n0_wtm.col3)));
    ikNode2 = v_sub(n2_wtm.col3, n0_wtm.col3);
  }

  vec3f ikNode1 = local_solve_2bones_ik(length01, length12, ikNode2, flexion_point);

  // Set forearm matrix.
  mat44f m0, m1;
  m1.col3 = v_add(n0_wtm.col3, ikNode1);
  m1.col0 = v_norm3(v_sub(n2_wtm.col3, m1.col3));
  m1.col1 = v_norm3(v_cross3(m1.col0, n1_wtm.col2));
  m1.col2 = v_norm3(v_cross3(m1.col1, m1.col0));

  n1_wtm = m1;

  // Set upper arm matrix.
  m0.col3 = n0_wtm.col3;
  m0.col0 = v_norm3(v_sub(m1.col3, m0.col3));
  m0.col1 = v_norm3(v_cross3(m0.col0, n0_wtm.col2));
  m0.col2 = v_norm3(v_cross3(m0.col1, m0.col0));

  n0_wtm = m0;
}
