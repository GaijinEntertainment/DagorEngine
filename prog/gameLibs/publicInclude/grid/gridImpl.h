//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <vecmath/dag_vecMath.h>
#include <math/dag_bounds3.h>
#include <math/dag_TMatrix.h>
#include <math/dag_mathUtils.h>
#include <math/dag_math3d.h>

enum extend_by_bounding : bool
{
  YES = true,
  NO = false
};

template <typename Object, typename Holder, typename Predicate>
__forceinline auto grid_find_in_box_by_pos_impl(const Holder &grid_holder, bbox3f bbox, const Predicate &pred)
{
  struct ObjectsIterator
  {
    __forceinline bool checkBoxBoundingInside(bbox3f bbox, bbox3f query_box) const { return v_bbox3_test_box_inside(query_box, bbox); }
    __forceinline bool checkBoxBounding(bbox3f bbox, bbox3f query_box) const { return v_bbox3_test_box_intersect(bbox, query_box); }
    __forceinline bool checkObjectBounding(vec4f wbsph, bbox3f query_bbox) const
    {
      return DAGOR_UNLIKELY(v_bbox3_test_pt_inside(query_bbox, wbsph));
    }
    const Predicate &predFunc;
  };

  return grid_holder.getBoxIterator(bbox, extend_by_bounding::NO).foreach(ObjectsIterator{pred});
}

template <typename Object, typename Holder, typename Predicate>
__forceinline auto grid_find_in_box_by_bounding_impl(const Holder &grid_holder, bbox3f bbox, const Predicate &pred)
{
  struct ObjectsIterator
  {
    __forceinline bool checkBoxBoundingInside(bbox3f bbox, bbox3f query_box) const { return v_bbox3_test_box_inside(query_box, bbox); }
    __forceinline bool checkBoxBounding(bbox3f bbox, bbox3f query_box) const { return v_bbox3_test_box_intersect(bbox, query_box); }
    __forceinline bool checkObjectBounding(vec4f wbsph, bbox3f query_bbox) const
    {
      vec4f objRad = v_splat_w(wbsph);
      return DAGOR_UNLIKELY(v_bbox3_test_sph_intersect(query_bbox, wbsph, v_mul_x(objRad, objRad)));
    }
    const Predicate &predFunc;
  };

  return grid_holder.getBoxIterator(bbox, extend_by_bounding::YES).foreach(ObjectsIterator{pred});
}

template <typename Object, typename Holder, typename Predicate>
__forceinline auto grid_find_in_sphere_by_pos_impl(const Holder &grid_holder, const Point3 &center, float radius,
  const Predicate &pred)
{
  struct ObjectsIterator
  {
    __forceinline bool checkBoxBoundingInside(bbox3f bbox, bbox3f) const
    {
      return v_bsph_test_box_inside(sphPos, v_set_x(sphRadSq), bbox);
    }
    __forceinline bool checkBoxBounding(bbox3f bbox, bbox3f) const
    {
      return v_bbox3_test_sph_intersect(bbox, sphPos, v_set_x(sphRadSq));
    }
    __forceinline bool checkObjectBounding(vec4f wbsph, bbox3f) const
    {
      vec4f distSq = v_length3_sq_x(v_sub(wbsph, sphPos));
      return DAGOR_UNLIKELY(v_test_vec_x_le(distSq, v_set_x(sphRadSq)));
    }
    const Predicate &predFunc;
    vec3f sphPos;
    float sphRadSq;
  };

  bbox3f bbox;
  vec3f sphPos = v_ldu(&center.x);
  vec4f sphRad = v_splats(radius);
  v_bbox3_init_by_bsph(bbox, sphPos, sphRad);
  return grid_holder.getBoxIterator(bbox, extend_by_bounding::NO).foreach(ObjectsIterator{pred, sphPos, sqr(radius)});
}

