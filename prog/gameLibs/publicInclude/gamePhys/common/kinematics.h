//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <math/dag_Point3.h>

namespace gamephys
{

// Extrapolating position and velocity in circular motion
void extrapolate_circular(const Point3 &pos, // current position abs
  const Point3 &vel,                         // current velocity abs
  const Point3 &acc,                         // current acceleration abs,
                     // but it is assumed that the acceleration is constant in aircraft veloicty frame reference
  float time,       // extrapolation time
  Point3 &out_pos,  // position at current time + time
  Point3 &out_vel); // velocity at current time + time

} // namespace gamephys
