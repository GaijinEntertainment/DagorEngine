//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <vecmath/dag_vecMath.h>
#include <math/dag_geomTree.h>

VECTORCALL VECMATH_FINLINE vec4f local_solve_2bones_ik(float A, float B, vec3f P, vec3f Z)
{
  // Consider two circles:
  //  - The 1st of radius A in the center of coordinates
  //  - The 2nd of radius B with the center in coordinates (|P|, 0)
  //
  //            x^2 + y^2 = A^2
  //            (x - |P|)^2 + y^2 = B^2
  //
  // This system will have two solutions, guaranteed to be on the vertical line.
  // Solve it by substituting y^2, get:
  //
  //            |P|^2 + A^2 - B^2
  //        x = -----------------
  //                  2*|P|
  //
  // One solution, in case when |P| = A + B
  // And no solutions, in case when |P| > A + B

  auto C = v_extract_x(v_length3_x(P));
  auto x = (C + (A * A - B * B) / C) * 0.5f;

  // Find distance of the intersection from Ox axis
  auto h = sqrtf(fabsf(A * A - x * x));

  // Compute parallel to P part of the joint vector.
  auto D = v_mul(v_splats(x), v_norm3(P));

  // Use Z of the first node to determine which way the joint
  // should bend. Then compute orthogonal to P part of the joint vector.
  auto H = v_mul(v_splats(h), v_norm3(v_cross3(P, Z)));

  return v_add(D, H);
}

static void solve_2bones_ik(mat44f &n0_wtm, mat44f &n1_wtm, mat44f &n2_wtm, const mat44f &target_tm, float length01, float length12,
  float max_reach_scale = 1.0f)
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

  vec3f ikNode1 = local_solve_2bones_ik(length01, length12, ikNode2, n0_wtm.col2);

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
