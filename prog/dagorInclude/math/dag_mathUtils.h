//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include "math/dag_Point2.h"
#include "math/dag_Point3.h"
#include "math/dag_Point4.h"
#include "math/dag_TMatrix.h"
#include "math/dag_TMatrix4.h"
#include "math/dag_bounds3.h"
#include <math/dag_bounds2.h>
#include <vecmath/dag_vecMath.h>
#include <EASTL/utility.h> // swap

extern const Point3 P3_DOWN;

inline bool check_nan(const Point4 &p4) { return check_nan(p4.x) || check_nan(p4.y) || check_nan(p4.z) || check_nan(p4.w); }
inline bool check_nan(const Point3 &p3) { return check_nan(p3.x) || check_nan(p3.y) || check_nan(p3.z); }
inline bool check_nan(const DPoint3 &p3) { return check_nan(p3.x) || check_nan(p3.y) || check_nan(p3.z); }
inline bool check_nan(const Point2 &p2) { return check_nan(p2.x) || check_nan(p2.y); }
inline bool check_nan(const TMatrix &m)
{
  return check_nan(m.getcol(0)) || check_nan(m.getcol(1)) || check_nan(m.getcol(2)) || check_nan(m.getcol(3));
}

inline Point3 normalizeDef(const Point3 &a, const Point3 &def)
{
  real l = length(a);
  if (l > 1e-9)
  {
    real il = 1.f / l;
    return Point3(a.x * il, a.y * il, a.z * il);
  }
  else
    return def;
}

inline void normalizeDef(Point3 &a, const Point3 &def)
{
  real l = length(a);
  if (l > 1e-9)
  {
    real il = 1.f / l;
    a = Point3(a.x * il, a.y * il, a.z * il);
  }
  else
  {
    a = def;
    return;
  }
}

inline Point2 normalizeDef(const Point2 &a, const Point2 &def)
{
  real l = length(a);
  if (l > 1e-9)
  {
    real il = 1.f / l;
    return Point2(a.x * il, a.y * il);
  }
  else
    return def;
}

inline void normalizeDef(Point2 &a, const Point2 &def)
{
  real l = length(a);
  if (l > 1e-9)
  {
    real il = 1.f / l;
    a = Point2(a.x * il, a.y * il);
  }
  else
  {
    a = def;
    return;
  }
}

inline bool is_zero_length(const Point3 &p) { return p.x == 0.f && p.y == 0.f && p.z == 0.f; }

// Makes vectors normalized and orthogonal to each other.
// Normalizes forward. Normalizes up and makes sure it is orthogonal to forward
// Creates orthonormal basis of 3 unit vectors
inline void ortho_normalize(const Point3 &forward, const Point3 &up, Point3 &out_forward, Point3 &out_up, Point3 &out_right)
{
  out_forward = normalizeDef(forward, Point3(1.0f, 0.0f, 0.0f));
  out_up = normalizeDef(up, Point3(0.0f, 1.0f, 0.0f));
  if (fabsf(out_forward * out_up) >= 1.0f - REAL_EPS)
    out_up = fabsf(out_up * Point3(0, 1, 0)) >= 1.0f - REAL_EPS ? Point3(0, 0, 1) : Point3(0, 1, 0);

  out_right = normalize(out_forward % out_up);
  out_up = normalize(out_right % out_forward);
}

// Creates a rotation with the specified forward and upwards directions. Returns the computed quaternion.
inline Quat quat_rotation_look(const Point3 &forward, const Point3 &upward)
{
  Point3 f, up, left;
  ortho_normalize(forward, upward, f, up, left);

  TMatrix mat;
  mat.setcol(0, f);
  mat.setcol(1, up);
  mat.setcol(2, left);

  return Quat(mat);
}

inline float ssmooth(float t, float min = 0.f, float max = 1.f) { return min + t * t * (3 - 2 * t) * (max - min); }

template <class T>
inline T sign(T x)
{
  return (x * 1 < 0) ? -1 : ((x > 0) ? 1 : 0); // *1 - to avoid pointers
};

// Integer division with round to positive infinity (ceil).
template <class T>
inline T div_ceil(T x, T y)
{
  T div_result = x / y;
  bool div_has_remainder = x % y != 0;
  if constexpr (std::is_signed_v<T>)
    return div_result + (((x < 0) == (y < 0)) && div_has_remainder);
  else
    return div_result + div_has_remainder;
}

template <typename T>
struct DoNotPutPointersIntoClamp
{};

template <typename T>
struct DoNotPutPointersIntoClamp<T *>;


