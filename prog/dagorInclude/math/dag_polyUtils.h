//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <math/dag_Point2.h>
#include <math/dag_mathUtils.h>
#include <generic/dag_tab.h>
#include <EASTL/type_traits.h>

// Simple struct to calc 2 dimensional lines for convexes/triangles.
// Stores normal and the lentgth of the radius vector from line to the origin along the normal

struct Line2
{
  Point2 norm = Point2(1.f, 0.f);
  float d = 0.f;

  Line2(const Point2 &p0, const Point2 &p1) { set(p0, p1); }

  void set(const Point2 &p0, const Point2 &p1)
  {
    norm = normalize(Point2(p0.y - p1.y, p1.x - p0.x)); // flipped 90 degrees to the left side (-y, +x)
    d = -(p1 * norm);
  }
  float distance(const Point2 &pt) const { return pt * norm + d; }
  void flip()
  {
    norm = -norm;
    d = -d;
  }
};

void triangulate_poly(dag::ConstSpan<Point2> points, Tab<int> &indices);

bool get_lines_intersection(const Line2 &ln0, const Line2 &ln1, Point2 &intPt);

// Returns cw or ccw of poly by computing doubly signed summ of trapeze area under the vectors
template <class V, typename T = typename V::value_type>
__forceinline bool is_poly_ccw(const V &pts)
{
  static_assert(eastl::is_same<T, Point2>::value || eastl::is_same<T, Point3>::value, "Only points are acceptable");
  int xi = 0;
  int yi = eastl::is_same<T, Point2>::value ? 1 : 2;
  float area = 0.f;
  for (int c = 0; c < pts.size(); ++c)
  {
    int n = (c + 1) % pts.size();
    area += (pts[n][xi] - pts[c][xi]) * (pts[n][yi] + pts[c][yi]);
  }

  return area < 0.f;
}
