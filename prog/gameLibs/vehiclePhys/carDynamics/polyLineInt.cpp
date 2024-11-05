// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <vehiclePhys/carDynamics/polyLineInt.h>
#include <ioSys/dag_dataBlock.h>
#include <generic/dag_span.h>

void PolyLineInt::load(const DataBlock &blk)
{
  clear_and_resize(p, blk.paramCount());
  for (int i = 0; i < p.size(); i++)
    p[i] = blk.getPoint2(i);
}

int PolyLineInt::findRightPoint(float x) const
{
  for (int i = 0; i < p.size(); i++)
    if (p[i].x > x)
      return i;
  return p.size();
}