template <typename T>
T cvt_type_(T val, T vmin, T vmax, T omin, T omax)
{
  using eastl::swap;
  if (vmin > vmax)
  {
    swap(vmin, vmax);
    swap(omin, omax);
  }

  if (val <= vmin)
    return omin;
  if (val >= vmax)
    return omax;

  T d = vmax - vmin;
  return fsel(vmin - val, omin, omin + safediv((omax - omin) * (val - vmin), d));
}


inline float cvt(float val, float vmin, float vmax, float omin, float omax) { return cvt_type_<float>(val, vmin, vmax, omin, omax); }

inline double cvt_double(double val, double vmin, double vmax, double omin, double omax)
{
  return cvt_type_<double>(val, vmin, vmax, omin, omax);
}

inline float piecewise_linear_interpolation(float x, const Point2 points[], int count)
{
  for (int i = 0; i < count - 1; i++)
    if (x <= points[i + 1].x || (i == count - 2))
    {
      float k = safediv(points[i + 1].y - points[i].y, points[i + 1].x - points[i].x);
      return (x - points[i].x) * k + points[i].y;
    }

  return count < 1 ? 0.f : points[0].y;
}


// Compute regular cubic hermite spline with tangents defined by hand. The same curve is default interpolation
// curve in 3d studio max (TCB controler - parameters Tension, Continuity, Bias are used to calculate tangents)
inline float chs(float val, float vmin, float vmax, float omin, float omax, float tmin, float tmax)
{
  float t = cvt(val, vmin, vmax, 0.0f, 1.0f);
  float dt = fabsf(vmax - vmin);

  float m0 = tmin * dt;
  float m1 = tmax * dt;

  float t2 = t * t;
  float t3 = t2 * t;

  float a = 2 * t3 - 3 * t2 + 1;
  float b = t3 - 2 * t2 + t;
  float c = t3 - t2;
  float d = -2 * t3 + 3 * t2;

  return a * omin + b * m0 + c * m1 + d * omax;
}

inline float move_to(float from, float to, float dt, float vel)
{
  float d = vel * dt;
  if (fabsf(from - to) < d)
    return to;

  if (to < from)
    return from - d;
  else
    return from + d;
}

inline float angle_diff(float source, float target) { return norm_s_ang(target - source); }

inline Point2 angle_diff(const Point2 &source, const Point2 &target)
{
  return {norm_s_ang(target.x - source.x), norm_s_ang(target.y - source.y)};
}

inline float angle_move_to(float from, float to, float dt, float vel)
{
  float d = vel * dt;
  float angleD = angle_diff(from, to);
  if (fabsf(angleD) < d)
    return to;

  if (angleD < 0)
    return from - d;
  else
    return from + d;
}

inline float angle_approach(float from_radians, float to_radians, float dt, float viscosity)
{
  float angleD = angle_diff(from_radians, to_radians);
  if (fabsf(angleD) <= 2.39e-7f) // error in the last mantissa bit for angles in radians
    return to_radians;

  return approach(from_radians, from_radians + angleD, dt, viscosity);
}

__forceinline bbox3f v_ldu_bbox3(const BBox3 &bbox) { return bbox3f{v_ldu(&bbox.lim[0].x), v_ldu_p3(&bbox.lim[1].x)}; }

__forceinline void v_stu_bbox3(BBox3 &out, bbox3f_cref box)
{
  vec4f v1 = v_perm_yzwx(box.bmax);
  vec4f v0 = v_perm_xyzd(box.bmin, v1);
  v_stu(&out.lim[0].x, v0);
  v_stu_half(&out.lim[1].y, v1);
}

template <class T>
inline T closest_pt_on_line(const T &point, const T &a, const T &b)
{
  T dir = normalize(b - a);
  float t = (point - a) * dir;
  return a + dir * t;
};

template <class T>
inline T closest_pt_on_line(const T &point, const T &a, const T &b, float &t)
{
  T dir = normalize(b - a);
  t = (point - a) * dir;
  return a + dir * t;
};

template <class T>
float distanceToLine(const T &point, const T &a, const T &dir, float &t, T &pt)
{
  t = (point - a) * dir;
  pt = a + dir * t;
  return length(point - pt);
};

template <class T>
T closest_pt_on_ray(const T &point, const T &a, const T &dir)
{
  float t = (point - a) * dir;
  if (t < 0)
    return a;

  return a + dir * t;
}

template <class T>
float distanceSqToRay(const T &point, const T &a, const T &dir, T &pt)
{
  pt = closest_pt_on_ray(point, a, dir);
  return lengthSq(point - pt);
};

template <class T>
T closest_pt_on_seg(const T &point, const T &a, const T &b, float &t)
{
  T dir = normalize(b - a);
  t = (point - a) * dir;
  if (t < 0)
    return a;
  else if (t * t > lengthSq(b - a))
    return b;

  return a + dir * t;
}

