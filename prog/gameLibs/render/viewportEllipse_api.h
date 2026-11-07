// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <math/dag_Point2.h>

class TMatrix;
class TMatrix4;

// the default holds no pixel: the center is off screen
struct ViewportEllipse
{
  Point2 xAxis = Point2(1.f, 0.f);
  Point2 yAxis = Point2(0.f, 1.f);
  Point2 center = Point2(-2.f, -2.f);
  Point2 halfExtent = Point2(0.f, 0.f);
  float yScale = 1.f;
};

ViewportEllipse calc_viewport_ellipse(float lens_bounding_sphere_radius, const TMatrix &lens_wtm,
  const TMatrix4 &view_rot_jitter_proj_tm, float viewport_scale);
