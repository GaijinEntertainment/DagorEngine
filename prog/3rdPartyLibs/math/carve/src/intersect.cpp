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
typedef std::vector<std::string>::iterator TOK;

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
  carve::csg::CSG::CLASSIFY_TYPE classifier;

  std::string stream;

  void _read(std::istream& in) {
    while (in.good()) {
      char buf[1024];
      in.read(buf, 1024);
      size_t sz = in.gcount();
      if (sz) {
        stream.append(buf, sz);
      }
    }
  }

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
    if (o == "--edge" || o == "-e") {
      classifier = carve::csg::CSG::CLASSIFY_EDGE;
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
    if (o == "--file" || o == "-f") {
      from_file = true;
      if (v == "-") {
        _read(std::cin);
      } else {
        std::ifstream in(v.c_str());
        _read(in);
      }
      return;
    }
  }

  std::string usageStr() override {
    return std::string("Usage: ") + progname +
           std::string(" [options] expression");
  };

  void arg(const std::string& a) override {
    if (from_file) {
      std::cerr << "Can't mix command line arguments and -f" << std::endl;
      exit(1);
    }
    if (stream.size()) {
      stream += " ";
    }
    stream += a;
  }

  void help(std::ostream& out) override {
    this->opt::Parser::help(out);
    out << std::endl;
    out << "expression is an infix expression:" << std::endl;
    out << std::endl;
    out << "For objects A and B, the following operators are defined"
        << std::endl;
    out << "  A | B      A UNION B                - CSG union of A and B."
        << std::endl;
    out << "  A & B      A INTERSECTION B         - CSG intersection of A and "
           "B."
        << std::endl;
    out << "  A ^ B      A SYMMETRIC_DIFFERENCE B - CSG symmetric difference "
           "of A and B."
        << std::endl;
    out << "             A A_MINUS_B B            - CSG subtraction of B from "
           "a."
        << std::endl;
    out << "             A B_MINUS_A B            - CSG subtraction of A from "
           "B."
        << std::endl;
    out << std::endl;
    out << "an object can be any of the following:" << std::endl;
    out << "  CUBE                                - a cube from {-1,-1,-1} to "
           "{+1,+1,+1}."
        << std::endl;
    out << "  TORUS(slices, rings, rad1, rad2)    - a torus defined by the "
           "provided."
        << std::endl;
    out << "                                        parameters." << std::endl;
    out << "  file.ply                            - an object in stanford .ply "
           "format."
        << std::endl;
    out << "  file.obj                            - an object in wavefront "
           ".obj format."
        << std::endl;
    out << "  (expression)                        - a subexpression to be "
           "evaluated."
        << std::endl;
    out << std::endl;
    out << "an object, A, may be operated on by the following functions:"
        << std::endl;
    out << "  SCALE(x, y, z, A)                   - scaling of A by {x,y,z}."
        << std::endl;
    out << "  TRANS(x, y, z, A)                   - translation of A by "
           "{x,y,z}."
        << std::endl;
    out << "  ROT(a, x, y, z, A)                  - rotation of about the "
           "axis{x,y,z}"
        << std::endl;
    out << "                                        by a radians." << std::endl;
    out << "  FLIP(A)                             - normal-flipped A."
        << std::endl;
    out << "  SELECT(i, A)                        - selection of manifold i "
           "from A."
        << std::endl;
    out << std::endl;
    out << "examples:" << std::endl;
    out << "  CUBE & ROT(0.78539816339744828,1,1,1,CUBE)" << std::endl;
    out << "  data/cylinderx.ply | data/cylindery.ply | data/cylinderz.ply"
        << std::endl;
  }

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
    classifier = carve::csg::CSG::CLASSIFY_NORMAL;

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
    option("edge", 'e', false, "Use edge classifier.");
    option("epsilon", 'E', true, "Set epsilon used for calculations.");
    option("file", 'f', true, "Read CSG expression from file.");
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

