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

#if defined(HAVE_CONFIG_H)
#include <carve_config.h>
#endif

#include <carve/csg.hpp>
#include <carve/debug_hooks.hpp>

#include <iostream>
#include <list>
#include <set>

#include <algorithm>

#include "intersect_classify_common.hpp"
#include "intersect_classify_common_impl.hpp"
#include "intersect_common.hpp"

namespace carve {
namespace csg {

namespace {
struct GroupPoly : public CSG::Collector {
  carve::mesh::MeshSet<3>* want_groups_from;
  std::list<std::pair<FaceClass, carve::mesh::MeshSet<3>*> >& out;

  GroupPoly(carve::mesh::MeshSet<3>* poly,
            std::list<std::pair<FaceClass, carve::mesh::MeshSet<3>*> >& _out)
      : CSG::Collector(), want_groups_from(poly), out(_out) {}

  ~GroupPoly() override {}

  void collect(FaceLoopGroup* grp, CSG::Hooks& /* hooks */) override {
    if (grp->face_loops.head->orig_face->mesh->meshset != want_groups_from) {
      return;
    }

    std::list<ClassificationInfo>& cinfo = (grp->classification);
    if (cinfo.size() == 0) {
      std::cerr << "WARNING! group " << grp << " has no classification info!"
                << std::endl;
      return;
    }
    // XXX: check all the cinfo elements for consistency.
    FaceClass fc = cinfo.front().classification;

    std::vector<carve::mesh::MeshSet<3>::face_t*> faces;
    faces.reserve(grp->face_loops.size());
    for (FaceLoop* loop = grp->face_loops.head; loop != nullptr;
         loop = loop->next) {
      faces.push_back(loop->orig_face->create(loop->vertices.begin(),
                                              loop->vertices.end(), false));
    }

    out.push_back(std::make_pair(fc, new carve::mesh::MeshSet<3>(faces)));
  }

  carve::mesh::MeshSet<3>* done(CSG::Hooks& /* hooks */) override {
    return nullptr;
  }
};

class FaceMaker {
 public:
  bool pointOn(VertexClassification& vclass, FaceLoop* f, size_t index) const {
    return vclass[f->vertices[index]].cls[0] == POINT_ON;
  }

  void explain(FaceLoop* f, size_t index, PointClass pc) const {
#if defined(CARVE_DEBUG)
    std::cerr << "face loop " << f << " from poly b is easy because vertex "
              << index << " (" << f->vertices[index]->v << ") is " << ENUM(pc)
              << std::endl;
#endif
  }
};

class HalfClassifyFaceGroups {
  HalfClassifyFaceGroups& operator=(const HalfClassifyFaceGroups&);

 public:
  std::list<std::pair<FaceClass, carve::mesh::MeshSet<3>*> >& b_out;
  CSG::Hooks& hooks;

  HalfClassifyFaceGroups(
      std::list<std::pair<FaceClass, carve::mesh::MeshSet<3>*> >& c,
      CSG::Hooks& h)
      : b_out(c), hooks(h) {}

  void classifySimple(FLGroupList& a_loops_grouped,
                      FLGroupList& b_loops_grouped,
                      VertexClassification& /* vclass */,
                      carve::mesh::MeshSet<3>* poly_a,
                      carve::mesh::MeshSet<3>* poly_b) const {
    GroupPoly group_poly(poly_b, b_out);
    performClassifySimpleOnFaceGroups(a_loops_grouped, b_loops_grouped, poly_a,
                                      poly_b, group_poly, hooks);
#if defined(CARVE_DEBUG)
    std::cerr << "after removal of simple on groups: " << b_loops_grouped.size()
              << " b groups" << std::endl;
#endif
  }

