// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <shaders/dag_shaders.h>
#include <math/dag_TMatrix4.h>
#include <math/dag_TMatrix4D.h>
#include <drv/3d/dag_driver.h>
#include <render/set_reprojection.h>
#include <render/viewVecs.h>

#define VAR(a) static ShaderVariableInfo a##VarId(#a, true);
SET_REPROJECTION_VARS_LIST
#undef VAR

void set_shadervars_for_reprojection(const TMatrix4 &tr_proj_tm, const TMatrix4 &prev_tr_proj_tm, const TMatrix4 &glob_tm,
  const TMatrix4 &prev_glob_tm, const DPoint3 &prev_world_pos, const DPoint3 &world_pos, const Point4 &prev_view_vec_lt,
  const Point4 &prev_view_vec_rt, const Point4 &prev_view_vec_lb, const Point4 &prev_view_vec_rb)
{
  ShaderGlobal::set_color4(globtm_no_ofs_psf_0VarId, Color4(glob_tm[0]));
  ShaderGlobal::set_color4(globtm_no_ofs_psf_1VarId, Color4(glob_tm[1]));
  ShaderGlobal::set_color4(globtm_no_ofs_psf_2VarId, Color4(glob_tm[2]));
  ShaderGlobal::set_color4(globtm_no_ofs_psf_3VarId, Color4(glob_tm[3]));

  ShaderGlobal::set_color4(projtm_psf_0VarId, Color4(tr_proj_tm[0]));
  ShaderGlobal::set_color4(projtm_psf_1VarId, Color4(tr_proj_tm[1]));
  ShaderGlobal::set_color4(projtm_psf_2VarId, Color4(tr_proj_tm[2]));
  ShaderGlobal::set_color4(projtm_psf_3VarId, Color4(tr_proj_tm[3]));

  // beware: this is correct code for both forward and reverse z, but float point precision is insufficient for forward case
  // better than set nothing
  const float zfar = -safediv(prev_tr_proj_tm._34, prev_tr_proj_tm._33);
  const float znear = -safediv(double(prev_tr_proj_tm._34), double(prev_tr_proj_tm._33) - 1.);
  ShaderGlobal::set_color4(prev_zn_zfarVarId, znear < zfar ? znear : zfar, znear < zfar ? zfar : znear, 0, 0);

  const DPoint3 move = world_pos - prev_world_pos;

  double reprojected_world_pos_d[4] = {(double)prev_glob_tm[0][0] * move.x + (double)prev_glob_tm[0][1] * move.y +
                                         (double)prev_glob_tm[0][2] * move.z + (double)prev_glob_tm[0][3],
    prev_glob_tm[1][0] * move.x + (double)prev_glob_tm[1][1] * move.y + (double)prev_glob_tm[1][2] * move.z +
      (double)prev_glob_tm[1][3],
    prev_glob_tm[2][0] * move.x + (double)prev_glob_tm[2][1] * move.y + (double)prev_glob_tm[2][2] * move.z +
      (double)prev_glob_tm[2][3],
    prev_glob_tm[3][0] * move.x + (double)prev_glob_tm[3][1] * move.y + (double)prev_glob_tm[3][2] * move.z +
      (double)prev_glob_tm[3][3]};

  float reprojected_world_pos[4] = {(float)reprojected_world_pos_d[0], (float)reprojected_world_pos_d[1],
    (float)reprojected_world_pos_d[2], (float)reprojected_world_pos_d[3]};

  ShaderGlobal::set_color4(reprojected_world_view_posVarId, Color4(reprojected_world_pos));
  TMatrix4 prev_glob_tm_ofs = prev_glob_tm;
  prev_glob_tm_ofs.setcol(3, reprojected_world_pos[0], reprojected_world_pos[1], reprojected_world_pos[2],
    reprojected_world_pos[3]); // prev_glob_tm_set.getcol(3) +
  ShaderGlobal::set_color4(prev_globtm_no_ofs_psf_0VarId, Color4(prev_glob_tm_ofs[0]));
  ShaderGlobal::set_color4(prev_globtm_no_ofs_psf_1VarId, Color4(prev_glob_tm_ofs[1]));
  ShaderGlobal::set_color4(prev_globtm_no_ofs_psf_2VarId, Color4(prev_glob_tm_ofs[2]));
  ShaderGlobal::set_color4(prev_globtm_no_ofs_psf_3VarId, Color4(prev_glob_tm_ofs[3]));

  ShaderGlobal::set_color4(move_world_view_posVarId, move.x, move.y, move.z, length(move));

  ShaderGlobal::set_color4(prev_view_vecLTVarId, Color4::xyzw(prev_view_vec_lt));
  ShaderGlobal::set_color4(prev_view_vecRTVarId, Color4::xyzw(prev_view_vec_rt));
  ShaderGlobal::set_color4(prev_view_vecLBVarId, Color4::xyzw(prev_view_vec_lb));
  ShaderGlobal::set_color4(prev_view_vecRBVarId, Color4::xyzw(prev_view_vec_rb));
  ShaderGlobal::set_color4(prev_world_view_posVarId, Color4(prev_world_pos.x, prev_world_pos.y, prev_world_pos.z, 0));
}

