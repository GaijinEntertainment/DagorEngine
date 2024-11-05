// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <math/dag_math3d.h>

// http://softsurfer.com/Archive/algorithm_0105/algorithm_0105.htm
static inline bool isPointInTriangleUV(const Point3 &p1, const Point3 &tv0, const Point3 &u, const Point3 &v, float eps)
{
#define flt_t double
  // #define flt_t float

  if (0)
  {
    // straightforward solution (slow)
    Point3 n = normalize(u % v);
    Point3 u_ = n % u;
    Point3 v_ = n % v;
    flt_t v_u_ = v * u_;
    flt_t u_v_ = u * v_;

    // check degenerate triangle
    if (fabs(u_v_) < 1e-10 || fabs(v_u_) < 1e-10)
      return false;

    Point3 w = p1 - tv0;
    flt_t w_u_ = w * u_;
    flt_t w_v_ = w * v_;

    float s, t;

    s = w_v_ / u_v_;
    if (s < eps || s > 1.0 - eps) // p1 is outside T
      return false;
    t = w_u_ / v_u_;
    if (t < eps || (s + t) > 1.0 - eps) // p1 is outside T
      return false;

    // p1 is in T
    return true;
  }
  else
  {
    // optimized solution (no crossproducts, fast)
    flt_t uu, uv, vv, wu, wv, D;
    uu = u * u;
    uv = u * v;
    vv = v * v;
    D = uv * uv - uu * vv;

    // check degenerate triangle (sufficient for tringles edges > 0.001)
    if (fabs(D) < 1e-6)
      return false;

    Point3 w = p1 - tv0;
    wu = w * u;
    wv = w * v;

    // get and test parametric coords
    float s, t;
    s = (uv * wv - vv * wu) / D;
    if (s < eps || s > 1.0 - eps) // p1 is outside T
      return false;
    t = (uv * wu - uu * wv) / D;
    if (t < eps || (s + t) > 1.0 - eps) // p1 is outside T
      return false;

    // p1 is in T
    return true;
  }
#undef flt_t
}

static inline bool isPointInTriangle(const Point3 &p1, const Point3 &tv0, const Point3 &tv1, const Point3 &tv2, float eps = 0)
{
  return isPointInTriangleUV(p1, tv0, tv1 - tv0, tv2 - tv0, eps);
}
