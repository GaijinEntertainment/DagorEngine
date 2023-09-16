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

#if !defined(DISABLE_GLU_TRIANGULATOR)
#include "glu_triangulator.hpp"
#endif

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
  bool rescale;
  bool canonicalize;
  bool from_file;
  bool triangulate;
  bool no_holes;
#if !defined(DISABLE_GLU_TRIANGULATOR)
  bool glu_triangulate;
#endif
  bool improve;

  std::vector<std::string> args;

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
    if (o == "--rescale" || o == "-r") {
      rescale = true;
      return;
    }
    if (o == "--triangulate" || o == "-t") {
      triangulate = true;
      return;
    }
    if (o == "--no-holes" || o == "-n") {
      no_holes = true;
      return;
    }
#if !defined(DISABLE_GLU_TRIANGULATOR)
    if (o == "--glu" || o == "-g") {
      glu_triangulate = true;
      return;
    }
#endif
    if (o == "--improve" || o == "-i") {
      improve = true;
      return;
    }
    if (o == "--epsilon" || o == "-E") {
      carve::setEpsilon(strtod(v.c_str(), nullptr));
      return;
    }
    if (o == "--help" || o == "-h") {
      help(std::cout);
      exit(0);
    }
  }

  std::string usageStr() override {
    return std::string("Usage: ") + progname +
           std::string(" [options] object slicer");
  };

  void arg(const std::string& arg) override { args.push_back(arg); }

  Options() {
    ascii = true;
    obj = false;
    vtk = false;
    rescale = false;
    triangulate = false;
    no_holes = false;
#if !defined(DISABLE_GLU_TRIANGULATOR)
    glu_triangulate = false;
#endif
    improve = false;

    option("canonicalize", 'c', false,
           "Canonicalize before output (for comparing output).");
    option("binary", 'b', false, "Produce binary output.");
    option("ascii", 'a', false, "ASCII output (default).");
    option("obj", 'O', false, "Output in .obj format.");
    option("vtk", 'V', false, "Output in .vtk format.");
    option("rescale", 'r', false, "Rescale prior to CSG operations.");
    option("triangulate", 't', false, "Triangulate output.");
    option("no-holes", 'n', false, "Split faces containing holes.");
#if !defined(DISABLE_GLU_TRIANGULATOR)
    option("glu", 'g', false, "Use GLU triangulator.");
#endif
    option("improve", 'i', false,
           "Improve triangulation by minimising internal edge lengths.");
    option("epsilon", 'E', true, "Set epsilon used for calculations.");
    option("help", 'h', false, "This help message.");
  }
};

static Options options;

class Slice : public carve::csg::CSG::Collector {
  Slice();
  Slice(const Slice&);
  Slice& operator=(const Slice&);

 protected:
  struct face_data_t {
    carve::mesh::MeshSet<3>::face_t* face;
    const carve::mesh::MeshSet<3>::face_t* orig_face;
    bool flipped;
    face_data_t(carve::mesh::MeshSet<3>::face_t* _face,
                const carve::mesh::MeshSet<3>::face_t* _orig_face,
                bool _flipped)
        : face(_face), orig_face(_orig_face), flipped(_flipped){};
  };

  std::list<face_data_t> faces;

  const carve::mesh::MeshSet<3>* src_a;
  const carve::mesh::MeshSet<3>* src_b;

 public:
  Slice(const carve::mesh::MeshSet<3>* _src_a,
        const carve::mesh::MeshSet<3>* _src_b)
      : carve::csg::CSG::Collector(), src_a(_src_a), src_b(_src_b) {}

  ~Slice() override {}

 protected:
  void collect(carve::csg::FaceLoopGroup* grp,
               carve::csg::CSG::Hooks& hooks) override {
    if (grp->src == src_a) {
      std::vector<carve::mesh::MeshSet<3>::face_t*> new_faces;

      for (carve::csg::FaceLoop* f = grp->face_loops.head; f; f = f->next) {
        new_faces.clear();
        new_faces.push_back(f->orig_face->create(f->vertices.begin(),
                                                 f->vertices.end(), false));
        hooks.processOutputFace(new_faces, f->orig_face, false);
        for (size_t i = 0; i < new_faces.size(); ++i) {
          faces.push_back(face_data_t(new_faces[i], f->orig_face, false));
        }
      }
    }

    std::list<carve::csg::ClassificationInfo>& cinfo = grp->classification;

    if (cinfo.size() == 0) {
      std::cerr << "WARNING! group " << grp << " has no classification info!"
                << std::endl;
      return;
    }

    bool include = false;

    for (std::list<carve::csg::ClassificationInfo>::const_iterator
             i = grp->classification.begin(),
             e = grp->classification.end();
         i != e; ++i) {
      if ((*i).classification == carve::csg::FACE_IN) {
        if ((*i).intersected_mesh == nullptr ||
            (*i).intersectedMeshIsClosed()) {
          include = true;
          break;
        }
      }
    }

    if (include) {
      std::vector<carve::mesh::MeshSet<3>::face_t*> new_faces;

      for (carve::csg::FaceLoop* f = grp->face_loops.head; f; f = f->next) {
        new_faces.clear();
        new_faces.push_back(f->orig_face->create(f->vertices.begin(),
                                                 f->vertices.end(), false));
        hooks.processOutputFace(new_faces, f->orig_face, false);
        for (size_t i = 0; i < new_faces.size(); ++i) {
          faces.push_back(face_data_t(new_faces[i], f->orig_face, false));
        }

        new_faces.clear();
        new_faces.push_back(
            f->orig_face->create(f->vertices.begin(), f->vertices.end(), true));
        hooks.processOutputFace(new_faces, f->orig_face, true);
        for (size_t i = 0; i < new_faces.size(); ++i) {
          faces.push_back(face_data_t(new_faces[i], f->orig_face, true));
        }
      }
    }
  }

