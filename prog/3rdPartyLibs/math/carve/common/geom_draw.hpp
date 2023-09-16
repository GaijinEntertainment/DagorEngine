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

#pragma once

#include <carve/carve.hpp>

#include <carve/mesh.hpp>
#include <carve/vector.hpp>

#include <carve/octree_decl.hpp>
#include <carve/octree_impl.hpp>

#include "rgb.hpp"

void drawMeshSetWireframe(carve::mesh::MeshSet<3>* poly, int group,
                          bool draw_normal, bool draw_edgeconn);
void drawMeshSet(carve::mesh::MeshSet<3>* poly, float r, float g, float b,
                 float a, int group = -1);

void drawColourPoly(const carve::geom3d::Vector& normal,
                    std::vector<std::pair<carve::geom3d::Vector, cRGBA> >& v);
void drawColourFace(carve::mesh::Face<3>* face, const std::vector<cRGBA>& vc);
void drawFace(carve::mesh::Face<3>* face, cRGBA fc);

void installDebugHooks();
void drawTri(const carve::geom::tri<3>&);
void drawSphere(const carve::geom::sphere<3>&);
void drawCube(const carve::geom3d::Vector&, const carve::geom3d::Vector&);
void drawSphere(const carve::geom3d::Vector&, double);
void drawOctree(const carve::csg::Octree&);

extern carve::geom3d::Vector g_translation;
extern double g_scale;
