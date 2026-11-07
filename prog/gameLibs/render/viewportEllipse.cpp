// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "viewportEllipse_api.h"

#include <math/dag_Point3.h>
#include <math/dag_TMatrix.h>
#include <math/dag_TMatrix4.h>

ViewportEllipse calc_viewport_ellipse(const float lens_bounding_sphere_radius, const TMatrix &lens_wtm,
  const TMatrix4 &view_rot_jitter_proj_tm, float viewport_scale)
{
  const Point3 lensRight = -lens_wtm.col[0] * lens_bounding_sphere_radius;
  const Point3 lensUp = lens_wtm.col[2] * lens_bounding_sphere_radius;

  const Point3 centerWpos = lens_wtm.col[3];
  const Point3 rightWpos = centerWpos + lensRight;
  const Point3 upWpos = centerWpos + lensUp;

  const auto worldToViewport = [&view_rot_jitter_proj_tm, viewport_scale](const Point3 &wpos) {
    const Point4 posCs = Point4::xyz1(wpos) * view_rot_jitter_proj_tm;
    Point2 posVp = Point2::xy(posCs);
    posVp = safediv(posVp, posCs.w);
    posVp = posVp * 0.5f + Point2(0.5f, 0.5f);
    posVp.y = 1.0 - posVp.y;
    posVp.y *= viewport_scale;

    return posVp;
  };

  const auto calcScaledEllipseAxis = [](const Point2 &center, const Point2 &one) {
    const Point2 axis = one - center;
    const float axisLenSq = lengthSq(axis);
    const Point2 axisDirPerLen = safediv(axis, axisLenSq);
    return axisDirPerLen;
  };

  const Point2 centerVp = worldToViewport(centerWpos);
  const Point2 rightVp = worldToViewport(rightWpos);
  const Point2 upVp = worldToViewport(upWpos);

  ViewportEllipse ve;
  ve.center = centerVp;
  ve.yScale = viewport_scale;
  ve.xAxis = calcScaledEllipseAxis(centerVp, rightVp);
  ve.yAxis = calcScaledEllipseAxis(centerVp, upVp);

  const float invDet = safediv(1.0f, fabsf(ve.xAxis.x * ve.yAxis.y - ve.xAxis.y * ve.yAxis.x));
  const float hExtX = sqrtf(pow(ve.yAxis.y, 2.0f) + pow(ve.xAxis.y, 2.0f));
  const float hExtY = sqrtf(pow(ve.yAxis.x, 2.0f) + pow(ve.xAxis.x, 2.0f));
  ve.halfExtent = invDet * Point2(hExtX, hExtY);

  return ve;
}
