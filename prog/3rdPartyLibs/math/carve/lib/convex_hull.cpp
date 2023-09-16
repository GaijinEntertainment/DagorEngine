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

#include <carve/convex_hull.hpp>
#include <carve/csg.hpp>

#include <algorithm>

namespace {

bool grahamScan(const std::vector<carve::geom2d::P2>& points, int vpp, int vp,
                const std::vector<int>& ordered, int start,
                std::vector<int>& result, int _i = 0) {
  carve::geom2d::P2 v1 = points[vp] - points[vpp];
  if (start == (int)ordered.size()) {
    return true;
  }

  for (int i = start; i < (int)ordered.size(); ++i) {
    int v = ordered[i];
    carve::geom2d::P2 v2 = points[v] - points[vp];

    double cp = v1.x * v2.y - v2.x * v1.y;
    if (cp < 0) {
      return false;
    }

    int j = i + 1;
    while (j < (int)ordered.size() && points[ordered[j]] == points[v]) {
      j++;
    }

    result.push_back(v);
    if (grahamScan(points, vp, v, ordered, j, result, _i + 1)) {
      return true;
    }
    result.pop_back();
  }

  return false;
}
}  // namespace

namespace carve {
namespace geom {

std::vector<int> convexHull(const std::vector<carve::geom2d::P2>& points) {
  double max_x = points[0].x;
  unsigned max_v = 0;

  for (unsigned i = 1; i < points.size(); ++i) {
    if (points[i].x > max_x) {
      max_x = points[i].x;
      max_v = i;
    }
  }

  std::vector<std::pair<double, double> > angle_dist;
  std::vector<int> ordered;
  angle_dist.reserve(points.size());
  ordered.reserve(points.size() - 1);
  for (unsigned i = 0; i < points.size(); ++i) {
    if (i == max_v) {
      continue;
    }
    angle_dist[i] = std::make_pair(
        carve::math::ANG(carve::geom2d::atan2(points[i] - points[max_v])),
        distance2(points[i], points[max_v]));
    ordered.push_back(i);
  }

  std::sort(ordered.begin(), ordered.end(),
            make_index_sort(angle_dist.begin()));

  std::vector<int> result;
  result.push_back(max_v);
  result.push_back(ordered[0]);

  if (!grahamScan(points, max_v, ordered[0], ordered, 1, result)) {
    result.clear();
    throw carve::exception("convex hull failed!");
  }

  return result;
}
}  // namespace geom
}  // namespace carve
