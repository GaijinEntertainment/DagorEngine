//
// DaEditorX
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <math/dag_Point2.h>

struct GroundHole
{
  Point2 xAxisInv, yAxisInv, center;
  bool round;
  GroundHole(const Point2 &xAxis, const Point2 &yAxis, const Point2 &c, bool is_round) : center(c), round(is_round)
  {
    xAxisInv = xAxis / xAxis.lengthSq();
    yAxisInv = yAxis / yAxis.lengthSq();
  }

  bool check(const Point2 &pos) const
  {
    const Point2 pOfs = pos - center;
    const Point2 lenNorm = abs(Point2(pOfs * xAxisInv, pOfs * yAxisInv));
    if (lenNorm.x > 1.f || lenNorm.y > 1.f)
      return false;
    if (round && (dot(lenNorm, lenNorm) > 1.f))
      return false;
    return true;
  }
};
