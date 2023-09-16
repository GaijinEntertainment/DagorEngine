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
#include <carve/classification.hpp>
#include <carve/collection_types.hpp>

namespace carve {
namespace csg {

struct FaceLoopGroup;

struct FaceLoop {
  FaceLoop *next, *prev;
  const carve::mesh::MeshSet<3>::face_t* orig_face;
  std::vector<carve::mesh::MeshSet<3>::vertex_t*> vertices;
  FaceLoopGroup* group;

  FaceLoop(const carve::mesh::MeshSet<3>::face_t* f,
           const std::vector<carve::mesh::MeshSet<3>::vertex_t*>& v)
      : next(nullptr),
        prev(nullptr),
        orig_face(f),
        vertices(v),
        group(nullptr) {}
};

struct FaceLoopList {
  FaceLoop *head, *tail;
  unsigned count;

  FaceLoopList() : head(nullptr), tail(nullptr), count(0) {}

  void append(FaceLoop* f) {
    f->prev = tail;
    f->next = nullptr;
    if (tail) {
      tail->next = f;
    }
    tail = f;
    if (!head) {
      head = f;
    }
    count++;
  }

  void prepend(FaceLoop* f) {
    f->next = head;
    f->prev = nullptr;
    if (head) {
      head->prev = f;
    }
    head = f;
    if (!tail) {
      tail = f;
    }
    count++;
  }

  unsigned size() const { return count; }

  FaceLoop* remove(FaceLoop* f) {
    FaceLoop* r = f->next;
    if (f->prev) {
      f->prev->next = f->next;
    } else {
      head = f->next;
    }
    if (f->next) {
      f->next->prev = f->prev;
    } else {
      tail = f->prev;
    }
    f->next = f->prev = nullptr;
    count--;
    return r;
  }

  ~FaceLoopList() {
    FaceLoop *a = head, *b;
    while (a) {
      b = a;
      a = a->next;
      delete b;
    }
  }
};

struct FaceLoopGroup {
  const carve::mesh::MeshSet<3>* src;
  FaceLoopList face_loops;
  V2Set perimeter;
  std::list<ClassificationInfo> classification;

  FaceLoopGroup(const carve::mesh::MeshSet<3>* _src) : src(_src) {}

  FaceClass classificationAgainst(
      const carve::mesh::MeshSet<3>::mesh_t* mesh) const;
};

typedef std::list<FaceLoopGroup> FLGroupList;
}  // namespace csg
}  // namespace carve
