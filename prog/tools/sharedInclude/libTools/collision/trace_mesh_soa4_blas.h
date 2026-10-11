// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daBVH/dag_meshBLAS.h>
#include <daBVH/dag_swBLAS_leaf.h>
#include <daBVH/dag_swBLAS_soa4.h>
#include <math/dag_Point3.h>
#include <vecmath/dag_vecMath.h>

// A world-space ray query over one MeshSoa4BLAS, for a tool with a bare BLAS and no resource.

// Closest hit of from + unnormalized_dir * [0, max_t), the far end exclusive.
// A hit writes its t into max_t and the winning triangle's unit normal, in the space of from, into out_norm when that is not null;
// a miss leaves both alone. Nothing culls, so a back face answers like any other.
// The leaf test is the eps-band kernel: it accepts a barycentric band of BVH_TRACE_EPSILON around each edge,
// so a ray near a shared edge can answer from either triangle.
// Answers false without walking: an empty BLAS, a non-finite query, a zero unnormalized_dir, a max_t outside (0, BVH_MISS_T).
// The walkers take the root untested, the leaf lane pick is undefined for an empty hit mask,
// and a max_t above the miss sentinel makes a leaf that hit nothing the nearest hit.
inline bool trace_mesh_soa4_blas(const MeshSoa4BLAS &blas, const Point3 &from, const Point3 &unnormalized_dir, real &max_t,
  Point3 *out_norm = nullptr)
{
  if (blas.empty())
    return false;
  const vec3f o = v_ldu_p3_safe(&from.x), d = v_ldu_p3_safe(&unnormalized_dir.x);
  // constraint: the builds take -fp:fast, so a NaN test has to be the bit one
  if (!v_test_xyz_finite(o) || !v_test_xyz_finite(d) || !v_test_xyz_finite(v_splats(max_t)) || v_extract_x(v_length3_sq_x(d)) <= 0.f ||
      max_t <= 0.f || max_t >= BVH_MISS_T)
    return false;

  RayData r;
  r.data = blas.tree.data();
  r.rayOrigin = v_madd(o, blas.scale, blas.ofs);
  r.rayDir = v_mul(d, blas.scale);
  r.t = max_t;
  r.calc();
  if (!soa4::rayClosestOOL(r, blas.root))
    return false;

  max_t = r.t;
  // constraint: BestRefCb leaves a raw LeafRef in bestTriOffset, and the leaf test picks its sub-triangle
  if (out_norm)
    v_stu_p3(&out_norm->x, soa4::refNormalToWorld(r.data, (soa4::LeafRef)r.bestTriOffset, r.bestSubTri, blas.scale));
  return true;
}