template <typename Object, typename Holder, typename Predicate>
__forceinline auto grid_find_in_sphere_by_bounding_impl(const Holder &grid_holder, const Point3 &center, float radius,
  const Predicate &pred)
{
  struct ObjectsIterator
  {
    __forceinline bool checkBoxBoundingInside(bbox3f bbox, bbox3f) const
    {
      return v_bsph_test_box_inside(sphPos, v_set_x(sqr(sphRad)), bbox);
    }
    __forceinline bool checkBoxBounding(bbox3f bbox, bbox3f) const
    {
      return v_bbox3_test_sph_intersect(bbox, sphPos, v_set_x(sqr(sphRad)));
    }
    __forceinline bool checkObjectBounding(vec4f wbsph, bbox3f) const
    {
      vec4f objRad = v_splat_w(wbsph);
      vec4f distSq = v_length3_sq_x(v_sub(wbsph, sphPos));
      vec4f maxDist = v_add_x(v_set_x(sphRad), objRad);
      return DAGOR_UNLIKELY(v_test_vec_x_le(distSq, v_mul_x(maxDist, maxDist)));
    }
    const Predicate &predFunc;
    vec3f sphPos;
    float sphRad;
  };

  bbox3f bbox;
  vec3f sphPos = v_ldu(&center.x);
  vec4f sphRad = v_splats(radius);
  v_bbox3_init_by_bsph(bbox, sphPos, sphRad);
  return grid_holder.getBoxIterator(bbox, extend_by_bounding::YES).foreach(ObjectsIterator{pred, sphPos, radius});
}

template <typename Object, typename Holder, typename Predicate>
__forceinline auto grid_find_in_capsule_by_pos_impl(const Holder &grid_holder, vec3f from, vec3f dir, const vec4f &len,
  const vec4f &radius, const Predicate &pred)
{
  struct ObjectsIterator
  {
    __forceinline bool isCapsule() const { return true; }
    __forceinline bool checkBoxBounding(bbox3f bbox, bool is_safe, vec3f from, vec3f dir, const vec4f &len, const vec4f &radius) const
    {
      v_bbox3_extend(bbox, radius);
      if (is_safe)
        return v_test_ray_box_intersection_unsafe(from, dir, len, bbox);
      else
        return v_test_ray_box_intersection(from, dir, len, bbox);
    }
    __forceinline bool checkObjectBounding(vec4f wbsph, vec3f from, vec3f dir, const vec4f &len, const vec4f &radius) const
    {
      vec3f pa = v_sub(wbsph, from);
      vec4f t = v_dot3(pa, dir); // t param along line
      vec4f segT = v_clamp(t, v_zero(), len);
      vec4f distSq = v_length3_sq_x(v_sub(pa, v_mul(dir, segT)));
      return DAGOR_UNLIKELY(v_test_vec_x_le(distSq, v_sqr(radius)));
    }
    __forceinline int checkFourObjectsBounding(const Object objects[4], vec3f from, vec3f dir, const vec4f &len,
      const vec4f &radius) const
    {
      mat44f objMat = {objects[0].getWBSph(), objects[1].getWBSph(), objects[2].getWBSph(), objects[3].getWBSph()};
      v_mat44_transpose(objMat, objMat);
      mat44f fromMat = {v_splat_x(from), v_splat_y(from), v_splat_z(from)};
      mat44f pa;
      v_mat44_sub(pa, objMat, fromMat);
      mat44f mul = {v_mul(pa.col0, v_splat_x(dir)), v_mul(pa.col1, v_splat_y(dir)), v_mul(pa.col2, v_splat_z(dir))};
      vec4f t = v_add(v_add(mul.col0, mul.col1), mul.col2);
      vec4f segT = v_clamp(t, v_zero(), len);
      mat44f projMat = {
        v_mul(dir, v_splat_x(segT)), v_mul(dir, v_splat_y(segT)), v_mul(dir, v_splat_z(segT)), v_mul(dir, v_splat_w(segT))};
      v_mat44_transpose(projMat, projMat);
      mat44f d;
      v_mat44_sub(d, pa, projMat);
      vec4f distSq = v_add(v_add(v_sqr(d.col0), v_sqr(d.col1)), v_sqr(d.col2));
      return v_signmask(v_cmp_le(distSq, v_sqr(radius)));
    }
    const Predicate &predFunc;
  };

  return grid_holder.getRayIterator(from, dir, len, radius, extend_by_bounding::NO).foreach(ObjectsIterator{pred});
}

