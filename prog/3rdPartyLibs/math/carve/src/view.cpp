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

#include <carve/carve.hpp>
#include <carve/pointset.hpp>
#include <carve/poly.hpp>
#include <carve/polyline.hpp>
#include <carve/rtree.hpp>

#include "geom_draw.hpp"
#include "opts.hpp"
#include "read_ply.hpp"
#include "scene.hpp"

#include <gloop/gl.hpp>
#include <gloop/glu.hpp>
#include <gloop/glut.hpp>

#include <algorithm>
#include <fstream>
#include <set>
#include <string>
#include <utility>

#include <time.h>

struct Options : public opt::Parser {
  bool wireframe;
  bool normal;
  bool edgeconn;
  bool fit;
  bool obj;
  bool vtk;
  std::vector<std::string> files;

  void optval(const std::string& o, const std::string& v) override {
    if (o == "--obj" || o == "-O") {
      obj = true;
      return;
    }
    if (o == "--vtk" || o == "-V") {
      vtk = true;
      return;
    }
    if (o == "--no-wireframe" || o == "-n") {
      wireframe = false;
      return;
    }
    if (o == "--no-fit" || o == "-f") {
      fit = false;
      return;
    }
    if (o == "--no-normals" || o == "-N") {
      normal = false;
      return;
    }
    if (o == "--no-edgeconn" || o == "-E") {
      edgeconn = false;
      return;
    }
    if (o == "--help" || o == "-h") {
      help(std::cout);
      exit(0);
    }
  }

  void arg(const std::string& a) override { files.push_back(a); }

  Options() {
    obj = false;
    vtk = false;
    fit = true;
    wireframe = true;
    normal = true;
    edgeconn = true;

    option("obj", 'O', false, "Read input in .obj format.");
    option("vtk", 'V', false, "Read input in .vtk format.");
    option("no-wireframe", 'n', false, "Don't display wireframes.");
    option("no-fit", 'f', false, "Don't scale/translate for viewing.");
    option("no-normals", 'N', false, "Don't display normals.");
    option("no-edgeconn", 'E', false, "Don't display edge connections.");
    option("help", 'h', false, "This help message.");
  }
};

static Options options;

bool odd(int x, int y, int z) {
  return ((x + y + z) & 1) == 1;
}

bool even(int x, int y, int z) {
  return ((x + y + z) & 1) == 0;
}

#undef min
#undef max

template <typename data_t>
size_t _treeDepth(carve::geom::RTreeNode<3, data_t>* rtree_node) {
  size_t t = 0;
  for (carve::geom::RTreeNode<3, data_t>* child = rtree_node->child;
       child != NULL; child = child->sibling) {
    t = std::max(t, _treeDepth(child));
  }
  return t + 1;
}

template <typename data_t>
void _drawNode(carve::geom::RTreeNode<3, data_t>* rtree_node, size_t depth,
               size_t depth_max) {
  for (carve::geom::RTreeNode<3, data_t>* child = rtree_node->child;
       child != NULL; child = child->sibling) {
    _drawNode(child, depth + 1, depth_max);
  }

  float r = depth / float(depth_max);
  float H = .7 - r * .2;
  float S = .4 + r * .6;
  float V = .2 + r * .8;
  cRGB col;

  col = HSV2RGB(H, S, V / 2);
  glColor4f(col.r, col.g, col.b, .1 + r * .5);
  glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
  drawCube(rtree_node->bbox.min(), rtree_node->bbox.max());

  // col = HSV2RGB(H, S, V);
  // glColor4f(col.r, col.g, col.b, .1 + r * .25);
  // glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  // drawCube(rtree_node->aabb.min(), rtree_node->aabb.max());
}

template <typename data_t>
void drawTree(carve::geom::RTreeNode<3, data_t>* rtree_node) {
  glDisable(GL_LIGHTING);
  glDisable(GL_CULL_FACE);
  glDepthMask(GL_FALSE);

  size_t d = _treeDepth(rtree_node);

  _drawNode(rtree_node, 0, d);

  glDepthMask(GL_TRUE);
  glEnable(GL_CULL_FACE);
  glEnable(GL_LIGHTING);
}

