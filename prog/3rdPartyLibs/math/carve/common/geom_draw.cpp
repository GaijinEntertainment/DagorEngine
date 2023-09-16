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

#include "geom_draw.hpp"

#include <carve/debug_hooks.hpp>
#include <carve/poly.hpp>

#include <gloop/gl.hpp>
#include <gloop/glu.hpp>
#include <gloop/glut.hpp>

#include <fstream>
#include <string>

#include <time.h>

#if defined(__GNUC__)
#define __stdcall
#endif

#if defined(GLU_TESS_CALLBACK_VARARGS)
typedef GLvoid(_stdcall* GLUTessCallback)(...);
#else
typedef void(__stdcall* GLUTessCallback)();
#endif

carve::geom3d::Vector g_translation;
double g_scale = 1.0;

static inline void glVertex(double x, double y, double z) {
  glVertex3f(g_scale * (x + g_translation.x), g_scale * (y + g_translation.y),
             g_scale * (z + g_translation.z));
}

static inline void glVertex(const carve::geom3d::Vector* v) {
  glVertex3f(g_scale * (v->x + g_translation.x),
             g_scale * (v->y + g_translation.y),
             g_scale * (v->z + g_translation.z));
}

static inline void glVertex(const carve::geom3d::Vector& v) {
  glVertex3f(g_scale * (v.x + g_translation.x),
             g_scale * (v.y + g_translation.y),
             g_scale * (v.z + g_translation.z));
}

static inline void glVertex(const carve::mesh::Vertex<3>* v) {
  glVertex(v->v);
}

static inline void glVertex(const carve::mesh::Vertex<3>& v) {
  glVertex(v.v);
}

static inline void glVertex(const carve::poly::Vertex<3>* v) {
  glVertex(v->v);
}

static inline void glVertex(const carve::poly::Vertex<3>& v) {
  glVertex(v.v);
}

class DebugHooks : public carve::csg::IntersectDebugHooks {
 public:
  void drawIntersections(const carve::csg::VertexIntersections& vint) override;

  virtual void drawOctree(const carve::csg::Octree& o);

  virtual void drawPoint(const carve::geom3d::Vector* v, float r, float g,
                         float b, float a, float rad);

  virtual void drawEdge(const carve::geom3d::Vector* v1,
                        const carve::geom3d::Vector* v2, float rA, float gA,
                        float bA, float aA, float rB, float gB, float bB,
                        float aB, float thickness = 1.0);

  virtual void drawFaceLoopWireframe(
      const std::vector<const carve::geom3d::Vector*>& face_loop,
      const carve::geom3d::Vector& normal, float r, float g, float b, float a,
      bool inset = true);

  virtual void drawFaceLoop(
      const std::vector<const carve::geom3d::Vector*>& face_loop,
      const carve::geom3d::Vector& normal, float r, float g, float b, float a,
      bool lit = true);

  virtual void drawFaceLoop2(
      const std::vector<const carve::geom3d::Vector*>& face_loop,
      const carve::geom3d::Vector& normal, float rF, float gF, float bF,
      float aF, float rB, float gB, float bB, float aB, bool lit = true);
};

void DebugHooks::drawPoint(const carve::geom3d::Vector* v, float r, float g,
                           float b, float a, float rad) {
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_LIGHTING);
  glEnable(GL_POINT_SMOOTH);
  glPointSize(rad);
  glBegin(GL_POINTS);
  glColor4f(r, g, b, a);
  glVertex(v);
  glEnd();
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_LIGHTING);
}

void DebugHooks::drawEdge(const carve::geom3d::Vector* v1,
                          const carve::geom3d::Vector* v2, float rA, float gA,
                          float bA, float aA, float rB, float gB, float bB,
                          float aB, float thickness) {
  glDisable(GL_LIGHTING);
  glDisable(GL_DEPTH_TEST);
  glLineWidth(thickness);
  glEnable(GL_LINE_SMOOTH);

  glBegin(GL_LINES);

  glColor4f(rA, gA, bA, aA);
  glVertex(v1);

  glColor4f(rB, gB, bB, aB);
  glVertex(v2);

  glEnd();

  glDisable(GL_LINE_SMOOTH);
  glLineWidth(1.0);
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_LIGHTING);
}