template <class T>
float distanceSqToSeg(const T &point, const T &a, const T &b, float &t, T &pt)
{
  pt = closest_pt_on_seg(point, a, b, t);
  return lengthSq(point - pt);
};

template <class T>
float distanceToSeg(const T &point, const T &a, const T &b, float &t, T &pt)
{
  pt = closest_pt_on_seg(point, a, b, t);
  return length(point - pt);
};

void lookAt(const Point3 &eye, const Point3 &at, const Point3 &up, TMatrix &resultTm);

__forceinline bool test_segment_box_intersection(const Point3 &start1, const Point3 &end1, const BBox3 &box1)
{
  vec3f start = v_ldu(&start1.x);
  vec3f end = v_ldu(&end1.x);
  bbox3f box = v_ldu_bbox3(box1);
  return v_test_segment_box_intersection(start, end, box);
}

// warning: use carefully with small numbers. rayIntersectSphereDist can be better.
inline bool test_segment_sphere_intersection(const Point3 &p0, const Point3 &p1, const Point3 &sphere_center,
  float squared_sphere_radius, float &dist)
{
  Point3 oc = sphere_center - p0;
  float l2oc = oc * oc;
  if (l2oc < squared_sphere_radius)
  {
    dist = 0.f;
    return true; // Starts inside of the sphere.
  }

  Point3 dir = p1 - p0;
  float tca = oc * dir;
  if (tca <= 0.f)
    return false; // Points away.

  float dir2 = dir * dir;
  float tca2 = (tca * tca) / dir2;
  float l2hc = squared_sphere_radius - l2oc + tca2;
  if (l2hc < 0.f)
    return false; // Misses the sphere.

  float d2 = tca2 + l2hc - 2.f * sqrtf(tca2 * l2hc);
  if (d2 > dir2)
    return false; // Segment ends.

  dist = d2 > 0.f ? sqrtf(d2) : 0.f;
  return true;
}


inline bool test_segment_sphere_intersection(const Point3 &p0, const Point3 &p1, const Point3 &sphere_center,
  float squared_sphere_radius)
{
  Point3 oc = sphere_center - p0;
  float l2oc = oc * oc;
  if (l2oc < squared_sphere_radius)
    return true; // Starts inside of the sphere.

  Point3 dir = p1 - p0;
  float tca = oc * dir;
  if (tca <= 0.f)
    return false; // Points away.

  float dir2 = dir * dir;
  float tca2 = (tca * tca) / dir2;
  float l2hc = squared_sphere_radius - l2oc + tca2;
  if (l2hc < 0.f)
    return false; // Misses the sphere.

  if (tca2 + l2hc - 2.f * sqrtf(tca2 * l2hc) > dir2)
    return false; // Segment ends.

  return true;
}

// Returns closest point (not first one) and ignores intersections if origin is inside bbox.
inline bool rayIntersectWaBoxIgnoreInside(const Point3 &p0, const Point3 &dir, const Point3 &box_sz, float &out_t, Point3 &outNorm)
{
#define TEST_PLANES(NX, NY, NZ, X, Y, Z)                                           \
  den = dir.x * NX + dir.y * NY + dir.z * NZ;                                      \
  if (float_nonzero(den))                                                          \
  {                                                                                \
    t = -(p0.x * NX + p0.y * NY + p0.z * NZ) / den;                                \
    if (t > 0)                                                                     \
    {                                                                              \
      float Y = p0.Y + dir.Y * t;                                                  \
      float Z = p0.Z + dir.Z * t;                                                  \
      if (Y >= -1e-3 && Y <= box_sz.Y && Z >= -1e-3 && Z <= box_sz.Z && t < out_t) \
      {                                                                            \
        out_t = t;                                                                 \
        outNorm.set(-NX, -NY, -NZ);                                                \
        contact = true;                                                            \
      }                                                                            \
    }                                                                              \
    t = t + box_sz.X / den;                                                        \
    if (t > 0)                                                                     \
    {                                                                              \
      float Y = p0.Y + dir.Y * t;                                                  \
      float Z = p0.Z + dir.Z * t;                                                  \
      if (Y >= -1e-3 && Y <= box_sz.Y && Z >= -1e-3 && Z <= box_sz.Z && t < out_t) \
      {                                                                            \
        out_t = t;                                                                 \
        outNorm.set(NX, NY, NZ);                                                   \
        contact = true;                                                            \
      }                                                                            \
    }                                                                              \
  }

  float den, t;
  bool contact = false;
  if (p0.x > 0.f && p0.y > 0.f && p0.z > 0.f && p0.x < box_sz.x && p0.y < box_sz.y && p0.z < box_sz.z)
    return false;

  TEST_PLANES(1, 0, 0, x, y, z);
  TEST_PLANES(0, 1, 0, y, x, z);
  TEST_PLANES(0, 0, 1, z, x, y);

#undef TEST_PLANES
  return contact;
}

