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

#if defined(__APPLE__)
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#else

#ifdef WIN32
#include <windows.h>
#undef rad1
#undef rad2
#endif

#include <GL/gl.h>
#include <GL/glu.h>

#endif

#include <carve/csg.hpp>

class GLUTriangulator : public carve::csg::CSG::Hook {
  GLUtesselator* tess;
  GLenum curr_type;

  std::vector<const carve::poly::Vertex<3>*> vertices;
  std::vector<carve::poly::Face<3>*> new_faces;
  const carve::poly::Face<3>* orig_face;

 public:
  GLUTriangulator();
  ~GLUTriangulator() override;
  virtual void processOutputFace(std::vector<carve::poly::Face<3>*>& faces,
                                 const carve::poly::Face<3>* orig,
                                 bool flipped);

  void faceBegin(GLenum type);
  void faceVertex(const carve::poly::Vertex<3>* vertex);
  void faceEnd();
};
