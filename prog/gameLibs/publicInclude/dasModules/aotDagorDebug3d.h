//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <daScript/daScript.h>
#include <dasModules/aotEcs.h>
#include <dasModules/dasModulesCommon.h>
#include <dasModules/aotDagorMath.h>

#include <debug/dag_debug3d.h>
#include <debug/dag_textMarks.h>

namespace bind_dascript
{
inline void draw_debug_line_buffered(const Point3 &p0, const Point3 &p1, E3DCOLOR color, int frames)
{
  return ::draw_debug_line_buffered(p0, p1, color, (size_t)frames);
}

inline void draw_debug_rect_buffered(const Point3 &p0, const Point3 &p1, const Point3 &p2, E3DCOLOR color, int frames)
{
  return ::draw_debug_rect_buffered(p0, p1, p2, color, (size_t)frames);
}

inline void draw_debug_box_buffered(const BBox3 &box, E3DCOLOR color, int frames)
{
  return ::draw_debug_box_buffered(box, color, (size_t)frames);
}

inline void draw_debug_box_buffered_axis(const Point3 &p0, const Point3 &ax, const Point3 &ay, const Point3 &az, E3DCOLOR color,
  int frames)
{
  return ::draw_debug_box_buffered(p0, ax, ay, az, color, (size_t)frames);
}

inline void draw_debug_box_buffered_tm(const BBox3 &box, const TMatrix &tm, E3DCOLOR color, int frames)
{
  return ::draw_debug_box_buffered(box, tm, color, (size_t)frames);
}

inline void draw_debug_elipse_buffered(const Point3 &pos, const Point3 &half_diag1, const Point3 &half_diag2, E3DCOLOR col, int segs,
  size_t frames)
{
  return ::draw_debug_elipse_buffered(pos, half_diag1, half_diag2, col, segs, (size_t)frames);
}

inline void draw_debug_circle_buffered(const Point3 &pos, Point3 norm, real rad, E3DCOLOR col, int segs, int frames)
{
  return ::draw_debug_circle_buffered(pos, norm, rad, col, segs, (size_t)frames);
}

inline void draw_debug_sphere_buffered(const Point3 &c, real rad, E3DCOLOR color, int segs, int frames)
{
  return ::draw_debug_sphere_buffered(c, rad, color, segs, (size_t)frames);
}

inline void draw_debug_tube_buffered(const Point3 &p0, const Point3 &p1, float radius, E3DCOLOR col, int segs, float circle_density,
  int frames)
{
  return ::draw_debug_tube_buffered(p0, p1, radius, col, segs, circle_density, (size_t)frames);
}

inline void draw_debug_cone_buffered(const Point3 &p0, const Point3 &p1, float radius, E3DCOLOR col, int segs, int frames)
{
  return ::draw_debug_cone_buffered(p0, p1, radius, col, segs, (size_t)frames);
}

inline void draw_debug_capsule_buffered(const Point3 &p0, const Point3 &p1, real rad, E3DCOLOR color, int segs, int frames)
{
  return ::draw_debug_capsule_buffered(p0, p1, rad, color, segs, (size_t)frames);
}

inline void draw_debug_arrow_buffered(const Point3 &from, const Point3 &to, E3DCOLOR color, int frames)
{
  return ::draw_debug_arrow_buffered(from, to, color, (size_t)frames);
}

inline void draw_debug_tetrapod_buffered(const Point3 &c, real radius, E3DCOLOR col, int frames)
{
  return ::draw_debug_tetrapod_buffered(c, radius, col, (size_t)frames);
}

inline void draw_debug_tehedron_buffered(const Point3 &c, real radius, E3DCOLOR col, int frames)
{
  return ::draw_debug_tehedron_buffered(c, radius, col, (size_t)frames);
}

inline void draw_cached_debug_quad(const Point3 &p0, const Point3 &p1, const Point3 &p2, const Point3 &p3, E3DCOLOR col)
{
  Point3 p[4] = {p0, p1, p3, p2};
  return ::draw_cached_debug_quad(p, col);
}

inline void draw_cached_debug_trilist(const Point3 *arr, int tn, E3DCOLOR col) { return ::draw_cached_debug_trilist(arr, tn, col); }

} // namespace bind_dascript
