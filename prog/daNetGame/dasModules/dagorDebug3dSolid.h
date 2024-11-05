// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daScript/daScript.h>
#include <dasModules/aotEcs.h>
#include <dasModules/dasModulesCommon.h>
#include <dasModules/aotDagorMath.h>
#include <render/debug3dSolidBuffered.h>

namespace bind_dascript
{

inline void draw_debug_cube_buffered(const BBox3 &cube, const TMatrix &tm, const Color4 &color, int frames)
{
  return ::draw_debug_solid_cube_buffered(cube, tm, color, (size_t)frames);
}

inline void draw_debug_quad_buffered_4point(Point3 p1, Point3 p2, Point3 p3, Point3 p4, const Color4 &color, int frames)
{
  return ::draw_debug_solid_quad_buffered(p1, p2, p3, p4, color, (size_t)frames);
}

inline void draw_debug_quad_buffered_halfvec(Point3 half_vec_i, Point3 half_vec_j, const TMatrix &tm, const Color4 &color, int frames)
{
  return ::draw_debug_solid_quad_buffered(half_vec_i, half_vec_j, tm, color, (size_t)frames);
}

inline void draw_debug_triangle_buffered(Point3 a, Point3 b, Point3 c, const Color4 &color, int frames)
{
  return ::draw_debug_solid_triangle_buffered(a, b, c, color, (size_t)frames);
}

inline void draw_debug_ball_buffered(const Point3 &sphere_c, float sphere_r, const Color4 &color, int frames)
{
  return ::draw_debug_ball_buffered(sphere_c, sphere_r, color, (size_t)frames);
}

inline void draw_debug_disk_buffered(const Point3 pos, const Point3 norm, float radius, int segments, const Color4 &color, int frames)
{
  return ::draw_debug_solid_disk_buffered(pos, norm, radius, segments, color, (size_t)frames);
}
} // namespace bind_dascript
