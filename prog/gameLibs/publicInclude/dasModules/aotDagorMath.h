//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <dasModules/dasModulesCommon.h>
#include <dasModules/dasDataBlock.h>
#include <dasModules/dasManagedTab.h>

#include <math/dag_math3d.h>
#include <math/dag_mathUtils.h>
#include <math/dag_bounds3.h>
#include <math/dag_Point3.h>
#include <math/dag_plane3.h>
#include <math/dag_capsule.h>
#include <math/dag_Quat.h>
#include <math/dag_mathAng.h>
#include <math/dag_mathBase.h>
#include <math/dag_noise.h>
#include <math/dag_interpolator.h>
#include <math/dag_frustum.h>
#include <math/dag_rayIntersectSphere.h>
#include <vecmath/dag_vecMathDecl.h>
#include <gameMath/interpolateTabUtils.h>

MAKE_TYPE_FACTORY(BBox3, BBox3)
MAKE_TYPE_FACTORY(BBox2, BBox2)
MAKE_TYPE_FACTORY(bbox3f, bbox3f)
MAKE_TYPE_FACTORY(BSphere3, BSphere3)
MAKE_TYPE_FACTORY(DPoint3, DPoint3)
MAKE_TYPE_FACTORY(Capsule, Capsule)
MAKE_TYPE_FACTORY(quat, Quat)
MAKE_TYPE_FACTORY(Plane3, Plane3)
MAKE_TYPE_FACTORY(InterpolateTabFloat, InterpolateTabFloat)
MAKE_TYPE_FACTORY(mat44f, mat44f)
MAKE_TYPE_FACTORY(Frustum, Frustum)

typedef Tab<int> IndicesTab;
DAS_BIND_VECTOR(IndicesTab, IndicesTab, int, " IndicesTab");

namespace das
{
template <>
struct cast<Quat> : cast_fVec<Quat>
{};
template <>
struct WrapType<Quat>
{
  enum
  {
    value = true
  };
  typedef Point4 type;
};
template <>
struct WrapRetType<Quat>
{
  typedef Point4 type;
};
class Quat_WrapArg : public Quat
{
public:
  Quat_WrapArg(const Point4 &t) : Quat(t.x, t.y, t.z, t.w) {}
};

template <>
struct WrapArgType<Quat>
{
  typedef Quat_WrapArg type;
};

template <>
struct cast<Plane3> : cast_fVec<Plane3>
{};
} // namespace das

inline DPoint3 operator*(float a, const DPoint3 &p) { return DPoint3(p.x * a, p.y * a, p.z * a); }

