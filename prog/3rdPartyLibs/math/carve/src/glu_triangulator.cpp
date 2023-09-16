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

#include "glu_triangulator.hpp"

#if defined(__GNUC__)
#define __stdcall
#endif

#if defined(GLU_TESS_CALLBACK_VARARGS)
typedef GLvoid(__stdcall* GLUTessCallback)(...);
#else
typedef void(__stdcall* GLUTessCallback)();
#endif

void GLUTriangulator::faceBegin(GLenum type) {
  curr_type = type;
  vertices.clear();
}

void GLUTriangulator::faceVertex(const carve::poly::Vertex<3>* vertex) {
  vertices.push_back(vertex);
}

void GLUTriangulator::faceEnd() {
  std::vector<const carve::poly::Vertex<3>*> fv;
  fv.resize(3);

  switch (curr_type) {
    case GL_TRIANGLES: {
      for (int i = 0; i < vertices.size(); i += 3) {
        fv[0] = vertices[i];
        fv[1] = vertices[i + 1];
        fv[2] = vertices[i + 2];
        new_faces.push_back(orig_face->create(fv, false));
      }
      break;
    }
    case GL_TRIANGLE_STRIP: {
      bool fwd = true;
      for (int i = 2; i < vertices.size(); ++i) {
        if (fwd) {
          fv[0] = vertices[i - 2];
          fv[1] = vertices[i - 1];
          fv[2] = vertices[i];
        } else {
          fv[0] = vertices[i];
          fv[1] = vertices[i - 1];
          fv[2] = vertices[i - 2];
        }
        new_faces.push_back(orig_face->create(fv, false));
        fwd = !fwd;
      }
      break;
    }
    case GL_TRIANGLE_FAN: {
      for (int i = 2; i < vertices.size(); ++i) {
        fv[0] = vertices[0];
        fv[1] = vertices[i - 1];
        fv[2] = vertices[i];
        new_faces.push_back(orig_face->create(fv, false));
      }
      break;
    }
  }
}

static void __stdcall _faceBegin(GLenum type, void* data) {
  static_cast<GLUTriangulator*>(data)->faceBegin(type);
}

static void __stdcall _faceVertex(void* vertex_data, void* data) {
  static_cast<GLUTriangulator*>(data)->faceVertex(
      static_cast<const carve::poly::Vertex<3>*>(vertex_data));
}

static void __stdcall _faceEnd(void* data) {
  static_cast<GLUTriangulator*>(data)->faceEnd();
}

GLUTriangulator::GLUTriangulator() {
  tess = gluNewTess();
  gluTessCallback(tess, GLU_TESS_BEGIN_DATA, (GLUTessCallback)_faceBegin);
  gluTessCallback(tess, GLU_TESS_VERTEX_DATA, (GLUTessCallback)_faceVertex);
  gluTessCallback(tess, GLU_TESS_END_DATA, (GLUTessCallback)_faceEnd);
}

GLUTriangulator::~GLUTriangulator() {
  gluDeleteTess(tess);
}

void GLUTriangulator::processOutputFace(
    std::vector<carve::poly::Face<3>*>& faces, const carve::poly::Face<3>* orig,
    bool flipped) {
  size_t f = 0;
  while (f < faces.size()) {
    carve::poly::Face<3>* face = faces[f];
    if (face->nVertices() == 3) {
      ++f;
      continue;
    }

    orig_face = face;

    new_faces.clear();

    gluTessBeginPolygon(tess, (void*)this);
    gluTessBeginContour(tess);

    for (size_t i = 0; i < face->nVertices(); ++i) {
      gluTessVertex(tess, (GLdouble*)face->vertex(i)->v.v,
                    (GLvoid*)face->vertex(i));
    }

    gluTessEndContour(tess);
    gluTessEndPolygon(tess);

    faces.erase(faces.begin() + f);
    faces.reserve(faces.size() + new_faces.size());
    faces.insert(faces.begin() + f, new_faces.begin(), new_faces.end());
    f += new_faces.size();
    delete face;
  }
}