GLuint genSceneDisplayList(std::vector<carve::mesh::MeshSet<3>*>& polys,
                           std::vector<carve::line::PolylineSet*>& lines,
                           std::vector<carve::point::PointSet*>& points,
                           size_t* listSize, std::vector<bool>& is_wireframe) {
  int n = 0;
  int N = 1;

  is_wireframe.clear();

  if (options.wireframe) {
    N = 2;
  }

  for (size_t p = 0; p < polys.size(); ++p) {
    n += polys[p]->meshes.size() * N + 1;
  }
  for (size_t p = 0; p < lines.size(); ++p) {
    n += lines[p]->lines.size() * 2;
  }
  n += points.size();

  if (n == 0) {
    return 0;
  }

  carve::geom3d::AABB aabb;
  if (polys.size()) {
    aabb = polys[0]->getAABB();
  } else if (lines.size()) {
    aabb = lines[0]->aabb;
  } else if (points.size()) {
    aabb = points[0]->aabb;
  }
  for (size_t p = 0; p < polys.size(); ++p) {
    aabb.unionAABB(polys[p]->getAABB());
  }
  for (size_t p = 0; p < lines.size(); ++p) {
    std::cerr << lines[p]->aabb << std::endl;
    aabb.unionAABB(lines[p]->aabb);
  }
  for (size_t p = 0; p < points.size(); ++p) {
    aabb.unionAABB(points[p]->aabb);
  }

  GLuint dlist = glGenLists((GLsizei)(*listSize = n));
  is_wireframe.resize(n, false);

  double scale_fac = 20.0 / aabb.extent[carve::geom::largestAxis(aabb.extent)];

  if (options.fit) {
    g_translation = -aabb.pos;
    g_scale = scale_fac;
  } else {
    g_translation = carve::geom::VECTOR(0.0, 0.0, 0.0);
    g_scale = 1.0;
  }

  unsigned list_num = 0;

  for (size_t p = 0; p < polys.size(); ++p) {
    carve::mesh::MeshSet<3>* poly = polys[p];
    glEnable(GL_CULL_FACE);
    for (unsigned i = 0; i < poly->meshes.size(); i++) {
      if (!poly->meshes[i]->isClosed()) {
        is_wireframe[list_num] = false;
        glNewList(dlist + list_num++, GL_COMPILE);
        glCullFace(GL_BACK);
        drawMeshSet(poly, 0.3f, 0.8f, 0.5f, 1.0f, i);
        glCullFace(GL_FRONT);
        drawMeshSet(poly, 0.0f, 0.0f, 1.0f, 1.0f, i);
        glCullFace(GL_BACK);
        glEndList();

        if (options.wireframe) {
          is_wireframe[list_num] = true;
          glNewList(dlist + list_num++, GL_COMPILE);
          drawMeshSetWireframe(poly, i, options.normal, options.edgeconn);
          glEndList();
        }
      }
    }

    for (unsigned i = 0; i < poly->meshes.size(); i++) {
      if (poly->meshes[i]->isClosed()) {
        is_wireframe[list_num] = false;
        glNewList(dlist + list_num++, GL_COMPILE);
        glCullFace(GL_BACK);
        drawMeshSet(poly, 0.3f, 0.5f, 0.8f, 1.0f, i);
        glCullFace(GL_FRONT);
        drawMeshSet(poly, 1.0f, 0.0f, 0.0f, 1.0f, i);
        glCullFace(GL_BACK);
        glEndList();

        if (options.wireframe) {
          is_wireframe[list_num] = true;
          glNewList(dlist + list_num++, GL_COMPILE);
          drawMeshSetWireframe(poly, i, options.normal, options.edgeconn);
          glEndList();
        }
      }
    }

    typedef carve::geom::RTreeNode<3, carve::mesh::Face<3>*> face_rtree_t;
    face_rtree_t* tree =
        face_rtree_t::construct_STR(poly->faceBegin(), poly->faceEnd(), 4, 4);
    // face_rtree_t *tree = face_rtree_t::construct_TGS(poly->faceBegin(),
    // poly->faceEnd(), 50, 4);
    is_wireframe[list_num] = true;
    glNewList(dlist + list_num++, GL_COMPILE);
    // drawTree(tree);
    glEndList();
    delete tree;
  }

  for (size_t l = 0; l < lines.size(); ++l) {
    carve::line::PolylineSet* line = lines[l];

    for (carve::line::PolylineSet::line_iter i = line->lines.begin();
         i != line->lines.end(); ++i) {
      is_wireframe[list_num] = false;
      glNewList(dlist + list_num++, GL_COMPILE);
      glBegin((*i)->isClosed() ? GL_LINE_LOOP : GL_LINE_STRIP);
      glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
      for (carve::line::polyline_vertex_iter j = (*i)->vbegin();
           j != (*i)->vend(); ++j) {
        carve::geom3d::Vector v = (*j)->v;
        glVertex3f(g_scale * (v.x + g_translation.x),
                   g_scale * (v.y + g_translation.y),
                   g_scale * (v.z + g_translation.z));
      }
      glEnd();
      glEndList();
      is_wireframe[list_num] = true;
      glNewList(dlist + list_num++, GL_COMPILE);
      glPointSize(4.0);
      glColor4f(1.0f, 1.0f, 0.0f, 1.0f);
      glBegin(GL_POINTS);
      for (carve::line::polyline_vertex_iter j = (*i)->vbegin();
           j != (*i)->vend(); ++j) {
        carve::geom3d::Vector v = (*j)->v;
        glVertex3f(g_scale * (v.x + g_translation.x),
                   g_scale * (v.y + g_translation.y),
                   g_scale * (v.z + g_translation.z));
      }
      glEnd();
      glEndList();
    }
  }

  for (size_t l = 0; l < points.size(); ++l) {
    carve::point::PointSet* point = points[l];

    is_wireframe[list_num] = false;
    glNewList(dlist + list_num++, GL_COMPILE);
    glPointSize(4.0);
    glBegin(GL_POINTS);
    for (size_t i = 0; i < point->vertices.size(); ++i) {
      carve::geom3d::Vector v = point->vertices[i].v;
      glColor4f(0.0f, 1.0f, 1.0f, 1.0f);
      glVertex3f(g_scale * (v.x + g_translation.x),
                 g_scale * (v.y + g_translation.y),
                 g_scale * (v.z + g_translation.z));
    }
    glEnd();
    glEndList();
  }

  return dlist;
}

