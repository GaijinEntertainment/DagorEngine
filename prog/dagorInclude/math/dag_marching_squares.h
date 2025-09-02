//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <math/dag_Point2.h>

// returns number of segments written to segments
// inputs: (x0,y0) (x1,y1) quad with heights
// center - is center height (to resolve ambiguos cases. can be as simple as 0.25*(ht00 + ht10 + ht10 + ht11)
// iso_level - is where we want to create isoline
static inline int march_square(float x0, float y0, float x1, float y1, float ht00, float ht10, float ht01, float ht11, float center,
  float iso_level, Point2 segments[4])
{
  const uint32_t code = uint32_t(ht00 >= iso_level) | (uint32_t(ht10 >= iso_level) << 1) | (uint32_t(ht11 >= iso_level) << 2) |
                        (uint32_t(ht01 >= iso_level) << 3);

  if (uint32_t(code - 1u) >= 14u) // 0,15 cases - no segments added
    return 0;
  // Compute interpolated points on edges
  float d = ht01 - ht00;
  Point2 left_pt = {x0, y0 + (y1 - y0) * (fabsf(d) < 1e-6f ? 0.5f : clamp((iso_level - ht00) / d, 0.f, 1.f))};

  d = ht10 - ht00;
  Point2 bottom_pt = {x0 + (x1 - x0) * (fabsf(d) < 1e-6f ? 0.5f : clamp((iso_level - ht00) / d, 0.f, 1.f)), y0};

  d = ht11 - ht10;
  Point2 right_pt = {x1, y0 + (y1 - y0) * (fabsf(d) < 1e-6f ? 0.5f : clamp((iso_level - ht10) / d, 0.f, 1.f))};

  d = ht11 - ht01;
  Point2 top_pt = {x0 + (x1 - x0) * (fabsf(d) < 1e-6f ? 0.5f : clamp((iso_level - ht01) / d, 0.f, 1.f)), y1};

  switch (code)
  {
    case 1:
    case 14:
      segments[0] = bottom_pt;
      segments[1] = left_pt;
      return 1;

    case 2:
    case 13:
      segments[0] = bottom_pt;
      segments[1] = right_pt;
      return 1;

    case 3:
    case 12:
      segments[0] = left_pt;
      segments[1] = right_pt;
      return 1;

    case 4:
    case 11:
      segments[0] = right_pt;
      segments[1] = top_pt;
      return 1;

    case 6:
    case 9:
      segments[0] = bottom_pt;
      segments[1] = top_pt;
      return 1;

    case 7:
    case 8:
      segments[0] = left_pt;
      segments[1] = top_pt;
      return 1;

    case 5:
      segments[0] = left_pt;
      if (center >= iso_level)
      {
        segments[1] = top_pt;
        segments[2] = bottom_pt;
        segments[3] = right_pt;
      }
      else
      {
        segments[1] = bottom_pt;
        segments[2] = right_pt;
        segments[3] = top_pt;
      }
      return 2;
    case 10:
      segments[0] = left_pt;
      if (center >= iso_level)
      {
        segments[1] = bottom_pt;
        segments[2] = right_pt;
        segments[3] = top_pt;
      }
      else
      {
        segments[1] = top_pt;
        segments[2] = bottom_pt;
        segments[3] = right_pt;
      }
      return 2;

    default: return 0;
  }
}
