//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <math/dag_bezier.h>

//! bezier spline segment with accurate s->t conversion
template <class P>
class BezierSplinePrecInt : public BezierSplineInt<P>
{
public:
  float getTFromS(float s) const
  {
    float t = ((BezierSplineInt<P>::s2T[2] * s + BezierSplineInt<P>::s2T[1]) * s + BezierSplineInt<P>::s2T[0]) * s;

    if (t < 0.0f)
      t = 0.0f;
    else if (t > 1.0f)
      t = 1.0f;

    float startT = 0.0f, endT = 1.0f;
    float ts = BezierSplineInt<P>::getLength(t), delta = s - ts;

    while (fabsf(delta) > 0.02)
    {
      if (delta > 0)
      {
        startT = t;
        t = (t + endT) * 0.5f;
      }
      else
      {
        endT = t;
        t = (startT + t) * 0.5f;
      }

      if ((endT - startT) < 1e-7)
        return t;

      ts = BezierSplineInt<P>::getLength(t);
      delta = s - ts;
    }

    return t;
  }
};


// common spline segment types
typedef BezierSplinePrecInt<Point3> BezierSplinePrecInt3d;
typedef BezierSplinePrecInt<Point2> BezierSplinePrecInt2d;

// common spline classes
typedef BezierSpline<Point3, BezierSplinePrecInt3d> BezierSplinePrec3d;
typedef BezierSpline<Point2, BezierSplinePrecInt2d> BezierSplinePrec2d;


//! reinterpret BezierSpline as BezierSplinePrec
template <class P>
inline BezierSpline<P, BezierSplinePrecInt<P>> &toPrecSpline(BezierSpline<P, BezierSplineInt<P>> &s)
{
  return reinterpret_cast<BezierSpline<P, BezierSplinePrecInt<P>> &>(s);
}
template <class P>
inline const BezierSpline<P, BezierSplinePrecInt<P>> &toPrecSplineConst(const BezierSpline<P, BezierSplineInt<P>> &s)
{
  return reinterpret_cast<const BezierSpline<P, BezierSplinePrecInt<P>> &>(s);
}

//! reinterpret BezierSplinePrec as BezierSpline
template <class P>
inline BezierSpline<P, BezierSplineInt<P>> &toSpline(BezierSpline<P, BezierSplinePrecInt<P>> &s)
{
  return reinterpret_cast<BezierSpline<P, BezierSplineInt<P>> &>(s);
}
template <class P>
inline const BezierSpline<P, BezierSplineInt<P>> &toSplineConst(const BezierSpline<P, BezierSplinePrecInt<P>> &s)
{
  return reinterpret_cast<const BezierSpline<P, BezierSplineInt<P>> &>(s);
}