void DebugHooks::drawIntersections(
    const carve::csg::VertexIntersections& vint) {
  glEnable(GL_POINT_SMOOTH);
  glDisable(GL_DEPTH_TEST);
  for (carve::csg::VertexIntersections::const_iterator i = vint.begin(),
                                                       e = vint.end();
       i != e; ++i) {
    float sz = 4.0 + (*i).second.size() * 3.0;
    for (carve::csg::VertexIntersections::mapped_type::const_iterator
             j = (*i).second.begin(),
             je = (*i).second.end();
         j != je; ++j) {
      glPointSize(sz);
      sz -= 3.0;
      switch ((*j).first.obtype | (*j).second.obtype) {
        case 0:
          glColor4f(0, 0, 0, 1);
          break;
        case 1:
          glColor4f(0, 0, 1, 1);
          break;  // VERTEX - VERTEX
        case 2:
          glColor4f(0, 1, 1, 1);
          break;  // EDGE - EDGE
        case 3:
          glColor4f(1, 0, 0, 1);
          break;  // EDGE - VERTEX
        case 4:
          glColor4f(0, 0, 0, 1);
          break;
        case 5:
          glColor4f(1, 1, 0, 1);
          break;  // FACE - VERTEX
        case 6:
          glColor4f(0, 1, 0, 1);
          break;  // FACE - EDGE
        case 7:
          glColor4f(0, 0, 0, 1);
          break;
      }
      glBegin(GL_POINTS);
      glVertex((*i).first);
      glEnd();
    }
  }
  glEnable(GL_DEPTH_TEST);
}

carve::geom3d::Vector sphereAng(double elev, double ang) {
  return carve::geom::VECTOR(cos(ang) * sin(elev), cos(elev),
                             sin(ang) * sin(elev));
}

#define N_STACKS 32
#define N_SLICES 32

void emitSphereCoord(const carve::geom::vector<3>& c, double r, unsigned stack,
                     unsigned slice) {
  carve::geom::vector<3> v;

  v = sphereAng(stack * M_PI / N_STACKS, slice * 2 * M_PI / N_SLICES);

  glNormal3dv(v.v);
  glVertex(c + r * v);
}

void drawSphere(const carve::geom3d::Vector& c, double r) {
  glBegin(GL_TRIANGLES);
  for (unsigned slice = 0; slice < N_SLICES; ++slice) {
    emitSphereCoord(c, r, 0, 0);
    emitSphereCoord(c, r, 1, slice + 1);
    emitSphereCoord(c, r, 1, slice);
  }
  for (unsigned stack = 1; stack < N_STACKS - 1; ++stack) {
    for (unsigned slice = 0; slice < N_SLICES; ++slice) {
      emitSphereCoord(c, r, stack, slice);
      emitSphereCoord(c, r, stack, slice + 1);
      emitSphereCoord(c, r, stack + 1, slice);
      emitSphereCoord(c, r, stack, slice + 1);
      emitSphereCoord(c, r, stack + 1, slice + 1);
      emitSphereCoord(c, r, stack + 1, slice);
    }
  }
  for (unsigned slice = 0; slice < N_SLICES; ++slice) {
    emitSphereCoord(c, r, N_STACKS - 1, slice);
    emitSphereCoord(c, r, N_STACKS - 1, slice + 1);
    emitSphereCoord(c, r, N_STACKS, 0);
  }
  glEnd();
}

void drawSphere(const carve::geom::sphere<3>& sphere) {
  drawSphere(sphere.C, sphere.r);
}

void drawTri(const carve::geom::tri<3>& tri) {
  carve::geom::vector<3> n = tri.normal();
  glBegin(GL_TRIANGLES);
  glNormal3dv(n.v);
  glVertex(tri.v[0]);
  glVertex(tri.v[1]);
  glVertex(tri.v[2]);
  glNormal3dv((-n).v);
  glVertex(tri.v[0]);
  glVertex(tri.v[2]);
  glVertex(tri.v[1]);
  glEnd();
}

