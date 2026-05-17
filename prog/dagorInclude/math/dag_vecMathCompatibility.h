//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <vecmath/dag_vecMathDecl.h>
#include <vecmath/dag_vecMath.h>
#include <math/dag_Point3.h>

class Point4;
class Plane3;
class Quat;
class TMatrix4;

struct alignas(16) CvtStorage
{
  float v[4];
};

__forceinline Point4 &cvt_point4(vec4f v, CvtStorage &s)
{
  v_st(&s.v[0], v);
  return *(Point4 *)&s;
}
__forceinline Plane3 &cvt_plane3(vec4f v, CvtStorage &s)
{
  v_st(&s.v[0], v);
  return *(Plane3 *)&s;
}

/// these are unsafe operations!
/// pointer to vector doesn't require compiler to flush recently computed vector to memory (if you work with auto/on-stack variables),
/// and so result of addressing op (&) is invalid/undefined if you read immediately after you compute vector
/// (but valid if you read from 100% memory, or if you write)
__forceinline Point4 &as_point4(vec4f *v) { return *(Point4 *)(char *)v; }
__forceinline Point3 &as_point3(vec4f *v) { return *(Point3 *)(char *)v; }
__forceinline Plane3 &as_plane3(vec4f *v) { return *(Plane3 *)(char *)v; }

__forceinline const Point4 &as_point4(const vec4f *v) { return *(const Point4 *)(const char *)v; }
__forceinline const Point3 &as_point3(const vec4f *v) { return *(const Point3 *)(const char *)v; }
__forceinline const Plane3 &as_plane3(const vec4f *v) { return *(const Plane3 *)(const char *)v; }

VECMATH_FINLINE Point3 operator*(mat44f_cref tm, const Point3 &p)
{
  alignas(16) vec4f r = v_mat44_mul_vec3p(tm, v_ldu_p3(&p.x));
  return as_point3(&r);
}

VECMATH_FINLINE Point3 operator%(mat44f_cref tm, const Point3 &p)
{
  alignas(16) vec4f r = v_mat44_mul_vec3v(tm, v_ldu_p3(&p.x));
  return as_point3(&r);
}