template <typename Object, typename Holder, typename Predicate>
__forceinline auto grid_find_in_capsule_by_bounding_impl(const Holder &grid_holder, vec3f from, vec3f dir, const vec4f &len,
  const vec4f &radius, const Predicate &pred)
{
  struct ObjectsIterator
  {
    __forceinline bool isCapsule() const { return true; }
    __forceinline bool checkBoxBounding(bbox3f bbox, bool is_safe, vec3f from, vec3f dir, const vec4f &len, const vec4f &radius) const
    {
      v_bbox3_extend(bbox, radius);
      if (is_safe)
        return v_test_ray_box_intersection_unsafe(from, dir, len, bbox);
      else
        return v_test_ray_box_intersection(from, dir, len, bbox);
    }
    __forceinline bool checkObjectBounding(vec4f wbsph, vec3f from, vec3f dir, const vec4f &len, const vec4f &radius) const
    {
      vec3f pa = v_sub(wbsph, from);
      vec4f t = v_dot3(pa, dir); // t param along line
      vec4f segT = v_clamp(t, v_zero(), len);
      vec4f distSq = v_length3_sq_x(v_sub(pa, v_mul(dir, segT)));
      vec4f objRad = v_splat_w(wbsph);
      vec4f maxDist = v_add_x(radius, objRad);
      return DAGOR_UNLIKELY(v_test_vec_x_le(distSq, v_sqr_x(maxDist)));
    }
    __forceinline int checkFourObjectsBounding(const Object objects[4], vec3f from, vec3f dir, const vec4f &len,
      const vec4f &radius) const
    {
      mat44f objMat = {objects[0].getWBSph(), objects[1].getWBSph(), objects[2].getWBSph(), objects[3].getWBSph()};
      v_mat44_transpose(objMat, objMat);
      mat44f fromMat = {v_splat_x(from), v_splat_y(from), v_splat_z(from)};
      mat44f pa;
      v_mat44_sub(pa, objMat, fromMat);
      mat44f mul = {v_mul(pa.col0, v_splat_x(dir)), v_mul(pa.col1, v_splat_y(dir)), v_mul(pa.col2, v_splat_z(dir))};
      vec4f t = v_add(v_add(mul.col0, mul.col1), mul.col2);
      vec4f segT = v_clamp(t, v_zero(), len);
      mat44f projMat = {
        v_mul(dir, v_splat_x(segT)), v_mul(dir, v_splat_y(segT)), v_mul(dir, v_splat_z(segT)), v_mul(dir, v_splat_w(segT))};
      v_mat44_transpose(projMat, projMat);
      mat44f d;
      v_mat44_sub(d, pa, projMat);
      vec4f distSq = v_add(v_add(v_sqr(d.col0), v_sqr(d.col1)), v_sqr(d.col2));
      return v_signmask(v_cmp_le(distSq, v_sqr(v_add(objMat.col3, radius))));
    }
    const Predicate &predFunc;
  };

  return grid_holder.getRayIterator(from, dir, len, radius, extend_by_bounding::YES).foreach(ObjectsIterator{pred});
}

template <typename Object, typename Holder, typename Predicate>
__forceinline auto grid_find_in_transformed_box_by_pos_impl(const Holder &grid_holder, const TMatrix &tm, const BBox3 &bbox,
  const Predicate &pred)
{
  struct ObjectsIterator
  {
    __forceinline bool checkBoxBoundingInside(bbox3f, bbox3f) const
    {
      return false; // disable that optimization
    }
    __forceinline bool checkBoxBounding(bbox3f bbox, bbox3f query_box) const { return v_bbox3_test_box_intersect(bbox, query_box); }
    __forceinline bool checkObjectBounding(vec4f wbsph, bbox3f query_bbox) const
    {
      if (DAGOR_UNLIKELY(v_bbox3_test_pt_inside(query_bbox, wbsph)))
      {
        if (DAGOR_UNLIKELY(!itm))
        {
          itm = true;
          v_mat44_inverse43(mat44, mat44);
        }
        vec3f lpos = v_mat44_mul_vec3p(mat44, wbsph);
        return DAGOR_UNLIKELY(v_bbox3_test_pt_inside(lbbox, lpos));
      }
      return false;
    }
    const Predicate &predFunc;
    bbox3f lbbox;
    mutable mat44f mat44;
    mutable bool itm = false;
    ObjectsIterator(const ObjectsIterator &) = delete; // we have changeable flag
  };

  mat44f mat44;
  bbox3f wsbbox, lbbox = v_ldu_bbox3(bbox);
  v_mat44_make_from_43cu_unsafe(mat44, tm.array);
  v_bbox3_init(wsbbox, mat44, lbbox);
  return grid_holder.getBoxIterator(wsbbox, extend_by_bounding::NO).foreach(ObjectsIterator{pred, lbbox, mat44});
}

