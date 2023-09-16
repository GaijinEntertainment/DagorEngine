//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <math/dag_mathBase.h>


// Supplementary class used for heightmap heigth determination.

// Class HM must implement following methods:
// real getHeightmapCellSize();
// Point3 getHeightmapOffset
// bool getHeightmapCell5Pt(const IPoint2 &cell, real &h0, real &hx, real &hy, real &hxy, real &hmid);

inline bool get_height_diamond(float cellSize, const Point2 &dpt, real h0, real hx, real hy, real hxy, float &ht, Point3 *normal)
{
  float hmid = (h0 + hx + hy + hxy) * 0.25f;
  float a, b;
//  hy .-------. hxy
//     | \ 2 / |
//     |  \ /  |
//     | 1 * 3 |
//     |  / \  |
//     | / 4 \ |
//  h0 .-------. hx
//
#define VC00 Point3(-0.5 * cellSize, h0 - hmid, -0.5 * cellSize)
#define VC01 Point3(-0.5 * cellSize, hy - hmid, 0.5 * cellSize)
#define VC11 Point3(0.5 * cellSize, hxy - hmid, 0.5 * cellSize)
#define VC10 Point3(0.5 * cellSize, hx - hmid, -0.5 * cellSize)
  if (dpt.x >= dpt.y)
  {
    // case 3, 4
    if (dpt.x + dpt.y >= 0)
    {
      // case 3
      a = dpt.x - dpt.y;
      b = dpt.x + dpt.y;
      ht = hmid + a * (hx - hmid) + b * (hxy - hmid);
      if (normal)
        *normal = normalize(VC11 % VC10);
    }
    else
    {
      // case 4
      a = dpt.x - dpt.y;
      b = -(dpt.x + dpt.y);
      ht = hmid + a * (hx - hmid) + b * (h0 - hmid);
      if (normal)
        *normal = normalize(VC10 % VC00);
    }
  }
  else
  {
    // case 1, 2
    if (dpt.x + dpt.y >= 0)
    {
      // case 2
      a = dpt.y - dpt.x;
      b = dpt.x + dpt.y;
      ht = hmid + a * (hy - hmid) + b * (hxy - hmid);
      if (normal)
        *normal = normalize(VC01 % VC11);
    }
    else
    {
      // case 1
      a = dpt.y - dpt.x;
      b = -(dpt.x + dpt.y);
      ht = hmid + a * (hy - hmid) + b * (h0 - hmid);
      if (normal)
        *normal = normalize(VC00 % VC01);
    }
  }
#undef VC00
#undef VC01
#undef VC11
#undef VC10

  return true;
}

template <class HM>
bool get_height_midpoint_heightmap(HM &heightmap, const Point2 &_pt, real &ht, Point3 *normal)
{
  const Point2 pt = (_pt - Point2::xz(heightmap.getHeightmapOffset())) / heightmap.getHeightmapCellSize();
  const Point2 ptf(floorf(pt.x), floorf(pt.y));

  real h0, hx, hy, hxy, hmid;

  if (!heightmap.getHeightmapCell5Pt(IPoint2(int(ptf.x), int(ptf.y)), h0, hx, hy, hxy, hmid))
    return false;

  get_height_diamond(heightmap.getHeightmapCellSize(), Point2(pt.x - ptf.x - 0.5, pt.y - ptf.y - 0.5), h0, hx, hy, hxy, ht, normal);
  return true;
}

template <class HM>
bool get_height_max_heightmap(HM &heightmap, const Point2 &_pt0, const Point2 &_pt1, float hmin, real &hmax)
{
  const Point3 orig = heightmap.getHeightmapOffset();

  const Point2 pt0 = (_pt0 - Point2(orig.x, orig.z)) / heightmap.getHeightmapCellSize();
  const Point2 ptf0(floorf(pt0.x), floorf(pt0.y));
  const IPoint2 cpt0(int(ptf0.x), int(ptf0.y));

  const Point2 pt1 = (_pt1 - Point2(orig.x, orig.z)) / heightmap.getHeightmapCellSize();
  const Point2 ptf1(floorf(pt1.x), floorf(pt1.y));
  const IPoint2 cpt1(int(ptf1.x), int(ptf1.y));

  if (!heightmap.getHeightmapCellsMaxHeight(cpt0, cpt1, hmin, hmax))
    return false;

  return true;
}