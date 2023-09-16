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

static inline bool facesAreCoplanar(const carve::mesh::MeshSet<3>::face_t* a,
                                    const carve::mesh::MeshSet<3>::face_t* b) {
  carve::geom3d::Ray temp;
  // XXX: Find a better definition. This may be a source of problems
  // if floating point inaccuracies cause an incorrect answer.
  return !carve::geom3d::planeIntersection(a->plane, b->plane, temp);
}

#if defined(CARVE_DEBUG)

#include <carve/debug_hooks.hpp>

#endif

namespace carve {
namespace csg {

static inline carve::mesh::MeshSet<3>::vertex_t* map_vertex(
    const VVMap& vmap, carve::mesh::MeshSet<3>::vertex_t* v) {
  VVMap::const_iterator i = vmap.find(v);
  if (i == vmap.end()) {
    return v;
  }
  return (*i).second;
}

#if defined(CARVE_DEBUG)

class IntersectDebugHooks;
extern IntersectDebugHooks* g_debug;

#define HOOK(x)    \
  do {             \
    if (g_debug) { \
      g_debug->x   \
    }              \
  } while (0)

static inline void drawFaceLoopList(const FaceLoopList& ll, float rF, float gF,
                                    float bF, float aF, float rB, float gB,
                                    float bB, float aB, bool lit) {
  for (FaceLoop* flb = ll.head; flb; flb = flb->next) {
    const carve::mesh::MeshSet<3>::face_t* f = (flb->orig_face);
    std::vector<carve::mesh::MeshSet<3>::vertex_t*>& loop = flb->vertices;
    HOOK(drawFaceLoop2(loop, f->plane.N, rF, gF, bF, aF, rB, gB, bB, aB, true,
                       lit););
    HOOK(drawFaceLoopWireframe(loop, f->plane.N, 1, 1, 1, 0.1f););
  }
}

static inline void drawFaceLoopListWireframe(const FaceLoopList& ll) {
  for (FaceLoop* flb = ll.head; flb; flb = flb->next) {
    const carve::mesh::MeshSet<3>::face_t* f = (flb->orig_face);
    std::vector<carve::mesh::MeshSet<3>::vertex_t*>& loop = flb->vertices;
    HOOK(drawFaceLoopWireframe(loop, f->plane.N, 1, 1, 1, 0.1f););
  }
}

template <typename T>
static inline void drawEdges(T begin, T end, float rB, float gB, float bB,
                             float aB, float rE, float gE, float bE, float aE,
                             float w) {
  for (; begin != end; ++begin) {
    HOOK(drawEdge((*begin).first, (*begin).second, rB, gB, bB, aB, rE, gE, bE,
                  aE, w););
  }
}

#endif
}  // namespace csg
}  // namespace carve
