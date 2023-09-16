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

#include <carve/triangle_intersection.hpp>

#include <carve/geom2d.hpp>
#include <carve/geom3d.hpp>
#include <carve/shewchuk_predicates.hpp>

#include <cstdlib>

#include <iostream>

typedef carve::geom::vector<3> vec3;
typedef carve::geom::vector<2> vec2;

namespace {
enum sat_t {
  SAT_NONE = -1,
  SAT_TOUCH = 0,
  SAT_INTERSECT = 1,
  SAT_COPLANAR = 2
};

int dbl_sign(double d) {
  if (d == 0.0) {
    return 0;
  }
  if (d < 0.0) {
    return -1;
  }
  return +1;
}

inline double orient3d_exact(const vec3& a, const vec3& b, const vec3& c,
                             const vec3& d) {
  return shewchuk::orient3d(a.v, b.v, c.v, d.v);
}

inline double orient2d_exact(const vec2& a, const vec2& b, const vec2& c) {
  return shewchuk::orient2d(a.v, b.v, c.v);
}

vec3 normal(const vec3 tri[3]) {
  return carve::geom::cross(tri[1] - tri[0], tri[2] - tri[0]);
}

void extent(double a, double b, double c, double& lo, double& hi) {
  lo = std::min(std::min(a, b), c);
  hi = std::max(std::max(a, b), c);
}

template <typename val_t>
val_t min3(const val_t& a, const val_t& b, const val_t& c) {
  return std::min(std::min(a, b), c);
}

template <typename val_t>
val_t max3(const val_t& a, const val_t& b, const val_t& c) {
  return std::max(std::max(a, b), c);
}

template <unsigned dim>
bool axis_test(const vec3 tri_a[3], const vec3 tri_b[3]) {
  double a_lo, a_hi, b_lo, b_hi;
  extent(tri_a[0].v[dim], tri_a[1].v[dim], tri_a[2].v[dim], a_lo, a_hi);
  extent(tri_b[0].v[dim], tri_b[1].v[dim], tri_b[2].v[dim], b_lo, b_hi);
  return a_hi < b_lo || b_hi < a_lo;
}

// returns true if no intersection, based upon axis testing.
bool sat_bbox(const vec3 tri_a[3], const vec3 tri_b[3]) {
  return axis_test<0>(tri_a, tri_b) || axis_test<1>(tri_a, tri_b) ||
         axis_test<2>(tri_a, tri_b);
}

// returns true if no intersection, based upon normal testing.
sat_t sat_normal(const vec3 tri_a[3], const vec3 tri_b[3]) {
  double v, lo, hi;
  lo = hi = orient3d_exact(tri_a[0], tri_a[1], tri_a[2], tri_b[0]);
  v = orient3d_exact(tri_a[0], tri_a[1], tri_a[2], tri_b[1]);
  lo = std::min(lo, v);
  hi = std::max(hi, v);
  v = orient3d_exact(tri_a[0], tri_a[1], tri_a[2], tri_b[2]);
  lo = std::min(lo, v);
  hi = std::max(hi, v);

  if (lo == 0.0 && hi == 0.0) {
    return SAT_COPLANAR;
  }
  if (lo == 0.0 || hi == 0.0) {
    return SAT_TOUCH;
  }
  if (lo > 0.0 || hi < 0.0) {
    return SAT_NONE;
  }
  return SAT_INTERSECT;
}

// returns true if no intersection, based upon edge^a_i and vertex^b_j
// separating plane.
// shared vertex case. only one vertex of b (k) to test - the third
// (shared) vertex will have orientation equal to 0.
bool sat_plane(const vec3 tri_a[3], const vec3 tri_b[3], unsigned i, unsigned j,
               unsigned k) {
  double a;
  double b;
  a = orient3d_exact(tri_a[i], tri_a[(i + 1) % 3], tri_b[j],
                     tri_a[(i + 2) % 3]);
  b = orient3d_exact(tri_a[i], tri_a[(i + 1) % 3], tri_b[j], tri_b[k]);
  if (a * b < 0.0) {
    return true;
  }
  return false;
}

// returns true if no intersection, based upon edge^a_i and vertex^b_j
// separating plane.
bool sat_plane(const vec3 tri_a[3], const vec3 tri_b[3], unsigned i,
               unsigned j) {
  double a;
  double b_lo, b_hi;
  a = orient3d_exact(tri_a[i], tri_a[(i + 1) % 3], tri_b[j],
                     tri_a[(i + 2) % 3]);
  b_lo = orient3d_exact(tri_a[i], tri_a[(i + 1) % 3], tri_b[j],
                        tri_b[(j + 1) % 3]);
  b_hi = orient3d_exact(tri_a[i], tri_a[(i + 1) % 3], tri_b[j],
                        tri_b[(j + 2) % 3]);
  if (b_lo > b_hi) {
    std::swap(b_lo, b_hi);
  }
  std::cerr << "b_lo: " << b_lo << " b_hi: " << b_hi << " a: " << a
            << std::endl;
  if (a > 0.0 && b_hi < 0.0) {
    return true;
  }
  if (a < 0.0 && b_lo > 0.0) {
    return true;
  }
  return false;
}

// returns: -1 - no intersection
//           0 - touching
//          +1 - intersection
int sat_edge(const vec2 tri_a[3], const vec2 tri_b[3], unsigned i) {
  return max3(dbl_sign(orient2d_exact(tri_a[i], tri_a[(i + 1) % 3], tri_b[0])),
              dbl_sign(orient2d_exact(tri_a[i], tri_a[(i + 1) % 3], tri_b[1])),
              dbl_sign(orient2d_exact(tri_a[i], tri_a[(i + 1) % 3], tri_b[2])));
}

// returns: -1 - no intersection
//           0 - touching
//          +1 - intersection
int sat_edge(const vec2 tri_a[3], const vec2 tri_b[3], unsigned i,
              unsigned j) {
  return std::max(dbl_sign(orient2d_exact(tri_a[i], tri_a[(i + 1) % 3],
                                          tri_b[(j + 1) % 3])),
                  dbl_sign(orient2d_exact(tri_a[i], tri_a[(i + 1) % 3],
                                          tri_b[(j + 2) % 3])));
}

// test for vertex sharing. between a triangle pair.
// returns:
// +3 all vertices are shared, same orientation
// -3 all vertices are shared, opposite orientation
// +2 edge shared, same orienation
// -2 edge shared, opposite orientation
// +1 vertex shared
// 0 no sharing
template <unsigned ndim>
int test_sharing(const carve::geom::vector<ndim> tri_a[3],
                 const carve::geom::vector<ndim> tri_b[3], size_t& ia,
                 size_t& ib) {
  size_t a, b;

  for (b = 0; b < 3; ++b) {
    if (tri_a[0] == tri_b[b] && tri_a[1] == tri_b[(b + 1) % 3] &&
        tri_a[2] == tri_b[(b + 2) % 3]) {
      ia = 0;
      ib = b;
      return +3;
    }
    if (tri_a[0] == tri_b[b] && tri_a[1] == tri_b[(b + 2) % 3] &&
        tri_a[2] == tri_b[(b + 1) % 3]) {
      ia = 0;
      ib = b;
      return -3;
    }
  }

  for (a = 0; a < 3; ++a) {
    for (b = 0; b < 3; ++b) {
      if (tri_a[a] == tri_b[b] && tri_a[(a + 1) % 3] == tri_b[(b + 1) % 3]) {
        ia = a;
        ib = b;
        return +2;
      }
      if (tri_a[a] == tri_b[b] && tri_a[(a + 1) % 3] == tri_b[(b + 2) % 3]) {
        ia = a;
        ib = b;
        return -2;
      }
    }
  }

  for (a = 0; a < 3; ++a) {
    for (b = 0; b < 3; ++b) {
      if (tri_a[a] == tri_b[b]) {
        ia = a;
        ib = b;
        return +1;
      }
    }
  }

  return 0;
}

void find_projection_axes(const vec3 tri[3], unsigned& a1, unsigned& a2) {
  unsigned a0 = carve::geom::largestAxis(normal(tri));
  a1 = (a0 + 1) % 3;
  a2 = (a0 + 2) % 3;
}

void project_triangles(const vec3 tri_a[3], const vec3 tri_b[3], vec2 ptri_a[3],
                       vec2 ptri_b[3]) {
  unsigned a1, a2;
  find_projection_axes(tri_a, a1, a2);

  ptri_a[0] = carve::geom::select(tri_a[0], a1, a2);
  ptri_a[1] = carve::geom::select(tri_a[1], a1, a2);
  ptri_a[2] = carve::geom::select(tri_a[2], a1, a2);

  if (!carve::geom2d::isAnticlockwise(ptri_a)) {
    std::swap(ptri_a[0], ptri_a[2]);
  }

  ptri_b[0] = carve::geom::select(tri_b[0], a1, a2);
  ptri_b[1] = carve::geom::select(tri_b[1], a1, a2);
  ptri_b[2] = carve::geom::select(tri_b[2], a1, a2);

  if (!carve::geom2d::isAnticlockwise(ptri_b)) {
    std::swap(ptri_b[0], ptri_b[2]);
  }
}

carve::geom::TriangleInt triangle_intersection_coplanar(const vec3 tri_a[3],
                                                        const vec3 tri_b[3]) {
  vec2 ptri_a[3], ptri_b[3];
  project_triangles(tri_a, tri_b, ptri_a, ptri_b);
  return triangle_intersection(ptri_a, ptri_b);
}

bool triangle_intersection_coplanar_simple(const vec3 tri_a[3],
                                           const vec3 tri_b[3]) {
  vec2 ptri_a[3], ptri_b[3];
  project_triangles(tri_a, tri_b, ptri_a, ptri_b);
  return triangle_intersection_simple(ptri_a, ptri_b);
}

carve::geom::TriangleIntType triangle_intersection_coplanar_exact(
    const vec3 tri_a[3], const vec3 tri_b[3]) {
  vec2 ptri_a[3], ptri_b[3];
  project_triangles(tri_a, tri_b, ptri_a, ptri_b);
  return triangle_intersection_exact(ptri_a, ptri_b);
}

void count(int v[3], int c[3]) {
  c[0] = c[1] = c[2] = 0;
  c[v[0] + 1]++;
  c[v[1] + 1]++;
  c[v[2] + 1]++;
}

void normal_sign(const vec3 tri_a[3], const vec3 tri_b[3], int nb[3]) {
  nb[0] = dbl_sign(orient3d_exact(tri_a[0], tri_a[1], tri_a[2], tri_b[0]));
  nb[1] = dbl_sign(orient3d_exact(tri_a[0], tri_a[1], tri_a[2], tri_b[1]));
  nb[2] = dbl_sign(orient3d_exact(tri_a[0], tri_a[1], tri_a[2], tri_b[2]));
}

int line_segment_tri_test(const vec3 tri_a[3], const vec3& a, const vec3& b) {
  int o[3];
  int c[3];

  o[0] = dbl_sign(orient3d_exact(tri_a[0], tri_a[1], a, b));
  o[1] = dbl_sign(orient3d_exact(tri_a[1], tri_a[2], a, b));
  o[2] = dbl_sign(orient3d_exact(tri_a[2], tri_a[0], a, b));

  count(o, c);

  switch (c[1]) {
    case 0:
      if (c[0] == 0 || c[2] == 0) {
        return -3;  // face
      }
      return 0;
    case 1:
      if (c[0] == 2 || c[2] == 2) {
        return -2;  // edge
      }
      return 0;
    case 2:
      return -1;  // vertex
    case 3:
    default:
      CARVE_FAIL("should not be reached");
  }
}

bool touches_edge(const vec3 tri_a[3], const int nb[3], const vec3 tri_b[3]) {
  unsigned i = 0;
  if (nb[1] != 0) {
    i = 1;
  }
  if (nb[2] != 0) {
    i = 2;
  }

  unsigned a1, a2;
  find_projection_axes(tri_a, a1, a2);

  vec2 ptri_a[3], pline_b[2];
  ptri_a[0] = carve::geom::select(tri_a[0], a1, a2);
  ptri_a[1] = carve::geom::select(tri_a[1], a1, a2);
  ptri_a[2] = carve::geom::select(tri_a[2], a1, a2);

  if (!carve::geom2d::isAnticlockwise(ptri_a)) {
    std::swap(ptri_a[0], ptri_a[2]);
  }

  pline_b[0] = carve::geom::select(tri_b[(i + 1) % 3], a1, a2);
  pline_b[1] = carve::geom::select(tri_b[(i + 2) % 3], a1, a2);

  return triangle_linesegment_intersection_exact(ptri_a, pline_b) !=
         carve::geom::TR_TYPE_NONE;
}

bool touches_vert(const vec3 tri_a[3], const int nb[3], const vec3 tri_b[3]) {
  unsigned i = 0;
  if (nb[1] == 0) {
    i = 1;
  }
  if (nb[2] == 0) {
    i = 2;
  }

  unsigned a1, a2;
  find_projection_axes(tri_a, a1, a2);

  vec2 ptri_a[3], ppt_b;
  ptri_a[0] = carve::geom::select(tri_a[0], a1, a2);
  ptri_a[1] = carve::geom::select(tri_a[1], a1, a2);
  ptri_a[2] = carve::geom::select(tri_a[2], a1, a2);

  if (!carve::geom2d::isAnticlockwise(ptri_a)) {
    std::swap(ptri_a[0], ptri_a[2]);
  }

  ppt_b = carve::geom::select(tri_b[i], a1, a2);

  return triangle_point_intersection_exact(ptri_a, ppt_b) !=
         carve::geom::TR_TYPE_NONE;
}

unsigned half_test(const vec3 tri_a[3], const int nb[3], const vec3 tri_b[3]) {
  unsigned c = 0;
  for (unsigned i = 0; i < 3; ++i) {
    unsigned j = (i + 1) % 3;
    if (nb[i] == 0 || nb[i] == nb[j]) {
      continue;
    }

    int t = line_segment_tri_test(tri_a, tri_b[i], tri_b[j]);
    if (t == -3) {
      return 2;
    }
    if (t < 0) {
      c++;
    }
  }
  return c;
}
}  // namespace

