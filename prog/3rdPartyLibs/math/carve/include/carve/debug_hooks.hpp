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

#include <carve/csg.hpp>
#include <carve/geom3d.hpp>
#include <carve/vector.hpp>

#include <iomanip>

template <typename MAP>
void map_histogram(std::ostream& out, const MAP& map) {
  std::vector<int> hist;
  for (typename MAP::const_iterator i = map.begin(); i != map.end(); ++i) {
    size_t n = (*i).second.size();
    if (hist.size() <= n) {
      hist.resize(n + 1);
    }
    hist[n]++;
  }
  int total = (int)map.size();
  std::string bar(50, '*');
  for (size_t i = 0; i < hist.size(); i++) {
    if (hist[i] > 0) {
      out << std::setw(5) << i << " : " << std::setw(5) << hist[i] << " "
          << bar.substr((size_t)(50 - hist[i] * 50 / total)) << std::endl;
    }
  }
}

namespace carve {
namespace csg {
class IntersectDebugHooks {
 public:
  virtual void drawIntersections(const VertexIntersections& /* vint */) {}

  virtual void drawPoint(const carve::mesh::MeshSet<3>::vertex_t* /* v */,
                         float /* r */, float /* g */, float /* b */,
                         float /* a */, float /* rad */) {}
  virtual void drawEdge(const carve::mesh::MeshSet<3>::vertex_t* /* v1 */,
                        const carve::mesh::MeshSet<3>::vertex_t* /* v2 */,
                        float /* rA */, float /* gA */, float /* bA */,
                        float /* aA */, float /* rB */, float /* gB */,
                        float /* bB */, float /* aB */,
                        float /* thickness */ = 1.0) {}

  virtual void drawFaceLoopWireframe(
      const std::vector<carve::mesh::MeshSet<3>::vertex_t*>& /* face_loop */,
      const carve::mesh::MeshSet<3>::vertex_t& /* normal */, float /* r */,
      float /* g */, float /* b */, float /* a */, bool /* inset */ = true) {}

  virtual void drawFaceLoop(
      const std::vector<carve::mesh::MeshSet<3>::vertex_t*>& /* face_loop */,
      const carve::mesh::MeshSet<3>::vertex_t& /* normal */, float /* r */,
      float /* g */, float /* b */, float /* a */, bool /* offset */ = true,
      bool /* lit */ = true) {}

  virtual void drawFaceLoop2(
      const std::vector<carve::mesh::MeshSet<3>::vertex_t*>& /* face_loop */,
      const carve::mesh::MeshSet<3>::vertex_t& /* normal */, float /* rF */,
      float /* gF */, float /* bF */, float /* aF */, float /* rB */,
      float /* gB */, float /* bB */, float /* aB */, bool /* offset */ = true,
      bool /* lit */ = true) {}

  virtual ~IntersectDebugHooks() {}
};

IntersectDebugHooks* intersect_installDebugHooks(IntersectDebugHooks* hooks);
bool intersect_debugEnabled();
}  // namespace csg
}  // namespace carve
