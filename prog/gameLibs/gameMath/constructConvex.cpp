#include <gameMath/constructConvex.h>
#include <ioSys/dag_dataBlock.h>
#include <math/dag_mathBase.h>
#include <math/dag_TMatrix.h>
#include <vecmath/dag_vecMath.h>

void gamemath::construct_convex_from_frustum(ecs::List<Point4> &planes, float znear, float zfar, float fovx, float fovy,
  const TMatrix &tm)
{
  mat44f v_tm;
  v_mat44_make_from_43cu_unsafe(v_tm, tm.m[0]);
  plane3f nearPlane = v_transform_plane(v_make_vec4f(-1.f, 0.f, 0.f, znear), v_tm);
  plane3f farPlane = v_transform_plane(v_make_vec4f(1.f, 0.f, 0.f, -zfar), v_tm);
  plane3f leftPlane = v_transform_plane(v_make_vec4f(-sinf(fovx), 0.f, -cosf(fovx), 0.f), v_tm);
  plane3f rightPlane = v_transform_plane(v_make_vec4f(-sinf(fovx), 0.f, cosf(fovx), 0.f), v_tm);
  plane3f topPlane = v_transform_plane(v_make_vec4f(-sinf(fovy), cosf(fovy), 0.f, 0.f), v_tm);
  plane3f bottomPlane = v_transform_plane(v_make_vec4f(-sinf(fovy), -cosf(fovy), 0.f, 0.f), v_tm);
  planes.resize(6);
  v_stu(&planes[0], nearPlane);
  v_stu(&planes[1], farPlane);
  v_stu(&planes[2], leftPlane);
  v_stu(&planes[3], rightPlane);
  v_stu(&planes[4], topPlane);
  v_stu(&planes[5], bottomPlane);
}

void gamemath::construct_convex_from_box(const bbox3f &box, ecs::List<Point4> &planes)
{
  planes.resize(6);
  v_stu(&planes[0], v_make_vec4f(-1.f, 0.f, 0.f, v_extract_x(box.bmin)));
  v_stu(&planes[1], v_make_vec4f(1.f, 0.f, 0.f, -v_extract_x(box.bmax)));
  v_stu(&planes[2], v_make_vec4f(0.f, 0.f, 1.f, -v_extract_z(box.bmax)));
  v_stu(&planes[3], v_make_vec4f(0.f, 0.f, -1.f, v_extract_z(box.bmin)));
  v_stu(&planes[4], v_make_vec4f(0.f, 1.f, 0.f, -v_extract_y(box.bmax)));
  v_stu(&planes[5], v_make_vec4f(0.f, -1.f, 0.f, v_extract_y(box.bmin)));
}
