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

#include <carve/geom.hpp>

namespace carve {
namespace geom {

enum TriangleIntType { TR_TYPE_NONE = 0, TR_TYPE_TOUCH = 1, TR_TYPE_INT = 2 };

enum TriangleInt {
  TR_INT_NONE = 0,  // no intersection.
  TR_INT_INT = 1,   // intersection.
  TR_INT_VERT = 2,  // intersection due to shared vertex.
  TR_INT_EDGE = 3,  // intersection due to shared edge.
  TR_INT_TRI = 4    // intersection due to identical triangle.
};

TriangleInt triangle_intersection(const vector<2> tri_a[3],
                                  const vector<2> tri_b[3]);
TriangleInt triangle_intersection(const vector<3> tri_a[3],
                                  const vector<3> tri_b[3]);

bool triangle_intersection_simple(const vector<2> tri_a[3],
                                  const vector<2> tri_b[3]);
bool triangle_intersection_simple(const vector<3> tri_a[3],
                                  const vector<3> tri_b[3]);

TriangleIntType triangle_intersection_exact(const vector<2> tri_a[3],
                                            const vector<2> tri_b[3]);
TriangleIntType triangle_intersection_exact(const vector<3> tri_a[3],
                                            const vector<3> tri_b[3]);

TriangleIntType triangle_linesegment_intersection_exact(
    const vector<2> tri_a[3], const vector<2> line_b[2]);
TriangleIntType triangle_point_intersection_exact(const vector<2> tri_a[3],
                                                  const vector<2>& pt_b);
}  // namespace geom
}  // namespace carve
