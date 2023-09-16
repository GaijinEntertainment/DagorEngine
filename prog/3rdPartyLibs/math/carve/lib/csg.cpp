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
#include "csg_detail.hpp"

const char* carve::csg::ENUM(carve::csg::FaceClass f) {
  if (f == FACE_ON_ORIENT_OUT) {
    return "FACE_ON_ORIENT_OUT";
  }
  if (f == FACE_OUT) {
    return "FACE_OUT";
  }
  if (f == FACE_IN) {
    return "FACE_IN";
  }
  if (f == FACE_ON_ORIENT_IN) {
    return "FACE_ON_ORIENT_IN";
  }
  return "???";
}

const char* carve::csg::ENUM(carve::PointClass p) {
  if (p == POINT_UNK) {
    return "POINT_UNK";
  }
  if (p == POINT_OUT) {
    return "POINT_OUT";
  }
  if (p == POINT_ON) {
    return "POINT_ON";
  }
  if (p == POINT_IN) {
    return "POINT_IN";
  }
  if (p == POINT_VERTEX) {
    return "POINT_VERTEX";
  }
  if (p == POINT_EDGE) {
    return "POINT_EDGE";
  }
  return "???";
}

void carve::csg::detail::LoopEdges::addFaceLoop(FaceLoop* fl) {
  carve::mesh::MeshSet<3>::vertex_t *v1, *v2;
  v1 = fl->vertices[fl->vertices.size() - 1];
  for (unsigned j = 0; j < fl->vertices.size(); ++j) {
    v2 = fl->vertices[j];
    (*this)[std::make_pair(v1, v2)].push_back(fl);
    v1 = v2;
  }
}

void carve::csg::detail::LoopEdges::sortFaceLoopLists() {
  for (super::iterator i = begin(), e = end(); i != e; ++i) {
    (*i).second.sort();
  }
}

void carve::csg::detail::LoopEdges::removeFaceLoop(FaceLoop* fl) {
  carve::mesh::MeshSet<3>::vertex_t *v1, *v2;
  v1 = fl->vertices[fl->vertices.size() - 1];
  for (unsigned j = 0; j < fl->vertices.size(); ++j) {
    v2 = fl->vertices[j];
    iterator l(find(std::make_pair(v1, v2)));
    if (l != end()) {
      (*l).second.remove(fl);
      if (!(*l).second.size()) {
        erase(l);
      }
    }
    v1 = v2;
  }
}

carve::csg::FaceClass carve::csg::FaceLoopGroup::classificationAgainst(
    const carve::mesh::MeshSet<3>::mesh_t* mesh) const {
  for (std::list<ClassificationInfo>::const_iterator i = classification.begin();
       i != classification.end(); ++i) {
    if ((*i).intersected_mesh == mesh) {
      return (*i).classification;
    }
  }
  return FACE_UNCLASSIFIED;
}
