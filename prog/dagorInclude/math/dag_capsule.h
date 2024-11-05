//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <math/dag_TMatrix.h>
#include <math/dag_bounds3.h>
#include <math/dag_mathUtils.h>
#include <math/dag_bits.h>
#include <vecmath/dag_vecMath.h>
#include <util/dag_globDef.h>

struct Capsule
{
  // don't change a,r order
  Point3 a;
  float r;
  Point3 b;

  Capsule() = default;
  Capsule(const Point3 &v0, const Point3 &v1, float radius) { set(v0, v1, radius); }
  void set(const Point3 &v0, const Point3 &v1, float radius)
  {
    a = v0;
    b = v1;
    r = radius;
  }
  void set(vec3f v0, vec3f v1, float radius)
  {
    v_stu(&a.x, v_perm_xyzd(v0, v_splats(radius)));
    v_stu_p3(&b.x, v1);
  }
  void set(const BBox3 &box)
  {
    bbox3f bbox = v_ldu_bbox3(box);
    vec4f width = v_perm_xyzz(v_bbox3_size(bbox));
    vec4f maxWidth = v_bbox3_max_size(bbox);
    vec3f maxWidthMask = v_cmp_eqi(width, maxWidth);
    int signMask = v_signmask(maxWidthMask) & (1 | 2 | 4);
    vec3f singleMaxWidthMask = v_make_vec4f_mask((1 << __bsf_unsafe(signMask)));
    vec3f midWidth = v_andnot(maxWidthMask, width);
    midWidth = v_max(midWidth, v_rot_1(midWidth));
    midWidth = v_max(midWidth, v_rot_2(midWidth));
    vec3f rad = v_mul(midWidth, V_C_HALF);
    vec3f halfDl = v_and(v_sub(v_mul(width, V_C_HALF), rad), singleMaxWidthMask);
    vec3f c = v_bbox3_center(bbox);
    set(v_sub(c, halfDl), v_add(c, halfDl), v_extract_x(rad));
  }
  void transform(mat44f_cref tm)
  {
    vec4f v0 = v_ldu(&a.x);
    vec3f v1 = v_ldu(&b.x);
    vec4f radW = v_mul(v0, v_length3(tm.col0));
    v0 = v_mat44_mul_vec3p(tm, v0);
    v1 = v_mat44_mul_vec3p(tm, v1);
    v_stu(&a.x, v_perm_xyzd(v0, radW));
    v_stu_p3(&b.x, v1);
  }
  void transform(const TMatrix &tm)
  {
    mat44f vTm;
    v_mat44_make_from_43cu_unsafe(vTm, tm.array);
    transform(vTm);
  }
  float distToAxisSq(vec3f p) const
  {
    vec4f v0 = v_ldu(&a.x);
    vec3f v1 = v_ldu(&b.x);
    vec3f cp = closest_point_on_segment(p, v0, v1);
    cp = v_sel(cp, v0, v_cmp_eq(v0, v1));
    return v_extract_x(v_length3_sq_x(v_sub(p, cp)));
  }
  vec3f getCenter() const
  {
    vec4f v0 = v_ldu(&a.x);
    vec3f v1 = v_ldu(&b.x);
    return v_mul(v_add(v0, v1), V_C_HALF);
  }
  Point3 getCenterScalar() const
  {
    Point3_vec4 c;
    v_st(&c.x, getCenter());
    return c;
  }
  bbox3f getBoundingBox() const
  {
    vec4f v0 = v_ldu(&a.x);
    vec3f v1 = v_ldu(&b.x);
    vec4f rad = v_splat_w(v0);
    bbox3f bbox;
    v_bbox3_init(bbox, v0);
    v_bbox3_add_pt(bbox, v1);
    bbox.bmin = v_sub(bbox.bmin, rad);
    bbox.bmax = v_add(bbox.bmax, rad);
    return bbox;
  }
  BBox3 getBoundingBoxScalar() const
  {
    BBox3 bbox;
    v_stu_bbox3(bbox, getBoundingBox());
    return bbox;
  }
  vec4f getBoundingSphere() const // xyz|r
  {
    vec4f v0 = v_ldu(&a.x);
    vec3f v1 = v_ldu(&b.x);
    vec4f len = v_length3(v_sub(v0, v1));
    vec3f c = v_mul(v_add(v0, v1), V_C_HALF);
    vec4f rad = v_add(v0, v_mul(len, V_C_HALF)); // in .w
    return v_perm_xyzd(c, rad);
  }
  BSphere3 getBoundingSphereScalar() const
  {
    BSphere3 bsph;
    v_stu(&bsph.c.x, getBoundingSphere());
    bsph.r2 = sqr(bsph.r);
    return bsph;
  }
  bool isInside(vec3f p) const { return distToAxisSq(p) < sqr(v_extract_w(v_ldu(&a.x))); }
  bool isInside(const Point3 &p) const { return isInside(v_ldu(&p.x)); }
  bool cliptest(const Capsule &c) const
  {
    if (!v_bbox3_test_box_intersect(getBoundingBox(), c.getBoundingBox()))
      return false;
    vec4f v0 = v_ldu(&c.a.x);
    vec3f v1 = v_ldu(&c.b.x);
    float rad = v_extract_w(v0);
    vec4f vDir = v_sub(v1, v0);
    vec4f vLen = v_length3(vDir);
    float len = v_extract_x(vLen);
    vDir = v_safediv(vDir, vLen);
    return capsuleHit(v0, vDir, len, rad);
  }
  bool cliptest(bbox3f bbox) const // too bad implementation
  {
    vec4f v0 = v_ldu(&a.x);
    vec3f v1 = v_ldu(&b.x);
    vec4f rad = v_splat_w(v0);
    vec4f r2 = v_mul_x(rad, rad);

    if (v_bbox3_test_sph_intersect(bbox, v0, r2))
      return true;
    if (v_bbox3_test_sph_intersect(bbox, v1, r2))
      return true;

    vec3f p = closest_point_on_segment(v_bbox3_center(bbox), v0, v1);
    if (v_bbox3_test_sph_intersect(bbox, p, r2))
      return true;

    for (int i = 0; i < 8; i++)
    {
      vec3f v = v_bbox3_point(bbox, i);
      vec3f vp = closest_point_on_segment(v, v0, v1);
      if (v_bbox3_test_sph_intersect(bbox, vp, r2))
        return true;
    }
    return false;
  }
  bool cliptest(const BBox3 &bbox) const { return cliptest(v_ldu_bbox3(bbox)); }
  bool cliptest(const BSphere3 &sph) const
  {
    vec4f s = v_ldu(&sph.c.x);
    return distToAxisSq(s) < sqr(v_extract_w(v_add(s, v_ldu(&a.x))));
  }
  bool rayHit(vec3f from, vec3f dir, real len) const { return capsuleHit(from, dir, len, 0.f /*rad*/); }
  bool rayHit(const Point3 &from, const Point3 &dir, float len) const { return rayHit(v_ldu(&from.x), v_ldu(&dir.x), len); }
  /// returned maxt is approximate
  bool traceRay(vec3f from, vec3f dir, float &t, vec3f &norm) const
  {
    vec4f vPos;
    return traceCapsule(from, dir, t, 0, norm, vPos);
  }
  bool traceRay(const Point3 &from, const Point3 &dir, float &t, Point3 &norm) const
  {
    vec4f vNorm;
    bool hit = traceRay(v_ldu(&from.x), v_ldu(&dir.x), t, vNorm);
    v_stu_p3(&norm.x, vNorm);
    return hit;
  }
  bool capsuleHit(vec3f from, vec3f dir, float len, float rad) const
  {
    vec4f vNorm, vPos;
    return traceCapsule(from, dir, len, rad, vNorm, vPos);
  }
  bool traceCapsule(vec3f from, vec3f dir, float &len, float rad, vec3f &norm, vec3f &pos) const
  {
    vec4f v0 = v_ldu(&a.x);
    vec3f v1 = v_ldu(&b.x);
    vec4f vRad = v_splat_w(v0);
    vec3f vDir = v_sub(v1, v0);
    vec4f vLen = v_length3(vDir);
    vDir = v_safediv(vDir, vLen);

    vec4f k = v_dot3_x(vDir, dir);
    vec3f dp = v_sub(from, v0);
    vec3f h = v_nmsub(dir, v_splat_x(k), vDir);
    vec4f num = v_dot3_x(dp, h);
    vec4f denum = v_sub_x(v_set_x(1.f), v_mul_x(k, k));
    vec4f t = v_sel(v_div_x(num, denum), v_set_x(0.5f), v_cmp_eq(denum, v_zero()));
    t = v_clamp(v_splat_x(t), v_zero(), vLen);
    vec4f tb = v_dot3(v_sub(v_madd(vDir, t, v0), from), dir);
    tb = v_clamp(tb, v_zero(), v_splats(len));
    vec4f ta = v_dot3(v_sub(v_madd(dir, tb, from), v0), vDir);
    ta = v_clamp(ta, v_zero(), vLen);
    vec3f pa = v_madd(vDir, ta, v0);
    vec3f pb = v_madd(dir, tb, from);
    vec3f nDir = v_sub(pb, pa);
    vec4f distSq = v_length3_sq(nDir);
    vec4f radSum = v_add(vRad, v_splats(rad));
    vec4f hit = v_cmp_lt(distSq, v_mul(radSum, radSum));
    len = v_extract_x(v_sel(v_set_x(len), tb, hit));
    if (v_extract_xi(v_cast_vec4i(hit)) != 0)
    {
      norm = v_safediv(nDir, v_sqrt(distSq));
      pos = v_madd(norm, vRad, pa);
      return true;
    }
    return false;
  }
};