// Returns closest point (not first one) and ignores intersections if origin is inside bbox.
__forceinline bool rayIntersectBoxIgnoreInside(const Point3 &p0, const Point3 &dir, const BBox3 &box, const TMatrix &tm, float &t,
  Point3 &outNorm)
{
  TMatrix itm = inverse(tm);
  bool res = rayIntersectWaBoxIgnoreInside(itm * p0 - box[0], itm % dir, box[1] - box[0], t, outNorm);
  outNorm = tm % outNorm;
  return res;
}

inline bool test_box_box_intersection(const BBox3 &box0, const BBox3 &box1, const TMatrix &tm1)
{
  // Cutoff for cosine of angles between box axes.  This is used to catch
  // the cases when at least one pair of axes are parallel.  If this happens,
  // there is no need to test for separation along the Cross(A[i],B[j])
  // directions.
  const float fCutoff = 0.999999f;
  bool bExistsParallelPair = false;
  int i;

  // convenience variables
  Point3 akB[3] = {normalize(tm1.getcol(0)), normalize(tm1.getcol(1)), normalize(tm1.getcol(2))};

  Point3 afEA = 0.5f * box0.width();
  Point3 afEB = 0.5f * box1.width();
  afEB.x *= tm1.getcol(0).length();
  afEB.y *= tm1.getcol(1).length();
  afEB.z *= tm1.getcol(2).length();

  // compute difference of box centers, D = C1-C0
  Point3 kD = tm1 * box1.center() - box0.center();

  float aafC[3][3];    // matrix C = A^T B, c_{ij} = Dot(A_i,B_j)
  float aafAbsC[3][3]; // |c_{ij}|
  float afAD[3];       // Dot(A_i,D)
  float fR0, fR1, fR;  // interval radii and distance between centers
  float fR01;          // = R0 + R1

  // axis C0+t*A0
  for (i = 0; i < 3; i++)
  {
    aafC[0][i] = akB[i].x;
    aafAbsC[0][i] = fabsf(aafC[0][i]);
    if (aafAbsC[0][i] > fCutoff)
      bExistsParallelPair = true;
  }
  afAD[0] = kD.x;
  fR = fabsf(afAD[0]);
  fR1 = afEB[0] * aafAbsC[0][0] + afEB[1] * aafAbsC[0][1] + afEB[2] * aafAbsC[0][2];
  fR01 = afEA[0] + fR1;
  if (fR > fR01)
    return false;

  // axis C0+t*A1
  for (i = 0; i < 3; i++)
  {
    aafC[1][i] = akB[i].y;
    aafAbsC[1][i] = fabsf(aafC[1][i]);
    if (aafAbsC[1][i] > fCutoff)
      bExistsParallelPair = true;
  }
  afAD[1] = kD.y;
  fR = fabsf(afAD[1]);
  fR1 = afEB[0] * aafAbsC[1][0] + afEB[1] * aafAbsC[1][1] + afEB[2] * aafAbsC[1][2];
  fR01 = afEA[1] + fR1;
  if (fR > fR01)
    return false;

  // axis C0+t*A2
  for (i = 0; i < 3; i++)
  {
    aafC[2][i] = akB[i].z;
    aafAbsC[2][i] = fabsf(aafC[2][i]);
    if (aafAbsC[2][i] > fCutoff)
      bExistsParallelPair = true;
  }
  afAD[2] = kD.z;
  fR = fabsf(afAD[2]);
  fR1 = afEB[0] * aafAbsC[2][0] + afEB[1] * aafAbsC[2][1] + afEB[2] * aafAbsC[2][2];
  fR01 = afEA[2] + fR1;
  if (fR > fR01)
    return false;

  // axis C0+t*B0
  fR = fabsf(akB[0] * kD);
  fR0 = afEA[0] * aafAbsC[0][0] + afEA[1] * aafAbsC[1][0] + afEA[2] * aafAbsC[2][0];
  fR01 = fR0 + afEB[0];
  if (fR > fR01)
    return false;

  // axis C0+t*B1
  fR = fabsf(akB[1] * kD);
  fR0 = afEA[0] * aafAbsC[0][1] + afEA[1] * aafAbsC[1][1] + afEA[2] * aafAbsC[2][1];
  fR01 = fR0 + afEB[1];
  if (fR > fR01)
    return false;

  // axis C0+t*B2
  fR = fabsf(akB[2] * kD);
  fR0 = afEA[0] * aafAbsC[0][2] + afEA[1] * aafAbsC[1][2] + afEA[2] * aafAbsC[2][2];
  fR01 = fR0 + afEB[2];
  if (fR > fR01)
    return false;

  // At least one pair of box axes was parallel, so the separation is
  // effectively in 2D where checking the "edge" normals is sufficient for
  // the separation of the boxes.
  if (bExistsParallelPair)
    return true;

  // axis C0+t*A0xB0
  fR = fabsf(afAD[2] * aafC[1][0] - afAD[1] * aafC[2][0]);
  fR0 = afEA[1] * aafAbsC[2][0] + afEA[2] * aafAbsC[1][0];
  fR1 = afEB[1] * aafAbsC[0][2] + afEB[2] * aafAbsC[0][1];
  fR01 = fR0 + fR1;
  if (fR > fR01)
    return false;

  // axis C0+t*A0xB1
  fR = fabsf(afAD[2] * aafC[1][1] - afAD[1] * aafC[2][1]);
  fR0 = afEA[1] * aafAbsC[2][1] + afEA[2] * aafAbsC[1][1];
  fR1 = afEB[0] * aafAbsC[0][2] + afEB[2] * aafAbsC[0][0];
  fR01 = fR0 + fR1;
  if (fR > fR01)
    return false;

  // axis C0+t*A0xB2
  fR = fabsf(afAD[2] * aafC[1][2] - afAD[1] * aafC[2][2]);
  fR0 = afEA[1] * aafAbsC[2][2] + afEA[2] * aafAbsC[1][2];
  fR1 = afEB[0] * aafAbsC[0][1] + afEB[1] * aafAbsC[0][0];
  fR01 = fR0 + fR1;
  if (fR > fR01)
    return false;

  // axis C0+t*A1xB0
  fR = fabsf(afAD[0] * aafC[2][0] - afAD[2] * aafC[0][0]);
  fR0 = afEA[0] * aafAbsC[2][0] + afEA[2] * aafAbsC[0][0];
  fR1 = afEB[1] * aafAbsC[1][2] + afEB[2] * aafAbsC[1][1];
  fR01 = fR0 + fR1;
  if (fR > fR01)
    return false;

  // axis C0+t*A1xB1
  fR = fabsf(afAD[0] * aafC[2][1] - afAD[2] * aafC[0][1]);
  fR0 = afEA[0] * aafAbsC[2][1] + afEA[2] * aafAbsC[0][1];
  fR1 = afEB[0] * aafAbsC[1][2] + afEB[2] * aafAbsC[1][0];
  fR01 = fR0 + fR1;
  if (fR > fR01)
    return false;

  // axis C0+t*A1xB2
  fR = fabsf(afAD[0] * aafC[2][2] - afAD[2] * aafC[0][2]);
  fR0 = afEA[0] * aafAbsC[2][2] + afEA[2] * aafAbsC[0][2];
  fR1 = afEB[0] * aafAbsC[1][1] + afEB[1] * aafAbsC[1][0];
  fR01 = fR0 + fR1;
  if (fR > fR01)
    return false;

  // axis C0+t*A2xB0
  fR = fabsf(afAD[1] * aafC[0][0] - afAD[0] * aafC[1][0]);
  fR0 = afEA[0] * aafAbsC[1][0] + afEA[1] * aafAbsC[0][0];
  fR1 = afEB[1] * aafAbsC[2][2] + afEB[2] * aafAbsC[2][1];
  fR01 = fR0 + fR1;
  if (fR > fR01)
    return false;

  // axis C0+t*A2xB1
  fR = fabsf(afAD[1] * aafC[0][1] - afAD[0] * aafC[1][1]);
  fR0 = afEA[0] * aafAbsC[1][1] + afEA[1] * aafAbsC[0][1];
  fR1 = afEB[0] * aafAbsC[2][2] + afEB[2] * aafAbsC[2][0];
  fR01 = fR0 + fR1;
  if (fR > fR01)
    return false;

  // axis C0+t*A2xB2
  fR = fabsf(afAD[1] * aafC[0][2] - afAD[0] * aafC[1][2]);
  fR0 = afEA[0] * aafAbsC[1][2] + afEA[1] * aafAbsC[0][2];
  fR1 = afEB[0] * aafAbsC[2][1] + afEB[1] * aafAbsC[2][0];
  fR01 = fR0 + fR1;
  if (fR > fR01)
    return false;

  return true;
}

