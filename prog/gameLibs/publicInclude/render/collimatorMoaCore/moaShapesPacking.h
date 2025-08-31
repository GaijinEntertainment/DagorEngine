//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <math/dag_Point2.h>
#include <math/dag_Point4.h>


namespace collimator_moa
{

#define SDF_SHAPE(name, ecs_name, val) name = val,
enum class SDFShape
{
#include "shape_tags.hlsli"
  COUNT
};
#undef SDF_SHAPE

template <class T>
void pack_circle_to(T &dst, const Point2 &center, const float radius)
{
  Point4 r;
  r.x = (float)SDFShape::CIRCLE;
  r.y = center.x;
  r.z = center.y;
  r.w = radius;
  dst.push_back(r);
}

template <class T>
void pack_ring_to(T &dst, const Point2 &center, const float radius, const float width)
{
  const float radiusNear = radius;
  const float radiusFar = radius + width;

  Point4 r1, r2;
  r1.x = (float)SDFShape::RING;
  r1.y = center.x;
  r1.z = center.y;
  r1.w = radiusNear;

  r2.x = radiusFar;
  r2.y = r2.z = r2.w = 0;

  dst.push_back(r1);
  dst.push_back(r2);
}

template <class T>
void pack_line_to(T &dst, const Point2 &begin, const Point2 &end, const float half_width)
{
  Point4 r1, r2;
  r1.x = (float)SDFShape::LINE;
  r1.y = begin.x;
  r1.z = begin.y;
  r1.w = end.x;

  r2.x = end.y;
  r2.y = half_width;
  r2.z = r2.w = 0;

  dst.push_back(r1);
  dst.push_back(r2);
}

template <class T>
void pack_triangle_to(T &dst, const Point2 &a, const Point2 &b, const Point2 &c)
{
  Point4 r1, r2;
  r1.x = (float)SDFShape::TRIANGLE;
  r1.y = a.x;
  r1.z = a.y;
  r1.w = b.x;

  r2.x = b.y;
  r2.y = c.x;
  r2.z = c.y;
  r2.w = 0;

  dst.push_back(r1);
  dst.push_back(r2);
}

template <class T>
void pack_arc_to(T &dst, const Point2 &center, const float radius, const float half_width, const float begin_angle,
  const float end_angle)
{
  const float rotAngle = (end_angle + begin_angle) * 0.5f;
  float rotSin, rotCos;
  sincos(rotAngle, rotSin, rotCos);

  const float aperture = abs(end_angle - begin_angle) * 0.5f;
  float apertureSin, apertureCos;
  sincos(aperture, apertureSin, apertureCos);

  const Point2 edgePlaneN = Point2(apertureCos, -apertureSin);
  const Point2 edgePoint = Point2(apertureSin, apertureCos) * radius;

  Point4 r1, r2, r3;
  r1.x = (float)SDFShape::ARC;
  r1.y = center.x;
  r1.z = center.y;
  r1.w = radius;

  r2.x = half_width;
  r2.y = rotSin;
  r2.z = rotCos;
  r2.w = edgePlaneN.x;

  r3.x = edgePlaneN.y;
  r3.y = edgePoint.x;
  r3.z = edgePoint.y;
  r3.w = 0;

  dst.push_back(r1);
  dst.push_back(r2);
  dst.push_back(r3);
}

} // namespace collimator_moa
