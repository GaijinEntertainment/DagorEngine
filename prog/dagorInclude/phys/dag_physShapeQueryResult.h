//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

struct PhysShapeQueryResult
{
  Point3 res, norm, vel;
  float t;

  PhysShapeQueryResult() : t(1.f)
  {
#if DAGOR_DBGLEVEL > 0 && defined(_M_IX86_FP) && _M_IX86_FP == 0 // fill with NaNs if compiled for IA32
    for (int *iptr = (int *)(char *)&res; iptr < (int *)&t; ++iptr)
      *iptr = 0x7fbfffff;
#endif
  }
};
