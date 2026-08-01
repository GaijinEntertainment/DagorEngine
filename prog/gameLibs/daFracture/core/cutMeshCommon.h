// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daFracture/core/destrMesh.h>
#include <vecmath/dag_vecMath.h>


#if defined(_MSC_VER) && !defined(__clang__)
#define FORCE_INLINE_LAMBDA
#define NOINLINE_LAMBDA
#else
#define FORCE_INLINE_LAMBDA __attribute__((always_inline))
#define NOINLINE_LAMBDA     __attribute__((noinline))
#endif

#define LAMBDA_PROF_BEGIN ([&] () NOINLINE_LAMBDA {
#define LAMBDA_PROF_END \
  })();


#if 0
#define VERIFY_ALGORITHM(EXPR) \
  ([&] {                       \
    const bool e = (EXPR);     \
    G_ASSERT(e);               \
    return e;                  \
  })()
#else
#define VERIFY_ALGORITHM(EXPR) (EXPR)
#endif


namespace frx
{

static __forceinline void lerp_vertex_data_v(DestrMesh::Vertex &v, const DestrMesh::Vertex &v0, const DestrMesh::Vertex &v1,
  vec4f tttt)
{
  v_st(&v.pos.x, v_lerp_vec4f(tttt, v_ld(&v0.pos.x), v_ld(&v1.pos.x)));
  vec4f tcY_normXYZ = v_lerp_vec4f(tttt, v_ld(&v0.tc.y), v_ld(&v1.tc.y));
  vec4f norm = v_norm3(v_perm_yzwx(tcY_normXYZ));
  tcY_normXYZ = v_perm_ayzw(v_perm_wxyz(norm), tcY_normXYZ); // (tc.y, norm.xyz): keep lerped tc.y, take renormalized norm
  v_st(&v.tc.y, tcY_normXYZ);
}

static __forceinline plane3f transform_plane_to_local(plane3f world_plane, const TMatrix &tm)
{
  mat44f m44, m44t;
  v_mat44_make_from_43cu(m44, tm.array);
  v_mat44_transpose(m44t, m44);
  return v_mat44_mul_vec4(m44t, world_plane);
}

} // namespace frx