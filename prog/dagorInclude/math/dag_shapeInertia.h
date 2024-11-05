//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

// Cylinder
// Inertia against symmetry axis
inline float calc_cylinder_axis_inertia(float rad, float mass) { return 0.5f * mass * sqr(rad); }

// Inertia orthogonal to symmetry axis
inline float calc_cylinder_orth_inertia(float rad, float length, float mass)
{
  return 0.25f * mass * sqr(rad) + mass * sqr(length) / 12.f;
}

// Box
inline Point3 calc_box_inertia(const Point3 &size)
{
  return Point3(sqr(size.y) + sqr(size.z), sqr(size.x) + sqr(size.z), sqr(size.x) + sqr(size.y)) / 12.f;
}
