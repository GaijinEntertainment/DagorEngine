// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <math/dag_Point2.h>
#include <math/dag_Point3.h>
#include <math/dag_Point4.h>
#include <math/dag_Quat.h>
#include <math/integer/dag_IPoint2.h>
#include <math.h>

struct RtssLightXform
{
  DPoint3 c = DPoint3(0, 0, 0);
  double z0 = 0; // camera depth when made, then fixed
  Point3 ex, ey, d;
  float sy = 1; // light space y is sy * dot(ey, x)

  DPoint3 toLs(const DPoint3 &x) const { return DPoint3(dpoint3(ex) * x, sy * (dpoint3(ey) * x), dpoint3(d) * x) + c; }
  DPoint3 fromLs(const DPoint3 &ls) const
  {
    const DPoint3 r = ls - c;
    return dpoint3(ex) * r.x + dpoint3(ey) * (r.y / sy) + dpoint3(d) * r.z;
  }
};

// 1 / sin(sun elevation) in steps of sqrt(2), for a unit dir_to_sun
inline float rtss_aniso_scale(const Point3 &dir_to_sun, float max_aniso)
{
  const float s = 1.f / fmaxf(dir_to_sun.y, 1.f / max_aniso);
  return fminf(exp2f(roundf(2.f * log2f(s)) * 0.5f), max_aniso);
}

// Rows only; derive c after this call.
inline void rtss_orthonormalize(RtssLightXform &x)
{
  x.d = normalize(x.d);
  x.ex = normalize(x.ey % x.d);
  x.ey = x.d % x.ex;
}

inline RtssLightXform rtss_first_transform(const Point3 &dir_to_sun, const DPoint3 &camera, float sy = 1)
{
  RtssLightXform x;
  x.d = -normalize(dir_to_sun);
  x.ey = fabsf(x.d.y) > 0.999f ? Point3(1, 0, 0) : Point3(0, 1, 0);
  rtss_orthonormalize(x);
  x.sy = sy;
  x.z0 = x.toLs(camera).z;
  return x;
}

inline RtssLightXform rtss_next_transform(const RtssLightXform &old, const Point3 &dir_to_sun, const DPoint3 &pivot,
  const DPoint3 &camera, float sy = 1)
{
  RtssLightXform n;
  n.d = -normalize(dir_to_sun); // constraint: quat_rotation_arc snaps to 180 deg above 179.19 deg
  n.ey = sy == old.sy ? quat_rotation_arc(old.d, n.d) * old.ey : (fabsf(n.d.y) > 0.999f ? Point3(1, 0, 0) : Point3(0, 1, 0));
  rtss_orthonormalize(n);
  n.sy = sy;
  n.c = old.toLs(pivot) - n.toLs(pivot);
  n.z0 = n.toLs(camera).z;
  return n;
}

inline IPoint2 rtss_page(const RtssLightXform &x, const DPoint3 &pos, double page_world)
{
  const DPoint3 ls = x.toLs(pos);
  return IPoint2(int(floor(ls.x / page_world)), int(floor(ls.y / page_world)));
}

// False when the directions are parallel or antiparallel within 1e-6.
inline bool rtss_shift_dir(const RtssLightXform &old, const RtssLightXform &nw, Point2 &u)
{
  const DPoint3 oldD = dpoint3(old.d);
  const double ux = dpoint3(nw.ex) * oldD, uy = nw.sy * (dpoint3(nw.ey) * oldD);
  const double len = sqrt(ux * ux + uy * uy);
  if (len < 1e-6)
    return false;
  u = Point2(float(ux / len), float(uy / len));
  return true;
}

// w: M * origin + c - (page_origin * page_world, z0)
inline void rtss_pack_xform(const RtssLightXform &x, const DPoint3 &origin, const IPoint2 &page_origin, double page_world,
  Point4 rows[3])
{
  const DPoint3 k = x.toLs(origin) - DPoint3(page_origin.x * page_world, page_origin.y * page_world, x.z0);
  rows[0] = Point4::xyzV(x.ex, float(k.x));
  rows[1] = Point4::xyzV(x.ey * x.sy, float(k.y));
  rows[2] = Point4::xyzV(x.d, float(k.z));
}

// the rows of rtss_pack_xform applied to an origin-relative x; w = 0 transforms a direction
inline Point3 rtss_rows_to_ls(const Point4 rows[3], const Point3 &x, float w = 1)
{
  const Point4 p = Point4::xyzV(x, w);
  return Point3(rows[0] * p, rows[1] * p, rows[2] * p);
}

// the first t with (W/2 - 1) * (cos(t) - 1 + (h_over_e + 1) * sy * sin(t)) = 1
inline float rtss_window_limit_rad(int window_pages, float h_over_e, float sy = 1)
{
  const double m = double(window_pages / 2 - 1), a = m * (h_over_e + 1) * sy, b = m, c = 1 + m;
  return float(asin(c / hypot(a, b)) - atan2(b, a));
}

inline DPoint3 rtss_rotate(const DPoint3 &v, const DPoint3 &unit_axis, double angle)
{
  const double c = cos(angle), s = sin(angle);
  return v * c + (unit_axis % v) * s + unit_axis * ((unit_axis * v) * (1 - c));
}
