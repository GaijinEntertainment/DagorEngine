// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <phys/dag_fastPhys.h>
#include <debug/dag_debug3d.h>


void FastPhysRender::draw_sphere(float radius, float angle, float axis_length, E3DCOLOR color, bool is_selected)
{
  const int MAX_SEGS = 32;
  Point2 pt[MAX_SEGS];
  // spherical
  int segs = is_selected ? 32 : 16;

  for (int i = 0; i < segs; ++i)
  {
    real a = i * TWOPI / segs;
    pt[i] = Point2(cosf(a), sinf(a));
  }

  real maxAngle = DegToRad(90 - angle / 2);

  int ySegs = is_selected ? 8 : 4;

  real lastR = 0, lastY = radius;

  for (int yi = 1; yi <= ySegs; ++yi)
  {
    real a = maxAngle * yi / ySegs;
    real r = radius * sinf(a);
    real y = radius * cosf(a);

    for (int i = 0; i < segs; ++i)
    {
      const Point2 &p0 = pt[i == 0 ? segs - 1 : i - 1];
      const Point2 &p1 = pt[i];

      ::draw_cached_debug_line(Point3(p0.x * r, y, p0.y * r), Point3(p1.x * r, y, p1.y * r), color);
      ::draw_cached_debug_line(Point3(p1.x * r, y, p1.y * r), Point3(p1.x * lastR, lastY, p1.y * lastR), color);
    }

    lastR = r;
    lastY = y;
  }

  real dist = is_selected ? radius * 5 : radius;

  real r = lastR + cosf(maxAngle) * dist;
  real y = lastY - sinf(maxAngle) * dist;

  for (int i = 0; i < segs; ++i)
  {
    const Point2 &p1 = pt[i];
    ::draw_cached_debug_line(Point3(p1.x * r, y, p1.y * r), Point3(p1.x * lastR, lastY, p1.y * lastR), color);
  }

  ::draw_cached_debug_line(Point3(0, 0, 0), Point3(0, radius, 0), color);
}

void FastPhysRender::draw_cylinder(float radius, float angle, float axis_length, E3DCOLOR color, bool is_selected)
{
  const int MAX_SEGS = 8;
  Point2 pt[MAX_SEGS + 2];

  real maxAngle = DegToRad(90 - angle / 2);

  int segs = is_selected ? 8 : 4;

  for (int i = 0; i <= segs; ++i)
  {
    real a = i * maxAngle / segs;
    pt[i] = Point2(sinf(a), cosf(a)) * radius;
  }

  real dist = is_selected ? radius * 5 : radius;

  pt[segs + 1] = pt[segs] + Point2(cosf(maxAngle), -sinf(maxAngle)) * dist;

  int aSegs = is_selected ? 16 : 8;

  for (int ai = 0; ai <= aSegs; ++ai)
  {
    real x = (real(ai) / aSegs - 0.5f) * axis_length;

    for (int i = 1; i < segs + 2; ++i)
    {
      const Point2 &p0 = pt[i - 1];
      const Point2 &p1 = pt[i];
      ::draw_cached_debug_line(Point3(x, p0.y, +p0.x), Point3(x, p1.y, +p1.x), color);
      ::draw_cached_debug_line(Point3(x, p0.y, -p0.x), Point3(x, p1.y, -p1.x), color);
    }
  }

  real x0 = -0.5f * axis_length;
  real x1 = +0.5f * axis_length;

  for (int i = 0; i <= segs; ++i)
  {
    const Point2 &p1 = pt[i];
    ::draw_cached_debug_line(Point3(x0, p1.y, +p1.x), Point3(x1, p1.y, +p1.x), color);
    ::draw_cached_debug_line(Point3(x0, p1.y, -p1.x), Point3(x1, p1.y, -p1.x), color);
  }

  ::draw_cached_debug_line(Point3(0, 0, 0), Point3(0, radius, 0), color);
}
