//
// DaEditorX
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <generic/dag_tabSort.h>
#include <math/dag_Point3.h>
#include <math/dag_Point4.h>
#include <math/integer/dag_IPoint2.h>
#include <math/dag_bounds2.h>

struct FloatCmpLambda
{
  static inline int order(const float a, const float b) { return a < b ? -1 : (a > b ? 1 : 0); };
};

struct Ipoint2CmpLambda
{
  static inline int order(const IPoint2 &a, const IPoint2 &b) { return (a.x == b.x) ? (a.y - b.y) : a.x - b.x; }
};

// rasterize polygon on auto-adjusted bitmaps, even-odd filling rule
template <class BitMask, class Point>
float rasterize_poly(BitMask &out_bm, float &out_ofs_x, float &out_ofs_z, dag::ConstSpan<Point *> pt, float min_cell_sz = 1.0)
{
  BBox2 polyBBox;
  polyBBox.setempty();
  TabSortedInline<float, FloatCmpLambda> pos(tmpmem);
  SmallTab<Point4, TmpmemAlloc> seg;
  Point3 p0, p1;

  // compute bounds
  clear_and_resize(seg, pt.size());
  p1 = pt[0]->getPt();
  for (int i = pt.size() - 1; i >= 0; i--, p1 = p0)
  {
    p0 = pt[i]->getPt();
    polyBBox += Point2(p0.x, p0.z);
    seg[i].x = p0.x;
    seg[i].y = p0.z;
    seg[i][2] = p1.z;
    seg[i][3] = (p1.z - p0.z);
    if (float_nonzero(seg[i][3]))
      seg[i][3] = (p1.x - p0.x) / seg[i][3];
  }

  out_ofs_x = polyBBox[0].x;
  out_ofs_z = polyBBox[0].y;
  float cell = 1.0, fw = (polyBBox[1].x - polyBBox[0].x), fh = (polyBBox[1].y - polyBBox[0].y);
  if (fw >= fh)
    cell = fw / 512;
  else
    cell = fh / 512;
  if (cell < min_cell_sz)
    cell = min_cell_sz;

  int w = ceil(fw / cell);
  int h = ceil(fh / cell);

  out_bm.resize(w, h);
  pos.reserve(seg.size());
  for (int y = h - 1; y >= 0; y--)
  {
    float fy = y * cell + out_ofs_z, fx;

    // gather sorted intersections
    pos.clear();
    for (int j = 0; j < seg.size(); j++)
    {
      Point4 p = seg[j];
      if ((p.y - fy) * (p[2] - fy) > 0)
        continue;
      if (fabsf(p[2] - fy) < 1e-6) // skip segment end (to avoid duplicate scan points)
        continue;

      if (float_nonzero(p.y - fy))
        fx = p.x + (fy - p.y) * p[3] - out_ofs_x;
      else if (float_nonzero(p[2] - fy))
        fx = p.x - out_ofs_x;
      else
        continue;

      pos.insert(fx);
    }

    // rasterize bitmap using intersection points
    for (int j = 0; j < pos.size(); j += 2)
    {
      int x0 = pos[j] / cell, x1 = j + 1 < pos.size() ? pos[j + 1] / cell : w;
      if (x0 < 0)
        x0 = 0;
      if (x1 > w)
        x1 = w;
      for (; x0 < x1; x0++)
        out_bm.set(x0, y);
      if (x1 == w)
        break;
    }
  }
  return cell;
}