inline int does_line_intersect_box_side_two_points(const BBox3 &box, const Point3 &line_start, const Point3 &line_end, real &at,
  real &at_max)
{
  int ret = -1;
  for (int j = 0; j < 3; ++j)
  {
    BBox2 box2;
    const int j1_mask = (j + 1 - 3) >> 4; //>3 == 0 <3 == -1
    const int j2_mask = (j + 2 - 3) >> 4;
    const int j1 = ((j + 1) & j1_mask) | ((j + 1 - 3) & ~j1_mask);
    const int j2 = ((j + 2) & j2_mask) | ((j + 2 - 3) & ~j2_mask);
    box2[0] = Point2(box[0][j1], box[0][j2]);
    box2[1] = Point2(box[1][j1], box[1][j2]);
    const real denominator = line_end[j] - line_start[j];
    if (rabs(denominator) > REAL_EPS)
    {
      for (int i = 0; i < 2; ++i)
      {
        const real v1 = line_start[j] - box[i][j];
        const real v2 = line_end[j] - box[i][j];
        const real numerator = box[i][j] - line_start[j];
        if ((rabs(v2) > REAL_EPS) && (v1 * v2 > 0)) // same signs
          continue;

        const real at2 = numerator / denominator;
        if (at2 >= 0)
        {
          Point3 p = at2 * (line_end - line_start) + line_start;
          if (box2 & Point2(p[j1], p[j2]))
          {
            if (ret == -1)
            {
              at = at2;
              at_max = at2;
              ret = j + i * 3;
            }
            else if (at2 < at)
            {
              at = at2;
              ret = j + i * 3;
            }
            at_max = max(at2, at_max);
          }
        }
      }
    }
  }
  return ret;
}