void drawCube(const carve::geom3d::Vector& a, const carve::geom3d::Vector& b) {
  glBegin(GL_QUADS);
  glNormal3f(0, 0, -1);
  glVertex(a.x, a.y, a.z);
  glVertex(b.x, a.y, a.z);
  glVertex(b.x, b.y, a.z);
  glVertex(a.x, b.y, a.z);

  glNormal3f(0, 0, 1);
  glVertex(a.x, b.y, b.z);
  glVertex(b.x, b.y, b.z);
  glVertex(b.x, a.y, b.z);
  glVertex(a.x, a.y, b.z);

  glNormal3f(-1, 0, 0);
  glVertex(a.x, a.y, a.z);
  glVertex(a.x, b.y, a.z);
  glVertex(a.x, b.y, b.z);
  glVertex(a.x, a.y, b.z);

  glNormal3f(1, 0, 0);
  glVertex(b.x, a.y, b.z);
  glVertex(b.x, b.y, b.z);
  glVertex(b.x, b.y, a.z);
  glVertex(b.x, a.y, a.z);

  glNormal3f(0, -1, 0);
  glVertex(a.x, a.y, a.z);
  glVertex(b.x, a.y, a.z);
  glVertex(b.x, a.y, b.z);
  glVertex(a.x, a.y, b.z);

  glNormal3f(0, 1, 0);

  glVertex(a.x, b.y, b.z);
  glVertex(b.x, b.y, b.z);
  glVertex(b.x, b.y, a.z);
  glVertex(a.x, b.y, a.z);

  glEnd();
}

static void drawCell(int /* level */, carve::csg::Octree::Node* node) {
  // we only want to draw leaf nodes
  if (!node->hasChildren() && node->hasGeometry()) {
    glColor3f(1, 0, 0);
    drawCube(node->min, node->max);
  }
}

void drawOctree(const carve::csg::Octree& o) {
  glDisable(GL_LIGHTING);
  glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
  glDisable(GL_CULL_FACE);

  o.iterateNodes(&drawCell);

  glEnable(GL_LIGHTING);
}

void DebugHooks::drawOctree(const carve::csg::Octree& o) {
  ::drawOctree(o);
}

static void __stdcall _faceBegin(GLenum type, void* data) {
  carve::mesh::Face<3>* face = static_cast<carve::mesh::Face<3>*>(data);
  glBegin(type);
  glNormal3dv(face->plane.N.v);
}

static void __stdcall _faceEnd(void* /* data */) {
  glEnd();
}

static void __stdcall _normalBegin(GLenum type, void* data) {
  GLdouble* normal = static_cast<GLdouble*>(data);
  glBegin(type);
  glNormal3dv(normal);
}

static void __stdcall _normalEnd(void* /* data */) {
  glEnd();
}

static void __stdcall _colourVertex(void* vertex_data, void* /* data */) {
  std::pair<carve::geom3d::Vector, cRGBA>& vd(
      *static_cast<std::pair<carve::geom3d::Vector, cRGBA>*>(vertex_data));
  glColor4f(vd.second.r, vd.second.g, vd.second.b, vd.second.a);
  glVertex3f(vd.first.x, vd.first.y, vd.first.z);
}

void drawColourPoly(const carve::geom3d::Vector& normal,
                    std::vector<std::pair<carve::geom3d::Vector, cRGBA> >& v) {
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

  GLUtesselator* tess = gluNewTess();

  gluTessCallback(tess, GLU_TESS_BEGIN_DATA, (GLUTessCallback)_normalBegin);
  gluTessCallback(tess, GLU_TESS_VERTEX_DATA, (GLUTessCallback)_colourVertex);
  gluTessCallback(tess, GLU_TESS_END_DATA, (GLUTessCallback)_normalEnd);

  gluTessBeginPolygon(tess, (void*)(GLdouble*)normal.v);
  gluTessBeginContour(tess);

  for (size_t i = 0, l = v.size(); i != l; ++i) {
    gluTessVertex(tess, (GLdouble*)v[i].first.v, (GLvoid*)&v[i]);
  }

  gluTessEndContour(tess);
  gluTessEndPolygon(tess);

  gluDeleteTess(tess);
}

void drawColourFace(carve::mesh::Face<3>* face, const std::vector<cRGBA>& vc) {
  std::vector<std::pair<carve::geom3d::Vector, cRGBA> > v;
  v.resize(face->nVertices());
  carve::mesh::Edge<3>* e = face->edge;
  for (size_t i = 0, l = face->nVertices(); i != l; ++i) {
    v[i] = std::make_pair(g_scale * (e->v1()->v + g_translation), vc[i]);
    e = e->next;
  }
  drawColourPoly(face->plane.N, v);
}

void drawFace(carve::mesh::Face<3>* face, cRGBA fc) {
  std::vector<std::pair<carve::geom3d::Vector, cRGBA> > v;
  v.resize(face->nVertices());
  carve::mesh::Edge<3>* e = face->edge;
  for (size_t i = 0, l = face->nVertices(); i != l; ++i) {
    v[i] = std::make_pair(g_scale * (e->v1()->v + g_translation), fc);
    e = e->next;
  }
  drawColourPoly(face->plane.N, v);
}

