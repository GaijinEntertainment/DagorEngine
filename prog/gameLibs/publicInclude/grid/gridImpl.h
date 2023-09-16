//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
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
    __forceinline bool checkObjectBounding(const Object &object, bbox3f query_bbox) const
    {
      return EASTL_UNLIKELY(v_bbox3_test_pt_inside(query_bbox, object.getWBSph()));
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
    __forceinline bool checkObjectBounding(const Object &object, bbox3f query_bbox) const
    {
      vec4f wbsph = object.getWBSph();
      vec4f objRad = v_splat_w(wbsph);
      return EASTL_UNLIKELY(v_bbox3_test_sph_intersect(query_bbox, wbsph, v_mul_x(objRad, objRad)));
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
    __forceinline bool checkObjectBounding(const Object &object, bbox3f) const
    {
      vec4f distSq = v_length3_sq_x(v_sub(object.getWBSph(), sphPos));
      return EASTL_UNLIKELY(v_test_vec_x_le(distSq, v_set_x(sphRadSq)));
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
    __forceinline bool isCapsule() const { return true; }
    __forceinline bool checkObjectBounding(const Object &object, bbox3f) const
    {
      vec4f wbsph = object.getWBSph();
      vec4f objRad = v_splat_w(wbsph);
      vec4f distSq = v_length3_sq_x(v_sub(wbsph, sphPos));
      vec4f maxDist = v_add_x(v_set_x(sphRad), objRad);
      return EASTL_UNLIKELY(v_test_vec_x_le(distSq, v_mul_x(maxDist, maxDist)));
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
__forceinline auto grid_find_in_capsule_by_pos_impl(const Holder &grid_holder, vec3f from, vec3f dir, vec4f len, vec4f radius,
  const Predicate &pred)
{
  struct ObjectsIterator
  {
    __forceinline bool isCapsule() const { return true; }
    __forceinline bool checkObjectBounding(const Object &object, vec3f from, vec3f dir, vec4f len, vec4f radius) const
    {
      vec3f pa = v_sub(object.getWBSph(), from);
      vec4f t = v_dot3(pa, dir); // t param along line
      vec4f segT = v_clamp(t, v_zero(), len);
      vec4f distSq = v_length3_sq_x(v_sub(pa, v_mul(dir, segT)));
      return EASTL_UNLIKELY(v_test_vec_x_le(distSq, v_sqr(radius)));
    }
    const Predicate &predFunc;
  };

  return grid_holder.getRayIterator(from, dir, len, radius, extend_by_bounding::NO).foreach(ObjectsIterator{pred});
}

template <typename Object, typename Holder, typename Predicate>
__forceinline auto grid_find_in_capsule_by_bounding_impl(const Holder &grid_holder, vec3f from, vec3f dir, vec4f len, vec4f radius,
  const Predicate &pred)
{
  struct ObjectsIterator
  {
    __forceinline bool checkObjectBounding(const Object &object, vec3f from, vec3f dir, vec4f len, vec4f radius) const
    {
      vec4f wbsph = object.getWBSph();
      vec3f pa = v_sub(wbsph, from);
      vec4f t = v_dot3(pa, dir); // t param along line
      vec4f segT = v_clamp(t, v_zero(), len);
      vec4f distSq = v_length3_sq_x(v_sub(pa, v_mul(dir, segT)));
      vec4f objRad = v_splat_w(wbsph);
      vec4f maxDist = v_add_x(radius, objRad);
      return EASTL_UNLIKELY(v_test_vec_x_le(distSq, v_sqr_x(maxDist)));
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
    __forceinline bool checkObjectBounding(const Object &object, bbox3f query_bbox) const
    {
      vec4f wbsph = object.getWBSph();
      if (EASTL_UNLIKELY(v_bbox3_test_pt_inside(query_bbox, wbsph)))
      {
        if (EASTL_UNLIKELY(!itm))
        {
          itm = true;
          v_mat44_inverse43(mat44, mat44);
        }
        vec3f lpos = v_mat44_mul_vec3p(mat44, wbsph);
        return EASTL_UNLIKELY(v_bbox3_test_pt_inside(lbbox, lpos));
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
    __forceinline bool checkObjectBounding(const Object &object, bbox3f query_bbox) const
    {
      vec4f wbsph = object.getWBSph();
      if (EASTL_UNLIKELY(v_bbox3_test_pt_inside(query_bbox, wbsph)))
      {
        if (EASTL_UNLIKELY(!itm))
        {
          itm = true;
          v_mat44_inverse43(mat44, mat44);
        }
        vec3f lpos = v_mat44_mul_vec3p(mat44, wbsph);
        vec4f objRad = v_splat_w(wbsph);
        vec4f distSq = v_length3_sq_x(v_add(v_max(v_sub(lbbox.bmin, lpos), v_zero()), v_max(v_sub(lpos, lbbox.bmax), v_zero())));
        return EASTL_UNLIKELY(v_test_vec_x_le(distSq, v_mul_x(objRad, objRad)));
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
    __forceinline bool checkObjectBounding(const Object &object, vec3f from, vec3f dir, vec4f len, vec4f) const
    {
      vec4f wbsph = object.getWBSph();
      vec3f pa = v_sub(wbsph, from);
      vec4f t = v_dot3(pa, dir); // t param along line
      vec4f segT = v_clamp(t, v_zero(), len);
      vec4f distSq = v_length3_sq_x(v_sub(pa, v_mul(dir, segT)));
      vec4f objRad = v_splat_w(wbsph);
      return EASTL_UNLIKELY(v_test_vec_x_le(distSq, v_sqr_x(objRad)));
    }
    const Predicate &predFunc;
  };

  return grid_holder.getRayIterator(from, dir, len, v_zero(), extend_by_bounding::YES).foreach(ObjectsIterator{pred});
}
