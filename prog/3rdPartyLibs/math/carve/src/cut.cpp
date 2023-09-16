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
typedef std::vector<std::string>::iterator TOK;

struct Options : public opt::Parser {
  bool ascii;
  bool rescale;

  std::vector<std::string> args;

  void optval(const std::string& o, const std::string& v) override {
    if (o == "--binary" || o == "-b") {
      ascii = false;
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
           std::string(" [options] expression");
  };

  void arg(const std::string& a) override { args.push_back(a); }

  void help(std::ostream& out) override { this->opt::Parser::help(out); }

  Options() {
    ascii = true;
    rescale = false;

    option("binary", 'b', false, "Produce binary output.");
    option("ascii", 'a', false, "ASCII output (default).");
    option("rescale", 'r', false, "Rescale prior to CSG operations.");
    option("epsilon", 'E', true, "Set epsilon used for calculations.");
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

carve::mesh::MeshSet<3>* read(const std::string& s) {
  if (endswith(s, ".vtk")) {
    return readVTKasMesh(s);
  } else if (endswith(s, ".obj")) {
    return readOBJasMesh(s);
  } else {
    return readPLYasMesh(s);
  }
}

int main(int argc, char** argv) {
  options.parse(argc, argv);
  if (options.args.size() != 2) {
    std::cerr << "expected exactly two arguments" << std::endl;
    exit(1);
  }

  carve::mesh::MeshSet<3> *a, *b;
  a = read(options.args[0]);
  if (!a) {
    std::cerr << "failed to read [" << options.args[0] << "]" << std::endl;
    exit(1);
  }
  b = read(options.args[1]);
  if (!b) {
    std::cerr << "failed to read [" << options.args[1] << "]" << std::endl;
    exit(1);
  }

  std::list<carve::mesh::MeshSet<3> *> a_sliced, b_sliced;
  carve::csg::V2Set shared_edges;
  carve::csg::CSG csg;

  csg.slice(a, b, a_sliced, b_sliced, &shared_edges);
  std::cerr << "result: " << a_sliced.size() << " connected components from a"
            << std::endl;
  std::cerr << "      : " << b_sliced.size() << " connected components from b"
            << std::endl;
  std::cerr << "      : " << shared_edges.size()
            << " edges in the line of intersection" << std::endl;

  typedef std::unordered_map<
      const carve::mesh::MeshSet<3>::vertex_t*,
      std::set<const carve::mesh::MeshSet<3>::vertex_t*> >
      VVSMap;

  VVSMap edge_graph;

  for (carve::csg::V2Set::const_iterator i = shared_edges.begin();
       i != shared_edges.end(); ++i) {
    edge_graph[(*i).first].insert((*i).second);
    edge_graph[(*i).second].insert((*i).first);
  }

  for (VVSMap::const_iterator i = edge_graph.begin(); i != edge_graph.end();
       ++i) {
    if ((*i).second.size() > 2) {
      std::cerr << "branch at: " << (*i).first << std::endl;
    }
    if ((*i).second.size() == 1) {
      std::cerr << "endpoint at: " << (*i).first << std::endl;
      std::cerr << "coordinate: " << (*i).first->v << std::endl;
    }
  }

  {
    carve::line::PolylineSet intersection_graph;
    intersection_graph.vertices.resize(edge_graph.size());
    std::map<const carve::mesh::MeshSet<3>::vertex_t*, size_t> vmap;

    size_t j = 0;
    for (VVSMap::const_iterator i = edge_graph.begin(); i != edge_graph.end();
         ++i) {
      intersection_graph.vertices[j].v = (*i).first->v;
      vmap[(*i).first] = j++;
    }

    while (edge_graph.size()) {
      VVSMap::iterator prior_i = edge_graph.begin();
      const carve::mesh::MeshSet<3>::vertex_t* prior = (*prior_i).first;
      std::vector<size_t> connected;
      connected.push_back(vmap[prior]);
      while (prior_i != edge_graph.end() && (*prior_i).second.size()) {
        const carve::mesh::MeshSet<3>::vertex_t* next =
            *(*prior_i).second.begin();
        VVSMap::iterator next_i = edge_graph.find(next);
        assert(next_i != edge_graph.end());
        connected.push_back(vmap[next]);
        (*prior_i).second.erase(next);
        (*next_i).second.erase(prior);
        if (!(*prior_i).second.size()) {
          edge_graph.erase(prior);
          prior = nullptr;
          prior_i = edge_graph.end();
        }
        if (!(*next_i).second.size()) {
          edge_graph.erase(next);
          next = nullptr;
          next_i = edge_graph.end();
        }
        prior_i = next_i;
        prior = next;
      }
      bool closed = connected.front() == connected.back();
      for (size_t k = 0; k < connected.size(); ++k) {
        std::cerr << " " << connected[k];
      }
      std::cerr << std::endl;
      intersection_graph.addPolyline(closed, connected.begin(),
                                     connected.end());
    }

    writePLY(std::cout, &intersection_graph, true);
  }
}