void drawFaceWireframe(carve::poly::Face<3>* face, bool normal, float r,
                       float g, float b) {
  glDisable(GL_LIGHTING);
  glDepthMask(GL_FALSE);
  glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
  glDisable(GL_CULL_FACE);

  glColor4f(r, g, b, 1.0);
  glBegin(GL_POLYGON);
  glColor4f(r, g, b, 0.1f);
  for (size_t i = 0, l = face->nVertices(); i != l; ++i) {
    glVertex(face->vertex(i));
  }
  glEnd();

  glDisable(GL_DEPTH_TEST);

  glColor4f(r, g, b, 0.01);
  glBegin(GL_POLYGON);
  glColor4f(r, g, b, 0.01f);
  for (size_t i = 0, l = face->nVertices(); i != l; ++i) {
    glVertex(face->vertex(i));
  }
  glEnd();

  glLineWidth(3.0f);
  glColor4f(1.0, 0.0, 0.0, 1.0);
  glBegin(GL_LINES);
  for (size_t i = 0, l = face->nEdges(); i != l; ++i) {
    if (static_cast<carve::poly::Polyhedron*>(face->owner)
            ->connectedFace(face, face->edge(i)) == nullptr) {
      glVertex(face->edge(i)->v1);
      glVertex(face->edge(i)->v2);
    }
  }
  glEnd();
  glLineWidth(1.0f);

  glEnable(GL_DEPTH_TEST);

  if (normal) {
    glBegin(GL_LINES);
    glColor4f(1.0, 1.0, 0.0, 1.0);
    glVertex(face->centroid());
    glColor4f(1.0, 1.0, 0.0, 0.0);
    glVertex(face->centroid() + 1 / g_scale * face->plane_eqn.N);
    glEnd();
  }

  glDepthMask(GL_TRUE);
  glEnable(GL_LIGHTING);
}

void drawFaceWireframe(carve::poly::Face<3>* face, bool normal) {
  drawFaceWireframe(face, normal, 0, 0, 0);
}

void drawFaceNormal(carve::mesh::Face<3>* face, float r, float g, float b) {
  glDisable(GL_LIGHTING);
  glDepthMask(GL_FALSE);

  glLineWidth(1.0f);

  glEnable(GL_DEPTH_TEST);

  glBegin(GL_LINES);
  glColor4f(1.0, 1.0, 0.0, 1.0);
  glVertex(face->centroid());
  glColor4f(1.0, 1.0, 0.0, 0.0);
  glVertex(face->centroid() + 1 / g_scale * face->plane.N);
  glEnd();

  glDepthMask(GL_TRUE);
  glEnable(GL_LIGHTING);
}

void drawFaceNormal(carve::mesh::Face<3>* face) {
  drawFaceNormal(face, 0, 0, 0);
}

void DebugHooks::drawFaceLoopWireframe(
    const std::vector<const carve::geom3d::Vector*>& face_loop,
    const carve::geom3d::Vector& normal, float r, float g, float b, float a,
    bool inset) {
  glDisable(GL_DEPTH_TEST);

  const size_t S = face_loop.size();

  double INSET = 0.005;

  if (inset) {
    glColor4f(r, g, b, a / 2.0);
    glBegin(GL_LINE_LOOP);

    for (size_t i = 0; i < S; ++i) {
      size_t i_pre = (i + S - 1) % S;
      size_t i_post = (i + 1) % S;

      carve::geom3d::Vector v1 =
          (*face_loop[i] - *face_loop[i_pre]).normalized();
      carve::geom3d::Vector v2 =
          (*face_loop[i] - *face_loop[i_post]).normalized();

      carve::geom3d::Vector n1 = cross(normal, v1);
      carve::geom3d::Vector n2 = cross(v2, normal);

      carve::geom3d::Vector v = *face_loop[i];

      carve::geom3d::Vector p1 = v + INSET * n1;
      carve::geom3d::Vector p2 = v + INSET * n2;

      carve::geom3d::Vector i1, i2;
      double mu1, mu2;

      carve::geom3d::Vector p;

      if (carve::geom3d::rayRayIntersection(carve::geom3d::Ray(v1, p1),
                                            carve::geom3d::Ray(v2, p2), i1, i2,
                                            mu1, mu2)) {
        p = (i1 + i2) / 2;
      } else {
        p = (p1 + p2) / 2;
      }

      glVertex(&p);
    }

    glEnd();
  }

  glColor4f(r, g, b, a);

  glBegin(GL_LINE_LOOP);

  for (unsigned i = 0; i < S; ++i) {
    glVertex(face_loop[i]);
  }

  glEnd();

  glColor4f(r, g, b, a);
  glPointSize(3.0);
  glBegin(GL_POINTS);

  for (unsigned i = 0; i < S; ++i) {
    carve::geom3d::Vector p = *face_loop[i];
    glVertex(face_loop[i]);
  }

  glEnd();

  glEnable(GL_DEPTH_TEST);
}