void set_shadervars_for_reprojection(const TMatrix4 &tr_proj_tm, const TMatrix4 &glob_tm, const TMatrix4 &prev_glob_tm,
  const DPoint3 &prev_world_pos, const DPoint3 &world_pos, const Point4 &prev_view_vec_lt, const Point4 &prev_view_vec_rt,
  const Point4 &prev_view_vec_lb, const Point4 &prev_view_vec_rb)
{
  set_shadervars_for_reprojection(tr_proj_tm, tr_proj_tm, glob_tm, prev_glob_tm, prev_world_pos, world_pos, prev_view_vec_lt,
    prev_view_vec_rt, prev_view_vec_lb, prev_view_vec_rb);
}

void set_reprojection(const TMatrix4 &proj_tm, const TMatrix4 &glob_tm, const TMatrix4 &prev_proj_tm, const TMatrix4 &prev_glob_tm,
  const Point4 &view_vec_lt, const Point4 &view_vec_rt, const Point4 &view_vec_lb, const Point4 &view_vec_rb,
  const Point4 &prev_view_vec_lt, const Point4 &prev_view_vec_rt, const Point4 &prev_view_vec_lb, const Point4 &prev_view_vec_rb,
  const DPoint3 &world_pos, const DPoint3 &prev_world_pos)
{
  set_viewvecs_to_shader(view_vec_lt, view_vec_rt, view_vec_lb, view_vec_rb);
  TMatrix4 trProjTm = proj_tm.transpose();
  TMatrix4 prevTrProjTm = prev_proj_tm.transpose();
  TMatrix4 globTm = glob_tm.transpose();
  TMatrix4 prevGlobTm = prev_glob_tm.transpose();
  set_shadervars_for_reprojection(trProjTm, prevTrProjTm, globTm, prevGlobTm, prev_world_pos, world_pos, prev_view_vec_lt,
    prev_view_vec_rt, prev_view_vec_lb, prev_view_vec_rb);
}

void set_reprojection(const TMatrix4 &proj_tm, const TMatrix4 &glob_tm, const TMatrix4 &prev_glob_tm, const Point4 &view_vec_lt,
  const Point4 &view_vec_rt, const Point4 &view_vec_lb, const Point4 &view_vec_rb, const Point4 &prev_view_vec_lt,
  const Point4 &prev_view_vec_rt, const Point4 &prev_view_vec_lb, const Point4 &prev_view_vec_rb, const DPoint3 &world_pos,
  const DPoint3 &prev_world_pos)
{
  set_reprojection(proj_tm, glob_tm, proj_tm, prev_glob_tm, view_vec_lt, view_vec_rt, view_vec_lb, view_vec_rb, prev_view_vec_lt,
    prev_view_vec_rt, prev_view_vec_lb, prev_view_vec_rb, world_pos, prev_world_pos);
}

void set_reprojection(const TMatrix4 &proj_tm, const TMatrix4 &glob_tm, const Point2 &prev_zn_zfar, const TMatrix4 &prev_glob_tm,
  const Point4 &view_vec_lt, const Point4 &view_vec_rt, const Point4 &view_vec_lb, const Point4 &view_vec_rb,
  const Point4 &prev_view_vec_lt, const Point4 &prev_view_vec_rt, const Point4 &prev_view_vec_lb, const Point4 &prev_view_vec_rb,
  const DPoint3 &world_pos, const DPoint3 &prev_world_pos)
{
  set_reprojection(proj_tm, glob_tm, proj_tm, prev_glob_tm, view_vec_lt, view_vec_rt, view_vec_lb, view_vec_rb, prev_view_vec_lt,
    prev_view_vec_rt, prev_view_vec_lb, prev_view_vec_rb, world_pos, prev_world_pos);
  ShaderGlobal::set_color4(prev_zn_zfarVarId, prev_zn_zfar.x, prev_zn_zfar.y, 0, 0);
}

