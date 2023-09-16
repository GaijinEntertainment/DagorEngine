//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <vecmath/dag_vecMath.h>
#include <math/dag_geomTree.h>

static Point3 local_solve_2bones_ik(float A, float B, const Point3 &P, const Point3 &D)
{
  TMatrix Minv;
  Minv.setcol(0, normalize(P));
  Minv.setcol(1, normalize(D - (D * Minv.getcol(0)) * Minv.getcol(0)));
  Minv.setcol(2, Minv.getcol(0) % Minv.getcol(1));
  Minv.setcol(3, 0.f, 0.f, 0.f);

  TMatrix Mfwd = inverse(Minv);

  Point3 R = Mfwd * P;

  float c = length(R);
  float d = max(0.f, min(A, (c + (A * A - B * B) / c) / 2.f));
  float e = sqrt(fabsf(A * A - d * d));

  Point3 S = Point3(d, e, 0.f);

  return Minv * S;
}

static void solve_2bones_ik(mat44f &n0_wtm, mat44f &n1_wtm, mat44f &n2_wtm, const mat44f &target_tm, float length01, float length12,
  const Point3 &flexion_point, float max_reach_scale = 1.0f)
{
  // Set target tm to hand.

  n2_wtm = target_tm;


  // Solve IK in local coordinates.

  Point3 ikNode2;
  v_stu_p3(&ikNode2.x, v_sub(n2_wtm.col3, n0_wtm.col3));

  float maxReachLen = max_reach_scale * (length01 + length12);
  float targetLenSq = lengthSq(ikNode2);
  if (SQR(maxReachLen) < targetLenSq)
  {
    n2_wtm.col3 = v_add(n0_wtm.col3, v_mul(v_splats(maxReachLen / sqrt(targetLenSq)), v_sub(n2_wtm.col3, n0_wtm.col3)));
    v_stu_p3(&ikNode2.x, v_sub(n2_wtm.col3, n0_wtm.col3));
  }

  Point3_vec4 ikNode1 = local_solve_2bones_ik(length01, length12, ikNode2, flexion_point);
  ikNode1.resv = 0;


  // Set forearm matrix.
  mat44f m0, m1;

  m1.col3 = v_add(n0_wtm.col3, v_ld(&ikNode1.x));
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
