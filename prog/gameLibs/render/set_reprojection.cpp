#include <shaders/dag_shaders.h>
#include <math/dag_TMatrix4.h>
#include <math/dag_TMatrix4D.h>
#include <3d/dag_drv3d.h>
#include <render/set_reprojection.h>

#define VAR(a) static int a##VarId = -2;
SET_REPROJECTION_VARS_LIST
#undef VAR

static void init_vars()
{
#define VAR(a) a##VarId = get_shader_variable_id(#a, true);
  SET_REPROJECTION_VARS_LIST
#undef VAR
}

void set_reprojection(const TMatrix &viewTm, const TMatrix4 &projTm, DPoint3 &prevWorldPos, TMatrix4 &prevGlobTm,
  Point4 &prevViewVecLT, Point4 &prevViewVecRT, Point4 &prevViewVecLB, Point4 &prevViewVecRB, const DPoint3 *world_pos)
{
  if (view_vecLTVarId == -2)
    init_vars();

  TMatrix4 viewRot = TMatrix4(viewTm);

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
  TMatrix4 globtm = viewRot * projTm;

  Point4 viewVecLT, viewVecRT, viewVecLB, viewVecRB;
  {
    TMatrix4 viewRotProjInv = inverse44(globtm);

    viewVecLT = Point4::xyz0(Point3(-1.f, 1.f, 1.f) * viewRotProjInv);
    viewVecRT = Point4::xyz0(Point3(1.f, 1.f, 1.f) * viewRotProjInv);
    viewVecLB = Point4::xyz0(Point3(-1.f, -1.f, 1.f) * viewRotProjInv);
    viewVecRB = Point4::xyz0(Point3(1.f, -1.f, 1.f) * viewRotProjInv);
  }
  TMatrix4 trProjTm = projTm.transpose();
  globtm = globtm.transpose();

  ShaderGlobal::set_color4(view_vecLTVarId, Color4(&viewVecLT.x));
  ShaderGlobal::set_color4(view_vecRTVarId, Color4(&viewVecRT.x));
  ShaderGlobal::set_color4(view_vecLBVarId, Color4(&viewVecLB.x));
  ShaderGlobal::set_color4(view_vecRBVarId, Color4(&viewVecRB.x));

  ShaderGlobal::set_color4(globtm_no_ofs_psf_0VarId, Color4(globtm[0]));
  ShaderGlobal::set_color4(globtm_no_ofs_psf_1VarId, Color4(globtm[1]));
  ShaderGlobal::set_color4(globtm_no_ofs_psf_2VarId, Color4(globtm[2]));
  ShaderGlobal::set_color4(globtm_no_ofs_psf_3VarId, Color4(globtm[3]));

  ShaderGlobal::set_color4(projtm_psf_0VarId, Color4(trProjTm[0]));
  ShaderGlobal::set_color4(projtm_psf_1VarId, Color4(trProjTm[1]));
  ShaderGlobal::set_color4(projtm_psf_2VarId, Color4(trProjTm[2]));
  ShaderGlobal::set_color4(projtm_psf_3VarId, Color4(trProjTm[3]));

  ShaderGlobal::set_color4(prev_globtm_no_ofs_psf_0VarId, Color4(prevGlobTm[0]));
  ShaderGlobal::set_color4(prev_globtm_no_ofs_psf_1VarId, Color4(prevGlobTm[1]));
  ShaderGlobal::set_color4(prev_globtm_no_ofs_psf_2VarId, Color4(prevGlobTm[2]));
  ShaderGlobal::set_color4(prev_globtm_no_ofs_psf_3VarId, Color4(prevGlobTm[3]));
  const DPoint3 move = worldPos - prevWorldPos;

  double reprojected_world_pos_d[4] = {(double)prevGlobTm[0][0] * move.x + (double)prevGlobTm[0][1] * move.y +
                                         (double)prevGlobTm[0][2] * move.z + (double)prevGlobTm[0][3],
    prevGlobTm[1][0] * move.x + (double)prevGlobTm[1][1] * move.y + (double)prevGlobTm[1][2] * move.z + (double)prevGlobTm[1][3],
    prevGlobTm[2][0] * move.x + (double)prevGlobTm[2][1] * move.y + (double)prevGlobTm[2][2] * move.z + (double)prevGlobTm[2][3],
    prevGlobTm[3][0] * move.x + (double)prevGlobTm[3][1] * move.y + (double)prevGlobTm[3][2] * move.z + (double)prevGlobTm[3][3]};

  float reprojected_world_pos[4] = {(float)reprojected_world_pos_d[0], (float)reprojected_world_pos_d[1],
    (float)reprojected_world_pos_d[2], (float)reprojected_world_pos_d[3]};

  ShaderGlobal::set_color4(move_world_view_posVarId, move.x, move.y, move.z, 0);
  ShaderGlobal::set_color4(reprojected_world_view_posVarId, Color4(reprojected_world_pos));

  ShaderGlobal::set_color4(prev_view_vecLTVarId, Color4::xyzw(prevViewVecLT));
  ShaderGlobal::set_color4(prev_view_vecRTVarId, Color4::xyzw(prevViewVecRT));
  ShaderGlobal::set_color4(prev_view_vecLBVarId, Color4::xyzw(prevViewVecLB));
  ShaderGlobal::set_color4(prev_view_vecRBVarId, Color4::xyzw(prevViewVecRB));
  ShaderGlobal::set_color4(prev_world_view_posVarId, Color4(prevWorldPos.x, prevWorldPos.y, prevWorldPos.z, 0));

  prevGlobTm = globtm;
  prevViewVecLT = viewVecLT;
  prevViewVecRT = viewVecRT;
  prevViewVecLB = viewVecLB;
  prevViewVecRB = viewVecRB;
  prevWorldPos = worldPos;
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