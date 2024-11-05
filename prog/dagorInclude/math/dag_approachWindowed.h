//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <math/dag_mathBase.h>
#include <math/dag_Point2.h>
#include <math/dag_Point3.h>

inline float distance_between(float a, float b) { return fabs(a - b); }

inline double distance_between(double a, double b) { return fabs(a - b); }

inline float distance_between(const Point2 &a, const Point2 &b) { return length(a - b); }

inline float distance_between(const Point3 &a, const Point3 &b) { return length(a - b); }

template <typename T>
inline T approach_windowed(T from, T to, float dt, float viscosity_internal, float half_window_size, float viscosity_external)
{
  DisablePointersInMath<T>();
  T result = from;
  float dist = distance_between(from, to);

  if (dist > half_window_size)
  {
    T to2 = lerp(to, from, half_window_size / dist);
    result = approach(result, to2, dt, viscosity_external);
  }

  return approach(result, to, dt, viscosity_internal);
}