void DebugHooks::drawFaceLoop(
    const std::vector<const carve::geom3d::Vector*>& face_loop,
    const carve::geom3d::Vector& normal, float r, float g, float b, float a,
    bool lit) {
  if (lit) {
    glEnable(GL_LIGHTING);
  } else {
    glDisable(GL_LIGHTING);
  }

  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

  GLUtesselator* tess = gluNewTess();

  gluTessCallback(tess, GLU_TESS_BEGIN, (GLUTessCallback)glBegin);
  gluTessCallback(tess, GLU_TESS_VERTEX, (GLUTessCallback)glVertex3dv);
  gluTessCallback(tess, GLU_TESS_END, (GLUTessCallback)glEnd);

  glNormal3dv(normal.v);
  glColor4f(r, g, b, a);

  gluTessBeginPolygon(tess, (void*)nullptr);
  gluTessBeginContour(tess);

  std::vector<carve::geom3d::Vector> v;
  v.resize(face_loop.size());
  for (size_t i = 0, l = face_loop.size(); i != l; ++i) {
    v[i] = g_scale * (*face_loop[i] + g_translation);
    gluTessVertex(tess, (GLdouble*)v[i].v, (GLvoid*)v[i].v);
  }

  gluTessEndContour(tess);
  gluTessEndPolygon(tess);

  gluDeleteTess(tess);

  glEnable(GL_LIGHTING);
}

void DebugHooks::drawFaceLoop2(
    const std::vector<const carve::geom3d::Vector*>& face_loop,
    const carve::geom3d::Vector& normal, float rF, float gF, float bF, float aF,
    float rB, float gB, float bB, float aB, bool lit) {
  glEnable(GL_CULL_FACE);
  glCullFace(GL_BACK);
  drawFaceLoop(face_loop, normal, rF, gF, bF, aF, lit);
  glCullFace(GL_FRONT);
  drawFaceLoop(face_loop, normal, rB, gB, bB, aB, lit);
  glCullFace(GL_BACK);
}

void drawMesh(carve::mesh::Mesh<3>* mesh, float r, float g, float b, float a) {
  glColor4f(r, g, b, a);

  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  glBegin(GL_TRIANGLES);

  std::vector<carve::mesh::Vertex<3>*> v;

  for (size_t i = 0, l = mesh->faces.size(); i != l; ++i) {
    carve::mesh::Face<3>* f = mesh->faces[i];
    if (f->nVertices() == 3) {
      glNormal3dv(f->plane.N.v);
      f->getVertices(v);
      glVertex(v[0]->v);
      glVertex(v[1]->v);
      glVertex(v[2]->v);
    }
  }
  glEnd();

  for (size_t i = 0, l = mesh->faces.size(); i != l; ++i) {
    carve::mesh::Face<3>* f = mesh->faces[i];
    if (f->nVertices() != 3) {
      drawFace(f, cRGBA(r, g, b, a));
    }
  }
}

void drawMeshSet(carve::mesh::MeshSet<3>* poly, float r, float g, float b,
                 float a, int group) {
  if (group >= 0) {
    if ((size_t)group < poly->meshes.size()) {
      drawMesh(poly->meshes[group], r, g, b, a);
    }
  } else {
    for (size_t i = 0; i < poly->meshes.size(); ++i) {
      drawMesh(poly->meshes[i], r, g, b, a);
    }
  }
}

double connScale(const carve::mesh::Edge<3>* edge) {
  double l = 1.0;
  carve::geom3d::Vector c = edge->face->centroid();
  const carve::mesh::Edge<3>* e = edge;
  do {
    double l2 = carve::geom::distance(
        carve::geom::linesegment<3>(e->v1()->v, e->v2()->v), c);
    if (l2 > 1e-5) {
      l = std::min(l, l2);
    }
    e = e->next;
  } while (e != edge);

  return l;
}

