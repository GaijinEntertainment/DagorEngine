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

#include "write_ply.hpp"

#include <gloop/model/obj_format.hpp>
#include <gloop/model/ply_format.hpp>
#include <gloop/model/vtk_format.hpp>

#include <stdio.h>
#include <stdlib.h>
#include <iomanip>

#ifndef WIN32
#include <stdint.h>
#endif

namespace {
struct vertex_base : public gloop::stream::null_writer {
  virtual const carve::geom3d::Vector& curr() const = 0;
};

template <typename container_t>
struct vertex : public vertex_base {
  const container_t& cnt;
  int i;
  vertex(const container_t& _cnt) : cnt(_cnt), i(-1) {}
  void next() override { ++i; }
  int length() override { return cnt.size(); }
  const carve::geom3d::Vector& curr() const override { return cnt[i].v; }
};

typedef vertex<std::vector<carve::point::Vertex> > pointset_vertex;
typedef vertex<std::vector<carve::poly::Vertex<3> > > poly_vertex;
typedef vertex<std::vector<carve::mesh::Vertex<3> > > mesh_vertex;
typedef vertex<std::vector<carve::line::Vertex> > line_vertex;

template <int idx>
struct vertex_component : public gloop::stream::writer<double> {
  vertex_base& r;
  vertex_component(vertex_base& _r) : r(_r) {}
  double value() override { return r.curr().v[idx]; }
};

struct mesh_face : public gloop::stream::null_writer {
  std::vector<const carve::mesh::MeshSet<3>::face_t*> faces;
  int i;
  mesh_face(const carve::mesh::MeshSet<3>* poly) : faces(), i(-1) {
    std::copy(poly->faceBegin(), poly->faceEnd(), std::back_inserter(faces));
  }
  void next() override { ++i; }
  int length() override { return faces.size(); }
  const carve::mesh::MeshSet<3>::face_t* curr() const { return faces[i]; }
};

struct mesh_face_idx : public gloop::stream::writer<size_t> {
  mesh_face& r;
  const carve::mesh::MeshSet<3>::face_t* f;
  carve::mesh::MeshSet<3>::face_t::const_edge_iter_t i;
  gloop::stream::Type data_type;
  int max_length;

  mesh_face_idx(mesh_face& _r, gloop::stream::Type _data_type, int _max_length)
      : r(_r), f(nullptr), data_type(_data_type), max_length(_max_length) {}
  void begin() override {
    f = r.curr();
    i = f->begin();
  }
  int length() override { return f->nVertices(); }

  bool isList() override { return true; }
  gloop::stream::Type dataType() override { return data_type; }
  int maxLength() override { return max_length; }

  size_t value() override {
    const carve::mesh::MeshSet<3>::vertex_t* v = (*i++).vert;
    return v - &f->mesh->meshset->vertex_storage[0];
  }
};

struct face : public gloop::stream::null_writer {
  const carve::poly::Polyhedron* poly;
  int i;
  face(const carve::poly::Polyhedron* _poly) : poly(_poly), i(-1) {}
  void next() override { ++i; }
  int length() override { return poly->faces.size(); }
  const carve::poly::Face<3>* curr() const { return &poly->faces[i]; }
};

struct face_idx : public gloop::stream::writer<size_t> {
  face& r;
  const carve::poly::Face<3>* f;
  size_t i;
  gloop::stream::Type data_type;
  int max_length;

  face_idx(face& _r, gloop::stream::Type _data_type, int _max_length)
      : r(_r),
        f(nullptr),
        i(0),
        data_type(_data_type),
        max_length(_max_length) {}
  void begin() override {
    f = r.curr();
    i = 0;
  }
  int length() override { return f->nVertices(); }

  bool isList() override { return true; }
  gloop::stream::Type dataType() override { return data_type; }
  int maxLength() override { return max_length; }

  size_t value() override {
    return static_cast<const carve::poly::Polyhedron*>(f->owner)
        ->vertexToIndex_fast(f->vertex(i++));
  }
};

struct lineset : public gloop::stream::null_writer {
  const carve::line::PolylineSet* polyline;
  carve::line::PolylineSet::const_line_iter c;
  carve::line::PolylineSet::const_line_iter n;
  lineset(const carve::line::PolylineSet* _polyline) : polyline(_polyline) {
    n = polyline->lines.begin();
  }
  void next() override {
    c = n;
    ++n;
  }
  int length() override { return polyline->lines.size(); }
  const carve::line::Polyline* curr() const { return *c; }
};

struct line_closed : public gloop::stream::writer<bool> {
  lineset& ls;
  line_closed(lineset& _ls) : ls(_ls) {}
  gloop::stream::Type dataType() override { return gloop::stream::U8; }
  bool value() override { return ls.curr()->isClosed(); }
};

struct line_vi : public gloop::stream::writer<size_t> {
  lineset& ls;
  const carve::line::Polyline* l;
  size_t i;
  gloop::stream::Type data_type;
  int max_length;