template <typename Object, typename Holder, typename Predicate>
__forceinline auto grid_find_in_transformed_box_by_bounding_impl(const Holder &grid_holder, const TMatrix &tm, const BBox3 &bbox,
  const Predicate &pred)
{
  struct ObjectsIterator
  {
    __forceinline bool checkBoxBoundingInside(bbox3f, bbox3f) const
    {
      return false; // disable that optimization
    }
    __forceinline bool checkBoxBounding(bbox3f bbox, bbox3f query_box) const { return v_bbox3_test_box_intersect(bbox, query_box); }
    __forceinline bool checkObjectBounding(vec4f wbsph, bbox3f query_bbox) const
    {
      if (DAGOR_UNLIKELY(v_bbox3_test_pt_inside(query_bbox, wbsph)))
      {
        if (DAGOR_UNLIKELY(!itm))
        {
          itm = true;
          v_mat44_inverse43(mat44, mat44);
        }
        vec3f lpos = v_mat44_mul_vec3p(mat44, wbsph);
        vec4f objRad = v_splat_w(wbsph);
        vec4f distSq = v_length3_sq_x(v_add(v_max(v_sub(lbbox.bmin, lpos), v_zero()), v_max(v_sub(lpos, lbbox.bmax), v_zero())));
        return DAGOR_UNLIKELY(v_test_vec_x_le(distSq, v_mul_x(objRad, objRad)));
      }
      return false;
    }
    const Predicate &predFunc;
    bbox3f lbbox;
    mutable mat44f mat44;
    mutable bool itm = false;
    ObjectsIterator(const ObjectsIterator &) = delete; // we have changeable flag
  };

  mat44f mat44;
  bbox3f wsbbox, lbbox = v_ldu_bbox3(bbox);
  v_mat44_make_from_43cu_unsafe(mat44, tm.array);
  v_bbox3_init(wsbbox, mat44, lbbox);
  return grid_holder.getBoxIterator(wsbbox, extend_by_bounding::YES).foreach(ObjectsIterator{pred, lbbox, mat44});
}

template <typename Object, typename Holder, typename Predicate>
__forceinline auto grid_find_ray_intersections_impl(const Holder &grid_holder, vec3f from, vec3f dir, vec4f len, const Predicate &pred)
{
  struct ObjectsIterator
  {
    __forceinline bool isCapsule() const { return false; }
    __forceinline bool checkBoxBounding(bbox3f bbox, bool is_safe, vec3f from, vec3f dir, const vec4f &len, const vec4f &) const
    {
      if (is_safe)
        return v_test_ray_box_intersection_unsafe(from, dir, len, bbox);
      else
        return v_test_ray_box_intersection(from, dir, len, bbox);
    }
    __forceinline bool checkObjectBounding(vec4f wbsph, vec3f from, vec3f dir, const vec4f &len, const vec4f &) const
    {
      vec3f pa = v_sub(wbsph, from);
      vec4f t = v_dot3(pa, dir); // t param along line
      vec4f segT = v_clamp(t, v_zero(), len);
      vec4f distSq = v_length3_sq_x(v_sub(pa, v_mul(dir, segT)));
      vec4f objRad = v_splat_w(wbsph);
      return DAGOR_UNLIKELY(v_test_vec_x_le(distSq, v_sqr_x(objRad)));
    }
    __forceinline int checkFourObjectsBounding(const Object objects[4], vec3f from, vec3f dir, const vec4f &len, const vec4f &) const
    {
      mat44f objMat = {objects[0].getWBSph(), objects[1].getWBSph(), objects[2].getWBSph(), objects[3].getWBSph()};
      v_mat44_transpose(objMat, objMat);
      mat44f fromMat = {v_splat_x(from), v_splat_y(from), v_splat_z(from)};
      mat44f pa;
      v_mat44_sub(pa, objMat, fromMat);
      mat44f mul = {v_mul(pa.col0, v_splat_x(dir)), v_mul(pa.col1, v_splat_y(dir)), v_mul(pa.col2, v_splat_z(dir))};
      vec4f t = v_add(v_add(mul.col0, mul.col1), mul.col2);
      vec4f segT = v_clamp(t, v_zero(), len);
      mat44f projMat = {
        v_mul(dir, v_splat_x(segT)), v_mul(dir, v_splat_y(segT)), v_mul(dir, v_splat_z(segT)), v_mul(dir, v_splat_w(segT))};
      v_mat44_transpose(projMat, projMat);
      mat44f d;
      v_mat44_sub(d, pa, projMat);
      vec4f distSq = v_add(v_add(v_sqr(d.col0), v_sqr(d.col1)), v_sqr(d.col2));
      return v_signmask(v_cmp_le(distSq, v_sqr(objMat.col3)));
    }
    const Predicate &predFunc;
  };

  return grid_holder.getRayIterator(from, dir, len, v_zero(), extend_by_bounding::YES).foreach(ObjectsIterator{pred});
}
