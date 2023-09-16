//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <math/dag_Point2.h>
#include <math/dag_bounds2.h>
#include <math/random/dag_random.h>
#include <vecmath/dag_vecMath.h>

#define DIFF_SIGN(a, b) ((a) > 0 && (b) < 0 || (a) < 0 && (b) > 0)


//==============================================================================
// is_point_in functions
//==============================================================================
inline bool VECTORCALL is_point_in_circle(Point2 p, Point2 c, real r);
inline bool VECTORCALL is_point_in_circle_sq(Point2 p, Point2 c, real r_sq);
inline bool VECTORCALL is_point_in_rect(Point2 p, const BBox2 &rect);
inline bool VECTORCALL is_point_in_rect(Point2 p, Point2 start, Point2 end, real width);
inline bool VECTORCALL is_point_in_ellipse(Point2 p, Point2 c, real a, real b);
inline bool VECTORCALL is_point_in_ellipse(Point2 p, Point2 c, real a, real b, Point2 a_dir);
inline bool VECTORCALL is_point_in_triangle(Point2 p, Point2 t1, Point2 t2, Point2 t3);
inline bool VECTORCALL is_point_in_triangle(Point2 p, const Point2 *t);
// for convex polygons only (for triangles use faster function is_point_in_triangle)
bool VECTORCALL is_point_in_conv_poly(Point2 p, const Point2 *poly, unsigned count);
// slower function for all types of polygons
// NOTE: under construction
bool is_point_in_poly(const Point2 &p, const Point2 *poly, int count);


//==============================================================================
// intersection functions
// returns:
// 0 - no intersection
// 1 - intersection present
// 2 - obj1 completely located in obj2
// 3 - obj2 completely located in obj1
//==============================================================================
inline int VECTORCALL inter_circles(Point2 c1, real r1, Point2 c2, real r2);
inline int inter_rects(const BBox2 &rect1, const BBox2 &rect2);

//== NOTE: BUG! returns wrong results in many cases, might report no intersection when there's one
inline int VECTORCALL inter_circle_rect(Point2 c, real r, const BBox2 &rect);

inline real crossZ(Point2 &v1, Point2 &v2) { return v1.x * v2.y - v1.y * v2.x; }
int VECTORCALL get_nearest_point_index(Point2 p, Point2 *pts, int num);
bool VECTORCALL get_lines_intersection(Point2 st1, Point2 en1, Point2 st2, Point2 en2, Point2 &intPt);
inline bool VECTORCALL get_lines_intersection(Point2 st1, Point2 en1, Point2 st2, Point2 en2, float &t)
{
  Point2 v1 = en1 - st1;
  Point2 v2 = en2 - st2;
  float det = v1.x * v2.y - v1.y * v2.x;

  if (fabsf(det) < FLT_EPSILON)
    return false;

  t = (v2.x * (st1.y - st2.y) - v2.y * (st1.x - st2.x)) / det;
  return true;
}
bool VECTORCALL get_lines_intersection(Point2 start1, Point2 end1, Point2 start2, Point2 end2, Point2 *out_intersection);
bool VECTORCALL is_lines_intersect(Point2 p11, Point2 p12, Point2 p21, Point2 p22);
real VECTORCALL distance_point_to_line_segment(Point2 pt, Point2 p1, Point2 p2);


// NOTE: Line is specified as point and direction vector.
bool VECTORCALL isect_line_box(Point2 p0, Point2 dir, const BBox2 &box, real &min_t, real &max_t);


// Returns:
//   0 - no intersection
//   1 - intersection
//   2 - line segment is inside box
int VECTORCALL isect_line_segment_box(Point2 p1, Point2 p2, const BBox2 &box);

//==============================================================================
// is_point_in functions
//==============================================================================
inline bool VECTORCALL is_point_in_circle(Point2 p, Point2 c, real r) { return lengthSq(c - p) <= r * r; }


//==============================================================================
inline bool VECTORCALL is_point_in_circle_sq(Point2 p, Point2 c, real r_sq) { return lengthSq(c - p) <= r_sq; }


//==============================================================================
inline bool VECTORCALL is_point_in_rect(Point2 p, const BBox2 &rect)
{
  return p.x >= rect[0].x && p.x <= rect[1].x && p.y >= rect[0].y && p.y <= rect[1].y;
}


//==============================================================================
inline bool VECTORCALL is_point_in_rect(Point2 p, Point2 start, Point2 end, real width)
{
  const Point2 norm = normalize(end - start);
  const real x = (p - start) * norm;

  if (x >= 0 && x <= length(end - start))
    if (length(p - (start + x * norm)) <= width)
      return true;

  return false;
}


