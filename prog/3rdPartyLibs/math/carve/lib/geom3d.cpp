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

#include <carve/geom3d.hpp>
#include <carve/math.hpp>

#include <algorithm>

namespace carve {
namespace geom3d {

namespace {
int is_same(const std::vector<const Vector*>& a,
            const std::vector<const Vector*>& b) {
  if (a.size() != b.size()) {
    return false;
  }

  const size_t S = a.size();
  size_t i, j, p;

  for (p = 0; p < S; ++p) {
    if (a[0] == b[p]) {
      break;
    }
  }
  if (p == S) {
    return 0;
  }

  for (i = 1, j = p + 1; j < S; ++i, ++j) {
    if (a[i] != b[j]) {
      goto not_fwd;
    }
  }
  for (j = 0; i < S; ++i, ++j) {
    if (a[i] != b[j]) {
      goto not_fwd;
    }
  }
  return +1;

not_fwd:
  for (i = 1, j = p - 1; j != (size_t)-1; ++i, --j) {
    if (a[i] != b[j]) {
      goto not_rev;
    }
  }
  for (j = S - 1; i < S; ++i, --j) {
    if (a[i] != b[j]) {
      goto not_rev;
    }
  }
  return -1;

not_rev:
  return 0;
}
}  // namespace

bool planeIntersection(const Plane& a, const Plane& b, Ray& r) {
  Vector N = cross(a.N, b.N);
  if (N.isZero()) {
    return false;
  }
  N.normalize();

  double dot_aa = dot(a.N, a.N);
  double dot_bb = dot(b.N, b.N);
  double dot_ab = dot(a.N, b.N);

  double determinant = dot_aa * dot_bb - dot_ab * dot_ab;

  double c1 = (a.d * dot_bb - b.d * dot_ab) / determinant;
  double c2 = (b.d * dot_aa - a.d * dot_ab) / determinant;

  r.D = N;
  r.v = c1 * a.N + c2 * b.N;

  return true;
}

IntersectionClass rayPlaneIntersection(const Plane& p, const Vector& v1,
                                       const Vector& v2, Vector& v, double& t) {
  Vector Rd = v2 - v1;
  double Vd = dot(p.N, Rd);
  double V0 = dot(p.N, v1) + p.d;

  if (carve::math::ZERO(Vd)) {
    if (carve::math::ZERO(V0)) {
      return INTERSECT_BAD;
    } else {
      return INTERSECT_NONE;
    }
  }

  t = -V0 / Vd;
  v = v1 + t * Rd;
  return INTERSECT_PLANE;
}

IntersectionClass lineSegmentPlaneIntersection(const Plane& p,
                                               const LineSegment& line,
                                               Vector& v) {
  double t;
  IntersectionClass r = rayPlaneIntersection(p, line.v1, line.v2, v, t);

  if (r <= 0) {
    return r;
  }

  if ((t < 0.0 && !equal(v, line.v1)) || (t > 1.0 && !equal(v, line.v2))) {
    return INTERSECT_NONE;
  }

  return INTERSECT_PLANE;
}

RayIntersectionClass rayRayIntersection(const Ray& r1, const Ray& r2,
                                        Vector& v1, Vector& v2, double& mu1,
                                        double& mu2) {
  if (!r1.OK() || !r2.OK()) {
    return RR_DEGENERATE;
  }

  Vector v_13 = r1.v - r2.v;

  double d1343 = dot(v_13, r2.D);
  double d4321 = dot(r2.D, r1.D);
  double d1321 = dot(v_13, r1.D);
  double d4343 = dot(r2.D, r2.D);
  double d2121 = dot(r1.D, r1.D);

  double numer = d1343 * d4321 - d1321 * d4343;
  double denom = d2121 * d4343 - d4321 * d4321;

  // dc - eb
  // -------
  // ab - cc

  // dc/eb - 1
  // ---------
  // a/e - cc/eb

  // dc/b - e
  // --------
  // a - cc/b

  // d/b - e/c
  // ---------
  // a/c - c/b

  if (fabs(denom) * double(1 << 10) <= fabs(numer)) {
    return RR_PARALLEL;
  }

  mu1 = numer / denom;
  mu2 = (d1343 + d4321 * mu1) / d4343;

  v1 = r1.v + mu1 * r1.D;
  v2 = r2.v + mu2 * r2.D;

  return (equal(v1, v2)) ? RR_INTERSECTION : RR_NO_INTERSECTION;
}
}  // namespace geom3d
}  // namespace carve
