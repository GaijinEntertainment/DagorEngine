// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

/** \addtogroup de3Common
  @{
*/

#include <math/dag_math3d.h>
#include <math/dag_capsule.h>
#include <drv/3d/dag_driver.h>
#include <debug/dag_debug3d.h>

/**
  \brief Draws box with debug lines.

  \param[in] transform box matrix.
  \param[in] dimensions box sizes.
  \param[in] color box color.

  @see TMatrix E3DCOLOR debugDrawSphereShape()
  */
static void debugDrawBoxShape(const TMatrix &transform, const Point3 &dimensions, E3DCOLOR color)
{
  static int f_idx[6][4] = {{4, 7, 0, 3}, {0, 3, 1, 2}, {1, 2, 5, 6}, {5, 6, 4, 7}, {7, 3, 6, 2}, {4, 0, 5, 1}};

  Point3 points[8];
  {
    Point3 halfDim = dimensions * 0.5f;

    points[0] = Point3(-halfDim.x, -halfDim.y, -halfDim.z);
    points[1] = Point3(halfDim.x, -halfDim.y, -halfDim.z);
    points[2] = Point3(halfDim.x, halfDim.y, -halfDim.z);
    points[3] = Point3(-halfDim.x, halfDim.y, -halfDim.z);

    points[4] = Point3(-halfDim.x, -halfDim.y, halfDim.z);
    points[5] = Point3(halfDim.x, -halfDim.y, halfDim.z);
    points[6] = Point3(halfDim.x, halfDim.y, halfDim.z);
    points[7] = Point3(-halfDim.x, halfDim.y, halfDim.z);
  }

  for (int n = 0; n < 8; n++)
    points[n] = transform * points[n];

  draw_cached_debug_line(points[0], points[1], color);
  draw_cached_debug_line(points[1], points[2], color);
  draw_cached_debug_line(points[2], points[3], color);
  draw_cached_debug_line(points[3], points[0], color);

  draw_cached_debug_line(points[4], points[5], color);
  draw_cached_debug_line(points[5], points[6], color);
  draw_cached_debug_line(points[6], points[7], color);
  draw_cached_debug_line(points[7], points[4], color);

  draw_cached_debug_line(points[0], points[4], color);
  draw_cached_debug_line(points[1], points[5], color);
  draw_cached_debug_line(points[2], points[6], color);
  draw_cached_debug_line(points[3], points[7], color);

  color.a /= 2;
  const real grid_step = 1.0;
  for (int i = 4; i < 5; i++)
  {
    float w = length(points[f_idx[i][0]] - points[f_idx[i][1]]);
    float h = length(points[f_idx[i][0]] - points[f_idx[i][2]]);
    if (w > grid_step)
    {
      float p = 1.0 / (floor(w + 0.1 / grid_step) + 1.0);
      for (float t = p; t < 1.0; t += p)
        draw_cached_debug_line(points[f_idx[i][0]] * t + points[f_idx[i][1]] * (1 - t),
          points[f_idx[i][2]] * t + points[f_idx[i][3]] * (1 - t), color);
    }
    if (h > grid_step)
    {
      float p = 1.0 / (floor(h + 0.1 / grid_step) + 1.0);
      for (float t = p; t < 1.0; t += p)
        draw_cached_debug_line(points[f_idx[i][1]] * t + points[f_idx[i][3]] * (1 - t),
          points[f_idx[i][0]] * t + points[f_idx[i][2]] * (1 - t), color);
    }
  }
}

/**
  \brief Draws sphere with debug lines.

  \param[in] transform sphere matrix.
  \param[in] radius sphere radius.
  \param[in] color sphere color.

  @see TMatrix E3DCOLOR debugDrawBoxShape()
  */
static void debugDrawSphereShape(const TMatrix &transform, real radius, E3DCOLOR c)
{
  BSphere3 sphere(Point3(0, 0, 0), radius);
  sphere = transform * sphere;
  draw_cached_debug_sphere(sphere.c, sphere.r, c);
  draw_cached_debug_line(sphere.c, sphere.c + transform.getcol(0) * sphere.r, E3DCOLOR(180, 0, 0));
  draw_cached_debug_line(sphere.c, sphere.c + transform.getcol(1) * sphere.r, E3DCOLOR(0, 180, 0));
  draw_cached_debug_line(sphere.c, sphere.c + transform.getcol(2) * sphere.r, E3DCOLOR(0, 0, 180));
}
/** @} */
