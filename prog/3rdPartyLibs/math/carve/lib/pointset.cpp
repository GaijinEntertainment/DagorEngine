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
#include <carve/pointset.hpp>

namespace carve {
namespace point {

PointSet::PointSet(const std::vector<carve::geom3d::Vector>& points) {
  vertices.resize(points.size());
  for (size_t i = 0; i < points.size(); ++i) {
    vertices[i].v = points[i];
  }
  aabb.fit(points.begin(), points.end());
}

void PointSet::sortVertices(const carve::geom3d::Vector& axis) {
  std::vector<std::pair<double, size_t> > temp;
  temp.reserve(vertices.size());
  for (size_t i = 0; i < vertices.size(); ++i) {
    temp.push_back(std::make_pair(dot(axis, vertices[i].v), i));
  }
  std::sort(temp.begin(), temp.end());

  std::vector<Vertex> vnew;
  vnew.reserve(vertices.size());

  // std::vector<int> revmap;
  // revmap.resize(vertices.size());

  for (size_t i = 0; i < vertices.size(); ++i) {
    vnew.push_back(vertices[temp[i].second]);
    // revmap[temp[i].second] = i;
  }

  vertices.swap(vnew);
}
}  // namespace point
}  // namespace carve