namespace bind_dascript
{

inline E3DCOLOR make_e3dcolor_from_color4(const Color4 &a) { return e3dcolor(a); }
inline E3DCOLOR make_color(das::uint4 a) { return E3DCOLOR(a.x, a.y, a.z, a.w); }
inline das::uint4 make_uint4(E3DCOLOR a) { return das::uint4{a.r, a.g, a.b, a.a}; }
inline uint32_t cast_to_uint(E3DCOLOR a) { return a.u; }
inline E3DCOLOR cast_from_uint(uint32_t a) { return E3DCOLOR(a); }

#if _MSC_VER && !defined(__clang__)
// this is UB/strict aliasing violation, but msvc is ignorant (and fast)
inline das::float3 from_color3(const Color3 &a) { return *(const das::float3 *)&a; }
inline das::float4 from_color4(const Color4 &a) { return *(const das::float4 *)&a; }
inline Color3 make_color3(das::float3 a) { return *(const Color3 *)&a; }
inline Color4 make_color4(das::float4 a) { return *(const Color4 *)&a; }
#else
// this is UB/strict aliasing violation, but union tye punning is ubiquotosly accepted by compilers
inline das::float3 from_color3(const Color3 &a)
{
  union
  {
    Color3 c;
    das::float3 f;
  } f2c;
  f2c.c = a;
  return f2c.f;
}
inline das::float4 from_color4(const Color4 &a)
{
  union
  {
    Color4 c;
    das::float4 f;
  } f2c;
  f2c.c = a;
  return f2c.f;
}
inline Color3 make_color3(das::float3 a)
{
  union
  {
    Color3 c;
    das::float3 f;
  } f2c;
  f2c.f = a;
  return f2c.c;
}
inline Color4 make_color4(das::float4 a)
{
  union
  {
    Color4 c;
    das::float4 f;
  } f2c;
  f2c.f = a;
  return f2c.c;
}
#endif
inline Color4 make_color4_2(das::float3 a, float alpha) { return Color4(a.x, a.y, a.z, alpha); }
inline Color4 make_color4_3(E3DCOLOR c) { return Color4(c); }
inline void bbox3_add_point(BBox3 &box, das::float3 point) { box += reinterpret_cast<Point3 &>(point); }
inline void bbox3_add_bbox3(BBox3 &box, const BBox3 &add) { box += add; }
inline void bbox3_inflate(BBox3 &box, float val) { box.inflate(val); }
inline void bbox3_inflateXZ(BBox3 &box, float val) { box.inflateXZ(val); }
inline bool bbox3_intersect_point(const BBox3 &box, das::float3 pos) { return box & reinterpret_cast<Point3 &>(pos); }
inline bool bbox3_intersect_bbox3(const BBox3 &box1, const BBox3 &box2) { return box1 & box2; }
inline bool bbox3_intersect_bsphere(const BBox3 &box, const BSphere3 &sphere) { return box & sphere; }
inline BBox3 make_empty_bbox3() { return BBox3(); }
inline BBox3 make_bbox3(das::float3 min, das::float3 max)
{
  return BBox3(reinterpret_cast<Point3 &>(min), reinterpret_cast<Point3 &>(max));
}
inline bool bsphere_intersect_bsphere(const BSphere3 &sphere1, const BSphere3 &sphere2) { return sphere1 & sphere2; }
inline BBox3 make_bbox_from_bsphere(const BSphere3 &sphere) { return BBox3(sphere); }
inline BSphere3 make_bsphere_from_bbox(const BBox3 &box)
{
  BSphere3 sphere;
  sphere = box;
  return sphere;
}
inline BSphere3 make_bsphere(const Point3 &center, float radius) { return BSphere3(center, radius); }
inline BBox3 make_bbox3_center(das::float3 center, float size) { return BBox3(reinterpret_cast<Point3 &>(center), size); }
inline float length_dpoint3(const DPoint3 &point) { return length(point); }
inline float length_sq_dpoint3(const DPoint3 &point) { return lengthSq(point); }
inline float distance_dpoint3(const DPoint3 &from, const DPoint3 &to) { return length(from - to); }
inline float distance_sq_dpoint3(const DPoint3 &from, const DPoint3 &to) { return lengthSq(from - to); }
inline float distance_plane_point3(const Point4 &plane, const Point3 &point)
{
  return v_extract_x(v_plane_dist_x(v_ldu(&plane.x), v_ldu(&point.x)));
}
inline das::float3 from_dpoint3(const DPoint3 &a) { return {float(a.x), float(a.y), float(a.z)}; }
inline DPoint3 dpoint3_from_point3(const das::float3 &a) { return DPoint3(double(a.x), double(a.y), double(a.z)); }
inline DPoint3 make_dpoint3(double x, double y, double z) { return {x, y, z}; }
inline bool check_bbox3_intersection(const BBox3 &box0, const TMatrix &tm0, const BBox3 &box1, const TMatrix &tm1)
{
  return check_bbox_intersection(box0, tm0, box1, tm1);
}
inline bool check_bbox3_intersection_scaled(const BBox3 &box0, const TMatrix &tm0, const BBox3 &box1, const TMatrix &tm1, float scale)
{
  return check_bbox_intersection(box0, tm0, box1, tm1, scale);
}
inline BBox3 bbox3_transform(const das::float3x4 &tm, const BBox3 &box) { return reinterpret_cast<const TMatrix &>(tm) * box; }
inline BBox3 bsphere_transform(const das::float3x4 &tm, const BSphere3 &sphere)
{
  return reinterpret_cast<const TMatrix &>(tm) * sphere;
}

inline void bbox3f_init(bbox3f &box, const das::float3 &point) { v_bbox3_init(box, point); }
inline void bbox3f_init2(bbox3f &box, mat44f_cref m, bbox3f_cref b2) { v_bbox3_init(box, m, b2); }
inline void bbox3f_add_pt(bbox3f &box, const das::float3 &point) { v_bbox3_add_pt(box, point); }
inline void bbox3f_add_box(bbox3f &box, const bbox3f &add_box) { v_bbox3_add_box(box, add_box); }
inline int bbox3f_test_pt_inside(bbox3f_cref box, const das::float3 &point) { return v_bbox3_test_pt_inside(box, point); }

inline Quat quat_add(Quat a, Quat b) { return a + b; }
inline Quat quat_sub(Quat a, Quat b) { return a - b; }
inline Quat quat_mul(Quat a, Quat b) { return a * b; }
inline Quat quat_mul_scalar(Quat q, float s) { return q * s; }
inline Quat quat_mul_scalar_first(float s, Quat q) { return q * s; }
inline das::float3 quat_mul_vec(Quat q, const das::float3 &v)
{
  const Point3 qv = q * Point3(&v.x, Point3::CTOR_FROM_PTR);
  return das::float3(qv.x, qv.y, qv.z);
}
inline Quat quat_inverse(Quat q) { return inverse(q); }
inline Quat quat_normalize(Quat q) { return normalize(q); }
inline Quat quat_identity() { return Quat(0.f, 0.f, 0.f, 1.f); }
inline Point3 quat_get_forward(Quat q) { return q.getForward(); }
inline Point3 quat_get_up(Quat q) { return q.getUp(); }
inline Point3 quat_get_left(Quat q) { return q.getLeft(); }
inline void quat_make_tm(Quat q, das::float3x4 &tm3x4)
{
  TMatrix tm;
  tm.makeTM(q);
  ::memcpy(tm3x4.m, tm.array, sizeof(tm.array));
}
inline void quat_make_tm_pos(Quat q, das::float3 p, das::float3x4 &tm3x4)
{
  quat_make_tm(q, tm3x4);
  tm3x4.m[3] = p;
}
inline Quat quat_make(float x, float y, float z, float w) { return Quat(x, y, z, w); }
inline Quat quat_make_float4(das::float4 v) { return Quat(v.x, v.y, v.z, v.w); }
inline Quat quat_make_axis_ang(das::float3 axis, float ang) { return Quat(Point3::xyz(axis), ang); }
inline Quat quat_make_matrix(const das::float3x4 &tm) { return Quat(reinterpret_cast<const TMatrix &>(tm)); }
inline das::float4 quat_to_float4(Quat q) { return das::float4(q.x, q.y, q.z, q.w); }
inline Quat quat_slerp(Quat a, Quat b, float t) { return qinterp(a, b, t); }
inline void mat_rotxTM(float a, das::float3x4 &tm3x4)
{
  TMatrix tm = rotxTM(a);
  ::memcpy(tm3x4.m, tm.array, sizeof(tm.array));
}
inline void mat_rotyTM(float a, das::float3x4 &tm3x4)
{
  TMatrix tm = rotyTM(a);
  ::memcpy(tm3x4.m, tm.array, sizeof(tm.array));
}
inline void mat_rotzTM(float a, das::float3x4 &tm3x4)
{
  TMatrix tm = rotzTM(a);
  ::memcpy(tm3x4.m, tm.array, sizeof(tm.array));
}
inline void make_tm(const Point3 &axis, real ang, das::float3x4 &tm3x4)
{
  TMatrix tm;
  tm.makeTM(axis, ang);
  ::memcpy(tm3x4.m, tm.array, sizeof(tm.array));
}
inline void orthonormalize(TMatrix &tm) { tm.orthonormalize(); }
inline float TMatrix_det(const TMatrix &tm) { return tm.det(); }
inline DPoint3 dpoint3_minus(const DPoint3 &a) { return -a; }
inline DPoint3 dpoint3_add(const DPoint3 &a, const DPoint3 &b) { return a + b; }
inline DPoint3 dpoint3_sub(const DPoint3 &a, const DPoint3 &b) { return a - b; }
inline DPoint3 dpoint3_mul_scalar_float(const DPoint3 &p, float s) { return p * s; }
inline DPoint3 dpoint3_mul_scalar_first_float(float s, const DPoint3 &p) { return p * s; }
inline DPoint3 dpoint3_mul_scalar(const DPoint3 &p, double s) { return p * s; }
inline DPoint3 dpoint3_mul_scalar_first(double s, const DPoint3 &p) { return p * s; }
inline DPoint3 dpoint3_lerp(const DPoint3 &from, const DPoint3 &to, float k) { return lerp(from, to, k); }
inline DPoint3 dpoint3_approach_vel(const DPoint3 &from, const DPoint3 &to, float dt, float viscosity, DPoint3 &vel,
  float vel_viscosity, float vel_factor)
{
  return approach_vel<DPoint3>(from, to, dt, viscosity, vel, vel_viscosity, vel_factor);
}
inline void dpoint3_add_dpoint3(DPoint3 &a, const DPoint3 &b) { a += b; }

inline float interpolate_tab_float_interpolate(const InterpolateTabFloat &tab, float val) { return tab.interpolate(val); }

inline mat44f mat44f_make_matrix(TMatrix const &tm)
{
  mat44f m;
  v_mat44_make_from_43cu_unsafe(m, tm.m[0]);
  return m;
}

inline TMatrix mat44f_make_tm(mat44f const &m)
{
  TMatrix tm;
  v_mat_43ca_from_mat44(tm.m[0], m);
  return tm;
}

inline float v_distance_sq_to_bbox(das::float4 bmin, das::float4 bmax, das::float4 c)
{
  return ::v_extract_x(::v_distance_sq_to_bbox_x(bmin, bmax, c));
}
inline int v_test_segment_box_intersection(das::float3 start, das::float3 end, bbox3f_cref box)
{
  return ::v_test_segment_box_intersection(start, end, box);
}

inline void read_interpolate_tab_as_params(InterpolateTabFloat &tab, const DataBlock &blk,
  const das::TBlock<bool, DataBlock &, int, float, float> &block, das::Context *context, das::LineInfoArg *at)
{
  ::read_interpolate_tab_as_params(tab, &blk, [&](const DataBlock *, int i, float x, float y) -> bool {
    vec4f args[] = {das::cast<DataBlock &>::from(blk), das::cast<int>::from(i), das::cast<float>::from(x), das::cast<float>::from(y)};
    return das::cast<bool>::to(context->invoke(block, args, nullptr, at));
  });
}

inline void read_interpolate_tab_float_p2(InterpolateTabFloat &tab, const DataBlock &blk)
{
  ::read_interpolate_tab_as_params(tab, &blk, [](const DataBlock *blk, int i, float &x, float &y) -> bool {
    Point2 p = blk->getPoint2(i);
    x = p.x;
    y = p.y;
    return true;
  });
}

inline float noise2(das::float2 input) { return perlin_noise::noise2(Point2::xy(input)); }

} // namespace bind_dascript
