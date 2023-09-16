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

#include <carve/aabb.hpp>
#include <carve/matrix.hpp>
#include <carve/vector.hpp>

#include <limits>

namespace carve {
namespace rescale {

template <typename T>
T calc_scale(T max) {
  const int radix = std::numeric_limits<T>::radix;

  T div = T(1);
  T m = fabs(max);
  while (div < m) {
    div *= radix;
  }
  m *= radix;
  while (div > m) {
    div /= radix;
  }
  return div;
}

template <typename T>
T calc_delta(T min, T max) {
  const int radix = std::numeric_limits<T>::radix;

  if (min >= T(0) || max <= T(0)) {
    bool neg = false;
    if (max <= T(0)) {
      min = -min;
      max = -max;
      std::swap(min, max);
      neg = true;
    }
    T t = T(1);
    while (t > max) {
      t /= radix;
    }
    while (t <= max / radix) {
      t *= radix;
    }
    volatile T temp = t + min;
    temp -= t;
    if (neg) {
      temp = -temp;
    }
    return temp;
  } else {
    return T(0);
  }
}

struct rescale {
  double dx, dy, dz, scale;

  void init(double minx, double miny, double minz, double maxx, double maxy,
            double maxz) {
    dx = calc_delta(minx, maxx);
    minx -= dx;
    maxx -= dx;
    dy = calc_delta(miny, maxy);
    miny -= dy;
    maxy -= dy;
    dz = calc_delta(minz, maxz);
    minz -= dz;
    maxz -= dz;
    scale = calc_scale(std::max(std::max(fabs(minz), fabs(maxz)),
                                std::max(std::max(fabs(minx), fabs(maxx)),
                                         std::max(fabs(miny), fabs(maxy)))));
  }

  rescale(double minx, double miny, double minz, double maxx, double maxy,
          double maxz) {
    init(minx, miny, minz, maxx, maxy, maxz);
  }
  rescale(const carve::geom3d::Vector& min, const carve::geom3d::Vector& max) {
    init(min.x, min.y, min.z, max.x, max.y, max.z);
  }
};

struct fwd {
  rescale r;
  fwd(const rescale& _r) : r(_r) {}
  carve::geom3d::Vector operator()(const carve::geom3d::Vector& v) const {
    return carve::geom::VECTOR((v.x - r.dx) / r.scale, (v.y - r.dy) / r.scale,
                               (v.z - r.dz) / r.scale);
  }
};

struct rev {
  rescale r;
  rev(const rescale& _r) : r(_r) {}
  carve::geom3d::Vector operator()(const carve::geom3d::Vector& v) const {
    return carve::geom::VECTOR((v.x * r.scale) + r.dx, (v.y * r.scale) + r.dy,
                               (v.z * r.scale) + r.dz);
  }
};
}  // namespace rescale
}  // namespace carve
