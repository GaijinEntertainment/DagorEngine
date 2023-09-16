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

namespace carve {
namespace csg {
struct IObj {
  enum {
    OBTYPE_NONE = 0,
    OBTYPE_VERTEX = 1,
    OBTYPE_EDGE = 2,
    OBTYPE_FACE = 4
  } obtype;

  union {
    carve::mesh::MeshSet<3>::vertex_t* vertex;
    carve::mesh::MeshSet<3>::edge_t* edge;
    carve::mesh::MeshSet<3>::face_t* face;
    intptr_t val;
  };

  IObj() : obtype(OBTYPE_NONE), val(0) {}
  IObj(carve::mesh::MeshSet<3>::vertex_t* v)
      : obtype(OBTYPE_VERTEX), vertex(v) {}
  IObj(carve::mesh::MeshSet<3>::edge_t* e) : obtype(OBTYPE_EDGE), edge(e) {}
  IObj(carve::mesh::MeshSet<3>::face_t* f) : obtype(OBTYPE_FACE), face(f) {}
  char typeChar() const { return "NVExF"[obtype]; }
};

struct IObj_hash {
  inline size_t operator()(const IObj& i) const { return (size_t)i.val; }
  inline size_t operator()(const std::pair<const IObj, const IObj>& i) const {
    return (size_t)i.first.val ^ (size_t)i.second.val;
  }
};

typedef std::unordered_set<std::pair<const IObj, const IObj>, IObj_hash>
    IObjPairSet;

typedef std::unordered_map<IObj, carve::mesh::MeshSet<3>::vertex_t*, IObj_hash>
    IObjVMap;
typedef std::map<IObj, carve::mesh::MeshSet<3>::vertex_t*> IObjVMapSmall;

class VertexIntersections
    : public std::unordered_map<carve::mesh::MeshSet<3>::vertex_t*,
                                IObjPairSet> {};

static inline bool operator==(const carve::csg::IObj& a,
                              const carve::csg::IObj& b) {
  return a.obtype == b.obtype && a.val == b.val;
}

static inline bool operator!=(const carve::csg::IObj& a,
                              const carve::csg::IObj& b) {
  return a.obtype != b.obtype || a.val != b.val;
}

static inline bool operator<(const carve::csg::IObj& a,
                             const carve::csg::IObj& b) {
  return a.obtype < b.obtype || (a.obtype == b.obtype && a.val < b.val);
}

static inline bool operator<=(const carve::csg::IObj& a,
                              const carve::csg::IObj& b) {
  return a.obtype < b.obtype || (a.obtype == b.obtype && a.val <= b.val);
}

static inline bool operator>(const carve::csg::IObj& a,
                             const carve::csg::IObj& b) {
  return a.obtype > b.obtype || (a.obtype == b.obtype && a.val > b.val);
}

static inline bool operator>=(const carve::csg::IObj& a,
                              const carve::csg::IObj& b) {
  return a.obtype > b.obtype || (a.obtype == b.obtype && a.val >= b.val);
}

static inline std::ostream& operator<<(std::ostream& o,
                                       const carve::csg::IObj& a) {
  switch (a.obtype) {
    case carve::csg::IObj::OBTYPE_NONE:
      o << "NONE{}";
      break;
    case carve::csg::IObj::OBTYPE_VERTEX:
      o << "VERT{" << a.vertex << "}";
      break;
    case carve::csg::IObj::OBTYPE_EDGE:
      o << "EDGE{" << a.edge << "}";
      break;
    case carve::csg::IObj::OBTYPE_FACE:
      o << "FACE{" << a.face << "}";
      break;
  }
  return o;
}
}  // namespace csg
}  // namespace carve
