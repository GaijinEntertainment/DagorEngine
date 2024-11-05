// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <math/dag_polyUtils.h>
#include <memory/dag_framemem.h>
#include <math/dag_mathUtils.h>
#include <math/dag_math2d.h>
#include <generic/dag_smallTab.h>

struct PolyPoint
{
  Point2 pt;
  int idx;
};

static Point2 rot_left(const Point2 &v) { return Point2(-v.y, v.x); }

static bool clip_poly_ear(Tab<PolyPoint> &pts, float dir, int &pt0, int &pt1, int &pt2)
{
  int sz = pts.size();
  if (sz < 3)
    return false;
  for (int i = 0; i < sz; ++i)
  {
    int i0 = i;
    int i1 = (i + 1) % sz;
    int i2 = (i + 2) % sz;

    const Point2 &p0 = pts[i0].pt;
    const Point2 &p1 = pts[i1].pt;
    const Point2 &p2 = pts[i2].pt;
    if (sign(rot_left(p1 - p0) * (p2 - p1)) != dir) // not same as dir, this is incorrect dir then
      continue;

    // construct lines which are used to detect if object lies inside this triangle or not.
    Line2 l0(p0, p1);
    Line2 l1(p1, p2);
    Line2 l2(p2, p0);
    bool needsFlip = l0.distance(p2) > 0.f;
    if (needsFlip)
    {
      l0.flip();
      l1.flip();
      l2.flip(); // flip if neccessary
    }
    // check other points if they're inside or not
    bool notEar = false;
    for (int j = 0; j < sz && !notEar; ++j)
    {
      if (pts[j].pt == p0 || pts[j].pt == p1 || pts[j].pt == p2)
        continue;
      const Point2 &tpt = pts[j].pt;
      if (l0.distance(tpt) <= 0.f && l1.distance(tpt) <= 0.f && l2.distance(tpt) <= 0.f) // this point lies inside triangle, so it's
                                                                                         // not an 'ear'
        notEar = true;
    }
    if (notEar)
      continue;
    // this is ear, so we can 'clip' it
    pt0 = pts[!needsFlip ? i0 : i2].idx;
    pt1 = pts[i1].idx;
    pt2 = pts[!needsFlip ? i2 : i0].idx;
    erase_items(pts, i1, 1);
    return true;
  }
  return false;
}

void triangulate_poly(dag::ConstSpan<Point2> points, Tab<int> &indices)
{
  int sz = points.size();
  Tab<PolyPoint> pts(framemem_ptr());
  pts.reserve(sz);
  float area = 0.f;
  for (int i = 0; i < sz; ++i)
  {
    Point2 ptCur = points[i];
    Point2 ptNext = points[(i + 1) % sz];

    if (ptCur == ptNext)
      continue;

    area += ptCur.x * ptNext.y - ptCur.y * ptNext.x;

    pts.push_back({ptCur, i});
  }
  int p0, p1, p2;
  while (clip_poly_ear(pts, sign(area), p0, p1, p2))
  {
    indices.push_back(p0);
    indices.push_back(p1);
    indices.push_back(p2);
  }
}

bool get_lines_intersection(const Line2 &ln0, const Line2 &ln1, Point2 &intPt)
{
  float linesDot = ln0.norm * ln1.norm;
  if (linesDot >= 1.f || linesDot <= -1.f)
    return false;
  Point2 st0, st1, end0, end1;
  st0 = ln0.norm * (-ln0.d);
  end0 = st0 + Point2(ln0.norm.y, -ln0.norm.x);
  st1 = ln1.norm * (-ln1.d);
  end1 = st1 + Point2(ln1.norm.y, -ln1.norm.x);
  if (get_lines_intersection(st0, end0, st1, end1, intPt))
    return true;
  G_ASSERTF(false, "Inconsistent lines intersection: lines dot: %.02d, seg1 len %.06f, seg2 len %.06f", linesDot, length(end0 - st0),
    length(end1 - st1));
  return false;
}