  void classifyEasy(
      FLGroupList& /* a_loops_grouped */, FLGroupList& b_loops_grouped,
      VertexClassification& vclass, carve::mesh::MeshSet<3>* poly_a,
      const carve::geom::RTreeNode<3, carve::mesh::Face<3>*>* poly_a_rtree,
      carve::mesh::MeshSet<3>* poly_b,
      const carve::geom::RTreeNode<3, carve::mesh::Face<3>*>* poly_b_rtree)
      const {
    GroupPoly group_poly(poly_b, b_out);
    performClassifyEasyFaceGroups(b_loops_grouped, poly_a, poly_a_rtree, vclass,
                                  FaceMaker(), group_poly, hooks);
#if defined(CARVE_DEBUG)
    std::cerr << "after removal of easy groups: " << b_loops_grouped.size()
              << " b groups" << std::endl;
#endif
  }

  void classifyHard(
      FLGroupList& /* a_loops_grouped */, FLGroupList& b_loops_grouped,
      VertexClassification& /* vclass */, carve::mesh::MeshSet<3>* poly_a,
      const carve::geom::RTreeNode<3, carve::mesh::Face<3>*>* poly_a_rtree,
      carve::mesh::MeshSet<3>* poly_b,
      const carve::geom::RTreeNode<3, carve::mesh::Face<3>*>* poly_b_rtree)
      const {
    GroupPoly group_poly(poly_b, b_out);
    performClassifyHardFaceGroups(b_loops_grouped, poly_a, poly_a_rtree,
                                  FaceMaker(), group_poly, hooks);
#if defined(CARVE_DEBUG)
    std::cerr << "after removal of hard groups: " << b_loops_grouped.size()
              << " b groups" << std::endl;
#endif
  }

  void faceLoopWork(
      FLGroupList& /* a_loops_grouped */, FLGroupList& b_loops_grouped,
      VertexClassification& /* vclass */, carve::mesh::MeshSet<3>* poly_a,
      const carve::geom::RTreeNode<3, carve::mesh::Face<3>*>* poly_a_rtree,
      carve::mesh::MeshSet<3>* poly_b,
      const carve::geom::RTreeNode<3, carve::mesh::Face<3>*>* poly_b_rtree)
      const {
    GroupPoly group_poly(poly_b, b_out);
    performFaceLoopWork(poly_a, poly_a_rtree, b_loops_grouped, *this,
                        group_poly, hooks);
  }

  void postRemovalCheck(FLGroupList& /* a_loops_grouped */,
                        FLGroupList& b_loops_grouped) const {
#if defined(CARVE_DEBUG)
    std::cerr << "after removal of on groups: " << b_loops_grouped.size()
              << " b groups" << std::endl;
#endif
  }

  bool faceLoopSanityChecker(FaceLoopGroup& i) const {
    return false;
    return i.face_loops.size() != 1;
  }

  void finish(FLGroupList& a_loops_grouped,
              FLGroupList& b_loops_grouped) const {
#if defined(CARVE_DEBUG)
    if (a_loops_grouped.size() || b_loops_grouped.size())
      std::cerr << "UNCLASSIFIED! a=" << a_loops_grouped.size()
                << ", b=" << b_loops_grouped.size() << std::endl;
#endif
  }
};
}  // namespace

void CSG::halfClassifyFaceGroups(
    const V2Set& /* shared_edges */, VertexClassification& vclass,
    carve::mesh::MeshSet<3>* poly_a,
    const carve::geom::RTreeNode<3, carve::mesh::Face<3>*>* poly_a_rtree,
    FLGroupList& a_loops_grouped, const detail::LoopEdges& /* a_edge_map */,
    carve::mesh::MeshSet<3>* poly_b,
    const carve::geom::RTreeNode<3, carve::mesh::Face<3>*>* poly_b_rtree,
    FLGroupList& b_loops_grouped, const detail::LoopEdges& /* b_edge_map */,
    std::list<std::pair<FaceClass, carve::mesh::MeshSet<3>*> >& b_out) {
  HalfClassifyFaceGroups classifier(b_out, hooks);
  GroupPoly group_poly(poly_b, b_out);
  performClassifyFaceGroups(a_loops_grouped, b_loops_grouped, vclass, poly_a,
                            poly_a_rtree, poly_b, poly_b_rtree, classifier,
                            group_poly, hooks);
}
}  // namespace csg
}  // namespace carve
