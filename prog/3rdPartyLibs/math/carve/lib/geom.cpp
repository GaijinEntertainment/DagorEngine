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

#if defined(HAVE_CONFIG_H)
#include <carve_config.h>
#endif

#include <carve/geom.hpp>

namespace carve {
namespace geom {

template <>
vector<2> closestPoint(const tri<2>& tri, const vector<2>& pt) {
  return vector<2>::ZERO();
}

template <>
vector<3> closestPoint(const tri<3>& tri, const vector<3>& pt) {
  const vector<3> e0 = tri.v[1] - tri.v[0];
  const vector<3> e1 = tri.v[2] - tri.v[0];
  const vector<3> dp = tri.v[0] - pt;

  // triangle is defined by
  // T(s,t) = tri.v[0] + e0.s + e1.t; s >= 0, t >= 0, s + t <= 1
  // distance from pt to point on triangle plane:
  // Q(s, t) = a(s^2) + 2b(st) + c(t^2) + 2d(s) + 2e(t) + f
  // grad(Q) = 2(as + bt + d, bs + ct + e)

  const double a = dot(e0, e0);
  const double b = dot(e1, e0);
  const double c = dot(e1, e1);
  const double d = dot(e0, dp);
  const double e = dot(e1, dp);
  const double f = dot(dp, dp);

  const double det = a * c - b * b;

  double s = b * e - c * d;
  double t = b * d - a * e;

  /*      t             */
  /*      ^             */
  /*    \4|             */
  /*     \|             */
  /*      \             */
  /*      |\6           */
  /*     1|3\           */
  /*   ---+--\----> s   */
  /*     0|2  \5        */

  int edge;

  if (s + t <= det) {  // regions 0, 1, 2, 3
    if (s < 0 && t < 0) {
      // region 0 - minimum on edge (1) or (2)
      if (d < 0) {
        // grad(Q)(0,0).(1,0) < 0
        edge = 1;
      } else {
        edge = 2;
      }
    } else if (s < 0) {
      // region 1 - minimum on edge (1)
      edge = 1;
    } else if (t < 0) {
      // region 2 - minimum on edge (2)
      edge = 2;
    } else {
      // region 3 - closest point within triangle bounds.
      edge = 0;
    }
  } else {  // regions 4, 5, 6
    if (s < 0) {
      // region 4 - minimum on edge (1) or (3)
      if (-(c + e) < 0) {
        // grad(Q)(0,1).(0,-1) < 0
        edge = 1;
      } else {
        edge = 3;
      }
    } else if (t < 0) {
      // region 5 - minimum on edge (2) or (3)
      if (-(a + d) < 0) {
        // grad(Q)(1,0).(-1,0) < 0
        edge = 2;
      } else {
        edge = 3;
      }
    } else {
      // region 6 - minimum on edge (3)
      edge = 3;
    }
  }

  switch (edge) {
    case 0: {
      s /= det;
      t /= det;
      break;
    }
    case 1: {
      // edge (1)
      // s = 0, t = [0,1]
      // :: Q(t)  = c(t^2) + 2e(t) + f
      // :: dQ/dt = 2ct + 2e
      s = 0;
      t = math::clamp(-e / c, 0.0, 1.0);
      break;
    }
    case 2: {
      // edge (2)
      // t = 0, s = [0,1]
      // :: Q(s)  = a(s^2) + 2d(s) + f
      // :: dQ/ds = 2as + 2d
      s = math::clamp(-d / a, 0.0, 1.0);
      t = 0;
      break;
    }
    case 3: {
      // edge (3)
      // t = 1 - s, s = [0,1]
      // Q(s)  = a(s^2) + 2b(s(1-s)) + c((1-s)^2) + 2d(s) + 2e(1-s) + f
      //       = a(s^2) + 2b(s-s^2) + c(1-2s+s^2) + 2d(s) + 2e(1-s) + f
      //       = a(s^2) + 2b(s) - 2b(s^2) + c - 2c(s) + c(s^2) + 2d(s) + 2e -
      //       2e(s) + f
      //       = (a - 2b + c)(s^2) + (2b - 2c + 2d - 2e)(s) + (c + 2e + f)
      // dQ/ds = 2(a - 2b + c)s + 2(b - c + d - e)
      s = math::clamp((c + e - b - d) / (a - 2 * b + c), 0.0, 1.0);
      t = 1 - s;
      break;
    }
  }

  const vector<3> closest = tri.v[0] + e0 * s + e1 * t;

  return closest;
}
}  // namespace geom
}  // namespace carve
