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
#include <carve/collection_types.hpp>

namespace carve {
namespace csg {

enum FaceClass {
  FACE_UNCLASSIFIED = -3,
  FACE_ON_ORIENT_OUT = -2,
  FACE_OUT = -1,
  FACE_ON = 0,
  FACE_IN = +1,
  FACE_ON_ORIENT_IN = +2
};

enum FaceClassBit {
  FACE_ON_ORIENT_OUT_BIT = 0x01,
  FACE_OUT_BIT = 0x02,
  FACE_IN_BIT = 0x04,
  FACE_ON_ORIENT_IN_BIT = 0x08,

  FACE_ANY_BIT = 0x0f,
  FACE_ON_BIT = 0x09,
  FACE_NOT_ON_BIT = 0x06
};

static inline FaceClass class_bit_to_class(unsigned i) {
  if (i & FACE_ON_ORIENT_OUT_BIT) {
    return FACE_ON_ORIENT_OUT;
  }
  if (i & FACE_OUT_BIT) {
    return FACE_OUT;
  }
  if (i & FACE_IN_BIT) {
    return FACE_IN;
  }
  if (i & FACE_ON_ORIENT_IN_BIT) {
    return FACE_ON_ORIENT_IN;
  }
  return FACE_UNCLASSIFIED;
}

static inline unsigned class_to_class_bit(FaceClass f) {
  switch (f) {
    case FACE_ON_ORIENT_OUT:
      return FACE_ON_ORIENT_OUT_BIT;
    case FACE_OUT:
      return FACE_OUT_BIT;
    case FACE_ON:
      return FACE_ON_BIT;
    case FACE_IN:
      return FACE_IN_BIT;
    case FACE_ON_ORIENT_IN:
      return FACE_ON_ORIENT_IN_BIT;
    case FACE_UNCLASSIFIED:
      return FACE_ANY_BIT;
    default:
      return 0;
  }
}

enum EdgeClass { EDGE_UNK = -2, EDGE_OUT = -1, EDGE_ON = 0, EDGE_IN = 1 };

const char* ENUM(FaceClass f);
const char* ENUM(PointClass p);

struct ClassificationInfo {
  const carve::mesh::Mesh<3>* intersected_mesh;
  FaceClass classification;

  ClassificationInfo()
      : intersected_mesh(nullptr), classification(FACE_UNCLASSIFIED) {}
  ClassificationInfo(const carve::mesh::Mesh<3>* _intersected_mesh,
                     FaceClass _classification)
      : intersected_mesh(_intersected_mesh), classification(_classification) {}
  bool intersectedMeshIsClosed() const { return intersected_mesh->isClosed(); }
};

struct EC2 {
  EdgeClass cls[2];
  EC2() { cls[0] = cls[1] = EDGE_UNK; }
  EC2(EdgeClass a, EdgeClass b) {
    cls[0] = a;
    cls[1] = b;
  }
};

struct PC2 {
  PointClass cls[2];
  PC2() { cls[0] = cls[1] = POINT_UNK; }
  PC2(PointClass a, PointClass b) {
    cls[0] = a;
    cls[1] = b;
  }
};

typedef std::unordered_map<std::pair<const carve::mesh::MeshSet<3>::vertex_t*,
                                     const carve::mesh::MeshSet<3>::vertex_t*>,
                           EC2, hash_pair>
    EdgeClassification;

typedef std::unordered_map<const carve::mesh::Vertex<3>*, PC2>
    VertexClassification;
}  // namespace csg
}  // namespace carve