void drawEdgeConn(const carve::mesh::Edge<3>* edge, float r, float g, float b,
                  float a) {
  carve::geom3d::Vector base = (edge->v1()->v + edge->v2()->v) / 2;

  double elen = (edge->v1()->v - edge->v2()->v).length();

  carve::geom3d::Vector x1 = (edge->v1()->v - edge->v2()->v) / elen;
  carve::geom3d::Vector y1 =
      carve::geom::cross(x1, edge->face->plane.N).normalized();
  double s1 = connScale(edge);

  carve::geom3d::Vector x2 =
      (edge->rev->v1()->v - edge->rev->v2()->v).normalized();
  carve::geom3d::Vector y2 =
      carve::geom::cross(x2, edge->rev->face->plane.N).normalized();
  double s2 = connScale(edge->rev);

  double s = std::min(s1, s2);

  x1 *= s;
  y1 *= s;
  x2 *= s;
  y2 *= s;

  glColor4f(r, g, b, a / 2);
  glBegin(GL_TRIANGLE_STRIP);
  glVertex(base + x1 * .2 + y1 * .4);
  glVertex(base - x1 * .2 + y1 * .4);
  glVertex(base + x1 * .1);
  glVertex(base - x1 * .1);
  glVertex(base + y2 * .4);
  glEnd();

  glColor4f(r, g, b, a);
  glBegin(GL_LINE_LOOP);
  glVertex(base + x1 * .2 + y1 * .4);
  glVertex(base - x1 * .2 + y1 * .4);
  glVertex(base - x1 * .1);
  glVertex(base + y2 * .4);
  glVertex(base + x1 * .1);
  glEnd();
}

void drawEdges(carve::mesh::Mesh<3>* mesh, double alpha, bool draw_edgeconn) {
  glColor4f(1.0, 0.0, 0.0, alpha);
  glBegin(GL_LINES);
  for (size_t i = 0, l = mesh->closed_edges.size(); i != l; ++i) {
    carve::mesh::Edge<3>* edge = mesh->closed_edges[i];
    glVertex(edge->v1()->v);
    glVertex(edge->v2()->v);
  }
  glEnd();

  std::unordered_map<
      std::pair<const carve::mesh::Vertex<3>*, const carve::mesh::Vertex<3>*>,
      int, carve::hash_pair>
      colour;

  if (draw_edgeconn) {
    for (size_t i = 0, l = mesh->closed_edges.size(); i != l; ++i) {
      const carve::mesh::Edge<3>* edge = mesh->closed_edges[i];
      int c = colour[std::make_pair(std::min(edge->v1(), edge->v2()),
                                    std::max(edge->v1(), edge->v2()))]++;
      drawEdgeConn(edge, (c & 1) ? 0.0 : 1.0, (c & 2) ? 0.0 : 1.0,
                   (c & 4) ? 1.0 : 0.0, alpha);
    }
  }

  glColor4f(1.0, 1.0, 0.0, alpha);
  glBegin(GL_LINES);
  for (size_t i = 0, l = mesh->open_edges.size(); i != l; ++i) {
    carve::mesh::Edge<3>* edge = mesh->open_edges[i];
    glVertex(edge->v1()->v);
    glVertex(edge->v2()->v);
  }
  glEnd();
}

void drawMeshWireframe(carve::mesh::Mesh<3>* mesh, bool draw_normal,
                       bool draw_edgeconn) {
  if (draw_normal) {
    for (size_t i = 0, l = mesh->faces.size(); i != l; ++i) {
      carve::mesh::Face<3>* f = mesh->faces[i];
      drawFaceNormal(f);
    }
  }

  glDisable(GL_LIGHTING);
  glDepthMask(GL_FALSE);
  glDisable(GL_DEPTH_TEST);

  drawEdges(mesh, 0.2, draw_edgeconn);

  glEnable(GL_DEPTH_TEST);

  drawEdges(mesh, 0.8, draw_edgeconn);

  glDepthMask(GL_TRUE);
  glEnable(GL_LIGHTING);
}

void drawMeshSetWireframe(carve::mesh::MeshSet<3>* poly, int group,
                          bool draw_normal, bool draw_edgeconn) {
  if (group >= 0) {
    if ((size_t)group < poly->meshes.size()) {
      drawMeshWireframe(poly->meshes[group], draw_normal, draw_edgeconn);
    }
  } else {
    for (size_t i = 0; i < poly->meshes.size(); ++i) {
      drawMeshWireframe(poly->meshes[i], draw_normal, draw_edgeconn);
    }
  }
}

void installDebugHooks() {
  if (carve::csg::intersect_debugEnabled()) {
    carve::csg::intersect_installDebugHooks(new DebugHooks());
  }
}