  line_vi(lineset& _ls, gloop::stream::Type _data_type, int _max_length)
      : ls(_ls),
        l(nullptr),
        i(0),
        data_type(_data_type),
        max_length(_max_length) {}
  bool isList() override { return true; }
  gloop::stream::Type dataType() override { return data_type; }
  int maxLength() override { return max_length; }
  void begin() override {
    l = ls.curr();
    i = 0;
  }
  int length() override { return l->vertexCount(); }
  size_t value() override {
    return ls.polyline->vertexToIndex_fast(l->vertex(i++));
  }
};

void setup(gloop::stream::model_writer& file,
           const carve::mesh::MeshSet<3>* poly) {
  size_t face_max = 0;
  for (carve::mesh::MeshSet<3>::const_face_iter i = poly->faceBegin();
       i != poly->faceEnd(); ++i) {
    face_max = std::max(face_max, (*i)->nVertices());
  }

  file.newBlock("polyhedron");
  mesh_vertex* vi = new mesh_vertex(poly->vertex_storage);
  file.addWriter("polyhedron.vertex", vi);
  file.addWriter("polyhedron.vertex.x", new vertex_component<0>(*vi));
  file.addWriter("polyhedron.vertex.y", new vertex_component<1>(*vi));
  file.addWriter("polyhedron.vertex.z", new vertex_component<2>(*vi));

  mesh_face* fi = new mesh_face(poly);
  file.addWriter("polyhedron.face", fi);
  file.addWriter("polyhedron.face.vertex_indices",
                 new mesh_face_idx(*fi, gloop::stream::smallest_type(
                                            poly->vertex_storage.size()),
                                   face_max));
}

void setup(gloop::stream::model_writer& file,
           const carve::poly::Polyhedron* poly) {
  size_t face_max = 0;
  for (size_t i = 0; i < poly->faces.size(); ++i) {
    face_max = std::max(face_max, poly->faces[i].nVertices());
  }

  file.newBlock("polyhedron");
  poly_vertex* vi = new poly_vertex(poly->vertices);
  file.addWriter("polyhedron.vertex", vi);
  file.addWriter("polyhedron.vertex.x", new vertex_component<0>(*vi));
  file.addWriter("polyhedron.vertex.y", new vertex_component<1>(*vi));
  file.addWriter("polyhedron.vertex.z", new vertex_component<2>(*vi));

  face* fi = new face(poly);
  file.addWriter("polyhedron.face", fi);
  file.addWriter(
      "polyhedron.face.vertex_indices",
      new face_idx(*fi, gloop::stream::smallest_type(poly->vertices.size()),
                   face_max));
}

void setup(gloop::stream::model_writer& file,
           const carve::line::PolylineSet* lines) {
  size_t line_max = 0;
  for (std::list<carve::line::Polyline *>::const_iterator
           i = lines->lines.begin(),
           e = lines->lines.end();
       i != e; ++i) {
    line_max = std::max(line_max, (*i)->vertexCount());
  }

  file.newBlock("polyline");
  line_vertex* vi = new line_vertex(lines->vertices);
  file.addWriter("polyline.vertex", vi);
  file.addWriter("polyline.vertex.x", new vertex_component<0>(*vi));
  file.addWriter("polyline.vertex.y", new vertex_component<1>(*vi));
  file.addWriter("polyline.vertex.z", new vertex_component<2>(*vi));

  lineset* pi = new lineset(lines);
  file.addWriter("polyline.polyline", pi);
  file.addWriter("polyline.polyline.closed", new line_closed(*pi));
  file.addWriter(
      "polyline.polyline.vertex_indices",
      new line_vi(*pi, gloop::stream::smallest_type(lines->vertices.size()),
                  line_max));
}

void setup(gloop::stream::model_writer& file,
           const carve::point::PointSet* points) {
  file.newBlock("pointset");
  pointset_vertex* vi = new pointset_vertex(points->vertices);
  file.addWriter("pointset.vertex", vi);
  file.addWriter("pointset.vertex.x", new vertex_component<0>(*vi));
  file.addWriter("pointset.vertex.y", new vertex_component<1>(*vi));
  file.addWriter("pointset.vertex.z", new vertex_component<2>(*vi));
}
}  // namespace

void writePLY(std::ostream& out, const carve::mesh::MeshSet<3>* poly,
              bool ascii) {
  gloop::ply::PlyWriter file(!ascii, false);
  if (ascii) {
    out << std::setprecision(30);
  }
  setup(file, poly);
  file.write(out);
}

void writePLY(const std::string& out_file, const carve::mesh::MeshSet<3>* poly,
              bool ascii) {
  std::ofstream out(out_file.c_str(), std::ios_base::binary);
  if (!out.is_open()) {
    std::cerr << "File '" << out_file << "' could not be opened." << std::endl;
    return;
  }
  writePLY(out, poly, ascii);
}

void writePLY(std::ostream& out, const carve::poly::Polyhedron* poly,
              bool ascii) {
  gloop::ply::PlyWriter file(!ascii, false);
  if (ascii) {
    out << std::setprecision(30);
  }
  setup(file, poly);
  file.write(out);
}

