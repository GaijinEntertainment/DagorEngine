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

#include <carve/polyhedron_base.hpp>

namespace carve {
namespace csg {
namespace detail {
typedef std::map<
    carve::mesh::MeshSet<3>::vertex_t*,
    std::set<std::pair<carve::mesh::MeshSet<3>::face_t*, double> > >
    EdgeIntInfo;

typedef std::unordered_set<carve::mesh::MeshSet<3>::vertex_t*> VSet;
typedef std::unordered_set<carve::mesh::MeshSet<3>::face_t*> FSet;

typedef std::set<carve::mesh::MeshSet<3>::vertex_t*> VSetSmall;
typedef std::set<csg::V2> V2SetSmall;
typedef std::set<carve::mesh::MeshSet<3>::face_t*> FSetSmall;

typedef std::unordered_map<carve::mesh::MeshSet<3>::vertex_t*, VSetSmall>
    VVSMap;
typedef std::unordered_map<carve::mesh::MeshSet<3>::edge_t*, EdgeIntInfo>
    EIntMap;
typedef std::unordered_map<carve::mesh::MeshSet<3>::face_t*, VSetSmall> FVSMap;

typedef std::unordered_map<carve::mesh::MeshSet<3>::vertex_t*, FSetSmall>
    VFSMap;
typedef std::unordered_map<carve::mesh::MeshSet<3>::face_t*, V2SetSmall>
    FV2SMap;

typedef std::unordered_map<carve::mesh::MeshSet<3>::edge_t*,
                           std::vector<carve::mesh::MeshSet<3>::vertex_t*> >
    EVVMap;

typedef std::unordered_map<carve::mesh::MeshSet<3>::vertex_t*,
                           std::vector<carve::mesh::MeshSet<3>::edge_t*> >
    VEVecMap;

class LoopEdges
    : public std::unordered_map<V2, std::list<FaceLoop*>, hash_pair> {
  typedef std::unordered_map<V2, std::list<FaceLoop*>, hash_pair> super;

 public:
  void addFaceLoop(FaceLoop* fl);
  void sortFaceLoopLists();
  void removeFaceLoop(FaceLoop* fl);
};
}  // namespace detail
}  // namespace csg
}  // namespace carve

static inline std::ostream& operator<<(std::ostream& o,
                                       const carve::csg::detail::FSet& s) {
  const char* sep = "";
  for (carve::csg::detail::FSet::const_iterator i = s.begin(); i != s.end();
       ++i) {
    o << sep << *i;
    sep = ",";
  }
  return o;
}
