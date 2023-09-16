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

#include <carve/matrix.hpp>
#include <carve/poly.hpp>

#ifdef WIN32
#undef rad1
#undef rad2
#endif

carve::mesh::MeshSet<3>* makeCube(
    const carve::math::Matrix& transform = carve::math::Matrix());

carve::mesh::MeshSet<3>* makeSubdividedCube(
    int sub_x = 3, int sub_y = 3, int sub_z = 3,
    bool (*inc)(int, int, int) = nullptr,
    const carve::math::Matrix& transform = carve::math::Matrix());

carve::mesh::MeshSet<3>* makeDoubleCube(
    const carve::math::Matrix& transform = carve::math::Matrix());

carve::mesh::MeshSet<3>* makeTorus(
    int slices, int rings, double rad1, double rad2,
    const carve::math::Matrix& transform = carve::math::Matrix());

carve::mesh::MeshSet<3>* makeCylinder(
    int slices, double rad, double height,
    const carve::math::Matrix& transform = carve::math::Matrix());

carve::mesh::MeshSet<3>* makeCone(
    int slices, double rad, double height,
    const carve::math::Matrix& transform = carve::math::Matrix());
