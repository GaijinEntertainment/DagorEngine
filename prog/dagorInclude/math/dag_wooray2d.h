//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <math/dag_Point2.h>
#include <math/integer/dag_IPoint2.h>
#include <math/integer/dag_IBBox2.h>
#include <util/dag_globDef.h>

class WooRay2d
{
public:
  WooRay2d(const Point2 &start, const Point2 &dir, float t, const Point2 &leaf_size, const IBBox2 &leaf_limits);

  // returns false if out of rtbox
  inline bool nextCell(double &t);
  template <class RayCallback>
  __forceinline bool nextCell(double &t, RayCallback &cb);

  template <class RayCallback>
  __forceinline bool nextCellPosX(double &t, RayCallback &cb);

  void initWoo(const DPoint2 &leafSize, const IBBox2 &leafLimits, const IPoint2 &end_cell);
  inline void initWoo(const Point2 &leafSize, const IBBox2 &leafLimits, const IPoint2 &end_cell)
  {
    initWoo(dpoint2(leafSize), leafLimits, end_cell);
  }

  inline const IPoint2 &currentCell() const { return pt; }
  const IPoint2 &getEndCell() const { return endCell; }
  int getStepX() const { return step.x; }
  int getStepY() const { return step.y; }

protected:
  DPoint2 tMax, tDelta;

  // Woo helpers
  IPoint2 pt, endCell;
  IPoint2 out, step;
};

__forceinline bool WooRay2d::nextCell(double &t)
{
  if (getEndCell() == currentCell())
    return false;
  if (tMax.x < tMax.y)
  {
    pt.x += step.x;
    t = tMax.x;
    if (pt.x == out.x)
      return false;
    tMax.x += tDelta.x;
  }
  else
  {
    pt.y += step.y;
    t = tMax.y;
    if (pt.y == out.y)
      return false;
    tMax.y += tDelta.y;
  }

  return true;
}

template <class RayCallback>
__forceinline bool WooRay2d::nextCell(double &t, RayCallback &cb)
{
  if (getEndCell() == currentCell())
    return false;
  if (tMax.x < tMax.y)
  {
    pt.x += step.x;
    cb.stepX(step.x);
    t = tMax.x;
    if (pt.x == out.x)
      return false;
    tMax.x += tDelta.x;
  }
  else
  {
    pt.y += step.y;
    cb.stepY(step.y);
    t = tMax.y;
    if (pt.y == out.y)
      return false;
    tMax.y += tDelta.y;
  }

  return true;
}

template <class RayCallback>
__forceinline bool WooRay2d::nextCellPosX(double &t, RayCallback &cb)
{
  if (getEndCell() == currentCell())
    return false;
  if (tMax.x < tMax.y)
  {
    pt.x++;
    cb.stepX(1);
    t = tMax.x;
    if (pt.x == out.x)
      return false;
    tMax.x += tDelta.x;
  }
  else
  {
    pt.y += step.y;
    cb.stepY(step.y);
    t = tMax.y;
    if (pt.y == out.y)
      return false;
    tMax.y += tDelta.y;
  }

  return true;
}

class WooRay2dInf // one should manually stop such ray, it is infinite
{
public:
  inline void nextCell(float &t)
  {
    if (tMax.x < tMax.y)
    {
      pt.x += step.x;
      t = tMax.x;
      tMax.x += tDelta.x;
    }
    else
    {
      pt.y += step.y;
      t = tMax.y;
      tMax.y += tDelta.y;
    }
  }
  void init(const Point2 &p, const Point2 &wdir, const Point2 &leafSize)
  {
    pt = IPoint2(floorf(p.x / leafSize.x), floorf(p.y / leafSize.y));
    step = IPoint2(wdir.x >= 0.0f ? 1 : -1, wdir.y >= 0.0f ? 1 : -1);
    const IPoint2 nextPt = pt + IPoint2(max(0, step.x), max(0, step.y));
    Point2 absDir(fabsf(wdir.x), fabsf(wdir.y));
    tDelta = Point2(absDir.x > 1e-6f ? leafSize.x / absDir.x : 0, absDir.y > 1e-6f ? leafSize.y / absDir.y : 0);

    // this calculations should be made in doubles for precision
    tMax = Point2(absDir.x > 1e-6f ? (nextPt.x * double(leafSize.x) - double(p.x)) / double(wdir.x) : VERY_BIG_NUMBER,
      absDir.y > 1e-6f ? (nextPt.y * double(leafSize.y) - double(p.y)) / double(wdir.y) : VERY_BIG_NUMBER);
  }

  inline const IPoint2 &currentCell() const { return pt; }
  int getStepX() const { return step.x; }
  int getStepY() const { return step.y; }

protected:
  Point2 tMax, tDelta;
  IPoint2 pt, step;
};
