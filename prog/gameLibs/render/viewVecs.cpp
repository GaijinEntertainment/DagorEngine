#include <render/viewVecs.h>
#include <3d/dag_drv3d.h>
#include <shaders/dag_shaders.h>
#include <math/dag_TMatrix4.h>

#define GLOBTM_VARS_LIST VAR(globtm_inv)

#define VIEW_VARS_LIST \
  VAR(view_vecLT)      \
  VAR(view_vecRT)      \
  VAR(view_vecLB)      \
  VAR(view_vecRB)

#define VAR(a) static int a##VarId = -2;
GLOBTM_VARS_LIST
VIEW_VARS_LIST
#undef VAR
#define VAR(a) a##VarId = get_shader_variable_id(#a, optional);
inline void init_view_vars(bool optional)
{
  if (view_vecLTVarId != -2)
    return;
  VIEW_VARS_LIST
}
inline void init_inv_globtm_vars(bool optional)
{
  if (globtm_invVarId != -2)
    return;
  GLOBTM_VARS_LIST
}
#undef VAR

void set_inv_globtm_to_shader(const TMatrix4 &viewTm, const TMatrix4 &projTm, bool optional)
{
  init_inv_globtm_vars(optional);
  if (globtm_invVarId < 0)
    return;
  TMatrix4 globTmInv = inverse44(viewTm * projTm).transpose(); // fixme: we'd better store separately inverse(viewTm), inverse(projTm)
                                                               // for precision
  ShaderGlobal::set_float4x4(globtm_invVarId, globTmInv);
}

void set_inv_globtm_to_shader(bool optional)
{
  init_inv_globtm_vars(optional);
  if (globtm_invVarId < 0)
    return;
  TMatrix4 viewTm, projTm;
  d3d::gettm(TM_PROJ, &projTm);
  d3d::gettm(TM_VIEW, &viewTm);
  set_inv_globtm_to_shader(viewTm, projTm, optional);
}

void get_viewvecs(Point4 &viewVecLT, Point4 &viewVecRT, Point4 &viewVecLB, Point4 &viewVecRB, const TMatrix &viewTm,
  const TMatrix4 &projTm)
{
  TMatrix4 viewTmOffs = TMatrix4(viewTm);
  viewTmOffs.setrow(3, 0.f, 0.f, 0.f, 1.0f);
  TMatrix4 viewRotProjInv = inverse44(viewTmOffs * projTm);

  viewVecLT = Point4::xyz0(Point3(-1.f, 1.f, 1.f) * viewRotProjInv);
  viewVecRT = Point4::xyz0(Point3(1.f, 1.f, 1.f) * viewRotProjInv);
  viewVecLB = Point4::xyz0(Point3(-1.f, -1.f, 1.f) * viewRotProjInv);
  viewVecRB = Point4::xyz0(Point3(1.f, -1.f, 1.f) * viewRotProjInv);
}

void set_viewvecs_to_shader(Point4 &viewVecLT, Point4 &viewVecRT, Point4 &viewVecLB, Point4 &viewVecRB, const TMatrix &viewTm,
  const TMatrix4 &projTm)
{
  get_viewvecs(viewVecLT, viewVecRT, viewVecLB, viewVecRB, viewTm, projTm);

  init_view_vars(false);
  ShaderGlobal::set_color4(view_vecLTVarId, Color4(&viewVecLT.x));
  ShaderGlobal::set_color4(view_vecRTVarId, Color4(&viewVecRT.x));
  ShaderGlobal::set_color4(view_vecLBVarId, Color4(&viewVecLB.x));
  ShaderGlobal::set_color4(view_vecRBVarId, Color4(&viewVecRB.x));
}


void set_viewvecs_to_shader(const TMatrix &viewTm, const TMatrix4 &projTm)
{
  Point4 viewVecLT, viewVecRT, viewVecLB, viewVecRB;
  set_viewvecs_to_shader(viewVecLT, viewVecRT, viewVecLB, viewVecRB, viewTm, projTm);
}

void set_viewvecs_to_shader()
{
  TMatrix viewTm;
  TMatrix4 projTm;
  d3d::gettm(TM_PROJ, &projTm);
  d3d::gettm(TM_VIEW, viewTm);
  set_viewvecs_to_shader(viewTm, projTm);
}