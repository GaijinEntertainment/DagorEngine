// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/viewVecs.h>
#include <drv/3d/dag_matricesAndPerspective.h>
#include <drv/3d/dag_driver.h>
#include <shaders/dag_shaders.h>
#include <math/dag_TMatrix4.h>

#define VIEW_VARS_LIST \
  VAR(globtm_inv)      \
  VAR(view_vecLT)      \
  VAR(view_vecRT)      \
  VAR(view_vecLB)      \
  VAR(view_vecRB)

#define VAR(a) static ShaderVariableInfo a##VarId(#a, true);
VIEW_VARS_LIST
#undef VAR

void set_inv_globtm_to_shader(const TMatrix4 &view_tm, const TMatrix4 &proj_tm, bool optional)
{
  if (!globtm_invVarId)
  {
    if (!optional)
      LOGERR_ONCE("globtm_inv shadserVar not present, but is required");
    return;
  }
  TMatrix4 globTmInv = inverse44(view_tm * proj_tm).transpose(); // fixme: we'd better store separately inverse(viewTm),
                                                                 // inverse(projTm) for precision
  ShaderGlobal::set_float4x4(globtm_invVarId, globTmInv);
}

ViewVecs calc_view_vecs(const TMatrix &view_tm, const TMatrix4 &proj_tm)
{
  TMatrix4 viewTmOffs = TMatrix4(view_tm);
  viewTmOffs.setrow(3, 0.f, 0.f, 0.f, 1.0f);
  TMatrix4 viewRotProjInv = inverse44(viewTmOffs * proj_tm);
  return {Point4::xyz0(Point3(-1.f, 1.f, 1.f) * viewRotProjInv), Point4::xyz0(Point3(1.f, 1.f, 1.f) * viewRotProjInv),
    Point4::xyz0(Point3(-1.f, -1.f, 1.f) * viewRotProjInv), Point4::xyz0(Point3(1.f, -1.f, 1.f) * viewRotProjInv)};
}

void set_viewvecs_to_shader(Point4 &view_vec_lt, Point4 &view_vec_rt, Point4 &view_vec_lb, Point4 &view_vec_rb, const TMatrix &view_tm,
  const TMatrix4 &proj_tm)
{
  ViewVecs viewVecs = calc_view_vecs(view_tm, proj_tm);
  view_vec_lt = viewVecs.viewVecLT;
  view_vec_rt = viewVecs.viewVecRT;
  view_vec_lb = viewVecs.viewVecLB;
  view_vec_rb = viewVecs.viewVecRB;

  ShaderGlobal::set_color4(view_vecLTVarId, Color4(&view_vec_lt.x));
  ShaderGlobal::set_color4(view_vecRTVarId, Color4(&view_vec_rt.x));
  ShaderGlobal::set_color4(view_vecLBVarId, Color4(&view_vec_lb.x));
  ShaderGlobal::set_color4(view_vecRBVarId, Color4(&view_vec_rb.x));
}

void set_viewvecs_to_shader(const Point4 &view_vec_lt, const Point4 &view_vec_rt, const Point4 &view_vec_lb, const Point4 &view_vec_rb)
{
  ShaderGlobal::set_color4(view_vecLTVarId, Color4(&view_vec_lt.x));
  ShaderGlobal::set_color4(view_vecRTVarId, Color4(&view_vec_rt.x));
  ShaderGlobal::set_color4(view_vecLBVarId, Color4(&view_vec_lb.x));
  ShaderGlobal::set_color4(view_vecRBVarId, Color4(&view_vec_rb.x));
}


void set_viewvecs_to_shader(const TMatrix &view_tm, const TMatrix4 &proj_tm)
{
  Point4 viewVecLT, viewVecRT, viewVecLB, viewVecRB;
  set_viewvecs_to_shader(viewVecLT, viewVecRT, viewVecLB, viewVecRB, view_tm, proj_tm);
}

void set_viewvecs_to_shader()
{
  TMatrix viewTm;
  TMatrix4 projTm;
  d3d::gettm(TM_PROJ, &projTm);
  d3d::gettm(TM_VIEW, viewTm);
  set_viewvecs_to_shader(viewTm, projTm);
}