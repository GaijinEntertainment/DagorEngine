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

#include <algorithm>

#include <carve/carve.hpp>
#include <carve/intersection.hpp>
#include <carve/poly.hpp>
#include <carve/timing.hpp>

void carve::csg::Intersections::collect(
    const IObj& obj, std::vector<carve::mesh::MeshSet<3>::vertex_t*>* collect_v,
    std::vector<carve::mesh::MeshSet<3>::edge_t*>* collect_e,
    std::vector<carve::mesh::MeshSet<3>::face_t*>* collect_f) const {
  carve::csg::Intersections::const_iterator i = find(obj);
  if (i != end()) {
    Intersections::mapped_type::const_iterator a, b;
    for (a = (*i).second.begin(), b = (*i).second.end(); a != b; ++a) {
      switch ((*a).first.obtype) {
        case carve::csg::IObj::OBTYPE_VERTEX:
          if (collect_v) {
            collect_v->push_back((*a).first.vertex);
          }
          break;
        case carve::csg::IObj::OBTYPE_EDGE:
          if (collect_e) {
            collect_e->push_back((*a).first.edge);
          }
          break;
        case carve::csg::IObj::OBTYPE_FACE:
          if (collect_f) {
            collect_f->push_back((*a).first.face);
          }
          break;
        default:
          throw carve::exception("should not happen " __FILE__
                                 ":" XSTR(__LINE__));
      }
    }
  }
}

bool carve::csg::Intersections::intersectsFace(
    carve::mesh::MeshSet<3>::vertex_t* v,
    carve::mesh::MeshSet<3>::face_t* f) const {
  const_iterator i = find(v);
  if (i != end()) {
    mapped_type::const_iterator a, b;

    for (a = (*i).second.begin(), b = (*i).second.end(); a != b; ++a) {
      switch ((*a).first.obtype) {
        case IObj::OBTYPE_VERTEX: {
          const carve::mesh::MeshSet<3>::edge_t* edge = f->edge;
          do {
            if (edge->vert == (*a).first.vertex) {
              return true;
            }
            edge = edge->next;
          } while (edge != f->edge);
          break;
        }
        case carve::csg::IObj::OBTYPE_EDGE: {
          const carve::mesh::MeshSet<3>::edge_t* edge = f->edge;
          do {
            if (edge == (*a).first.edge) {
              return true;
            }
            edge = edge->next;
          } while (edge != f->edge);
          break;
        }
        case carve::csg::IObj::OBTYPE_FACE: {
          if ((*a).first.face == f) {
            return true;
          }
          break;
        }
        default:
          throw carve::exception("should not happen " __FILE__
                                 ":" XSTR(__LINE__));
      }
    }
  }
  return false;
}