bool charTok(char ch) {
  return strchr("()|&^,-:", ch) != nullptr;
}

bool beginsNumber(char ch) {
  return strchr("+-0123456789.", ch) != nullptr;
}

bool STRTOD(const std::string& str, double& v) {
  char* ptr;
  v = strtod(str.c_str(), &ptr);
  return *ptr == 0;
}

bool STRTOUL(const std::string& str, unsigned long& v) {
  char* ptr;
  v = strtoul(str.c_str(), &ptr, 0);
  return *ptr == 0;
}

std::vector<std::string> tokenize(const std::string& stream) {
  size_t i = 0;
  std::string token;
  std::vector<std::string> result;
  while (i < stream.size()) {
    if (isspace(stream[i])) {
      ++i;
      continue;
    }

    token = "";

    if (beginsNumber(stream[i])) {
      char* t;
      strtod(stream.c_str() + i, &t);
      if (t != stream.c_str() + i) {
        token = stream.substr(i, t - stream.c_str() - i);
        result.push_back(token);
        i += token.size();
        continue;
      }
    }

    if (charTok(stream[i])) {
      token = stream.substr(i, 1);
      result.push_back(token);
      ++i;
      continue;
    }

    if (stream[i] == '"' || stream[i] == '\'') {
      char close = stream[i++];
      while (i < stream.size() && stream[i] != close) {
        if (stream[i] == '\\') {
          if (++i == stream.size()) {
            std::cerr << "unterminated escape" << std::endl;
            exit(1);
          }
        }
        token.push_back(stream[i++]);
      }
      if (i == stream.size()) {
        std::cerr << "unterminated string" << std::endl;
        exit(1);
      }
      ++i;
      result.push_back(token);
      continue;
    }

    // not space, single char, number, or quoted string.
    // generally this will be a function name, or a file name.
    // will extend to the next (unescaped) space, single char, or quote.

    while (i < stream.size()) {
      if (stream[i] == '\\') {
        if (++i == stream.size()) {
          std::cerr << "unterminated escape" << std::endl;
          exit(1);
        }
      } else {
        if (isspace(stream[i])) {
          break;
        }
        if (charTok(stream[i])) {
          break;
        }
        if (stream[i] == '"' || stream[i] == '\'') {
          break;
        }
      }
      token += stream[i++];
    }
    result.push_back(token);
  }

  return result;
}

carve::csg::CSG_TreeNode* parseBracketExpr(TOK& tok);
carve::csg::CSG_TreeNode* parseAtom(TOK& tok);
carve::csg::CSG_TreeNode* parseTransform(TOK& tok);
carve::csg::CSG_TreeNode* parseExpr(TOK& tok);

bool parseOP(TOK& tok, carve::csg::CSG::OP& op);

carve::csg::CSG_TreeNode* parseBracketExpr(TOK& tok) {
  carve::csg::CSG_TreeNode* result;
  if (*tok != "(") {
    return nullptr;
  }
  ++tok;
  result = parseExpr(tok);
  if (result == nullptr || *tok != ")") {
    return nullptr;
  }
  ++tok;
  return result;
}