int does_line_intersect_box_side(const BBox3 &box, const Point3 &line_start, const Point3 &line_end, real &at);

inline bool does_line_intersect_box(const BBox3 &box, const Point3 &line_start, const Point3 &line_end, real &at)
{
  return does_line_intersect_box_side(box, line_start, line_end, at) != -1;
}

bool test_bbox_bbox_intersection(const BBox3 &box0, const BBox3 &box1, const TMatrix &tm1);
bool check_bbox_intersection(const BBox3 &box0, const TMatrix &tm0, const BBox3 &box1, const TMatrix &tm1);
bool check_bbox_intersection(const BBox3 &box0, const TMatrix &tm0, const BBox3 &box1, const TMatrix &tm1, float size_factor);

inline BBox3 scale_box(const BBox3 &bbox, float size_factor)
{
  const Point3 center = bbox.center();
  BBox3 result;
  result.lim[0] = lerp(center, bbox.lim[0], size_factor);
  result.lim[1] = lerp(center, bbox.lim[1], size_factor);
  return result;
}

inline float get_box_bounding_plane_dist(const BBox3 &bbox, const Point3 &axis)
{
  float dist = -VERY_BIG_NUMBER;
  for (int i = 0; i < 8; ++i)
    dist = max(axis * bbox.point(i), dist);
  return dist;
}

inline float perlin_noise_1d(int x)
{
  const unsigned int prime0 = 19379;
  const unsigned int prime1 = 819233;
  const unsigned int prime2 = 1266122899;

  x = (x << 13) ^ x;
  x = x * (x * x * prime0 + prime1) + prime2;
  return (float(x & 0x7fffffff) / 1073741823.5f) - 1.f;
}


inline float perlin_soft_noise_1d(int x)
{
  float a = perlin_noise_1d((x >> 16));
  float b = perlin_noise_1d((x >> 16) + 1);
  float t = float(x & 0xffff) / 65536.f;
  t = (1.f - cosf(t * PI)) * 0.5f;
  return a * (1.f - t) + b * t;
}

// Point on line p1p2 closest to line p3p4 is at p1 + mua * (p2 - p1)
inline void lineLineIntersect(const Point3 &p1, const Point3 &p2, const Point3 &p3, const Point3 &p4, float &mua, float &mub)
{
  Point3 p13 = p1 - p3;
  Point3 p43 = p4 - p3;
  Point3 p21 = p2 - p1;
  float p43lenSq = p43.lengthSq();

  float denom = p21.lengthSq() * p43lenSq - sqr(p43 * p21);
  float numer = (p13 * p43) * (p43 * p21) - p13 * p21 * p43lenSq;

  mua = denom > REAL_EPS ? numer / denom : denom;
  mub = p43lenSq > REAL_EPS ? (p13 * p43 + p43 * p21 * mua) / p43lenSq : p43lenSq;
}

inline BBox2 to2dBox(const BBox3 &box) { return BBox2(Point2::xz(box.lim[0]), Point2::xz(box.lim[1])); }

