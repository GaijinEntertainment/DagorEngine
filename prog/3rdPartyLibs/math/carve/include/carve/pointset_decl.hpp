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

#include <iterator>
#include <iterator>
#include <limits>
#include <list>

#include <carve/aabb.hpp>
#include <carve/carve.hpp>
#include <carve/geom.hpp>
#include <carve/geom3d.hpp>
#include <carve/kd_node.hpp>
#include <carve/tag.hpp>

namespace carve {
namespace point {

struct Vertex : public tagable {
  carve::geom3d::Vector v;
};

struct vec_adapt_vertex_ptr {
  const carve::geom3d::Vector& operator()(const Vertex* const& v) {
    return v->v;
  }
  carve::geom3d::Vector& operator()(Vertex*& v) { return v->v; }
};

struct PointSet {
  std::vector<Vertex> vertices;
  carve::geom3d::AABB aabb;

  PointSet(const std::vector<carve::geom3d::Vector>& points);
  PointSet() {}

  void sortVertices(const carve::geom3d::Vector& axis);

  size_t vertexToIndex_fast(const Vertex* v) const;
};
}  // namespace point
}  // namespace carve
