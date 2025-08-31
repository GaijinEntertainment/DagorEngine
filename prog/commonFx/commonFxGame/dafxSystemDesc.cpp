// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "dafxSystemDesc.h"

#include <math/dag_TMatrix.h>
#include <math/dag_mathUtils.h>

void dafx_ex::SystemInfo::DistanceScale::apply(TMatrix &tm, float distance)
{
  if (!enabled)
    return;

  distance = cvt(distance, begin_distance, end_distance, 0.f, 1.f);
  float scale = max(curve.sample(distance), 0.f);
  tm.col[0] *= scale;
  tm.col[1] *= scale;
  tm.col[2] *= scale;
}
