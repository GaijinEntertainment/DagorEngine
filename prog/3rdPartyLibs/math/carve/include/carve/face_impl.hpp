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

namespace std {
template <unsigned ndim>
inline void swap(carve::poly::Face<ndim>& a, carve::poly::Face<ndim>& b) {
  a.swap(b);
}
}

namespace carve {
namespace poly {
template <unsigned ndim>
void Face<ndim>::swap(Face<ndim>& other) {
  std::swap(vertices, other.vertices);
  std::swap(edges, other.edges);
  std::swap(owner, other.owner);
  std::swap(aabb, other.aabb);
  std::swap(plane_eqn, other.plane_eqn);
  std::swap(manifold_id, other.manifold_id);
  std::swap(group_id, other.group_id);
  std::swap(project, other.project);
  std::swap(unproject, other.unproject);
}

template <unsigned ndim>
template <typename iter_t>
Face<ndim>* Face<ndim>::init(const Face<ndim>* base, iter_t vbegin, iter_t vend,
                             bool flipped) {
  CARVE_ASSERT(vbegin < vend);

  vertices.reserve((size_t)std::distance(vbegin, vend));

  if (flipped) {
    std::reverse_copy(vbegin, vend, std::back_inserter(vertices));
    plane_eqn = -base->plane_eqn;
  } else {
    std::copy(vbegin, vend, std::back_inserter(vertices));
    plane_eqn = base->plane_eqn;
  }

  edges.clear();
  edges.resize(nVertices(), nullptr);

  aabb.fit(vertices.begin(), vertices.end(), vec_adapt_vertex_ptr());
  untag();

  int da = carve::geom::largestAxis(plane_eqn.N);

  project = getProjector(plane_eqn.N.v[da] > 0, da);
  unproject = getUnprojector(plane_eqn.N.v[da] > 0, da);

  return this;
}

template <unsigned ndim>
template <typename iter_t>
Face<ndim>* Face<ndim>::create(iter_t vbegin, iter_t vend, bool flipped) const {
  return (new Face)->init(this, vbegin, vend, flipped);
}

template <unsigned ndim>
Face<ndim>* Face<ndim>::create(const std::vector<const vertex_t*>& _vertices,
                               bool flipped) const {
  return (new Face)->init(this, _vertices.begin(), _vertices.end(), flipped);
}

template <unsigned ndim>
Face<ndim>* Face<ndim>::clone(bool flipped) const {
  return (new Face)->init(this, vertices, flipped);
}

template <unsigned ndim>
void Face<ndim>::getVertexLoop(std::vector<const vertex_t*>& loop) const {
  loop.resize(nVertices(), nullptr);
  std::copy(vbegin(), vend(), loop.begin());
}

template <unsigned ndim>
const typename Face<ndim>::edge_t*& Face<ndim>::edge(size_t idx) {
  return edges[idx];
}

template <unsigned ndim>
const typename Face<ndim>::edge_t* Face<ndim>::edge(size_t idx) const {
  return edges[idx];
}

template <unsigned ndim>
size_t Face<ndim>::nEdges() const {
  return edges.size();
}

template <unsigned ndim>
const typename Face<ndim>::vertex_t*& Face<ndim>::vertex(size_t idx) {
  return vertices[idx];
}

template <unsigned ndim>
const typename Face<ndim>::vertex_t* Face<ndim>::vertex(size_t idx) const {
  return vertices[idx];
}

template <unsigned ndim>
size_t Face<ndim>::nVertices() const {
  return vertices.size();
}

template <unsigned ndim>
typename Face<ndim>::vector_t Face<ndim>::centroid() const {
  vector_t c;
  carve::geom::centroid(vertices.begin(), vertices.end(),
                        vec_adapt_vertex_ptr(), c);
  return c;
}

template <unsigned ndim>
std::vector<carve::geom::vector<2> > Face<ndim>::projectedVertices() const {
  p2_adapt_project<ndim> proj = projector();
  std::vector<carve::geom::vector<2> > result;
  result.reserve(nVertices());
  for (size_t i = 0; i < nVertices(); ++i) {
    result.push_back(proj(vertex(i)->v));
  }
  return result;
}
}  // namespace poly
}  // namespace carve