struct TestScene : public Scene {
  GLuint draw_list_base;
  std::vector<bool> draw_flags;
  std::vector<bool> is_wireframe;

  bool key(unsigned char k, int x, int y) override {
    const char* t;
    static const char* l = "1234567890!@#$%^&*()";
    if (k == '\\') {
      for (unsigned i = 1; i < draw_flags.size(); i += 2) {
        draw_flags[i] = !draw_flags[i];
      }
    } else if (k == '|') {
      for (unsigned i = 1; i < draw_flags.size(); i++) {
        if (is_wireframe[i]) {
          draw_flags[i] = !draw_flags[i];
        }
      }
    } else if (k == 'n') {
      bool n = true;
      for (unsigned i = 0; i < draw_flags.size(); ++i) {
        if (is_wireframe[i] && draw_flags[i]) {
          n = false;
        }
      }
      for (unsigned i = 0; i < draw_flags.size(); ++i) {
        if (is_wireframe[i]) {
          draw_flags[i] = n;
        }
      }
    } else if (k == 'm') {
      bool n = true;
      for (unsigned i = 0; i < draw_flags.size(); ++i) {
        if (!is_wireframe[i] && draw_flags[i]) {
          n = false;
        }
      }
      for (unsigned i = 0; i < draw_flags.size(); ++i) {
        if (!is_wireframe[i]) {
          draw_flags[i] = n;
        }
      }
    } else {
      t = strchr(l, k);
      if (t != nullptr) {
        CARVE_ASSERT(t >= l);
        unsigned layer = t - l;
        if (layer < draw_flags.size()) {
          draw_flags[layer] = !draw_flags[layer];
        }
      }
    }
    return true;
  }

