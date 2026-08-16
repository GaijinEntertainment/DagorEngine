// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <gameRes/dag_collisionResource.h>
#include <math/dag_math3d.h>
#include <math/dag_mathUtils.h>
#include <vecmath/dag_vecMath.h>
#include <debug/dag_debug.h>
#include <float.h>

// Helpers shared by collisionGameRes.cpp and collisionResourceInstance.cpp - the instance file
// holds the CollisionResourceInstance method definitions, the resource file keeps the shared
// geometry, loading and trace dispatch.

// Largest singular value, used where column lengths under-bound shear. Rounded a few ULPs
// upward so every consumer may treat it as an upper bound. Defined in collisionGameRes.cpp:
// its degenerate-scale diagnostic must latch once per process, not once per translation unit.
float mat33_spectral_norm(mat44f_cref m_in);

// Shared ownership/freshness gate for updates and dispatch. Defined in collisionGameRes.cpp
// for the same single-latch reason.
bool check_instance_owned_and_fresh(const CollisionResource *res, const CollisionResourceInstance &instance, const char *site);

// Bit-exact signed-permutation bases (identity, axis-aligned rotations and reflections) are the
// only rigid bases floats represent exactly; every other member of an epsilon class owes the
// conservative pad. Within a near-orthogonal class three unit-axis columns are necessarily
// distinct axes, so no orthogonality check is needed here.
static inline bool is_exact_unit_axis_col(const Point3 &c)
{
  const float ax = fabsf(c.x), ay = fabsf(c.y), az = fabsf(c.z);
  return (ax == 1.f && ay == 0.f && az == 0.f) || (ax == 0.f && ay == 1.f && az == 0.f) || (ax == 0.f && ay == 0.f && az == 1.f);
}

static inline bool is_exact_rigid_basis(const TMatrix &tm)
{
  return is_exact_unit_axis_col(tm.getcol(0)) && is_exact_unit_axis_col(tm.getcol(1)) && is_exact_unit_axis_col(tm.getcol(2));
}

// Row norm without squaring raw elements: a finite 2e20 component would overflow into an
// infinite extent. Outside the normalizable band the max element times sqrt(3) is a
// conservative bound (exact 0 for a zero row; NaN propagates).
static inline float conservative_row_norm(float x, float y, float z)
{
  const float m = max(fabsf(x), max(fabsf(y), fabsf(z)));
  if (DAGOR_UNLIKELY(!(m >= FLT_MIN && m <= 1.f / FLT_MIN)))
    return m == 0.f ? 0.f : m * 1.7320509f;
  const float ix = x / m, iy = y / m, iz = z / m;
  return m * sqrtf(ix * ix + iy * iy + iz * iz);
}


static inline bool is_exact_rigid_basis_v(mat44f_cref m)
{
  TMatrix t;
  v_mat_43cu_from_mat44(t.array, m);
  return is_exact_rigid_basis(t);
}

// Analytic AABB avoids the rotation inflation of corner-mapping a sphere.
// TMatrix is column-major (m[i] IS column i), so conservative_row_norm(tm[0][k], tm[1][k],
// tm[2][k]) is ROW k's norm -- r * |row_k| is the tight Cauchy-Schwarz extent along axis k.
static inline BBox3 composed_sphere_box(const TMatrix &tm, const Point3 &c, float r)
{
  const Point3 center = tm * c;
  const Point3 ext(r * conservative_row_norm(tm[0][0], tm[1][0], tm[2][0]), r * conservative_row_norm(tm[0][1], tm[1][1], tm[2][1]),
    r * conservative_row_norm(tm[0][2], tm[1][2], tm[2][2]));
  return BBox3(center - ext, center + ext);
}


// NaN (a poisoned compose) never joins; a merely overflowed (Inf) lane saturates to the float
// range so a node whose reachable part is finite still bounds instead of leaving the root
// boxes short while the node stays traceable.
static inline void join_saturated_finite(bbox3f &dst, bbox3f b)
{
  if (!v_check_xyz_all_true(v_and(v_cmp_eq(b.bmin, b.bmin), v_cmp_eq(b.bmax, b.bmax))))
    return;
  const vec4f lim = v_splats(FLT_MAX);
  b.bmin = v_min(v_max(b.bmin, v_neg(lim)), lim);
  b.bmax = v_min(v_max(b.bmax, v_neg(lim)), lim);
  v_bbox3_add_box(dst, b);
}
