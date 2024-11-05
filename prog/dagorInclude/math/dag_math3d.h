//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

/// @addtogroup math
/// @{

/** @file
  Common classes and functions for 2D, 3D and 4D math.
*/

#include <math/dag_mathBase.h>
#include <math/dag_Point2.h>
#include <math/dag_Point3.h>
#include <math/dag_Point4.h>
#include <math/dag_Quat.h>
#include <math/dag_Matrix3.h>
#include <math/dag_TMatrix.h>
#include <math/dag_TMatrix4.h>
#include <math/dag_bounds2.h>
#include <math/dag_bounds3.h>
#include <math/dag_frustum.h>

#include <math/integer/dag_IPoint2.h>
#include <math/integer/dag_IPoint3.h>
#include <math/integer/dag_IBBox2.h>
#include <math/integer/dag_IBBox3.h>


#define INLINE __forceinline

/// rotation matrix from #Quat
INLINE Matrix3 makeM3(const Quat &q)
{
  Matrix3 m;

  if (q.x == 0 && q.y == 0 && q.z == 0 && q.w == 0)
  {
    m.identity();
    return m;
  }

  m.m[0][0] = (q.x * q.x + q.w * q.w) * 2 - 1;
  m.m[1][0] = (q.x * q.y - q.z * q.w) * 2;
  m.m[2][0] = (q.x * q.z + q.y * q.w) * 2;

  m.m[0][1] = (q.y * q.x + q.z * q.w) * 2;
  m.m[1][1] = (q.y * q.y + q.w * q.w) * 2 - 1;
  m.m[2][1] = (q.y * q.z - q.x * q.w) * 2;

  m.m[0][2] = (q.z * q.x - q.y * q.w) * 2;
  m.m[1][2] = (q.z * q.y + q.x * q.w) * 2;
  m.m[2][2] = (q.z * q.z + q.w * q.w) * 2 - 1;

  return m;
}

INLINE BBox3 operator*(const TMatrix &tm, const BBox3 &b)
{
  BBox3 box;
  if (b.isempty())
    return box;
  for (int i = 0; i < 2; ++i)
    for (int j = 0; j < 2; ++j)
    {
      box += tm * Point3(b.lim[i].x, b.lim[j].y, b.lim[0].z);
      box += tm * Point3(b.lim[i].x, b.lim[j].y, b.lim[1].z);
    }
  return box;
}

INLINE BSphere3 operator*(const TMatrix &tm, BSphere3 b)
{
  if (b.r < 0)
    return b;
  b.c = tm * b.c;
  real rd[3] = {lengthSq(tm.getcol(0)), lengthSq(tm.getcol(1)), lengthSq(tm.getcol(2))};
  if (rd[0] < rd[1])
    rd[0] = rd[1];
  if (rd[0] < rd[2])
    rd[0] = rd[2];
  b.r2 = b.r2 * rd[0];
  b.r = sqrtf(b.r2);
  return b;
}


bool test_triangle_sphere_intersection(const Point3 *triangle, const BSphere3 &sphere);
bool test_triangle_sphere_intersection(const Point3 &v0, const Point3 &v1, const Point3 &v2, const BSphere3 &sphere);

bool test_triangle_cylinder_intersection(const Point3 &v0, const Point3 &v1, const Point3 &v2, const Point3 &p0, const Point3 &p1,
  float radius);

bool test_segment_cylinder_intersection(const Point3 &p0, const Point3 &p1, const Point3 &cylinder_p0, const Point3 &cylinder_p1,
  float cylinder_radius);

// note: can return true when point is on short vertical distance from triangle plane
VECTORCALL inline bool is_point_in_triangle(vec3f p, vec3f t1, vec3f t2, vec3f t3)
{
  vec3f a = v_sub(t1, p);
  vec3f b = v_sub(t2, p);
  vec3f c = v_sub(t3, p);
  vec3f u = v_cross3(b, c);
  vec3f v = v_cross3(c, a);
  vec3f w = v_cross3(a, b);
  float dp1 = v_extract_x(v_dot3_x(u, v));
  float dp2 = v_extract_x(v_dot3_x(u, w));
  if (dp1 < 0.f || dp2 < 0.f)
    return false;
  return true;
}

inline bool is_point_in_triangle(const Point3 &p, const Point3 &t1, const Point3 &t2, const Point3 &t3)
{
  return is_point_in_triangle(v_ldu(&p.x), v_ldu(&t1.x), v_ldu(&t2.x), v_ldu(&t3.x));
}

inline Point3 project_onto_plane(const Point3 &vec, const Point3 &normal) { return vec - vec * normal * normal; }

/// Returns deterministic but unremarkable normal
/// Important: (0, 1, 0) returns (1, 0, 0)
inline Point3 get_some_normal(const Point3 &vec)
{
  Point3 altDir = abs(dot(vec, Point3(0, 0, 1))) > 0.5 * vec.length() ? Point3(1, 0, 0) : Point3(0, 0, 1);
  return normalize(cross(vec, altDir));
}

#undef INLINE


/// @}
