// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <util/dag_globDef.h>
#include <vecmath/dag_vecMath.h>
#include <math/dag_TMatrix.h>
#include <math/dag_TMatrix4.h>
#include <drv/3d/dag_matricesAndPerspective.h>

namespace framestateflags
{
enum
{
  M2VTM_OK = 0x0001,
  GLOBTM_OK = 0x0002,
  PERSP_OK = 0x0004,
  PROJTM_OK = 0x0008,
  V2MTM_OK = 0x0010,
  IDENT_WTM_SET = 0x0020,

  VIEWPORT_SET = 0x1000,
  VIEWPORT_VALID = 0x2000,
};
};

struct alignas(16) FrameStateTM
{
  mat44f d3d_mat[TM__NUM];
  mat44f globtm;
  Driver3dPerspective persp;
  uint32_t flags;

public:
  FrameStateTM() { init(); }

  void init()
  {
    v_mat44_ident(globtm);
    for (int i = 0; i < TM__NUM; ++i)
      d3d_mat[i] = globtm;
    flags = framestateflags::IDENT_WTM_SET;
    persp.wk = persp.hk = persp.zn = persp.zf = persp.ox = persp.oy = 0.f;
  }

  void calc_globtm()
  {
    if (flags & framestateflags::GLOBTM_OK)
      return;
    v_mat44_mul(d3d_mat[TM_LOCAL2VIEW], d3d_mat[TM_VIEW], d3d_mat[TM_WORLD]); //
    calcglobtm(d3d_mat[TM_LOCAL2VIEW], d3d_mat[TM_PROJ], globtm);

    flags |= framestateflags::GLOBTM_OK | framestateflags::M2VTM_OK;
  }

  void calc_m2vtm() { calc_globtm(); }
  void calc_v2mtm()
  {
    if (flags & framestateflags::V2MTM_OK)
      return;
    mat44f itmView, itmWorld;
    v_mat44_inverse43(itmView, d3d_mat[TM_VIEW]);
    v_mat44_inverse43(itmWorld, d3d_mat[TM_WORLD]);
    v_mat44_mul(d3d_mat[TM_VIEW2LOCAL], itmWorld, itmView);
    flags |= framestateflags::V2MTM_OK;
  }

  // d3d:: functions
  __forceinline void calcproj(const Driver3dPerspective &p, mat44f &proj_tm)
  {
    v_mat44_make_persp(proj_tm, p.wk, p.hk, p.zn, p.zf);
    if (p.ox != 0.f || p.oy != 0.f)
      proj_tm.col2 = v_add(proj_tm.col2, v_make_vec4f(p.ox, p.oy, 0.f, 0.f));
  }

  __forceinline void calcproj(const Driver3dPerspective &p, TMatrix4 &proj_tm)
  {
    mat44f projTm;
    calcproj(p, projTm);

    v_stu(proj_tm.m[0], projTm.col0);
    v_stu(proj_tm.m[1], projTm.col1);
    v_stu(proj_tm.m[2], projTm.col2);
    v_stu(proj_tm.m[3], projTm.col3);
  }

  __forceinline void calcglobtm(const mat44f &view_tm, const mat44f &proj_tm, mat44f &result)
  {
    v_mat44_mul(result, proj_tm, view_tm);
  }

  __forceinline void calcglobtm(const mat44f &view_tm, const Driver3dPerspective &p, mat44f &result)
  {
    mat44f proj;
    calcproj(p, proj);
    calcglobtm(view_tm, proj, result);
  }

  __forceinline void calcglobtm(const TMatrix &view_tm, const TMatrix4 &proj_tm, TMatrix4 &result)
  {
    mat44f v, p, r;
    v_mat44_make_from_43cu(v, view_tm.m[0]);
    p.col0 = v_ldu(proj_tm.m[0]);
    p.col1 = v_ldu(proj_tm.m[1]);
    p.col2 = v_ldu(proj_tm.m[2]);
    p.col3 = v_ldu(proj_tm.m[3]);
    calcglobtm(v, p, r);
    v_stu(result.m[0], r.col0);
    v_stu(result.m[1], r.col1);
    v_stu(result.m[2], r.col2);
    v_stu(result.m[3], r.col3);
  }

