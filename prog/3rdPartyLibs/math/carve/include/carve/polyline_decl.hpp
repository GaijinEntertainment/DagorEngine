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
namespace line {

struct PolylineEdge;
struct Polyline;
struct polyline_vertex_const_iter;
struct polyline_vertex_iter;
struct polyline_edge_const_iter;
struct polyline_edge_iter;

struct Vertex : public tagable {
  carve::geom3d::Vector v;
  std::list<std::pair<PolylineEdge*, PolylineEdge*> > edge_pairs;

  void addEdgePair(PolylineEdge* in, PolylineEdge* out) {
    edge_pairs.push_back(std::make_pair(in, out));
  }
};

struct vec_adapt_vertex_ptr {
  const carve::geom3d::Vector& operator()(const Vertex* const& v) {
    return v->v;
  }
  carve::geom3d::Vector& operator()(Vertex*& v) { return v->v; }
};

struct PolylineEdge : public tagable {
  Polyline* parent;
  size_t edgenum;
  Vertex *v1, *v2;

  PolylineEdge(Polyline* _parent, size_t _edgenum, Vertex* _v1, Vertex* _v2);

  carve::geom3d::AABB aabb() const;

  inline PolylineEdge* prevEdge() const;
  inline PolylineEdge* nextEdge() const;
};

struct Polyline {
  bool closed;
  std::vector<PolylineEdge*> edges;

  Polyline();

  size_t vertexCount() const;

  size_t edgeCount() const;

  const PolylineEdge* edge(size_t e) const;

  PolylineEdge* edge(size_t e);

  const Vertex* vertex(size_t v) const;

  Vertex* vertex(size_t v);

  bool isClosed() const;

  polyline_vertex_const_iter vbegin() const;
  polyline_vertex_const_iter vend() const;
  polyline_vertex_iter vbegin();
  polyline_vertex_iter vend();

  polyline_edge_const_iter ebegin() const;
  polyline_edge_const_iter eend() const;
  polyline_edge_iter ebegin();
  polyline_edge_iter eend();

  carve::geom3d::AABB aabb() const;

  template <typename iter_t>
  void _init(bool c, iter_t begin, iter_t end, std::vector<Vertex>& vertices);

  template <typename iter_t>
  void _init(bool closed, iter_t begin, iter_t end,
             std::vector<Vertex>& vertices, std::forward_iterator_tag);

  template <typename iter_t>
  void _init(bool closed, iter_t begin, iter_t end,
             std::vector<Vertex>& vertices, std::random_access_iterator_tag);

  template <typename iter_t>
  Polyline(bool closed, iter_t begin, iter_t end,
           std::vector<Vertex>& vertices);

  ~Polyline() {
    for (size_t i = 0; i < edges.size(); ++i) {
      delete edges[i];
    }
  }
};

struct PolylineSet {
  typedef std::list<Polyline*> line_list;
  typedef line_list::iterator line_iter;
  typedef line_list::const_iterator const_line_iter;

  std::vector<Vertex> vertices;
  line_list lines;
  carve::geom3d::AABB aabb;

  PolylineSet(const std::vector<carve::geom3d::Vector>& points);
  PolylineSet() {}
  ~PolylineSet() {
    for (line_iter i = lines.begin(); i != lines.end(); ++i) {
      delete *i;
    }
  }

  template <typename iter_t>
  void addPolyline(bool closed, iter_t begin, iter_t end);

  void sortVertices(const carve::geom3d::Vector& axis);

  size_t vertexToIndex_fast(const Vertex* v) const;
};
}  // namespace line
}  // namespace carve
