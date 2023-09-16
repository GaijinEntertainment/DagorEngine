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

#include <algorithm>
#include <list>
#include <vector>

#include <carve/carve.hpp>

#include <carve/geom2d.hpp>

namespace carve {
namespace geom {
std::vector<int> convexHull(const std::vector<carve::geom2d::P2>& points);

template <typename project_t, typename polygon_container_t>
std::vector<int> convexHull(const project_t& project,
                            const polygon_container_t& points) {
  std::vector<carve::geom2d::P2> proj;
  proj.reserve(points.size());
  for (typename polygon_container_t::const_iterator i = points.begin();
       i != points.end(); ++i) {
    proj.push_back(project(*i));
  }
  return convexHull(proj);
}

template <typename project_t, typename iter_t>
std::vector<int> convexHull(const project_t& project, iter_t beg, iter_t end,
                            size_t size_hint = 0) {
  std::vector<carve::geom2d::P2> proj;
  if (size_hint) {
    proj.reserve(size_hint);
  }
  for (; beg != end; ++beg) {
    proj.push_back(project(*beg));
  }
  return convexHull(proj);
}
}  // namespace geom
}  // namespace carve