  __forceinline void calcglobtm(const TMatrix &view_tm, const Driver3dPerspective &p, TMatrix4 &result)
  {
    TMatrix4 proj;
    calcproj(p, proj);
    calcglobtm(view_tm, proj, result);
  }

  __forceinline void setpersp(const Driver3dPerspective &p, TMatrix4 *proj_tm)
  {
    persp = p;
    calcproj(p, d3d_mat[TM_PROJ]);

    flags &= ~(framestateflags::GLOBTM_OK | framestateflags::PROJTM_OK);
    flags |= framestateflags::PERSP_OK;

    if (proj_tm)
    {
      v_stu(proj_tm->m[0], d3d_mat[TM_PROJ].col0);
      v_stu(proj_tm->m[1], d3d_mat[TM_PROJ].col1);
      v_stu(proj_tm->m[2], d3d_mat[TM_PROJ].col2);
      v_stu(proj_tm->m[3], d3d_mat[TM_PROJ].col3);
    }
  }

  __forceinline bool validatepersp(mat44f &proj)
  {
    vec4f zero = v_zero();
    vec4f result = v_cmp_eq(v_and(proj.col0, (vec4f)V_CI_MASK0111), zero);
    result = v_and(result, v_cmp_eq(v_and(proj.col1, (vec4f)V_CI_MASK1011), zero));
    result = v_and(result, v_cmp_eq(v_and(proj.col2, (vec4f)V_CI_MASK1100), zero));
    result = v_and(result, v_cmp_eq(v_and(proj.col3, (vec4f)V_CI_MASK1101), zero));
    result = v_andnot(v_and(v_cmp_eq(proj.col0, zero), (vec4f)V_CI_MASK1000), result);
    result = v_andnot(v_and(v_cmp_eq(proj.col1, zero), (vec4f)V_CI_MASK0100), result);
    result = v_andnot(v_and(v_cmp_eq(proj.col2, zero), (vec4f)V_CI_MASK0010), result);
    result = v_andnot(v_and(v_cmp_eq(proj.col3, zero), (vec4f)V_CI_MASK0010), result);
    result = v_and(result, v_cmp_eq(v_and(proj.col2, (vec4f)V_CI_MASK0001), V_C_UNIT_0001));
#if !_TARGET_SIMD_SSE
    result = v_and(result, v_perm_yzwx(result));
    result = v_and(result, v_perm_zwxy(result));

    return !v_test_vec_x_eqi_0(result);
#else
    return _mm_movemask_ps(result) == 0xF;
#endif
  }

  __forceinline bool validatepersp(const Driver3dPerspective &p)
  {
    mat44f projTm;
    calcproj(p, projTm);
    return validatepersp(projTm);
  }

  __forceinline bool getpersp(Driver3dPerspective &p)
  {
    if (!(flags & framestateflags::PERSP_OK))
    {
      if (validatepersp(d3d_mat[TM_PROJ]))
      {
        float c3z = v_extract_z(d3d_mat[TM_PROJ].col3);
        float c2z = v_extract_z(d3d_mat[TM_PROJ].col2);
        persp.zf = -c3z / c2z; // v_mat44_make_persp defaults to reverse projection.
        persp.zn = c3z / (1.f - c2z);
        persp.wk = v_extract_x(d3d_mat[TM_PROJ].col0);
        persp.hk = v_extract_y(d3d_mat[TM_PROJ].col1);
        persp.ox = 0.f;
        persp.oy = 0.f;
        flags |= framestateflags::PERSP_OK;
      }
      else
        return false;
    }

    p = persp;

    return true;
  }

  __forceinline void setglobtm(Matrix44 &tm)
  {
    globtm.col0 = v_ldu(tm.m[0]);
    globtm.col1 = v_ldu(tm.m[1]);
    globtm.col2 = v_ldu(tm.m[2]);
    globtm.col3 = v_ldu(tm.m[3]);
    flags =
      (flags & ~(framestateflags::M2VTM_OK | framestateflags::PROJTM_OK | framestateflags::PERSP_OK)) | framestateflags::GLOBTM_OK;
  }