  GLvoid draw() override {
    for (unsigned i = 0; i < draw_flags.size(); ++i) {
      if (draw_flags[i] && !is_wireframe[i]) {
        glCallList(draw_list_base + i);
      }
    }

    GLfloat proj[16];
    glGetFloatv(GL_PROJECTION_MATRIX, proj);
    GLfloat p33 = proj[10];
    proj[10] += 1e-4;
    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf(proj);

    for (unsigned i = 0; i < draw_flags.size(); ++i) {
      if (draw_flags[i] && is_wireframe[i]) {
        glCallList(draw_list_base + i);
      }
    }

    proj[10] = p33;
    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf(proj);
  }

  TestScene(int argc, char** argv, int n_dlist) : Scene(argc, argv) {
    draw_list_base = 0;
  }

  ~TestScene() override { glDeleteLists(draw_list_base, draw_flags.size()); }
};

int main(int argc, char** argv) {
  TestScene* scene = new TestScene(argc, argv, std::min(1, argc - 1));
  options.parse(argc, argv);

  size_t count = 0;

  carve::input::Input inputs;
  std::vector<carve::mesh::MeshSet<3>*> polys;
  std::vector<carve::line::PolylineSet*> lines;
  std::vector<carve::point::PointSet*> points;

  if (options.files.size() == 0) {
    if (options.obj) {
      readOBJ(std::cin, inputs);
    } else if (options.vtk) {
      readVTK(std::cin, inputs);
    } else {
      readPLY(std::cin, inputs);
    }

  } else {
    for (size_t idx = 0; idx < options.files.size(); ++idx) {
      std::string& s(options.files[idx]);
      std::string::size_type i = s.rfind(".");

      if (i != std::string::npos) {
        std::string ext = s.substr(i, s.size() - i);
        if (!strcasecmp(ext.c_str(), ".obj")) {
          readOBJ(s, inputs);
        } else if (!strcasecmp(ext.c_str(), ".vtk")) {
          readVTK(s, inputs);
        } else {
          readPLY(s, inputs);
        }
      } else {
        readPLY(s, inputs);
      }
    }
  }

  for (std::list<carve::input::Data*>::const_iterator i = inputs.input.begin();
       i != inputs.input.end(); ++i) {
    carve::mesh::MeshSet<3>* p;
    carve::point::PointSet* ps;
    carve::line::PolylineSet* l;

    if ((p = carve::input::Input::create<carve::mesh::MeshSet<3> >(
             *i, carve::input::opts("avoid_cavities", "true"))) != nullptr) {
      polys.push_back(p);
      std::cerr << "loaded polyhedron " << polys.back() << " has "
                << polys.back()->meshes.size() << " manifolds ("
                << std::count_if(polys.back()->meshes.begin(),
                                 polys.back()->meshes.end(),
                                 carve::mesh::Mesh<3>::IsClosed())
                << " closed)" << std::endl;

      std::cerr << "closed:    ";
      for (size_t i = 0; i < polys.back()->meshes.size(); ++i) {
        std::cerr << (polys.back()->meshes[i]->isClosed() ? '+' : '-');
      }
      std::cerr << std::endl;

      std::cerr << "negative:  ";
      for (size_t i = 0; i < polys.back()->meshes.size(); ++i) {
        std::cerr << (polys.back()->meshes[i]->isNegative() ? '+' : '-');
      }
      std::cerr << std::endl;

    } else if ((l = carve::input::Input::create<carve::line::PolylineSet>(
                    *i)) != nullptr) {
      lines.push_back(l);
      std::cerr << "loaded polyline set " << lines.back() << std::endl;
    } else if ((ps = carve::input::Input::create<carve::point::PointSet>(*i)) !=
               nullptr) {
      points.push_back(ps);
      std::cerr << "loaded point set " << points.back() << std::endl;
    }
  }

  scene->draw_list_base =
      genSceneDisplayList(polys, lines, points, &count, scene->is_wireframe);
  scene->draw_flags.assign(count, true);

  scene->run();

  delete scene;

  return 0;
}
