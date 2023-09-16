//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <math/dag_Point2.h>
#include <generic/dag_smallTab.h>

class DataBlock;
class CarEngineModel;

// Polyline (x-ordered array of Point2) interpolator
class PolyLineInt
{
public:
  PolyLineInt() {}
  ~PolyLineInt() {}

  //! load points from blk
  void load(const DataBlock &blk);

  //! get y for f(x)
  float getValue(float x) const
  {
    if (!p.size())
      return 0.0;

    int rp = findRightPoint(x);

    if (rp == 0)
      return p[0].y;
    else if (rp == p.size())
      return p.back().y;

    float dx = p[rp].x - p[rp - 1].x;
    float dy = p[rp].y - p[rp - 1].y;
    return (x - p[rp - 1].x) / dx * dy + p[rp - 1].y;
  }


protected:
  SmallTab<Point2, MidmemAlloc> p;
  friend class CarEngineModel;

  int findRightPoint(float x) const;
};
