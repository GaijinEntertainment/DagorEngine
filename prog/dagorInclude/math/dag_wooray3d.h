//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <math/dag_Point3.h>
#include <math/integer/dag_IPoint3.h>
#include <math/integer/dag_IBBox3.h>

class WooRay3d
{
public:
  Point3 p, wdir;
  // real hitU, hitV;///can be used for thread safe usage
  WooRay3d(const Point3 &start, const Point3 &dir, const Point3 &leafSize) { initWoo(start, dir, leafSize); }
  inline float nextCell(); // return false if out of rtbox
  void initWoo(const Point3 &start, const Point3 &dir, const Point3 &leafSize);
  inline IPoint3 currentCell() const { return pt; }

  // Woo  helpers
protected:
  IPoint3 pt, step;
  Point3 tMax, tDelta;
};

inline float WooRay3d::nextCell()
{
  float t;
  if (tMax.x < tMax.y)
  {
    if (tMax.x < tMax.z)
    {
      pt.x += step.x;
      t = tMax.x;
      tMax.x += tDelta.x;
    }
    else
    {
      pt.z += step.z;
      t = tMax.z;
      tMax.z += tDelta.z;
    }
  }
  else
  {
    if (tMax.y < tMax.z)
    {
      pt.y += step.y;
      t = tMax.y;
      tMax.y += tDelta.y;
    }
    else
    {
      pt.z += step.z;
      t = tMax.z;
      tMax.z += tDelta.z;
    }
  }
  return t;
}