// rasterize polygon on pre-created bitmap, even-odd filling rule
template <class BitMask, class PointPtrTab>
void rasterize_poly_2(BitMask &bm, PointPtrTab &pt, float ofs_x, float ofs_z, float scale)
{
  TabSortedInline<float, FloatCmpLambda> pos(tmpmem);
  SmallTab<Point4, TmpmemAlloc> seg;
  Point3 p0, p1;
  int w = bm.getW(), h = bm.getH();
  float cell = 1.0f / scale;

  // compute bounds
  clear_and_resize(seg, pt.size());
  p1 = pt[0]->getPt();
  for (int i = pt.size() - 1; i >= 0; i--, p1 = p0)
  {
    p0 = pt[i]->getPt();
    seg[i].x = p0.x;
    seg[i].y = p0.z;
    seg[i][2] = p1.z;
    seg[i][3] = (p1.z - p0.z);
    if (float_nonzero(seg[i][3]))
      seg[i][3] = (p1.x - p0.x) / seg[i][3];
  }

  pos.reserve(seg.size());
  for (int y = h - 1; y >= 0; y--)
  {
    float fy = y * cell + ofs_z, fx;

    // gather sorted intersections
    pos.clear();
    for (int j = 0; j < seg.size(); j++)
    {
      Point4 p = seg[j];
      if ((p.y - fy) * (p[2] - fy) > 0)
        continue;
      if (fabsf(p[2] - fy) < 1e-6) // skip segment end (to avoid duplicate scan points)
        continue;

      if (float_nonzero(p.y - fy))
        fx = p.x + (fy - p.y) * p[3] - ofs_x;
      else if (float_nonzero(p[2] - fy))
        fx = p.x - ofs_x;
      else
        continue;

      pos.insert(fx);
    }

    // rasterize bitmap using intersection points
    for (int j = 0; j < pos.size(); j += 2)
    {
      int x0 = pos[j] * scale, x1 = j + 1 < pos.size() ? pos[j + 1] * scale : w;
      if (x0 < 0)
        x0 = 0;
      if (x1 > w)
        x1 = w;
      for (; x0 < x1; x0++)
        bm.set(x0, y);
      if (x1 == w)
        break;
    }
  }
}

// rasterize polygon on pre-created bitmap, non-zero filling rule
template <class BitMask, class PointPtrTab>
void rasterize_poly_2_nz(BitMask &bm, PointPtrTab &pt, float ofs_x, float ofs_z, float scale)
{
  TabSortedInline<IPoint2, Ipoint2CmpLambda> pos(tmpmem);
  SmallTab<Point4, TmpmemAlloc> seg;
  Point3 p0, p1;
  int w = bm.getW(), h = bm.getH();
  float cell = 1.0f / scale;

  // compute bounds
  clear_and_resize(seg, pt.size());
  p1 = pt[0]->getPt();
  for (int i = pt.size() - 1; i >= 0; i--, p1 = p0)
  {
    p0 = pt[i]->getPt();
    seg[i].x = p0.x;
    seg[i].y = p0.z;
    seg[i][2] = p1.z;
    seg[i][3] = (p1.z - p0.z);
    if (float_nonzero(seg[i][3]))
      seg[i][3] = (p1.x - p0.x) / seg[i][3];
  }

  pos.reserve(seg.size());
  for (int y = h - 1; y >= 0; y--)
  {
    float fy = y * cell + ofs_z, fx;

    // gather sorted intersections
    pos.clear();
    for (int j = 0; j < seg.size(); j++)
    {
      Point4 p = seg[j];
      if ((p.y - fy) * (p[2] - fy) > 0)
        continue;
      if (fabsf(p[2] - fy) < 1e-6) // skip segment end (to avoid duplicate scan points)
        continue;

      if (float_nonzero(p.y - fy))
        fx = p.x + (fy - p.y) * p[3] - ofs_x;
      else if (float_nonzero(p[2] - fy))
        fx = p.x - ofs_x;
      else
        continue;

      float dy = p[2] - p[1];
      pos.insert(IPoint2(fx * scale, dy > 0 ? 1 : (dy < 0 ? -1 : 0)));
    }

    // rasterize bitmap using intersection points (nonZero filling rule)
    int v0 = 0, x0 = -2000000;

    for (int j = 0; j < pos.size(); j++)
    {
      int x1 = pos[j].x, v1 = v0 + pos[j].y;
      if (!v0 && v1)
        x0 = x1;
      else if (v0 && !v1 && x1 >= 0)
      {
        if (x0 < 0)
          x0 = 0;
        if (x1 > w - 1)
          x1 = w - 1;
        for (; x0 <= x1; x0++)
          bm.set(x0, y);
        if (x1 == w - 1)
          break;
      }
      v0 = v1;
      if (x1 >= w - 1)
        break;
    }
    if (v0)
      for (; x0 < w; x0++)
        bm.set(x0, y);
  }
}
