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

#include <carve/aabb.hpp>
#include <carve/geom3d.hpp>

#include <carve/polyhedron_base.hpp>

namespace carve {

namespace csg {

const double SLACK_FACTOR = 1.0009765625;
const unsigned FACE_SPLIT_THRESHOLD = 50U;
const unsigned EDGE_SPLIT_THRESHOLD = 50U;
const unsigned POINT_SPLIT_THRESHOLD = 20U;
const unsigned MAX_SPLIT_DEPTH = 32;

class Octree {
 public:
  class Node {
   private:
    Node(const Node& node);             // undefined.
    Node& operator=(const Node& node);  // undefined.

   public:
    Node* parent;
    Node* children[8];
    bool is_leaf;

    carve::geom3d::Vector min;
    carve::geom3d::Vector max;

    std::vector<const carve::poly::Geometry<3>::face_t*> faces;
    std::vector<const carve::poly::Geometry<3>::edge_t*> edges;
    std::vector<const carve::poly::Geometry<3>::vertex_t*> vertices;

    carve::geom3d::AABB aabb;

    Node();

    Node(const carve::geom3d::Vector& newMin,
         const carve::geom3d::Vector& newMax);
    Node(Node* p, double x1, double y1, double z1, double x2, double y2,
         double z2);

    ~Node();

    bool mightContain(const carve::poly::Geometry<3>::face_t& face);
    bool mightContain(const carve::poly::Geometry<3>::edge_t& edge);
    bool mightContain(const carve::poly::Geometry<3>::vertex_t& p);
    bool hasChildren();
    bool hasGeometry();

    template <class T>
    void putInside(const T& input, Node* child, T& output);

    bool split();
  };

  Node* root;

  struct no_filter {
    bool operator()(const carve::poly::Geometry<3>::edge_t*) { return true; }
    bool operator()(const carve::poly::Geometry<3>::face_t*) { return true; }
  };

  Octree();

  ~Octree();

  void setBounds(const carve::geom3d::Vector& min,
                 const carve::geom3d::Vector& max);
  void setBounds(carve::geom3d::AABB aabb);

  void addEdges(const std::vector<carve::poly::Geometry<3>::edge_t>& edges);
  void addFaces(const std::vector<carve::poly::Geometry<3>::face_t>& faces);
  void addVertices(
      const std::vector<const carve::poly::Geometry<3>::vertex_t*>& vertices);

  static carve::geom3d::AABB makeAABB(const Node* node);

  void doFindEdges(const carve::geom::aabb<3>& aabb, Node* node,
                   std::vector<const carve::poly::Geometry<3>::edge_t*>& out,
                   unsigned depth) const;
  void doFindEdges(const carve::geom3d::LineSegment& l, Node* node,
                   std::vector<const carve::poly::Geometry<3>::edge_t*>& out,
                   unsigned depth) const;
  void doFindEdges(const carve::geom3d::Vector& v, Node* node,
                   std::vector<const carve::poly::Geometry<3>::edge_t*>& out,
                   unsigned depth) const;
  void doFindFaces(const carve::geom::aabb<3>& aabb, Node* node,
                   std::vector<const carve::poly::Geometry<3>::face_t*>& out,
                   unsigned depth) const;
  void doFindFaces(const carve::geom3d::LineSegment& l, Node* node,
                   std::vector<const carve::poly::Geometry<3>::face_t*>& out,
                   unsigned depth) const;

  void doFindVerticesAllowDupes(
      const carve::geom3d::Vector& v, Node* node,
      std::vector<const carve::poly::Geometry<3>::vertex_t*>& out,
      unsigned depth) const;

  void findVerticesNearAllowDupes(
      const carve::geom3d::Vector& v,
      std::vector<const carve::poly::Geometry<3>::vertex_t*>& out) const;

  template <typename filter_t>
  void doFindEdges(const carve::poly::Geometry<3>::face_t& f, Node* node,
                   std::vector<const carve::poly::Geometry<3>::edge_t*>& out,
                   unsigned depth, filter_t filter) const;

  template <typename filter_t>
  void findEdgesNear(const carve::poly::Geometry<3>::face_t& f,
                     std::vector<const carve::poly::Geometry<3>::edge_t*>& out,
                     filter_t filter) const;

  void findEdgesNear(
      const carve::poly::Geometry<3>::face_t& f,
      std::vector<const carve::poly::Geometry<3>::edge_t*>& out) const {
    return findEdgesNear(f, out, no_filter());
  }

  void findEdgesNear(
      const carve::geom::aabb<3>& aabb,
      std::vector<const carve::poly::Geometry<3>::edge_t*>& out) const;
  void findEdgesNear(
      const carve::geom3d::LineSegment& l,
      std::vector<const carve::poly::Geometry<3>::edge_t*>& out) const;
  void findEdgesNear(
      const carve::poly::Geometry<3>::edge_t& e,
      std::vector<const carve::poly::Geometry<3>::edge_t*>& out) const;
  void findEdgesNear(
      const carve::geom3d::Vector& v,
      std::vector<const carve::poly::Geometry<3>::edge_t*>& out) const;

  void findFacesNear(
      const carve::geom::aabb<3>& aabb,
      std::vector<const carve::poly::Geometry<3>::face_t*>& out) const;
  void findFacesNear(
      const carve::geom3d::LineSegment& l,
      std::vector<const carve::poly::Geometry<3>::face_t*>& out) const;
  void findFacesNear(
      const carve::poly::Geometry<3>::edge_t& e,
      std::vector<const carve::poly::Geometry<3>::face_t*>& out) const;

  static void doSplit(int maxSplit, Node* node);

  template <typename FUNC>
  void doIterate(int level, Node* node, const FUNC& f) const;

  template <typename FUNC>
  void iterateNodes(const FUNC& f) const;

  void splitTree();
};
}  // namespace csg
}  // namespace carve