namespace carve {
namespace geom {

TriangleInt triangle_intersection(const vec2 tri_a[3], const vec2 tri_b[3]) {
  // triangles must be anticlockwise, or colinear.
  CARVE_ASSERT(carve::geom2d::orient2d(tri_a[0], tri_a[1], tri_a[2]) >= 0.0);
  CARVE_ASSERT(carve::geom2d::orient2d(tri_b[0], tri_b[1], tri_b[2]) >= 0.0);

  size_t ia, ib;
  int sharing = test_sharing<2>(tri_a, tri_b, ia, ib);

  switch (std::abs(sharing)) {
    case 0: {
      // no shared vertices.
      if (sat_edge(tri_a, tri_b, 0) < 0) {
        return TR_INT_NONE;
      }
      if (sat_edge(tri_a, tri_b, 1) < 0) {
        return TR_INT_NONE;
      }
      if (sat_edge(tri_a, tri_b, 2) < 0) {
        return TR_INT_NONE;
      }
      if (sat_edge(tri_b, tri_a, 0) < 0) {
        return TR_INT_NONE;
      }
      if (sat_edge(tri_b, tri_a, 1) < 0) {
        return TR_INT_NONE;
      }
      if (sat_edge(tri_b, tri_a, 2) < 0) {
        return TR_INT_NONE;
      }

      return TR_INT_INT;
    }
    case 1: {
      // shared vertex (ia, ib) [but not shared edge]
      if (sat_edge(tri_a, tri_b, (ia + 2) % 3, ib) < 0) {
        return TR_INT_VERT;
      }
      if (sat_edge(tri_a, tri_b, ia, ib) < 0) {
        return TR_INT_VERT;
      }
      if (sat_edge(tri_b, tri_a, (ib + 2) % 3, ia) < 0) {
        return TR_INT_VERT;
      }
      if (sat_edge(tri_b, tri_a, ib, ia) < 0) {
        return TR_INT_VERT;
      }

      return TR_INT_INT;
    }
    case 2: {
      // shared edge (ia, ib) -> (ia + 1, ib + (sharing > 0) ? +1 : -1)
      size_t pa = (ia + 2) % 3;
      size_t pb = (ib + (sharing == +1 ? 2 : 1)) % 3;

      double sa = orient2d_exact(tri_a[ia], tri_a[(ia + 1) % 3], tri_a[pa]);
      double sb = orient2d_exact(tri_a[ia], tri_a[(ia + 1) % 3], tri_b[pb]);

      if (sa * sb < 0.0) {
        return TR_INT_EDGE;
      }

      return TR_INT_INT;
    }
    case 3: {
      return TR_INT_TRI;
    }
    default: { CARVE_FAIL("should not happen."); }
  }
}

TriangleInt triangle_intersection(const vec3 tri_a[3], const vec3 tri_b[3]) {
  if (sat_bbox(tri_a, tri_b)) {
    return TR_INT_NONE;
  }

  sat_t norm1 = sat_normal(tri_a, tri_b);
  sat_t norm2 = sat_normal(tri_b, tri_a);
  if (norm2 == SAT_NONE) {
    return TR_INT_NONE;
  }
  if (norm1 == SAT_NONE) {
    return TR_INT_NONE;
  }

  if (norm2 == SAT_COPLANAR) {
    return TR_INT_NONE;
  }
  if (norm1 == SAT_COPLANAR) {
    return TR_INT_NONE;
  }

  if (norm1 == SAT_COPLANAR) {
    return triangle_intersection_coplanar(tri_a, tri_b);
  }

  size_t ia, ib;
  int sharing = test_sharing<3>(tri_a, tri_b, ia, ib);
  switch (std::abs(sharing)) {
    case 0: {
      // no shared vertices.
      for (size_t i = 0; i < 3; ++i) {
        for (size_t j = 0; j < 3; ++j) {
          if (sat_plane(tri_a, tri_b, i, j)) {
            return TR_INT_NONE;
          }
          if (sat_plane(tri_b, tri_a, i, j)) {
            return TR_INT_NONE;
          }
        }
      }

      return TR_INT_INT;
    }
    case 1: {
      // shared vertex (ia, ib) [but not shared edge, and not coplanar]
      size_t ia0 = ia, ia1 = (ia + 1) % 3, ia2 = (ia + 2) % 3;
      size_t ib0 = ib, ib1 = (ib + 1) % 3, ib2 = (ib + 2) % 3;

      if (sat_plane(tri_a, tri_b, ia2, ib1, ib2)) {
        return TR_INT_VERT;
      }
      if (sat_plane(tri_a, tri_b, ia2, ib2, ib1)) {
        return TR_INT_VERT;
      }
      if (sat_plane(tri_a, tri_b, ia0, ib1, ib2)) {
        return TR_INT_VERT;
      }
      if (sat_plane(tri_a, tri_b, ia0, ib2, ib1)) {
        return TR_INT_VERT;
      }
      if (sat_plane(tri_b, tri_b, ib2, ia1, ia2)) {
        return TR_INT_VERT;
      }
      if (sat_plane(tri_b, tri_b, ib2, ia2, ia1)) {
        return TR_INT_VERT;
      }
      if (sat_plane(tri_b, tri_b, ib0, ia1, ia2)) {
        return TR_INT_VERT;
      }
      if (sat_plane(tri_b, tri_b, ib0, ia2, ia1)) {
        return TR_INT_VERT;
      }

      return TR_INT_INT;
    }
    case 2: {
      // shared edge (ia, ib) [not coplanar]
      return TR_INT_EDGE;
    }
    case 3: {
      return TR_INT_TRI;
    }
    default: { CARVE_FAIL("should not happen."); }
  }
}

bool triangle_intersection_simple(const vec2 tri_a[3], const vec2 tri_b[3]) {
  // triangles must be anticlockwise, or colinear.
  CARVE_ASSERT(orient2d_exact(tri_a[0], tri_a[1], tri_a[2]) >= 0.0);
  CARVE_ASSERT(orient2d_exact(tri_b[0], tri_b[1], tri_b[2]) >= 0.0);

  for (size_t i = 0; i < 3; ++i) {
    if (sat_edge(tri_a, tri_b, i) < 0) {
      return false;
    }
  }
  for (size_t i = 0; i < 3; ++i) {
    if (sat_edge(tri_b, tri_a, i) < 0) {
      return false;
    }
  }
  return true;
}

bool triangle_intersection_simple(const vec3 tri_a[3], const vec3 tri_b[3]) {
  if (sat_bbox(tri_a, tri_b)) {
    return false;
  }

  sat_t norm1 = sat_normal(tri_a, tri_b);
  if (norm1 == SAT_NONE) {
    return false;
  }
  if (norm1 == SAT_COPLANAR) {
    return triangle_intersection_coplanar_simple(tri_a, tri_b);
  }

  sat_t norm2 = sat_normal(tri_b, tri_a);
  if (norm2 == SAT_NONE) {
    return false;
  }

  for (size_t i = 0; i < 3; ++i) {
    for (size_t j = 0; j < 3; ++j) {
      if (sat_plane(tri_a, tri_b, i, j)) {
        return false;
      }
      if (sat_plane(tri_b, tri_a, i, j)) {
        return false;
      }
    }
  }
  return true;
}

TriangleIntType triangle_point_intersection_exact(const vec2 tri_a[3],
                                                  const vec2& pt_b) {
  CARVE_ASSERT(orient2d_exact(tri_a[0], tri_a[1], tri_a[2]) >= 0.0);

  int m = min3(dbl_sign(orient2d_exact(tri_a[0], tri_a[1], pt_b)),
               dbl_sign(orient2d_exact(tri_a[1], tri_a[2], pt_b)),
               dbl_sign(orient2d_exact(tri_a[2], tri_a[0], pt_b)));

  switch (m) {
    case -1:
      return TR_TYPE_NONE;
    case 0:
      return TR_TYPE_TOUCH;
    default:
      return TR_TYPE_INT;
  }
}

TriangleIntType triangle_linesegment_intersection_exact(const vec2 tri_a[3],
                                                        const vec2 line_b[2]) {
  CARVE_ASSERT(orient2d_exact(tri_a[0], tri_a[1], tri_a[2]) >= 0.0);

  int l1 = dbl_sign(orient2d_exact(line_b[0], line_b[1], tri_a[0]));
  int l2 = dbl_sign(orient2d_exact(line_b[0], line_b[1], tri_a[1]));
  int l3 = dbl_sign(orient2d_exact(line_b[0], line_b[1], tri_a[2]));

  if (min3(l1, l2, l3) == +1) {
    return TR_TYPE_NONE;
  }
  if (max3(l1, l2, l3) == -1) {
    return TR_TYPE_NONE;
  }

  int m = +1;
  for (size_t i = 0; m != -1 && i < 3; ++i) {
    int n = std::max(
        dbl_sign(orient2d_exact(tri_a[i], tri_a[(i + 1) % 3], line_b[0])),
        dbl_sign(orient2d_exact(tri_a[i], tri_a[(i + 1) % 3], line_b[1])));
    m = std::min(m, n);
  }

  switch (m) {
    case -1:
      return TR_TYPE_NONE;
    case 0:
      return TR_TYPE_TOUCH;
    default:
      return TR_TYPE_INT;
  }
}

TriangleIntType triangle_intersection_exact(const vec2 tri_a[3],
                                            const vec2 tri_b[3]) {
  // triangles must be anticlockwise, or colinear.
  CARVE_ASSERT(orient2d_exact(tri_a[0], tri_a[1], tri_a[2]) >= 0.0);
  CARVE_ASSERT(orient2d_exact(tri_b[0], tri_b[1], tri_b[2]) >= 0.0);

  int curr = +1;
  for (size_t i = 0; curr != -1 && i < 3; ++i) {
    curr = std::min(curr, sat_edge(tri_a, tri_b, i));
  }
  for (size_t i = 0; curr != -1 && i < 3; ++i) {
    curr = std::min(curr, sat_edge(tri_b, tri_a, i));
  }
  switch (curr) {
    case -1:
      return TR_TYPE_NONE;
    case 0:
      return TR_TYPE_TOUCH;
    case +1:
      return TR_TYPE_INT;
    default:
      CARVE_FAIL("should not happen.");
  }
}

TriangleIntType triangle_intersection_exact(const vec3 tri_a[3],
                                            const vec3 tri_b[3]) {
  int na[3], nb[3], ca[3], cb[3];

  normal_sign(tri_b, tri_a, na);
  normal_sign(tri_a, tri_b, nb);

  count(na, ca);
  count(nb, cb);

  if (ca[0] == 3 || ca[2] == 3 || cb[0] == 3 || cb[2] == 3) {
    return TR_TYPE_NONE;
  }

  if (ca[1] == 3 && cb[1] == 3) {
    TriangleIntType t = triangle_intersection_coplanar_exact(tri_a, tri_b);
    if (t != TR_TYPE_NONE) {
      t = TR_TYPE_TOUCH;
    }
    return t;
  }

  if (ca[1] == 2) {
    return touches_edge(tri_b, na, tri_a) ? TR_TYPE_TOUCH : TR_TYPE_NONE;
  }

  if (cb[1] == 2) {
    return touches_edge(tri_a, nb, tri_b) ? TR_TYPE_TOUCH : TR_TYPE_NONE;
  }

  if (ca[0] == 0 || ca[2] == 0) {
    return touches_vert(tri_b, na, tri_a) ? TR_TYPE_TOUCH : TR_TYPE_NONE;
  }

  if (cb[0] == 0 || cb[2] == 0) {
    return touches_vert(tri_a, nb, tri_b) ? TR_TYPE_TOUCH : TR_TYPE_NONE;
  }

  unsigned ia = half_test(tri_b, na, tri_a);
  unsigned ib = half_test(tri_a, nb, tri_b);

  if (ia == 2 || ib == 2) {
    return TR_TYPE_INT;
  }
  if (ia == 0 && ib == 0) {
    return TR_TYPE_NONE;
  }
  return TR_TYPE_TOUCH;
}
}  // namespace geom
}  // namespace carve