inline Quat matrix_to_quat(const TMatrix &m)
{
  Quat quat;

  float tr, s;
  float q[4];
  int i, j, k;

  int nxt[3] = {1, 2, 0};

  tr = m[0][0] + m[1][1] + m[2][2];

  // check the diagonal

  if (tr > 0.0)
  {
    s = sqrtf(tr + 1.f);

    quat.w = s / 2.f;

    s = 0.5f / s;

    quat.x = (m[1][2] - m[2][1]) * s;
    quat.y = (m[2][0] - m[0][2]) * s;
    quat.z = (m[0][1] - m[1][0]) * s;
  }
  else
  {

    // diagonal is negative

    i = 0;

    if (m[1][1] > m[0][0])
      i = 1;

    if (m[2][2] > m[i][i])
      i = 2;

    j = nxt[i];
    k = nxt[j];

    s = sqrtf(fabsf((m[i][i] - (m[j][j] + m[k][k])) + 1.f));

    q[i] = s * 0.5f;

    if (s != 0.f)
      s = 0.5f / s;

    q[3] = (m[j][k] - m[k][j]) * s;
    q[j] = (m[i][j] + m[j][i]) * s;
    q[k] = (m[i][k] + m[k][i]) * s;

    quat.x = q[0];
    quat.y = q[1];
    quat.z = q[2];
    quat.w = q[3];
  }

  return quat;
}

inline TMatrix quat_to_matrix(const Quat &quat)
{
  float wx, wy, wz, xx, yy, yz, xy, xz, zz, x2, y2, z2;

  x2 = quat.x + quat.x;
  y2 = quat.y + quat.y;
  z2 = quat.z + quat.z;

  xx = quat.x * x2;
  xy = quat.x * y2;
  xz = quat.x * z2;

  yy = quat.y * y2;
  yz = quat.y * z2;
  zz = quat.z * z2;

  wx = quat.w * x2;
  wy = quat.w * y2;
  wz = quat.w * z2;

  TMatrix m;

  m[0][0] = 1.f - (yy + zz);
  m[1][0] = xy - wz;
  m[2][0] = xz + wy;
  m[3][0] = 0.f;

  m[0][1] = xy + wz;
  m[1][1] = 1.f - (xx + zz);
  m[2][1] = yz - wx;
  m[3][1] = 0.f;

  m[0][2] = xz - wy;
  m[1][2] = yz + wx;
  m[2][2] = 1.f - (xx + yy);
  m[3][2] = 0.f;

  return m;
}

// Clamps to [0, 1]
inline float saturate(float f) { return fsel(f - 1.0f, 1.0f, fsel(f, f, 0.0f)); }
inline double saturate(double f) { return fsel(f - 1.0, 1.0, fsel(f, f, 0.0)); }

template <typename T>
inline int get_positional_seed(const T &p, float step)
{
  return int(p.x * step) ^ int(p.y * step) ^ int(p.z * step);
}

inline unsigned int solve_square_equation(float a, float b, float c, float (&out_x)[2])
{
  if (rabs(a) < FLT_EPSILON)
  {
    if (rabs(b) < FLT_EPSILON)
      return 0u;
    else
    {
      out_x[0] = -c / b;
      return 1u;
    }
  }
  float D = sqr(b) - 4.0f * a * c;
  if (D < 0.0f)
    return 0u;
  else if (rabs(D) < FLT_EPSILON)
  {
    out_x[0] = -b / (2.0f * a);
    return 1u;
  }
  else
  {
    const float sqrtD = sqrtf(D);
    out_x[0] = (-b + sqrtD) / (2.0f * a);
    out_x[1] = (-b - sqrtD) / (2.0f * a);
    return 2u;
  }
}

// Evaluating polynomial
template <unsigned int size>
inline float poly_impl(const float *tab, float v)
{
  return tab[0] + v * poly_impl<size - 1>(tab + 1, v);
}

template <>
inline float poly_impl<0>(const float *tab, float v)
{
  tab += 0;
  return v * 0.0f;
}

template <unsigned int size>
inline float poly(const float (&tab)[size], float v)
{
  return poly_impl<size>(tab, v);
}

template <unsigned int size>
inline double poly_impl(const double *tab, double v)
{
  return tab[0] + v * poly_impl<size - 1>(tab + 1, v);
}

template <>
inline double poly_impl<0>(const double *tab, double v)
{
  tab += 0;
  return v * 0.0f;
}

template <unsigned int size>
inline double poly(const double (&tab)[size], double v)
{
  return poly_impl<size>(tab, v);
}

// Solving func(arg) = 0 by Newton method
template <typename Func, typename T>
bool solve_newton(const Func &func, T arg, T delta_arg, T epsilon, unsigned int step_count_max, T &out_result)
{
  for (unsigned int i = 0u; i < step_count_max; ++i)
  {
    const T val = func(arg);
    const T der = safediv(func(arg + delta_arg) - val, delta_arg);
    if (rabs(der) < VERY_SMALL_NUMBER)
      return false;
    const T arg_correction = -val / der;
    arg += arg_correction;
    if (rabs(arg_correction) < epsilon)
    {
      out_result = arg;
      return true;
    }
  }
  return false;
}

