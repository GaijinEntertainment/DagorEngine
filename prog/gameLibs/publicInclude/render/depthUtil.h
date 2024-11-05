//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <math/dag_Point2.h>

inline Point2 get_decode_depth(const Point2 &zn_zfar)
{
  return Point2(1.0 / zn_zfar.y, (zn_zfar.y - zn_zfar.x) / (zn_zfar.x * zn_zfar.y));
}
