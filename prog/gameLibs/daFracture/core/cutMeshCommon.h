// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daFracture/core/destrMesh.h>
#include <vecmath/dag_vecMath.h>
#include <util/dag_compilerDefs.h>


#define LAMBDA_PROF_BEGIN ([&] () DAGOR_NOINLINE_LAMBDA {
#define LAMBDA_PROF_END \
  })();


#define FRX_VERIFY_ENABLED 0
#define FRX_VERIFY_LOG     FRX_VERIFY_ENABLED

#if FRX_VERIFY_ENABLED
#define FRX_CHECK_FAILURE(EXPR, ...)                \
  DAGOR_UNLIKELY(([&]() DAGOR_FORCE_INLINE_LAMBDA { \
    bool chk = (EXPR);                              \
    G_ASSERTF_EX(!chk, #EXPR, "" __VA_ARGS__);      \
    return chk;                                     \
  })())
#else
#define FRX_CHECK_FAILURE(EXPR, ...) (DAGOR_UNLIKELY(EXPR))
#endif

#if FRX_VERIFY_LOG
#define FRX_LOG_FAILURE(...) debug(__VA_ARGS__)
#else
#define FRX_LOG_FAILURE(...) ((void)0)
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

static __forceinline void lerp_vertex_data_barycentric(DestrMesh::Vertex &v, const DestrMesh::Vertex &v0, const DestrMesh::Vertex &v1,
  const DestrMesh::Vertex &v2, vec4f w0, vec4f w1, vec4f w2)
{
  v_st(&v.pos.x, v_madd(v_ld(&v0.pos.x), w0, v_madd(v_ld(&v1.pos.x), w1, v_mul(v_ld(&v2.pos.x), w2))));
  vec4f tcY_normXYZ = v_madd(v_ld(&v0.tc.y), w0, v_madd(v_ld(&v1.tc.y), w1, v_mul(v_ld(&v2.tc.y), w2)));
  vec4f norm = v_norm3(v_perm_yzwx(tcY_normXYZ));
  tcY_normXYZ = v_perm_ayzw(v_perm_wxyz(norm), tcY_normXYZ); // (tc.y, norm.xyz): keep lerped tc.y, take renormalized norm
  v_st(&v.tc.y, tcY_normXYZ);
}

static __forceinline DestrMesh::Vertex lerp_vertex_data(const DestrMesh::Vertex &v0, const DestrMesh::Vertex &v1, float t)
{
  DestrMesh::Vertex v;
  lerp_vertex_data_v(v, v0, v1, v_splats(t));
  return v;
}

static __forceinline plane3f transform_plane_to_local(plane3f world_plane, const TMatrix &tm)
{
  mat44f m44, m44t;
  v_mat44_make_from_43cu(m44, tm.array);
  v_mat44_transpose(m44t, m44);
  return v_mat44_mul_vec4(m44t, world_plane);
}

} // namespace frx