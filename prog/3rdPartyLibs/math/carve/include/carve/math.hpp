// Copyright 2006-2015 Tobias Sargeant (tobias.sargeant@gmail.com).
//
// This file is part of the Carve CSG Library (http://carve-csg.com/)
//
// Permission is hereby granted, free of charge, to any person
// obtaining a copy of this software and associated documentation
// files (the "Software"), to deal in the Software without
// restriction, including without limitation the rights to use, copy,
// modify, merge, publish, distribute, sublicense, and/or sell copies
// of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
// BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
// ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#pragma once

#include <carve/carve.hpp>

#include <carve/math_constants.hpp>

#include <math.h>

namespace carve {
namespace geom {
template <unsigned ndim>
struct vector;
}
}

namespace carve {
namespace math {
struct Matrix3;
int cubic_roots(double c3, double c2, double c1, double c0, double* roots);

void eigSolveSymmetric(const Matrix3& m, double& l1, carve::geom::vector<3>& e1,
                       double& l2, carve::geom::vector<3>& e2, double& l3,
                       carve::geom::vector<3>& e3);

void eigSolve(const Matrix3& m, double& l1, double& l2, double& l3);

static inline bool ZERO(double x) {
  return fabs(x) < carve::EPSILON;
}

static inline double radians(double deg) {
  return deg * M_PI / 180.0;
}
static inline double degrees(double rad) {
  return rad * 180.0 / M_PI;
}

static inline double ANG(double x) {
  return (x < 0) ? x + M_TWOPI : x;
}

template <typename T>
static inline const T& clamp(const T& val, const T& min, const T& max) {
  if (val < min) {
    return min;
  }
  if (val > max) {
    return max;
  }
  return val;
}
}  // namespace math
}  // namespace carve
