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

namespace carve {
namespace csg {
template <typename filter_t>
void Octree::doFindEdges(
    const carve::poly::Geometry<3>::face_t& f, Node* node,
    std::vector<const carve::poly::Geometry<3>::edge_t*>& out, unsigned depth,
    filter_t filter) const {
  if (node == nullptr) {
    return;
  }

  if (node->aabb.intersects(f.aabb) && node->aabb.intersects(f.plane_eqn)) {
    if (node->hasChildren()) {
      for (int i = 0; i < 8; ++i) {
        doFindEdges(f, node->children[i], out, depth + 1, filter);
      }
    } else {
      if (depth < MAX_SPLIT_DEPTH &&
          node->edges.size() > EDGE_SPLIT_THRESHOLD) {
        if (!node->split()) {
          for (int i = 0; i < 8; ++i) {
            doFindEdges(f, node->children[i], out, depth + 1, filter);
          }
          return;
        }
      }
      for (std::vector<const carve::poly::Geometry<3>::edge_t *>::const_iterator
               it = node->edges.begin(),
               e = node->edges.end();
           it != e; ++it) {
        if ((*it)->tag_once()) {
          if (filter(*it)) {
            out.push_back(*it);
          }
        }
      }
    }
  }
}

template <typename filter_t>
void Octree::findEdgesNear(
    const carve::poly::Geometry<3>::face_t& f,
    std::vector<const carve::poly::Geometry<3>::edge_t*>& out,
    filter_t filter) const {
  tagable::tag_begin();
  doFindEdges(f, root, out, 0, filter);
}

template <typename func_t>
void Octree::doIterate(int level, Node* node, const func_t& f) const {
  f(level, node);
  if (node->hasChildren()) {
    for (int i = 0; i < 8; ++i) {
      doIterate(level + 1, node->children[i], f);
    }
  }
}

template <typename func_t>
void Octree::iterateNodes(const func_t& f) const {
  doIterate(0, root, f);
}
}  // namespace csg
}  // namespace carve
