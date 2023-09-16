
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

#include <carve/mesh.hpp>

namespace carve {
namespace csg {

typedef std::pair<carve::mesh::MeshSet<3>::vertex_t*,
                  carve::mesh::MeshSet<3>::vertex_t*>
    V2;

typedef std::pair<carve::mesh::MeshSet<3>::face_t*,
                  carve::mesh::MeshSet<3>::face_t*>
    F2;

static inline V2 ordered_edge(carve::mesh::MeshSet<3>::vertex_t* a,
                              carve::mesh::MeshSet<3>::vertex_t* b) {
  return V2(std::min(a, b), std::max(a, b));
}

static inline V2 flip(const V2& v) {
  return V2(v.second, v.first);
}

// include/carve/csg.hpp include/carve/faceloop.hpp
// lib/intersect.cpp lib/intersect_classify_common_impl.hpp
// lib/intersect_classify_edge.cpp
// lib/intersect_classify_group.cpp
// lib/intersect_classify_simple.cpp
// lib/intersect_face_division.cpp lib/intersect_group.cpp
// lib/intersect_half_classify_group.cpp
typedef std::unordered_set<V2, hash_pair> V2Set;

// include/carve/csg.hpp include/carve/polyhedron_decl.hpp
// lib/csg_collector.cpp lib/intersect.cpp
// lib/intersect_common.hpp lib/intersect_face_division.cpp
// lib/polyhedron.cpp
typedef std::unordered_map<carve::mesh::MeshSet<3>::vertex_t*,
                           carve::mesh::MeshSet<3>::vertex_t*>
    VVMap;
}  // namespace csg
}  // namespace carve
