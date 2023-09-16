//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <util/dag_stdint.h>

enum
{
  DRAWSTAT_FRAME = 0,
  DRAWSTAT_DP,
  DRAWSTAT_LOCKVIB,
  DRAWSTAT_RT,
  DRAWSTAT_PS,
  DRAWSTAT_INS,
  DRAWSTAT_RENDERPASS_LOGICAL,

  DRAWSTAT_NUM
};

struct DrawStatSingle
{
  uint16_t val[DRAWSTAT_NUM];
  uint64_t tri;
  void clear()
  {
    for (int i = 0; i < DRAWSTAT_NUM; ++i)
      val[i] = 0;
    tri = 0;
  }
  DrawStatSingle() { clear(); }
};

inline void drawstat_diff(DrawStatSingle &a, const DrawStatSingle &b)
{
  for (int i = 0; i < DRAWSTAT_NUM; ++i)
    a.val[i] = b.val[i] - a.val[i];
  a.tri = b.tri - a.tri;
}