  __forceinline void settm(int which, const Matrix44 *m)
  {
    switch (which)
    {
      case TM_WORLD:
        if (m != &TMatrix4::IDENT)
          flags &= ~framestateflags::IDENT_WTM_SET;
        else if (flags & framestateflags::IDENT_WTM_SET)
          return;
        else
          flags |= framestateflags::IDENT_WTM_SET;
        [[fallthrough]];
      case TM_VIEW: flags &= ~(framestateflags::GLOBTM_OK | framestateflags::M2VTM_OK | framestateflags::V2MTM_OK); break;
      case TM_PROJ: flags &= ~(framestateflags::GLOBTM_OK | framestateflags::PROJTM_OK | framestateflags::PERSP_OK); break;
      default: G_ASSERTF(0, "settm(%d) is not allowed", which); return;
    }
    G_ASSERT(m);
    d3d_mat[which].col0 = v_ldu(m->m[0]);
    d3d_mat[which].col1 = v_ldu(m->m[1]);
    d3d_mat[which].col2 = v_ldu(m->m[2]);
    d3d_mat[which].col3 = v_ldu(m->m[3]);
  }

  __forceinline void settm(int which, const TMatrix &t)
  {
    switch (which)
    {
      case TM_WORLD:
        if (&t != &TMatrix::IDENT)
          flags &= ~framestateflags::IDENT_WTM_SET;
        else if (flags & framestateflags::IDENT_WTM_SET)
          return;
        else
          flags |= framestateflags::IDENT_WTM_SET;
        [[fallthrough]];
      case TM_VIEW: flags &= ~(framestateflags::GLOBTM_OK | framestateflags::M2VTM_OK | framestateflags::V2MTM_OK); break;
      case TM_PROJ: flags &= ~(framestateflags::GLOBTM_OK | framestateflags::PROJTM_OK | framestateflags::PERSP_OK); break;
      default: G_ASSERTF(0, "settm(%d) is not allowed", which); return;
    }
    v_mat44_make_from_43cu(d3d_mat[which], &t.m[0][0]);
  }

  __forceinline void settm(int which, const mat44f &m)
  {
    switch (which)
    {
      case TM_WORLD: flags &= ~framestateflags::IDENT_WTM_SET; [[fallthrough]];
      case TM_VIEW: flags &= ~(framestateflags::GLOBTM_OK | framestateflags::M2VTM_OK | framestateflags::V2MTM_OK); break;
      case TM_PROJ: flags &= ~(framestateflags::GLOBTM_OK | framestateflags::PROJTM_OK | framestateflags::PERSP_OK); break;
      default: G_ASSERTF(0, "settm(%d) is not allowed", which); return;
    }
    d3d_mat[which] = m;
  }

  __forceinline const mat44f &gettm_cref(int which)
  {
    G_ASSERTF((uint32_t)which < TM__NUM, "gettm(%d)", which);
    switch (which)
    {
      case TM_LOCAL2VIEW: calc_m2vtm(); break;
      case TM_VIEW2LOCAL: calc_v2mtm(); break;
      case TM_GLOBAL: calc_globtm(); break;
    }

    return d3d_mat[which];
  }

  __forceinline void gettm(int which, Matrix44 *out_m)
  {
    G_ASSERT(out_m);
    const mat44f &m = gettm_cref(which);
    v_stu(out_m->m[0], m.col0);
    v_stu(out_m->m[1], m.col1);
    v_stu(out_m->m[2], m.col2);
    v_stu(out_m->m[3], m.col3);
  }

  __forceinline void gettm(int which, mat44f &out_m) { out_m = gettm_cref(which); }

  __forceinline void gettm(int which, TMatrix &t) { v_mat_43cu_from_mat44(&t[0][0], gettm_cref(which)); }

  __forceinline void getm2vtm(TMatrix &tm) { gettm(TM_LOCAL2VIEW, tm); }

  __forceinline void getglobtm(Matrix44 &tm)
  {
    calc_globtm();
    v_stu(tm.m[0], globtm.col0);
    v_stu(tm.m[1], globtm.col1);
    v_stu(tm.m[2], globtm.col2);
    v_stu(tm.m[3], globtm.col3);
  }

  __forceinline void getglobtm(mat44f &tm)
  {
    calc_globtm();
    tm = globtm;
  }

  __forceinline void setglobtm(const mat44f &tm)
  {
    globtm = tm;
    flags =
      (flags & ~(framestateflags::M2VTM_OK | framestateflags::PROJTM_OK | framestateflags::PERSP_OK)) | framestateflags::GLOBTM_OK;
  }
};
