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
#include <carve/geom3d.hpp>
#include <carve/poly.hpp>

#include "opts.hpp"
#include "read_ply.hpp"
#include "write_ply.hpp"

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <set>
#include <string>
#include <utility>

struct Options : public opt::Parser {
  bool ascii;
  bool obj;
  bool vtk;
  bool flip;

  double pos;
  enum { ERR = -1, X = 0, Y = 1, Z = 2 } axis;

  std::string file;

  void optval(const std::string& o, const std::string& v) override {
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
    if (o == "--flip" || o == "-f") {
      flip = true;
      return;
    }
    if (o == "-x") {
      axis = X;
      pos = strtod(v.c_str(), nullptr);
    }
    if (o == "-y") {
      axis = Y;
      pos = strtod(v.c_str(), nullptr);
    }
    if (o == "-z") {
      axis = Z;
      pos = strtod(v.c_str(), nullptr);
    }
  }

  std::string usageStr() override {
    return std::string("Usage: ") + progname +
           std::string(" [options] expression");
  };

  void arg(const std::string& a) override {
    if (file == "") {
      file = a;
    }
  }

  void help(std::ostream& out) override { this->opt::Parser::help(out); }

  Options() {
    ascii = true;
    obj = false;
    vtk = false;
    flip = false;
    pos = 0.0;
    axis = ERR;
    file = "";

    option("binary", 'b', false, "Produce binary output.");
    option("ascii", 'a', false, "ASCII output (default).");
    option("obj", 'O', false, "Output in .obj format.");
    option("vtk", 'V', false, "Output in .vtk format.");
    option("flip", 'f', false, "Flip orientation of input faces.");
    option('x', true, "close with plane x={arg}.");
    option('y', true, "close with plane y={arg}.");
    option('z', true, "close with plane z={arg}.");
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

int main(int argc, char** argv) {
  options.parse(argc, argv);
  carve::mesh::MeshSet<3>* poly;

  if (options.axis == Options::ERR) {
    std::cerr << "need to specify a closure plane." << std::endl;
    exit(1);
  }

  if (options.file == "-") {
    poly = readPLYasMesh(std::cin);
  } else if (endswith(options.file, ".ply")) {
    poly = readPLYasMesh(options.file);
  } else if (endswith(options.file, ".vtk")) {
    poly = readVTKasMesh(options.file);
  } else if (endswith(options.file, ".obj")) {
    poly = readOBJasMesh(options.file);
  }

  if (poly == nullptr) {
    std::cerr << "failed to load polyhedron" << std::endl;
    exit(1);
  }

  std::cerr << "poly aabb = " << poly->getAABB() << std::endl;

  if (poly->getAABB().compareAxis(
          carve::geom::axis_pos(options.axis, options.pos)) == 0) {
    std::cerr << "poly aabb intersects closure plane." << std::endl;
    exit(1);
  }

  for (size_t i = 0; i < poly->meshes.size(); ++i) {
    carve::mesh::MeshSet<3>::mesh_t* mesh = poly->meshes[i];
    const size_t N = mesh->open_edges.size();
    if (N == 0) {
      continue;
    }

    mesh->faces.reserve(N + 1);

    carve::mesh::MeshSet<3>::edge_t* start = mesh->open_edges[0];

    std::vector<carve::mesh::MeshSet<3>::edge_t*> edges_to_close;
    edges_to_close.resize(N);
    carve::mesh::MeshSet<3>::edge_t* edge = start;
    size_t j = 0;
    do {
      edges_to_close[j++] = edge;
      edge = edge->perimNext();
    } while (edge != start);

    CARVE_ASSERT(j == N);

    std::vector<carve::mesh::MeshSet<3>::vertex_t> projected;
    projected.resize(N);

    for (j = 0; j < N; ++j) {
      edge = edges_to_close[j];
      projected[j].v = edge->vert->v;
      projected[j].v.v[options.axis] = options.pos;
    }

    for (j = 0; j < N; ++j) {
      edge = edges_to_close[j];
      carve::mesh::MeshSet<3>::face_t* quad =
          new carve::mesh::MeshSet<3>::face_t(
              edge->v2(), edge->v1(), &projected[j], &projected[(j + 1) % N]);
      quad->mesh = mesh;
      edge->rev = quad->edge;
      quad->edge->rev = edge;
      mesh->faces.push_back(quad);
    }

    for (j = 0; j < N; ++j) {
      carve::mesh::MeshSet<3>::edge_t* e1 = edges_to_close[j]->rev->prev;
      carve::mesh::MeshSet<3>::edge_t* e2 =
          edges_to_close[(j + 1) % N]->rev->next;
      e1->rev = e2;
      e2->rev = e1;
    }

    for (j = 0; j < N; ++j) {
      edge = edges_to_close[j]->rev;
      edge->validateLoop();
    }

    carve::mesh::MeshSet<3>::face_t* loop =
        carve::mesh::MeshSet<3>::face_t::closeLoop(
            edges_to_close[0]->rev->next->next);

    loop->mesh = mesh;
    mesh->faces.push_back(loop);

    poly->collectVertices();
  }

  if (options.obj) {
    writeOBJ(std::cout, poly);
  } else if (options.vtk) {
    writeVTK(std::cout, poly);
  } else {
    writePLY(std::cout, poly, options.ascii);
  }

  return 0;
}
