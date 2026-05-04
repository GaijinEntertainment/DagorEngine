//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <memory/dag_mem.h> // memset_dw

struct PhysShapeQueryResult //-V730
{
  Point3 res, norm, vel;
  float t = 1.f;

  PhysShapeQueryResult()
  {
#if DAGOR_DBGLEVEL > 0 && defined(_M_IX86_FP) && _M_IX86_FP == 0 // fill with NaNs if compiled for IA32
    memset_dw(this, 0x7fbfffff, ((char *)&t - (char *)this) / sizeof(int));
#endif
  }
};