  carve::mesh::MeshSet<3>* done(carve::csg::CSG::Hooks& hooks) override {
    std::vector<carve::mesh::MeshSet<3>::face_t*> f;
    f.reserve(faces.size());
    for (std::list<face_data_t>::iterator i = faces.begin(); i != faces.end();
         ++i) {
      f.push_back((*i).face);
    }

    carve::mesh::MeshSet<3>* p = new carve::mesh::MeshSet<3>(
        f, carve::mesh::MeshOptions().avoid_cavities(true));

    if (hooks.hasHook(carve::csg::CSG::Hooks::RESULT_FACE_HOOK)) {
      for (std::list<face_data_t>::iterator i = faces.begin(); i != faces.end();
           ++i) {
        hooks.resultFace((*i).face, (*i).orig_face, (*i).flipped);
      }
    }

    return p;
  }
};

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

static carve::mesh::MeshSet<3>* load(const std::string& file) {
  carve::input::Input inputs;

  if (file == "-") {
    readPLY(std::cin, inputs);
  } else if (endswith(file, ".ply")) {
    readPLY(file, inputs);
  } else if (endswith(file, ".vtk")) {
    readVTK(file, inputs);
  } else if (endswith(file, ".obj")) {
    readOBJ(file, inputs);
  }

  carve::mesh::MeshSet<3>* poly = nullptr;

  for (std::list<carve::input::Data*>::const_iterator i = inputs.input.begin();
       poly == nullptr && i != inputs.input.end(); ++i) {
    poly = inputs.create<carve::mesh::MeshSet<3> >(*i);
  }

  return poly;
}

int main(int argc, char** argv) {
  std::vector<std::string> tokens;

  options.parse(argc, argv);

  if (options.args.size() != 2) {
    std::cerr << options.usageStr();
    exit(1);
  }

  carve::mesh::MeshSet<3>* object = load(options.args[0]);
  if (object == nullptr) {
    std::cerr << "failed to load object file [" << options.args[0] << "]"
              << std::endl;
    exit(1);
  }

  carve::mesh::MeshSet<3>* planes = load(options.args[1]);
  if (planes == nullptr) {
    std::cerr << "failed to load split plane file [" << options.args[1] << "]"
              << std::endl;
    exit(1);
  }

  carve::mesh::MeshSet<3>* result = nullptr;

  carve::csg::CSG csg;

  if (options.triangulate) {
#if !defined(DISABLE_GLU_TRIANGULATOR)
    if (options.glu_triangulate) {
      csg.hooks.registerHook(new GLUTriangulator,
                             carve::csg::CSG::Hooks::PROCESS_OUTPUT_FACE_BIT);
      if (options.improve) {
        csg.hooks.registerHook(new carve::csg::CarveTriangulationImprover,
                               carve::csg::CSG::Hooks::PROCESS_OUTPUT_FACE_BIT);
      }
    } else {
#endif
      if (options.improve) {
        csg.hooks.registerHook(new carve::csg::CarveTriangulatorWithImprovement,
                               carve::csg::CSG::Hooks::PROCESS_OUTPUT_FACE_BIT);
      } else {
        csg.hooks.registerHook(new carve::csg::CarveTriangulator,
                               carve::csg::CSG::Hooks::PROCESS_OUTPUT_FACE_BIT);
      }
#if !defined(DISABLE_GLU_TRIANGULATOR)
    }
#endif
  } else if (options.no_holes) {
    csg.hooks.registerHook(new carve::csg::CarveHoleResolver,
                           carve::csg::CSG::Hooks::PROCESS_OUTPUT_FACE_BIT);
  }

  Slice slice_collector(object, planes);
  result = csg.compute(object, planes, slice_collector, nullptr,
                       carve::csg::CSG::CLASSIFY_EDGE);

  if (result) {
    result->separateMeshes();

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
  }

  if (object) {
    delete object;
  }
  if (planes) {
    delete planes;
  }
  if (result) {
    delete result;
  }
}