void writePLY(const std::string& out_file, const carve::poly::Polyhedron* poly,
              bool ascii) {
  std::ofstream out(out_file.c_str(), std::ios_base::binary);
  if (!out.is_open()) {
    std::cerr << "File '" << out_file << "' could not be opened." << std::endl;
    return;
  }
  writePLY(out, poly, ascii);
}

void writePLY(std::ostream& out, const carve::line::PolylineSet* lines,
              bool ascii) {
  gloop::ply::PlyWriter file(!ascii, false);

  if (ascii) {
    out << std::setprecision(30);
  }
  setup(file, lines);
  file.write(out);
}

void writePLY(const std::string& out_file,
              const carve::line::PolylineSet* lines, bool ascii) {
  std::ofstream out(out_file.c_str(), std::ios_base::binary);
  if (!out.is_open()) {
    std::cerr << "File '" << out_file << "' could not be opened." << std::endl;
    return;
  }
  writePLY(out, lines, ascii);
}

void writePLY(std::ostream& out, const carve::point::PointSet* points,
              bool ascii) {
  gloop::ply::PlyWriter file(!ascii, false);

  if (ascii) {
    out << std::setprecision(30);
  }
  setup(file, points);
  file.write(out);
}

void writePLY(const std::string& out_file, const carve::point::PointSet* points,
              bool ascii) {
  std::ofstream out(out_file.c_str(), std::ios_base::binary);
  if (!out.is_open()) {
    std::cerr << "File '" << out_file << "' could not be opened." << std::endl;
    return;
  }
  writePLY(out, points, ascii);
}

void writeOBJ(std::ostream& out, const carve::mesh::MeshSet<3>* poly) {
  gloop::obj::ObjWriter file;
  setup(file, poly);
  file.write(out);
}

void writeOBJ(const std::string& out_file,
              const carve::mesh::MeshSet<3>* poly) {
  std::ofstream out(out_file.c_str(), std::ios_base::binary);
  if (!out.is_open()) {
    std::cerr << "File '" << out_file << "' could not be opened." << std::endl;
    return;
  }
  writeOBJ(out, poly);
}

void writeOBJ(std::ostream& out, const carve::poly::Polyhedron* poly) {
  gloop::obj::ObjWriter file;
  setup(file, poly);
  file.write(out);
}

void writeOBJ(const std::string& out_file,
              const carve::poly::Polyhedron* poly) {
  std::ofstream out(out_file.c_str(), std::ios_base::binary);
  if (!out.is_open()) {
    std::cerr << "File '" << out_file << "' could not be opened." << std::endl;
    return;
  }
  writeOBJ(out, poly);
}

void writeOBJ(std::ostream& out, const carve::line::PolylineSet* lines) {
  gloop::obj::ObjWriter file;
  setup(file, lines);
  file.write(out);
}

void writeOBJ(const std::string& out_file,
              const carve::line::PolylineSet* lines, bool /* ascii */) {
  std::ofstream out(out_file.c_str(), std::ios_base::binary);
  if (!out.is_open()) {
    std::cerr << "File '" << out_file << "' could not be opened." << std::endl;
    return;
  }
  writeOBJ(out, lines);
}

void writeVTK(std::ostream& out, const carve::mesh::MeshSet<3>* poly) {
  gloop::vtk::VtkWriter file(gloop::vtk::VtkWriter::ASCII);
  setup(file, poly);
  file.write(out);
}

void writeVTK(const std::string& out_file,
              const carve::mesh::MeshSet<3>* poly) {
  std::ofstream out(out_file.c_str(), std::ios_base::binary);
  if (!out.is_open()) {
    std::cerr << "File '" << out_file << "' could not be opened." << std::endl;
    return;
  }
  writeVTK(out, poly);
}

void writeVTK(std::ostream& out, const carve::poly::Polyhedron* poly) {
  gloop::vtk::VtkWriter file(gloop::vtk::VtkWriter::ASCII);
  setup(file, poly);
  file.write(out);
}

void writeVTK(const std::string& out_file,
              const carve::poly::Polyhedron* poly) {
  std::ofstream out(out_file.c_str(), std::ios_base::binary);
  if (!out.is_open()) {
    std::cerr << "File '" << out_file << "' could not be opened." << std::endl;
    return;
  }
  writeVTK(out, poly);
}

void writeVTK(std::ostream& out, const carve::line::PolylineSet* lines) {
  gloop::vtk::VtkWriter file(gloop::vtk::VtkWriter::ASCII);
  setup(file, lines);
  file.write(out);
}

void writeVTK(const std::string& out_file,
              const carve::line::PolylineSet* lines, bool /* ascii */) {
  std::ofstream out(out_file.c_str(), std::ios_base::binary);
  if (!out.is_open()) {
    std::cerr << "File '" << out_file << "' could not be opened." << std::endl;
    return;
  }
  writeVTK(out, lines);
}