void set_reprojection(const TMatrix &view_tm, const TMatrix4 &proj_tm, TMatrix4 &prev_proj_tm, DPoint3 &prev_world_pos,
  TMatrix4 &prev_glob_tm, Point4 &prev_view_vec_lt, Point4 &prev_view_vec_rt, Point4 &prev_view_vec_lb, Point4 &prev_view_vec_rb,
  const DPoint3 *world_pos)
{
  TMatrix4 viewRot = TMatrix4(view_tm);

  DPoint3 worldPos;
  if (world_pos)
    worldPos = *world_pos;
  else
  {
    TMatrix4D viewRotInv;
    double det;
    inverse44(TMatrix4D(viewRot), viewRotInv, det);
    worldPos = DPoint3(viewRotInv.m[3][0], viewRotInv.m[3][1], viewRotInv.m[3][2]);
  }
  viewRot.setrow(3, 0.0f, 0.0f, 0.0f, 1.0f);
  TMatrix4 globTm = viewRot * proj_tm;

  Point4 viewVecLT, viewVecRT, viewVecLB, viewVecRB;
  TMatrix4 trProjTm = proj_tm.transpose();
  TMatrix4 prevTrProjTm = prev_proj_tm.transpose();
  globTm = globTm.transpose();

  set_viewvecs_to_shader(viewVecLT, viewVecRT, viewVecLB, viewVecRB, view_tm, proj_tm);
  set_shadervars_for_reprojection(trProjTm, prevTrProjTm, globTm, prev_glob_tm, prev_world_pos, worldPos, prev_view_vec_lt,
    prev_view_vec_rt, prev_view_vec_lb, prev_view_vec_rb);

  prev_glob_tm = globTm;
  prev_proj_tm = proj_tm;
  prev_view_vec_lt = viewVecLT;
  prev_view_vec_rt = viewVecRT;
  prev_view_vec_lb = viewVecLB;
  prev_view_vec_rb = viewVecRB;
  prev_world_pos = worldPos;
}

void set_reprojection(const TMatrix &view_tm, const TMatrix4 &proj_tm, DPoint3 &prev_world_pos, TMatrix4 &prev_glob_tm,
  Point4 &prev_view_vec_lt, Point4 &prev_view_vec_rt, Point4 &prev_view_vec_lb, Point4 &prev_view_vec_rb, const DPoint3 *world_pos)
{
  TMatrix4 prev_proj_tm = proj_tm;
  set_reprojection(view_tm, proj_tm, prev_proj_tm, prev_world_pos, prev_glob_tm, prev_view_vec_lt, prev_view_vec_rt, prev_view_vec_lb,
    prev_view_vec_rb, world_pos);
}

void set_reprojection(const TMatrix &view_tm, const TMatrix4 &proj_tm, const Point2 &prev_zn_zfar, DPoint3 &prev_world_pos,
  TMatrix4 &prev_glob_tm, Point4 &prev_view_vec_lt, Point4 &prev_view_vec_rt, Point4 &prev_view_vec_lb, Point4 &prev_view_vec_rb,
  const DPoint3 *world_pos)
{
  TMatrix4 prev_proj_tm = proj_tm;
  set_reprojection(view_tm, proj_tm, prev_proj_tm, prev_world_pos, prev_glob_tm, prev_view_vec_lt, prev_view_vec_rt, prev_view_vec_lb,
    prev_view_vec_rb, world_pos);
  ShaderGlobal::set_color4(prev_zn_zfarVarId, prev_zn_zfar.x, prev_zn_zfar.y, 0, 0);
}

ScopeReprojection::ScopeReprojection()
{
#define VAR(a) a = ShaderGlobal::get_color4(a##VarId);
  SET_REPROJECTION_VARS_LIST
#undef VAR
}

ScopeReprojection::~ScopeReprojection()
{
#define VAR(a) ShaderGlobal::set_color4(a##VarId, a);
  SET_REPROJECTION_VARS_LIST
#undef VAR
}