//==============================================================================
inline bool VECTORCALL is_point_in_ellipse(Point2 p, Point2 c, real a, real b)
{
  const Point2 coord = p - c;
  return (coord.x * coord.x) * (b * b) + (coord.y * coord.y) * (a * a) <= a * a * b * b;
}


//==============================================================================
inline bool VECTORCALL is_point_in_ellipse(Point2 p, Point2 c, real a, real b, Point2 a_dir)
{
  const Point2 coord = p - c;
  Point2 aDir = normalize(a_dir);
  Point2 bDir = Point2(aDir.y, -aDir.x);

  real x = coord * aDir;
  real y = coord * bDir;

  return x * x * b * b + y * y * a * a <= a * a * b * b;
}


//==============================================================================
inline bool VECTORCALL is_point_in_triangle(Point2 p, const Point2 *t) { return is_point_in_triangle(p, t[0], t[1], t[2]); }


//==============================================================================
inline bool VECTORCALL is_point_in_triangle(Point2 p, Point2 t1, Point2 t2, Point2 t3)
{
  real fi1 = (p.y - t1.y) * (t2.x - t1.x) - (p.x - t1.x) * (t2.y - t1.y);
  real fi2 = (p.y - t2.y) * (t3.x - t2.x) - (p.x - t2.x) * (t3.y - t2.y);
  if (DIFF_SIGN(fi1, fi2))
    return false;

  real fi3 = (p.y - t3.y) * (t1.x - t3.x) - (p.x - t3.x) * (t1.y - t3.y);
  if (DIFF_SIGN(fi2, fi3) || DIFF_SIGN(fi1, fi3))
    return false;

  return true;
}

//==============================================================================
inline Point2 VECTORCALL get_random_point_in_triangle(Point2 a, Point2 b, Point2 c)
{
  float r1 = gfrnd();
  float r2 = gfrnd();
  if (r2 + r1 <= 1.f)
    return a + (b - a) * r1 + (c - a) * r2;
  else
    return a + (b - a) * (1.f - r1) + (c - a) * (1.f - r2);
}


//==============================================================================
// intersection functions
// returns:
// 0 - no intersection
// 1 - intersection present
// 2 - obj1 completely located in obj2
// 3 - obj2 completely located in obj1
//==============================================================================
inline int VECTORCALL inter_circles(Point2 c1, real r1, Point2 c2, real r2)
{
  real dist = length(c2 - c1);

  if (dist > r1 + r2)
    return 0;
  if (dist + r1 <= r2)
    return 2;
  if (dist + r2 <= r1)
    return 3;

  return 1;
}


//==============================================================================
inline int inter_rects(const BBox2 &rect1, const BBox2 &rect2)
{
  if (rect1[1].x < rect2[0].x || rect1[0].x > rect2[1].x || rect1[1].y < rect2[0].y || rect1[0].y > rect2[1].y)
    return 0;

  if (is_point_in_rect(rect1[0], rect2) && is_point_in_rect(rect1[1], rect2))
    return 2;

  if (is_point_in_rect(rect2[0], rect1) && is_point_in_rect(rect2[1], rect1))
    return 3;

  return 1;
}


//==============================================================================
int VECTORCALL inter_circle_rect(Point2 c, real r, const BBox2 &rect);
bool VECTORCALL isect_box_triangle(const BBox2 &box, Point2 t1, Point2 t2, Point2 t3);

// line, not segment
inline bool VECTORCALL test_2d_line_rect_intersection_b(vec4f a, vec4f b, vec4f bbox2d)
{
  vec4f x1 = v_splat_x(a);
  vec4f y1 = v_splat_y(a);
  vec4f x2 = v_splat_x(b);
  vec4f y2 = v_splat_y(b);
  vec4f px = v_perm_xxzz(bbox2d);
  vec4f py = v_perm_ywyw(bbox2d);
  vec4f side = v_madd(v_sub(y2, y1), px, v_mul(v_sub(x1, x2), py));
  side = v_add(side, v_msub(x2, y1, v_mul(x1, y2)));
  int mask = v_signmask(side);
  return mask != 0 && mask != 0b1111;
}

inline bool VECTORCALL test_2d_line_rect_intersection_b(const Point2 &a, const Point2 &b, const BBox2 &bbox2d)
{
  return test_2d_line_rect_intersection_b(v_ldu_half(&a), v_ldu_half(&b), v_ldu(&bbox2d.lim[0].x));
}
