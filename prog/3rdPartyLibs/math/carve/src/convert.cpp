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

#include "geometry.hpp"
#include "glu_triangulator.hpp"

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

struct Options : public opt::Parser {
  bool ascii;
  bool obj;
  bool vtk;
  bool canonicalize;
  bool triangulate;

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
    if (o == "--triangulate" || o == "-t") {
      triangulate = true;
      return;
    }
    if (o == "--help" || o == "-h") {
      help(std::cout);
      exit(0);
    }
  }

  std::string usageStr() override {
    return std::string("Usage: ") + progname + std::string(" [options] file");
  };

  void arg(const std::string& a) override { file = a; }

  void help(std::ostream& out) override { this->opt::Parser::help(out); }

  Options() {
    ascii = true;
    obj = false;
    vtk = false;
    triangulate = false;

    option("canonicalize", 'c', false,
           "Canonicalize before output (for comparing output).");
    option("binary", 'b', false, "Produce binary output.");
    option("ascii", 'a', false, "ASCII output (default).");
    option("obj", 'O', false, "Output in .obj format.");
    option("vtk", 'V', false, "Output in .vtk format.");
    option("triangulate", 't', false, "Triangulate output.");
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

int main(int argc, char** argv) {
  options.parse(argc, argv);

  carve::input::Input inputs;
  std::vector<carve::mesh::MeshSet<3>*> polys;
  std::vector<carve::line::PolylineSet*> lines;
  std::vector<carve::point::PointSet*> points;

  if (options.file == "") {
    readPLY(std::cin, inputs);
  } else {
    if (endswith(options.file, ".ply")) {
      readPLY(options.file, inputs);
    } else if (endswith(options.file, ".vtk")) {
      readVTK(options.file, inputs);
    } else if (endswith(options.file, ".obj")) {
      readOBJ(options.file, inputs);
    }
  }

  for (std::list<carve::input::Data*>::const_iterator i = inputs.input.begin();
       i != inputs.input.end(); ++i) {
    carve::mesh::MeshSet<3>* p;
    carve::point::PointSet* ps;
    carve::line::PolylineSet* l;

    if ((p = carve::input::Input::create<carve::mesh::MeshSet<3> >(*i)) !=
        nullptr) {
      if (options.canonicalize) {
        p->canonicalize();
      }
      if (options.obj) {
        writeOBJ(std::cout, p);
      } else if (options.vtk) {
        writeVTK(std::cout, p);
      } else {
        writePLY(std::cout, p, options.ascii);
      }
      delete p;
    } else if ((l = carve::input::Input::create<carve::line::PolylineSet>(
                    *i)) != nullptr) {
      if (options.obj) {
        writeOBJ(std::cout, l);
      } else if (options.vtk) {
        writeVTK(std::cout, l);
      } else {
        writePLY(std::cout, l, options.ascii);
      }
      delete l;
    } else if ((ps = carve::input::Input::create<carve::point::PointSet>(*i)) !=
               nullptr) {
      if (options.obj) {
        std::cerr << "Can't write a point set in .obj format" << std::endl;
      } else if (options.vtk) {
        std::cerr << "Can't write a point set in .vtk format" << std::endl;
      } else {
        writePLY(std::cout, ps, options.ascii);
      }
      delete ps;
    }
  }

  return 0;
}
