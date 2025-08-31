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

#include <carve/polyline_decl.hpp>

namespace carve {
namespace line {

struct polyline_vertex_iter
    : public StdIterator<std::random_access_iterator_tag, Vertex*> {
  Polyline* base;
  ssize_t idx;

  polyline_vertex_iter(Polyline* _base) : base(_base), idx(0) {}

  polyline_vertex_iter(Polyline* _base, ssize_t _idx)
      : base(_base), idx(_idx) {}

  polyline_vertex_iter operator++(int) {
    return polyline_vertex_iter(base, idx++);
  }
  polyline_vertex_iter& operator++() {
    ++idx;
    return *this;
  }
  polyline_vertex_iter& operator+=(int v) {
    idx += v;
    return *this;
  }

  polyline_vertex_iter operator--(int) {
    return polyline_vertex_iter(base, idx--);
  }
  polyline_vertex_iter& operator--() {
    --idx;
    return *this;
  }
  polyline_vertex_iter& operator-=(int v) {
    idx -= v;
    return *this;
  }

  Vertex* operator*() const {
    CARVE_ASSERT(idx >= 0 && idx < base->vertexCount());
    return base->vertex((size_t)idx);
  }
};

static inline ssize_t operator-(const polyline_vertex_iter& a,
                                const polyline_vertex_iter& b) {
  return a.idx - b.idx;
}

static inline bool operator==(const polyline_vertex_iter& a,
                              const polyline_vertex_iter& b) {
  return a.idx == b.idx;
}
static inline bool operator!=(const polyline_vertex_iter& a,
                              const polyline_vertex_iter& b) {
  return a.idx != b.idx;
}
static inline bool operator<(const polyline_vertex_iter& a,
                             const polyline_vertex_iter& b) {
  return a.idx < b.idx;
}
static inline bool operator>(const polyline_vertex_iter& a,
                             const polyline_vertex_iter& b) {
  return a.idx > b.idx;
}
static inline bool operator<=(const polyline_vertex_iter& a,
                              const polyline_vertex_iter& b) {
  return a.idx <= b.idx;
}
static inline bool operator>=(const polyline_vertex_iter& a,
                              const polyline_vertex_iter& b) {
  return a.idx >= b.idx;
}

struct polyline_vertex_const_iter
    : public StdIterator<std::random_access_iterator_tag, Vertex*> {
  const Polyline* base;
  ssize_t idx;

  polyline_vertex_const_iter(const Polyline* _base) : base(_base), idx(0) {}

  polyline_vertex_const_iter(const Polyline* _base, ssize_t _idx)
      : base(_base), idx(_idx) {}

  polyline_vertex_const_iter operator++(int) {
    return polyline_vertex_const_iter(base, idx++);
  }
  polyline_vertex_const_iter& operator++() {
    ++idx;
    return *this;
  }
  polyline_vertex_const_iter& operator+=(int v) {
    idx += v;
    return *this;
  }

  polyline_vertex_const_iter operator--(int) {
    return polyline_vertex_const_iter(base, idx--);
  }
  polyline_vertex_const_iter& operator--() {
    --idx;
    return *this;
  }
  polyline_vertex_const_iter& operator-=(int v) {
    idx -= v;
    return *this;
  }

  const Vertex* operator*() const {
    CARVE_ASSERT(idx >= 0 && idx < base->vertexCount());
    return base->vertex((size_t)idx);
  }
};

static inline ssize_t operator-(const polyline_vertex_const_iter& a,
                                const polyline_vertex_const_iter& b) {
  return a.idx - b.idx;
}

static inline bool operator==(const polyline_vertex_const_iter& a,
                              const polyline_vertex_const_iter& b) {
  return a.idx == b.idx;
}
static inline bool operator!=(const polyline_vertex_const_iter& a,
                              const polyline_vertex_const_iter& b) {
  return a.idx != b.idx;
}
static inline bool operator<(const polyline_vertex_const_iter& a,
                             const polyline_vertex_const_iter& b) {
  return a.idx < b.idx;
}
static inline bool operator>(const polyline_vertex_const_iter& a,
                             const polyline_vertex_const_iter& b) {
  return a.idx > b.idx;
}
static inline bool operator<=(const polyline_vertex_const_iter& a,
                              const polyline_vertex_const_iter& b) {
  return a.idx <= b.idx;
}
static inline bool operator>=(const polyline_vertex_const_iter& a,
                              const polyline_vertex_const_iter& b) {
  return a.idx >= b.idx;
}

inline polyline_vertex_const_iter Polyline::vbegin() const {
  return polyline_vertex_const_iter(this, 0);
}
inline polyline_vertex_const_iter Polyline::vend() const {
  return polyline_vertex_const_iter(this, (ssize_t)vertexCount());
}
inline polyline_vertex_iter Polyline::vbegin() {
  return polyline_vertex_iter(this, 0);
}
inline polyline_vertex_iter Polyline::vend() {
  return polyline_vertex_iter(this, (ssize_t)vertexCount());
}

struct polyline_edge_iter
    : public StdIterator<std::random_access_iterator_tag, PolylineEdge*> {
  Polyline* base;
  ssize_t idx;

  polyline_edge_iter(Polyline* _base) : base(_base), idx(0) {}

  polyline_edge_iter(Polyline* _base, ssize_t _idx) : base(_base), idx(_idx) {}

  polyline_edge_iter operator++(int) { return polyline_edge_iter(base, idx++); }
  polyline_edge_iter& operator++() {
    ++idx;
    return *this;
  }
  polyline_edge_iter& operator+=(int v) {
    idx += v;
    return *this;
  }

  polyline_edge_iter operator--(int) { return polyline_edge_iter(base, idx--); }
  polyline_edge_iter& operator--() {
    --idx;
    return *this;
  }
  polyline_edge_iter& operator-=(int v) {
    idx -= v;
    return *this;
  }

  PolylineEdge* operator*() const {
    CARVE_ASSERT(idx >= 0 && idx < base->edgeCount());
    return base->edge((size_t)idx);
  }
};

static inline ssize_t operator-(const polyline_edge_iter& a,
                                const polyline_edge_iter& b) {
  return a.idx - b.idx;
}

static inline bool operator==(const polyline_edge_iter& a,
                              const polyline_edge_iter& b) {
  return a.idx == b.idx;
}
static inline bool operator!=(const polyline_edge_iter& a,
                              const polyline_edge_iter& b) {
  return a.idx != b.idx;
}
static inline bool operator<(const polyline_edge_iter& a,
                             const polyline_edge_iter& b) {
  return a.idx < b.idx;
}
static inline bool operator>(const polyline_edge_iter& a,
                             const polyline_edge_iter& b) {
  return a.idx > b.idx;
}
static inline bool operator<=(const polyline_edge_iter& a,
                              const polyline_edge_iter& b) {
  return a.idx <= b.idx;
}
static inline bool operator>=(const polyline_edge_iter& a,
                              const polyline_edge_iter& b) {
  return a.idx >= b.idx;
}

struct polyline_edge_const_iter
    : public StdIterator<std::random_access_iterator_tag, PolylineEdge*> {
  const Polyline* base;
  ssize_t idx;

  polyline_edge_const_iter(const Polyline* _base) : base(_base), idx(0) {}

  polyline_edge_const_iter(const Polyline* _base, ssize_t _idx)
      : base(_base), idx(_idx) {}

  polyline_edge_const_iter operator++(int) {
    return polyline_edge_const_iter(base, idx++);
  }
  polyline_edge_const_iter& operator++() {
    ++idx;
    return *this;
  }
  polyline_edge_const_iter& operator+=(int v) {
    idx += v;
    return *this;
  }

  polyline_edge_const_iter operator--(int) {
    return polyline_edge_const_iter(base, idx--);
  }
  polyline_edge_const_iter& operator--() {
    --idx;
    return *this;
  }
  polyline_edge_const_iter& operator-=(int v) {
    idx -= v;
    return *this;
  }

  const PolylineEdge* operator*() const {
    CARVE_ASSERT(idx >= 0 && idx < base->edgeCount());
    return base->edge((size_t)idx);
  }
};

static inline ssize_t operator-(const polyline_edge_const_iter& a,
                                const polyline_edge_const_iter& b) {
  return a.idx - b.idx;
}

static inline bool operator==(const polyline_edge_const_iter& a,
                              const polyline_edge_const_iter& b) {
  return a.idx == b.idx;
}
static inline bool operator!=(const polyline_edge_const_iter& a,
                              const polyline_edge_const_iter& b) {
  return a.idx != b.idx;
}
static inline bool operator<(const polyline_edge_const_iter& a,
                             const polyline_edge_const_iter& b) {
  return a.idx < b.idx;
}
static inline bool operator>(const polyline_edge_const_iter& a,
                             const polyline_edge_const_iter& b) {
  return a.idx > b.idx;
}
static inline bool operator<=(const polyline_edge_const_iter& a,
                              const polyline_edge_const_iter& b) {
  return a.idx <= b.idx;
}
static inline bool operator>=(const polyline_edge_const_iter& a,
                              const polyline_edge_const_iter& b) {
  return a.idx >= b.idx;
}

inline polyline_edge_const_iter Polyline::ebegin() const {
  return polyline_edge_const_iter(this, 0);
}
inline polyline_edge_const_iter Polyline::eend() const {
  return polyline_edge_const_iter(this, (ssize_t)edgeCount());
}
inline polyline_edge_iter Polyline::ebegin() {
  return polyline_edge_iter(this, 0);
}
inline polyline_edge_iter Polyline::eend() {
  return polyline_edge_iter(this, (ssize_t)edgeCount());
}
}  // namespace line
}  // namespace carve