template <typename Func, typename Derr, typename T>
bool solve_newton_derr(const Func &func, const Derr &derr, T arg, T epsilon, unsigned int step_count_max, T &out_result)
{
  for (unsigned int i = 0u; i < step_count_max; ++i)
  {
    const T val = func(arg);
    const T der = derr(arg);
    if (rabs(der) < VERY_SMALL_NUMBER)
      return false;
    const T arg_correction = -val / der;
    arg += arg_correction;
    if (rabs(arg_correction) < epsilon)
    {
      out_result = arg;
      return true;
    }
  }
  return false;
}

// The golden section search is a technique for finding the extremum (minimum or maximum) of a strictly unimodal
// function by successively narrowing the range of values inside which the extremum is known to exist
// a, b - bounds of the interval
// diff = [-1; 1] - indicate which extremum to be find, where 1 - maximum extremum value, -1 minimum extremum value
// eps - maximum acceptable error on which search stops
// fn - function f(x) defined in [a; b]
template <class C, class T>
inline C golden_section_calculate(C a, C b, C eps, int diff, T fn, int max_num_iter = 100)
{
  static C gr = (sqrt((C)5) - (C)1) * (C)0.5;
  C c = b - gr * (b - a);
  C d = a + gr * (b - a);
  C fc = fn(c);
  C fd = fn(d);
  int iter = 0;
  while (fabs(c - d) > eps && iter++ < max_num_iter)
  {
    if ((fc - fd) * diff > 0)
    {
      b = d;
      d = c;
      c = b - gr * (b - a);
      fd = fc;
      fc = fn(c);
    }
    else
    {
      a = c;
      c = d;
      d = a + gr * (b - a);
      fc = fd;
      fd = fn(d);
    }
  }
  return (b + a) * (C)0.5;
}

// In numerical analysis, the secant method is a root-finding algorithm that uses
// a succession of roots of secant lines to better approximate a root of a function f
// a, b - bounds of the interval
// eps - maximum acceptable error on which search stops
// fn - function f(x) defined in [a; b]
template <class C, class T>
inline C secant_calculate(C a, C b, C eps, T fn, int max_num_iter = 100)
{
  int iter = 0;
  while (fabs(b - a) > eps && iter++ < max_num_iter)
  {
    C fa = fn(a);
    C fb = fn(b);
    if (fabs(fb - fa) < eps)
      return b;
    a = b - (b - a) * fabs(fb / (fb - fa));
    fa = fn(a);
    b = a + (a - b) * fabs(fa / (fb - fa));
  }
  return b;
}

// Modify roughness maps to reduce aliasing
// Based on The Order : 1886 SIGGRAPH course notes implementation
inline float adjust_roughness(float input_roughness, float avg_normal_length)
{
  if (avg_normal_length < 1.0f)
  {
    float avgNormLen2 = avg_normal_length * avg_normal_length;
    float kappa = safediv(3 * avg_normal_length - avg_normal_length * avgNormLen2, 1 - avgNormLen2);
    float variance = safediv(1.0f, 2.0f * kappa);
    return safe_sqrt(input_roughness * input_roughness + variance);
  }
  return input_roughness;
}

// component-wise product of two vectors
// 3d
template <typename T>
T hadamard_product(const T &lhs, const T &rhs)
{
  return T(lhs.x * rhs.x, lhs.y * rhs.y, lhs.z * rhs.z);
}
// 2d
template <typename T>
T hadamard_product_2d(const T &lhs, const T &rhs)
{
  return T(lhs.x * rhs.x, lhs.y * rhs.y);
}

inline float point_to_box_distance_sq(const Point3 &p, const BBox3 &box)
{
  float distSq = 0.f;
  for (int i = 0; i < 3; ++i)
    distSq += sqr(p[i] - clamp(p[i], box.lim[0][i], box.lim[1][i]));
  return distSq;
}

/// Set source's 'up' to the 'up' from v
inline Point3 basis_aware_xVz(const Point3 &source, const Point3 &v, const Point3 &up) { return source - (source - v) * up * up; }

/// Nullify 'up' component of source
inline Point3 basis_aware_x0z(const Point3 &source, const Point3 &up) { return source - source * up * up; }

inline Point3 relative_2d_dir_to_absolute_3d_dir(const Point2 &dir_2d, const Point3 &fwd, const Point3 &side)
{
  return fwd * dir_2d.x + side * dir_2d.y;
}

inline Point2 absolute_3d_dir_to_relative_2d_dir(const Point3 &dir_3d, const Point3 &fwd, const Point3 &side)
{
  return normalize(Point2(dir_3d * fwd, dir_3d * side));
}