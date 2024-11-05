#include <drv/3d/dag_matricesAndPerspective.h>
bool d3d::setpersp(const Driver3dPerspective &p, TMatrix4 *proj_tm)
{
  CHECK_MAIN_THREAD();
  g_frameState.setpersp(p, proj_tm);
  return true;
}

bool d3d::calcproj(const Driver3dPerspective &p, TMatrix4 &proj_tm)
{
  g_frameState.calcproj(p, proj_tm);
  return true;
}

bool d3d::calcproj(const Driver3dPerspective &p, mat44f &proj_tm)
{
  g_frameState.calcproj(p, proj_tm);
  return true;
}

void d3d::calcglobtm(const mat44f &view_tm, const mat44f &proj_tm, mat44f &result)
{
  g_frameState.calcglobtm(view_tm, proj_tm, result);
}

void d3d::calcglobtm(const mat44f &view_tm, const Driver3dPerspective &persp, mat44f &result)
{
  g_frameState.calcglobtm(view_tm, persp, result);
}

void d3d::calcglobtm(const TMatrix &view_tm, const TMatrix4 &proj_tm, TMatrix4 &result)
{
  g_frameState.calcglobtm(view_tm, proj_tm, result);
}

void d3d::calcglobtm(const TMatrix &view_tm, const Driver3dPerspective &persp, TMatrix4 &result)
{
  g_frameState.calcglobtm(view_tm, persp, result);
}

bool d3d::getpersp(Driver3dPerspective &p) { return g_frameState.getpersp(p); }

bool d3d::validatepersp(const Driver3dPerspective &p) { return g_frameState.validatepersp(p); }

void d3d::setglobtm(Matrix44 &tm) { g_frameState.setglobtm(tm); }

bool d3d::settm(int which, const Matrix44 *m)
{
  CHECK_MAIN_THREAD();
  g_frameState.settm(which, m);
  return true;
}

void d3d::settm(int which, const mat44f &m) { g_frameState.settm(which, m); }
bool d3d::settm(int which, const TMatrix &t)
{
  CHECK_MAIN_THREAD();
  g_frameState.settm(which, t);
  return true;
}

bool d3d::gettm(int which, Matrix44 *out_m)
{
  g_frameState.gettm(which, out_m);
  return true;
}

void d3d::gettm(int which, mat44f &out_m) { g_frameState.gettm(which, out_m); }
const mat44f &d3d::gettm_cref(int which) { return g_frameState.gettm_cref(which); }


bool d3d::gettm(int which, TMatrix &t)
{
  g_frameState.gettm(which, t);
  return true;
}

void d3d::getm2vtm(TMatrix &tm) { g_frameState.getm2vtm(tm); }

void d3d::getglobtm(Matrix44 &tm) { g_frameState.getglobtm(tm); }

void d3d::getglobtm(mat44f &tm) { g_frameState.getglobtm(tm); }

void d3d::setglobtm(const mat44f &tm) { g_frameState.setglobtm(tm); }
