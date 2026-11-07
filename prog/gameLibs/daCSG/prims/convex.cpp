// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "mesh/solidUtil.h"
#include "prims/prims.h"
#include <math/dag_convexHullComputer.h>
#include <vecmath/dag_vecMath.h>
#include <float.h>

namespace dacsg
{
static bool spans_volume(dag::ConstSpan<Point3> pts)
{
  const DPoint3 p0(pts[0]);
  DPoint3 p1 = p0, p2 = p0;
  double d1 = 0, d2 = 0, d3 = 0;
  for (const Point3 &p : pts)
    if (lengthSq(DPoint3(p) - p0) > d1)
    {
      d1 = lengthSq(DPoint3(p) - p0);
      p1 = DPoint3(p);
    }
  const DPoint3 axis = unit_or_zero(p1 - p0);
  for (const Point3 &p : pts)
    if (lengthSq((DPoint3(p) - p0) % axis) > d2)
    {
      d2 = lengthSq((DPoint3(p) - p0) % axis);
      p2 = DPoint3(p);
    }
  const DPoint3 n = unit_or_zero((p1 - p0) % (p2 - p0));
  for (const Point3 &p : pts)
    d3 = eastl::max(d3, fabs((DPoint3(p) - p0) * n));
  // the float guard's floor is 8 float steps of a corner's size, and a corner is at least sqrt(d1) / 2 from the origin
  const double eps = 4 * FLT_EPSILON * sqrt(d1);
  return sqrt(d2) > eps && d3 > eps;
}

bool make_convex(const PrimArgs &a, PrimBuilder &b, eastl::string &err)
{
  const dag::ConstSpan<Point3> pts = a.g.points3(a.d.points);
  for (const Point3 &p : pts)
    if (!check_finite(p.x) || !check_finite(p.y) || !check_finite(p.z))
      return fail(err, "a point is not finite");
    else if (!(lengthSq(DPoint3(p)) < 1e36))
      return fail(err, "beyond float range at this size");
  if (pts.size() < 4 || !spans_volume(pts))
    return fail(err, "fewer than four non-coplanar points");

  dag::Vector<Point3_vec4> in(pts.begin(), pts.end());
  ConvexHullComputer hull;
  hull.compute(&in[0].x, sizeof(Point3_vec4), in.size(), 0, 0);
  if (hull.faces.size() < 4)
    return fail(err, "fewer than four non-coplanar points");

  // Hull is grid-snapped; restore exact inputs
  dag::Vector<uint32_t> vert(hull.vertices.size());
  dag::Vector<int> used(pts.size(), -1);
  for (uint32_t i = 0; i < hull.vertices.size(); ++i)
  {
    const Point3 h(v_extract_x(hull.vertices[i]), v_extract_y(hull.vertices[i]), v_extract_z(hull.vertices[i]));
    uint32_t best = 0;
    float bestD = FLT_MAX;
    for (uint32_t k = 0; k < pts.size(); ++k)
      if (lengthSq(pts[k] - h) < bestD)
      {
        bestD = lengthSq(pts[k] - h);
        best = k;
      }
    if (used[best] >= 0)
      return fail(err, "points are too close to tell apart");
    used[best] = i;
    vert[i] = b.vert(pts[best]);
  }
  dag::Vector<uint32_t> unused;
  for (uint32_t k = 0; k < pts.size(); ++k)
    if (used[k] < 0)
      unused.push_back(k);

  DPoint3 center(0, 0, 0);
  double size = 0;
  for (uint32_t v : vert)
  {
    center += DPoint3(b.m.pos[v]) / double(vert.size());
    size = eastl::max(size, double(length(b.m.pos[v])));
  }
  struct Face
  {
    dag::Vector<uint32_t> poly;
    Point3 normal;
  };
  const auto newell = [&](const dag::Vector<uint32_t> &poly) {
    DPoint3 n(0, 0, 0);
    for (uint32_t i = 0, j = poly.size() - 1; i < poly.size(); j = i++)
      n += DPoint3(b.m.pos[poly[j]]) % DPoint3(b.m.pos[poly[i]]);
    return n;
  };
  dag::Vector<Face> faces;
  for (int first : hull.faces)
  {
    dag::Vector<uint32_t> &poly = faces.push_back().poly;
    const ConvexHullComputer::Edge *start = &hull.edges[first], *e = start;
    do
    {
      poly.push_back(vert[e->getTargetVertex()]);
      e = e->getNextEdgeOfFace();
    } while (e != start);
    DPoint3 n = newell(poly), mid(0, 0, 0);
    for (uint32_t v : poly)
      mid += DPoint3(b.m.pos[v]);
    if (n * (mid / double(poly.size()) - center) < 0)
    {
      eastl::reverse(poly.begin(), poly.end());
      n *= -1;
    }
    faces.back().normal = Point3(unit_or_zero(n));
  }

  // the hull's grid snaps a point within a step of a face onto it: a tent over the faces it is beyond puts it back
  dag::Vector<uint64_t> rim;
  for (uint32_t k : unused)
  {
    const auto beyond = [&](const Face &f) {
      return DPoint3(f.normal) * (DPoint3(pts[k]) - DPoint3(b.m.pos[f.poly[0]])) > 4 * FLT_EPSILON * size;
    };
    rim.clear();
    for (const Face &f : faces)
      if (beyond(f))
        for (uint32_t i = 0, j = f.poly.size() - 1; i < f.poly.size(); j = i++)
          rim.push_back(uint64_t(f.poly[j]) << 32 | f.poly[i]);
    if (rim.empty())
      continue;
    faces.erase(eastl::remove_if(faces.begin(), faces.end(), beyond), faces.end());
    const uint32_t apex = b.vert(pts[k]);
    for (uint64_t edge : rim)
      if (eastl::find(rim.begin(), rim.end(), edge << 32 | edge >> 32) == rim.end())
      {
        Face &f = faces.push_back();
        f.poly = {uint32_t(edge >> 32), uint32_t(edge), apex};
        f.normal = Point3(unit_or_zero(newell(f.poly)));
      }
  }

  dag::Vector<uint32_t> tris;
  dag::Vector<Point2> flat;
  for (const Face &f : faces)
  {
    const Chart axes(f.normal);
    flat.clear();
    for (uint32_t v : f.poly)
      flat.push_back(Point2(b.m.pos[v] * axes.axisU, b.m.pos[v] * axes.axisV));
    if (!triangulate_polygon(make_span_const(flat), {}, tris, err))
      return false;
    b.cap(f.normal, f.poly.size(), make_span_const(tris), false, [&](uint32_t i) { return f.poly[i]; });
  }
  return true;
}
} // namespace dacsg
