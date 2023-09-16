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
#include <carve/csg_triangulator.hpp>
#include <carve/tree.hpp>

#include "read_ply.hpp"
#include "write_ply.hpp"

#include "opts.hpp"

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <set>
#include <string>
#include <utility>

#include <time.h>
typedef std::vector<std::string>::iterator TOK;

struct Options : public opt::Parser {
  bool improve;
  bool ascii;
  bool obj;
  bool vtk;
  bool canonicalize;

  std::string file;

  void optval(const std::string& o, const std::string& v) override {
    if (o == "--canonicalize" || o == "-c") {
      canonicalize = true;
      return;
    }
    if (o == "--binary" || o == "-b") {
      ascii = false;
      return;
    }
    if (o == "--obj" || o == "-O") {
      obj = true;
      return;
    }
    if (o == "--vtk" || o == "-V") {
      vtk = true;
      return;
    }
    if (o == "--ascii" || o == "-a") {
      ascii = true;
      return;
    }
    if (o == "--help" || o == "-h") {
      help(std::cout);
      exit(0);
    }
    if (o == "--improve" || o == "-i") {
      improve = true;
      return;
    }
  }

  std::string usageStr() override {
    return std::string("Usage: ") + progname +
           std::string(" [options] expression");
  };

  void arg(const std::string& a) override { file = a; }

  void help(std::ostream& out) override { this->opt::Parser::help(out); }

  Options() {
    improve = false;
    ascii = true;
    obj = false;
    vtk = false;
    canonicalize = false;
    file = "";

    option("canonicalize", 'c', false,
           "Canonicalize before output (for comparing output).");
    option("binary", 'b', false, "Produce binary output.");
    option("ascii", 'a', false, "ASCII output (default).");
    option("obj", 'O', false, "Output in .obj format.");
    option("vtk", 'V', false, "Output in .vtk format.");
    option("improve", 'i', false,
           "Improve triangulation by minimising internal edge lengths.");
    option("help", 'h', false, "This help message.");
  }
};

static Options options;

static bool endswith(const std::string& a, const std::string& b) {
  if (a.size() < b.size()) {
    return false;
  }

  for (unsigned i = a.size(), j = b.size(); j;) {
    if (tolower(a[--i]) != tolower(b[--j])) {
      return false;
    }
  }
  return true;
}

carve::mesh::MeshSet<3>* readModel(const std::string& file) {
  carve::mesh::MeshSet<3>* poly;

  if (file == "") {
    if (options.obj) {
      poly = readOBJasMesh(std::cin);
    } else if (options.vtk) {
      poly = readVTKasMesh(std::cin);
    } else {
      poly = readPLYasMesh(std::cin);
    }
  } else if (endswith(file, ".ply")) {
    poly = readPLYasMesh(file);
  } else if (endswith(file, ".vtk")) {
    poly = readVTKasMesh(file);
  } else if (endswith(file, ".obj")) {
    poly = readOBJasMesh(file);
  }

  if (poly == nullptr) {
    return nullptr;
  }

  std::cerr << "loaded polyhedron " << poly << " has " << poly->meshes.size()
            << " manifolds ("
            << std::count_if(poly->meshes.begin(), poly->meshes.end(),
                             carve::mesh::Mesh<3>::IsClosed())
            << " closed)" << std::endl;

  std::cerr << "closed:    ";
  for (size_t i = 0; i < poly->meshes.size(); ++i) {
    std::cerr << (poly->meshes[i]->isClosed() ? '+' : '-');
  }
  std::cerr << std::endl;

  std::cerr << "negative:  ";
  for (size_t i = 0; i < poly->meshes.size(); ++i) {
    std::cerr << (poly->meshes[i]->isNegative() ? '+' : '-');
  }
  std::cerr << std::endl;

  return poly;
}

int main(int argc, char** argv) {
  options.parse(argc, argv);

  carve::mesh::MeshSet<3>* poly = readModel(options.file);
  if (!poly) {
    exit(1);
  }

  std::vector<carve::mesh::MeshSet<3>::face_t*> out_faces;

  size_t N = 0;
  for (carve::mesh::MeshSet<3>::face_iter i = poly->faceBegin();
       i != poly->faceEnd(); ++i) {
    carve::mesh::MeshSet<3>::face_t* f = *i;
    N += f->nVertices() - 2;
  }
  out_faces.reserve(N);

  for (carve::mesh::MeshSet<3>::face_iter i = poly->faceBegin();
       i != poly->faceEnd(); ++i) {
    carve::mesh::MeshSet<3>::face_t* f = *i;
    std::vector<carve::triangulate::tri_idx> result;

    std::vector<carve::mesh::MeshSet<3>::vertex_t*> vloop;
    f->getVertices(vloop);

    carve::triangulate::triangulate(
        carve::mesh::MeshSet<3>::face_t::projection_mapping(f->project), vloop,
        result);
    if (options.improve) {
      carve::triangulate::improve(
          carve::mesh::MeshSet<3>::face_t::projection_mapping(f->project),
          vloop, carve::mesh::vertex_distance(), result);
    }

    for (size_t j = 0; j < result.size(); ++j) {
      out_faces.push_back(new carve::mesh::MeshSet<3>::face_t(
          vloop[result[j].a], vloop[result[j].b], vloop[result[j].c]));
    }
  }

  carve::mesh::MeshSet<3>* result = new carve::mesh::MeshSet<3>(out_faces);

  if (options.canonicalize) {
    result->canonicalize();
  }

  if (options.obj) {
    writeOBJ(std::cout, result);
  } else if (options.vtk) {
    writeVTK(std::cout, result);
  } else {
    writePLY(std::cout, result, options.ascii);
  }

  delete result;
  delete poly;
}
