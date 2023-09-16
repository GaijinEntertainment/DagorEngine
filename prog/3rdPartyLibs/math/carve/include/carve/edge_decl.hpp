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

#include <carve/tag.hpp>
#include <carve/vector.hpp>

#include <list>
#include <vector>

namespace carve {
namespace poly {

struct Object;

template <unsigned ndim>
class Vertex;

template <unsigned ndim>
class Edge : public tagable {
 public:
  typedef Vertex<ndim> vertex_t;
  typedef typename Vertex<ndim>::vector_t vector_t;
  typedef Object obj_t;

  const vertex_t *v1, *v2;
  const obj_t* owner;

  Edge(const vertex_t* _v1, const vertex_t* _v2, const obj_t* _owner)
      : tagable(), v1(_v1), v2(_v2), owner(_owner) {}

  ~Edge() {}
};

struct hash_edge_ptr {
  template <unsigned ndim>
  size_t operator()(const Edge<ndim>* const& e) const {
    return (size_t)e;
  }
};
}  // namespace poly
}  // namespace carve
