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
#include <carve/geom2d.hpp>
#include <carve/geom3d.hpp>
#include <carve/matrix.hpp>
#include <carve/tag.hpp>
#include <carve/vector.hpp>

#include <list>
#include <map>
#include <vector>

namespace carve {
namespace poly {

struct Object;

template <unsigned ndim>
class Vertex : public tagable {
 public:
  typedef carve::geom::vector<ndim> vector_t;
  typedef Object obj_t;

  vector_t v;
  obj_t* owner;

  Vertex() : tagable(), v() {}

  ~Vertex() {}

  Vertex(const vector_t& _v) : tagable(), v(_v) {}
};

struct hash_vertex_ptr {
  template <unsigned ndim>
  size_t operator()(const Vertex<ndim>* const& v) const {
    return (size_t)v;
  }

  template <unsigned ndim>
  size_t operator()(
      const std::pair<const Vertex<ndim>*, const Vertex<ndim>*>& v) const {
    size_t r = (size_t)v.first;
    size_t s = (size_t)v.second;
    return r ^ ((s >> 16) | (s << 16));
  }
};

template <unsigned ndim>
double distance(const Vertex<ndim>* v1, const Vertex<ndim>* v2) {
  return distance(v1->v, v2->v);
}

template <unsigned ndim>
double distance(const Vertex<ndim>& v1, const Vertex<ndim>& v2) {
  return distance(v1.v, v2.v);
}

struct vec_adapt_vertex_ref {
  template <unsigned ndim>
  const typename Vertex<ndim>::vector_t& operator()(
      const Vertex<ndim>& v) const {
    return v.v;
  }

  template <unsigned ndim>
  typename Vertex<ndim>::vector_t& operator()(Vertex<ndim>& v) const {
    return v.v;
  }
};

struct vec_adapt_vertex_ptr {
  template <unsigned ndim>
  const typename Vertex<ndim>::vector_t& operator()(
      const Vertex<ndim>* v) const {
    return v->v;
  }

  template <unsigned ndim>
  typename Vertex<ndim>::vector_t& operator()(Vertex<ndim>* v) const {
    return v->v;
  }
};
}  // namespace poly
}  // namespace carve