carve::csg::CSG_TreeNode* parseAtom(TOK& tok) {
  if (*tok == "(") {
    return parseBracketExpr(tok);
  } else {
    carve::mesh::MeshSet<3>* poly = nullptr;

    if (*tok == "CUBE") {
      poly = makeCube();
    } else if (*tok == "CONE") {
      unsigned long slices;
      double rad, height;
      ++tok;
      if (*tok != "(") {
        return nullptr;
      }
      ++tok;
      if (!STRTOUL(*tok, slices)) {
        return nullptr;
      }
      ++tok;
      if (*tok != ",") {
        return nullptr;
      }
      ++tok;
      if (!STRTOD(*tok, rad)) {
        return nullptr;
      }
      ++tok;
      if (*tok != ",") {
        return nullptr;
      }
      ++tok;
      if (!STRTOD(*tok, height)) {
        return nullptr;
      }
      ++tok;
      if (*tok != ")") {
        return nullptr;
      }
      poly = makeCone(slices, rad, height);
    } else if (*tok == "CYLINDER") {
      unsigned long slices;
      double rad, height;
      ++tok;
      if (*tok != "(") {
        return nullptr;
      }
      ++tok;
      if (!STRTOUL(*tok, slices)) {
        return nullptr;
      }
      ++tok;
      if (*tok != ",") {
        return nullptr;
      }
      ++tok;
      if (!STRTOD(*tok, rad)) {
        return nullptr;
      }
      ++tok;
      if (*tok != ",") {
        return nullptr;
      }
      ++tok;
      if (!STRTOD(*tok, height)) {
        return nullptr;
      }
      ++tok;
      if (*tok != ")") {
        return nullptr;
      }
      poly = makeCylinder(slices, rad, height);
    } else if (*tok == "TORUS") {
      unsigned long slices, rings;
      double rad1, rad2;
      ++tok;
      if (*tok != "(") {
        return nullptr;
      }
      ++tok;
      if (!STRTOUL(*tok, slices)) {
        return nullptr;
      }
      ++tok;
      if (*tok != ",") {
        return nullptr;
      }
      ++tok;
      if (!STRTOUL(*tok, rings)) {
        return nullptr;
      }
      ++tok;
      if (*tok != ",") {
        return nullptr;
      }
      ++tok;
      if (!STRTOD(*tok, rad1)) {
        return nullptr;
      }
      ++tok;
      if (*tok != ",") {
        return nullptr;
      }
      ++tok;
      if (!STRTOD(*tok, rad2)) {
        return nullptr;
      }
      ++tok;
      if (*tok != ")") {
        return nullptr;
      }
      poly = makeTorus(slices, rings, rad1, rad2);
    } else if (endswith(*tok, ".ply")) {
      poly = readPLYasMesh(*tok);
    } else if (endswith(*tok, ".vtk")) {
      poly = readVTKasMesh(*tok);
    } else if (endswith(*tok, ".obj")) {
      poly = readOBJasMesh(*tok);
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

    ++tok;
    return new carve::csg::CSG_PolyNode(poly, true);
  }
}

carve::csg::CSG_TreeNode* parseTransform(TOK& tok) {
  carve::csg::CSG_TreeNode* result;
  double ang, x, y, z;
  if (*tok == "FLIP") {
    ++tok;
    if (*tok != "(") {
      return nullptr;
    }
    ++tok;
    carve::csg::CSG_TreeNode* child = parseTransform(tok);
    if (*tok != ")") {
      delete child;
      return nullptr;
    }
    ++tok;

    result = new carve::csg::CSG_InvertNode(child);
  } else if (*tok == "SELECT") {
    unsigned long id, id2;
    std::set<unsigned long> sel_ids;

    ++tok;
    if (*tok != "(") {
      return nullptr;
    }
    ++tok;
    while (1) {
      if (!STRTOUL(*tok, id)) {
        break;
      }
      ++tok;
      if (*tok == ":") {
        ++tok;
        if (!STRTOUL(*tok, id2)) {
          return nullptr;
        }
        ++tok;
        if (*tok != ",") {
          return nullptr;
        }
        ++tok;
        while (id <= id2) {
          sel_ids.insert(id++);
        }
      } else {
        if (*tok != ",") {
          return nullptr;
        }
        ++tok;
        sel_ids.insert(id);
      }
    }

    carve::csg::CSG_TreeNode* child = parseTransform(tok);
    if (child == nullptr) {
      return nullptr;
    }

    if (*tok != ")") {
      delete child;
      return nullptr;
    }
    ++tok;

    result =
        new carve::csg::CSG_SelectNode(sel_ids.begin(), sel_ids.end(), child);
  } else if (*tok == "ROT") {
    bool deg = false;
    ++tok;
    if (*tok != "(") {
      return nullptr;
    }
    ++tok;
    if (!STRTOD(*tok, ang)) {
      return nullptr;
    }
    ++tok;
    if (*tok == "deg") {
      deg = true;
      ++tok;
    }
    if (*tok != ",") {
      return nullptr;
    }
    ++tok;
    if (!STRTOD(*tok, x)) {
      return nullptr;
    }
    ++tok;
    if (*tok != ",") {
      return nullptr;
    }
    ++tok;
    if (!STRTOD(*tok, y)) {
      return nullptr;
    }
    ++tok;
    if (*tok != ",") {
      return nullptr;
    }
    ++tok;
    if (!STRTOD(*tok, z)) {
      return nullptr;
    }
    ++tok;
    if (*tok != ",") {
      return nullptr;
    }
    ++tok;

    carve::csg::CSG_TreeNode* child = parseTransform(tok);
    if (child == nullptr) {
      return nullptr;
    }

    if (*tok != ")") {
      delete child;
      return nullptr;
    }
    ++tok;
    if (deg) {
      ang *= M_PI / 180.0;
    }
    result = new carve::csg::CSG_TransformNode(
        carve::math::Matrix::ROT(ang, x, y, z), child);
  } else if (*tok == "TRANS") {
    ++tok;
    if (*tok != "(") {
      return nullptr;
    }
    ++tok;
    if (!STRTOD(*tok, x)) {
      return nullptr;
    }
    ++tok;
    if (*tok != ",") {
      return nullptr;
    }
    ++tok;
    if (!STRTOD(*tok, y)) {
      return nullptr;
    }
    ++tok;
    if (*tok != ",") {
      return nullptr;
    }
    ++tok;
    if (!STRTOD(*tok, z)) {
      return nullptr;
    }
    ++tok;
    if (*tok != ",") {
      return nullptr;
    }
    ++tok;

    carve::csg::CSG_TreeNode* child = parseTransform(tok);
    if (child == nullptr) {
      return nullptr;
    }

    if (*tok != ")") {
      delete child;
      return nullptr;
    }
    ++tok;
    result = new carve::csg::CSG_TransformNode(
        carve::math::Matrix::TRANS(x, y, z), child);
  } else if (*tok == "SCALE") {
    ++tok;
    if (*tok != "(") {
      return nullptr;
    }
    ++tok;
    if (!STRTOD(*tok, x)) {
      return nullptr;
    }
    ++tok;
    if (*tok != ",") {
      return nullptr;
    }
    ++tok;
    if (!STRTOD(*tok, y)) {
      return nullptr;
    }
    ++tok;
    if (*tok != ",") {
      return nullptr;
    }
    ++tok;
    if (!STRTOD(*tok, z)) {
      return nullptr;
    }
    ++tok;
    if (*tok != ",") {
      return nullptr;
    }
    ++tok;

    carve::csg::CSG_TreeNode* child = parseTransform(tok);
    if (child == nullptr) {
      return nullptr;
    }

    if (*tok != ")") {
      delete child;
      return nullptr;
    }
    ++tok;
    result = new carve::csg::CSG_TransformNode(
        carve::math::Matrix::SCALE(x, y, z), child);
  } else {
    result = parseAtom(tok);
  }
  return result;
}

bool parseOP(TOK& tok, carve::csg::CSG::OP& op) {
  if (*tok == "INTERSECTION" || *tok == "&") {
    op = carve::csg::CSG::INTERSECTION;
  } else if (*tok == "UNION" || *tok == "|") {
    op = carve::csg::CSG::UNION;
  } else if (*tok == "A_MINUS_B" || *tok == "-") {
    op = carve::csg::CSG::A_MINUS_B;
  } else if (*tok == "B_MINUS_A") {
    op = carve::csg::CSG::B_MINUS_A;
  } else if (*tok == "SYMMETRIC_DIFFERENCE" || *tok == "^") {
    op = carve::csg::CSG::SYMMETRIC_DIFFERENCE;
  } else {
    return false;
  }
  ++tok;
  return true;
}

carve::csg::CSG_TreeNode* parseExpr(TOK& tok) {
  carve::csg::CSG_TreeNode* lhs = parseTransform(tok);
  carve::csg::CSG::OP op;
  if (lhs == nullptr) {
    return nullptr;
  }

  while (parseOP(tok, op)) {
    carve::csg::CSG_TreeNode* rhs = parseTransform(tok);
    if (rhs == nullptr) {
      delete lhs;
      return nullptr;
    }
    lhs = new carve::csg::CSG_OPNode(lhs, rhs, op, options.rescale,
                                     options.classifier);
  }
  return lhs;
}

carve::csg::CSG_TreeNode* parse(TOK& tok) {
  carve::csg::CSG_TreeNode* result = parseExpr(tok);
  if (result == nullptr || *tok != "$") {
    return nullptr;
  }
  return result;
}

int main(int argc, char** argv) {
  static carve::TimingName MAIN_BLOCK("Application");
  static carve::TimingName PARSE_BLOCK("Parse");
  static carve::TimingName EVAL_BLOCK("Eval");
  static carve::TimingName WRITE_BLOCK("Write");

  carve::Timing::start(MAIN_BLOCK);

  double duration;
  std::vector<std::string> tokens;

  options.parse(argc, argv);

  tokens = tokenize(options.stream);
  tokens.push_back("$");

  carve::Timing::start(PARSE_BLOCK);
  TOK tok = tokens.begin();
  carve::csg::CSG_TreeNode* p = parse(tok);
  duration = carve::Timing::stop();

  std::cerr << "Parse time " << duration << " seconds" << std::endl;

  if (p != nullptr) {
    carve::Timing::start(EVAL_BLOCK);
    carve::mesh::MeshSet<3>* result = nullptr;

    try {
      carve::csg::CSG csg;

      if (options.triangulate) {
#if !defined(DISABLE_GLU_TRIANGULATOR)
        if (options.glu_triangulate) {
          csg.hooks.registerHook(
              new GLUTriangulator,
              carve::csg::CSG::Hooks::PROCESS_OUTPUT_FACE_BIT);
          if (options.improve) {
            csg.hooks.registerHook(
                new carve::csg::CarveTriangulationImprover,
                carve::csg::CSG::Hooks::PROCESS_OUTPUT_FACE_BIT);
          }
        } else {
#endif
          if (options.improve) {
            csg.hooks.registerHook(
                new carve::csg::CarveTriangulatorWithImprovement,
                carve::csg::CSG::Hooks::PROCESS_OUTPUT_FACE_BIT);
          } else {
            csg.hooks.registerHook(
                new carve::csg::CarveTriangulator,
                carve::csg::CSG::Hooks::PROCESS_OUTPUT_FACE_BIT);
          }
#if !defined(DISABLE_GLU_TRIANGULATOR)
        }
#endif
      } else if (options.no_holes) {
        csg.hooks.registerHook(new carve::csg::CarveHoleResolver,
                               carve::csg::CSG::Hooks::PROCESS_OUTPUT_FACE_BIT);
      }

      result = p->eval(csg);
    } catch (carve::exception e) {
      std::cerr << "CSG failed, exception: " << e.str() << std::endl;
    }
    duration = carve::Timing::stop();
    std::cerr << "Eval time " << duration << " seconds" << std::endl;

    carve::Timing::start(WRITE_BLOCK);
    if (result) {
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
    duration = carve::Timing::stop();
    std::cerr << "Output time " << duration << " seconds" << std::endl;

    {
      static carve::TimingName FUNC_NAME("Main app delete polyhedron");
      carve::TimingBlock block(FUNC_NAME);

      delete p;
      if (result) {
        delete result;
      }
    }
  } else {
    std::cerr << "syntax error at [" << *tok << "]" << std::endl;
  }

  carve::Timing::stop();

  carve::Timing::printTimings();
}
