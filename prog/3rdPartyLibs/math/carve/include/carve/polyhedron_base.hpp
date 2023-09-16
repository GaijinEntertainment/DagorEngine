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

#include <carve/geom3d.hpp>

#include <carve/edge_decl.hpp>
#include <carve/face_decl.hpp>
#include <carve/vertex_decl.hpp>

namespace carve {
namespace poly {

struct Object {};

template <typename array_t>
ptrdiff_t ptrToIndex_fast(const array_t& a,
                          const typename array_t::value_type* v) {
  return v - &a[0];
}

template <typename array_t>
ptrdiff_t ptrToIndex(const array_t& a, const typename array_t::value_type* v) {
  if (v < &a.front() || v > &a.back()) {
    return -1;
  }
  return v - &a[0];
}

template <unsigned ndim>
struct Geometry : public Object {
  struct Connectivity {
  } connectivity;
};

template <>
struct Geometry<2> : public Object {
  typedef Vertex<2> vertex_t;
  typedef Edge<2> edge_t;

  struct Connectivity {
    std::vector<std::vector<const edge_t*> > vertex_to_edge;
  } connectivity;

  std::vector<vertex_t> vertices;
  std::vector<edge_t> edges;

  ptrdiff_t vertexToIndex_fast(const vertex_t* v) const {
    return ptrToIndex_fast(vertices, v);
  }
  ptrdiff_t vertexToIndex(const vertex_t* v) const {
    return ptrToIndex(vertices, v);
  }

  ptrdiff_t edgeToIndex_fast(const edge_t* e) const {
    return ptrToIndex_fast(edges, e);
  }
  ptrdiff_t edgeToIndex(const edge_t* e) const { return ptrToIndex(edges, e); }

  // *** connectivity queries

  template <typename T>
  int vertexToEdges(const vertex_t* v, T result) const;
};

template <>
struct Geometry<3> : public Object {
  typedef Vertex<3> vertex_t;
  typedef Edge<3> edge_t;
  typedef Face<3> face_t;

  struct Connectivity {
    std::vector<std::vector<const edge_t*> > vertex_to_edge;
    std::vector<std::vector<const face_t*> > vertex_to_face;
    std::vector<std::vector<const face_t*> > edge_to_face;
  } connectivity;

  std::vector<vertex_t> vertices;
  std::vector<edge_t> edges;
  std::vector<face_t> faces;

  ptrdiff_t vertexToIndex_fast(const vertex_t* v) const {
    return ptrToIndex_fast(vertices, v);
  }
  ptrdiff_t vertexToIndex(const vertex_t* v) const {
    return ptrToIndex(vertices, v);
  }

  ptrdiff_t edgeToIndex_fast(const edge_t* e) const {
    return ptrToIndex_fast(edges, e);
  }
  ptrdiff_t edgeToIndex(const edge_t* e) const { return ptrToIndex(edges, e); }

  ptrdiff_t faceToIndex_fast(const face_t* f) const {
    return ptrToIndex_fast(faces, f);
  }
  ptrdiff_t faceToIndex(const face_t* f) const { return ptrToIndex(faces, f); }

  template <typename order_t>
  bool orderVertices(order_t order);

  bool orderVertices() {
    return orderVertices(std::less<vertex_t::vector_t>());
  }

  // *** connectivity queries

  const face_t* connectedFace(const face_t*, const edge_t*) const;

  template <typename T>
  int _faceNeighbourhood(const face_t* f, int depth, T* result) const;

  template <typename T>
  int faceNeighbourhood(const face_t* f, int depth, T result) const;

  template <typename T>
  int faceNeighbourhood(const edge_t* e, int m_id, int depth, T result) const;

  template <typename T>
  int faceNeighbourhood(const vertex_t* v, int m_id, int depth, T result) const;

  template <typename T>
  int vertexToEdges(const vertex_t* v, T result) const;

  template <typename T>
  int edgeToFaces(const edge_t* e, T result) const;

  template <typename T>
  int vertexToFaces(const vertex_t* v, T result) const;
};
}  // namespace poly
}  // namespace carve
