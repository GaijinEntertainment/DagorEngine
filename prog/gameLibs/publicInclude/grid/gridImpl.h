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
#include <generic/dag_span.h>

enum extend_by_bounding : bool
{
  YES = true,
  NO = false
};

// A 6-plane convex volume in SoA form, for the per-object test of the convex queries below. Six is
// what both gamemath builders emit (construct_convex_from_frustum, construct_convex_from_box) and
// all the callers use; other counts do not reach here, the entry points route them elsewhere.
// Outward plane convention, the same one CollisionResource::testInclusion uses: a point is inside
// when dot(n, p) + d < eps for every plane, and a sphere when dot(n, c) + d - r < eps.
struct GridConvexPlanesSoA
{
  static constexpr int PLANE_COUNT = 6;

  vec4f nx[2], ny[2], nz[2], pd[2];

  // Transposed in local space and transformed as SoA, once per query. Doing it the other way round -
  // v_transform_plane per plane, then transpose - pays 7 splats and a horizontal v_dot3 six times
  // over for work the SoA layout does with plain madd. Precondition: planes.size() == PLANE_COUNT.
  void build(dag::ConstSpan<plane3f> planes, mat44f_cref tm)
  {
    setQuad(0, planes[0], planes[1], planes[2], planes[3]);
    setPair(1, planes[4], planes[5]);
    transformToWorld(tm);
  }

  // The normal is rotated as a vector, as v_transform_plane does, and then renormalized, which
  // v_transform_plane does not: testSphere weighs a distance against a radius, so a plane equation
  // still carrying a uniform scale s would measure every distance s times too large and reject
  // spheres that do intersect. Non-uniform scale is unsupported here as it is there.
  __forceinline void transformToWorld(mat44f_cref tm)
  {
    // mXY = column X, component Y; all 12 are loop invariant, so they are hoisted out of the batches
    vec4f m00 = v_splat_x(tm.col0), m01 = v_splat_y(tm.col0), m02 = v_splat_z(tm.col0);
    vec4f m10 = v_splat_x(tm.col1), m11 = v_splat_y(tm.col1), m12 = v_splat_z(tm.col1);
    vec4f m20 = v_splat_x(tm.col2), m21 = v_splat_y(tm.col2), m22 = v_splat_z(tm.col2);
    vec4f tx = v_splat_x(tm.col3), ty = v_splat_y(tm.col3), tz = v_splat_z(tm.col3);
    for (int i = 0; i < 2; ++i)
    {
      vec4f wx = v_madd(m20, nz[i], v_madd(m10, ny[i], v_mul(m00, nx[i])));
      vec4f wy = v_madd(m21, nz[i], v_madd(m11, ny[i], v_mul(m01, nx[i])));
      vec4f wz = v_madd(m22, nz[i], v_madd(m12, ny[i], v_mul(m02, nx[i])));
      // clamped so a degenerate zero-length normal yields a bounded invLen instead of a NaN
      vec4f lenSq = v_max(v_madd(wz, wz, v_madd(wy, wy, v_mul(wx, wx))), V_C_EPS_VAL);
      vec4f invLen = v_rsqrt(lenSq);
      wx = v_mul(wx, invLen);
      wy = v_mul(wy, invLen);
      wz = v_mul(wz, invLen);
      // d scales with the normal length, then shifts by the translation along the unit normal
      pd[i] = v_sub(v_mul(pd[i], v_mul(lenSq, invLen)), v_madd(wz, tz, v_madd(wy, ty, v_mul(wx, tx))));
      nx[i] = wx;
      ny[i] = wy;
      nz[i] = wz;
    }
  }

  __forceinline void setQuad(int batch, vec4f p0, vec4f p1, vec4f p2, vec4f p3)
  {
    v_mat44_transpose(p0, p1, p2, p3);
    nx[batch] = p0;
    ny[batch] = p1;
    nz[batch] = p2;
    pd[batch] = p3;
  }

  // The trailing 2 planes are replicated into the spare lanes rather than padded with a sentinel, so
  // every lane holds a real plane and none needs masking. 6 shuffles, where transposing a padded
  // quad through v_mat44_transpose would cost 8.
  __forceinline void setPair(int batch, vec4f p0, vec4f p1)
  {
    vec4f xy = v_merge_hw(p0, p1); // x0 x1 y0 y1
    vec4f zd = v_merge_lw(p0, p1); // z0 z1 d0 d1
    nx[batch] = v_perm_xyxy(xy);
    ny[batch] = v_perm_zwzw(xy);
    nz[batch] = v_perm_xyxy(zd);
    pd[batch] = v_perm_zwzw(zd);
  }

  // limit - dist is negative exactly for the planes the object is outside of, and v_or keeps that
  // sign bit, so one sign test answers for all 6 planes with no compare per batch. The two batches
  // are independent madd chains on purpose: they pipeline, where accumulating into one register
  // would serialize them.
  // The eps is absolute, matching the exact test this narrows for rather than the local coordinate
  // magnitude, so far from the origin it is below the rounding of dist itself. A broadphase does not
  // need a meaningful boundary, only one no stricter than the exact test's.
  __forceinline bool testInside(vec4f wbsph, vec4f limit) const
  {
    vec4f cx = v_splat_x(wbsph), cy = v_splat_y(wbsph), cz = v_splat_z(wbsph);
    vec4f dist0 = v_madd(cz, nz[0], v_madd(cy, ny[0], v_madd(cx, nx[0], pd[0])));
    vec4f dist1 = v_madd(cz, nz[1], v_madd(cy, ny[1], v_madd(cx, nx[1], pd[1])));
    return !v_is_any_neg_b(v_or(v_sub(limit, dist0), v_sub(limit, dist1)));
  }
  __forceinline bool testPos(vec4f wbsph) const { return testInside(wbsph, V_C_EPS_VAL); }
  __forceinline bool testSphere(vec4f wbsph) const { return testInside(wbsph, v_add(v_splat_w(wbsph), V_C_EPS_VAL)); }
};

template <typename Object>
static inline auto grid_default_filter = [](Object, vec4f /*wbsph*/) { return true; };

template <typename Object, typename Holder, typename Predicate, typename Filter = decltype(grid_default_filter<Object>)>
__forceinline auto grid_find_in_box_by_pos_impl(const Holder &grid_holder, bbox3f bbox, const Predicate &pred,
  const Filter &filter = grid_default_filter<Object>)
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
    const Filter &filterFunc;
  };

  return grid_holder.getBoxIterator(bbox, extend_by_bounding::NO).foreach(ObjectsIterator{pred, filter});
}

template <typename Object, typename Holder, typename Predicate, typename Filter = decltype(grid_default_filter<Object>)>
__forceinline auto grid_find_in_box_by_bounding_impl(const Holder &grid_holder, bbox3f bbox, const Predicate &pred,
  const Filter &filter = grid_default_filter<Object>)
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
    const Filter &filterFunc;
  };

  return grid_holder.getBoxIterator(bbox, extend_by_bounding::YES).foreach(ObjectsIterator{pred, filter});
}

template <typename Object, typename Holder, typename Predicate, typename Filter = decltype(grid_default_filter<Object>)>
__forceinline auto grid_find_in_sphere_by_pos_impl(const Holder &grid_holder, const Point3 &center, float radius,
  const Predicate &pred, const Filter &filter = grid_default_filter<Object>)
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
    const Filter &filterFunc;
    vec3f sphPos;
    float sphRadSq;
  };

  bbox3f bbox;
  vec3f sphPos = v_ldu(&center.x);
  vec4f sphRad = v_splats(radius);
  v_bbox3_init_by_bsph(bbox, sphPos, sphRad);
  return grid_holder.getBoxIterator(bbox, extend_by_bounding::NO).foreach(ObjectsIterator{pred, filter, sphPos, sqr(radius)});
}

template <typename Object, typename Holder, typename Predicate, typename Filter = decltype(grid_default_filter<Object>)>
__forceinline auto grid_find_in_sphere_by_bounding_impl(const Holder &grid_holder, const Point3 &center, float radius,
  const Predicate &pred, const Filter &filter = grid_default_filter<Object>)
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
    const Filter &filterFunc;
    vec3f sphPos;
    float sphRad;
  };

  bbox3f bbox;
  vec3f sphPos = v_ldu(&center.x);
  vec4f sphRad = v_splats(radius);
  v_bbox3_init_by_bsph(bbox, sphPos, sphRad);
  return grid_holder.getBoxIterator(bbox, extend_by_bounding::YES).foreach(ObjectsIterator{pred, filter, sphPos, radius});
}

template <typename Object, typename Holder, typename Predicate, typename Filter = decltype(grid_default_filter<Object>)>
__forceinline auto grid_find_in_capsule_by_pos_impl(const Holder &grid_holder, vec3f from, vec3f dir, const vec4f &len,
  const vec4f &radius, const Predicate &pred, const Filter &filter = grid_default_filter<Object>)
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
      return v_truemask(v_cmp_le(distSq, v_sqr(radius)));
    }
    const Predicate &predFunc;
    const Filter &filterFunc;
  };

  return grid_holder.getRayIterator(from, dir, len, radius, extend_by_bounding::NO).foreach(ObjectsIterator{pred, filter});
}

template <typename Object, typename Holder, typename Predicate, typename Filter = decltype(grid_default_filter<Object>)>
__forceinline auto grid_find_in_capsule_by_bounding_impl(const Holder &grid_holder, vec3f from, vec3f dir, const vec4f &len,
  const vec4f &radius, const Predicate &pred, const Filter &filter = grid_default_filter<Object>)
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
      return v_truemask(v_cmp_le(distSq, v_sqr(v_add(objMat.col3, radius))));
    }
    const Predicate &predFunc;
    const Filter &filterFunc;
  };

  return grid_holder.getRayIterator(from, dir, len, radius, extend_by_bounding::YES).foreach(ObjectsIterator{pred, filter});
}

template <typename Object, typename Holder, typename Predicate, typename Filter = decltype(grid_default_filter<Object>)>
__forceinline auto grid_find_in_transformed_box_by_pos_impl(const Holder &grid_holder, const TMatrix &tm, const BBox3 &bbox,
  const Predicate &pred, const Filter &filter = grid_default_filter<Object>)
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
    const Filter &filterFunc;
    bbox3f lbbox;
    mutable mat44f mat44;
    mutable bool itm;
  };

  mat44f mat44;
  bbox3f wsbbox, lbbox = v_ldu_bbox3(bbox);
  v_mat44_make_from_43cu_unsafe(mat44, tm.array);
  v_bbox3_init(wsbbox, mat44, lbbox);
  return grid_holder.getBoxIterator(wsbbox, extend_by_bounding::NO)
    .foreach(ObjectsIterator{pred, filter, lbbox, mat44, /*itm*/ false});
}

template <typename Object, typename Holder, typename Predicate, typename Filter = decltype(grid_default_filter<Object>)>
__forceinline auto grid_find_in_transformed_box_by_bounding_impl(const Holder &grid_holder, const TMatrix &tm, const BBox3 &bbox,
  const Predicate &pred, const Filter &filter = grid_default_filter<Object>)
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
    const Filter &filterFunc;
    bbox3f lbbox;
    mutable mat44f mat44;
    mutable bool itm;
  };

  mat44f mat44;
  bbox3f wsbbox, lbbox = v_ldu_bbox3(bbox);
  v_mat44_make_from_43cu_unsafe(mat44, tm.array);
  v_bbox3_init(wsbbox, mat44, lbbox);
  return grid_holder.getBoxIterator(wsbbox, extend_by_bounding::YES)
    .foreach(ObjectsIterator{pred, filter, lbbox, mat44, /*itm*/ false});
}

// Convex volume queries, 6 planes exactly - see the entry point for what other counts do. tm orients
// bbox and planes alike: both are given in the volume's local space, so a caller cannot pass a world
// AABB that disagrees with its tm, the one the cells are walked with being derived here. Containing
// the convex is still the caller's precondition.
template <typename Object, typename Holder, typename Predicate, typename Filter = decltype(grid_default_filter<Object>)>
__forceinline auto grid_find_in_convex_by_pos_impl(const Holder &grid_holder, const TMatrix &tm, const BBox3 &bbox,
  dag::ConstSpan<plane3f> planes, const Predicate &pred, const Filter &filter = grid_default_filter<Object>)
{
  struct ObjectsIterator
  {
    __forceinline bool checkBoxBoundingInside(bbox3f, bbox3f) const
    {
      return false; // the convex test still has to run per object
    }
    __forceinline bool checkBoxBounding(bbox3f bbox, bbox3f query_box) const { return v_bbox3_test_box_intersect(bbox, query_box); }
    __forceinline bool checkObjectBounding(vec4f wbsph, bbox3f query_bbox) const
    {
      return DAGOR_UNLIKELY(v_bbox3_test_pt_inside(query_bbox, wbsph)) && convex.testPos(wbsph);
    }
    const Predicate &predFunc;
    const Filter &filterFunc;
    GridConvexPlanesSoA convex;
  };

  mat44f mat44;
  bbox3f wsbbox;
  v_mat44_make_from_43cu_unsafe(mat44, tm.array);
  v_bbox3_init(wsbbox, mat44, v_ldu_bbox3(bbox));
  ObjectsIterator objectsIterator{pred, filter};
  objectsIterator.convex.build(planes, mat44);
  return grid_holder.getBoxIterator(wsbbox, extend_by_bounding::NO).foreach(objectsIterator);
}

template <typename Object, typename Holder, typename Predicate, typename Filter = decltype(grid_default_filter<Object>)>
__forceinline auto grid_find_in_convex_by_bounding_impl(const Holder &grid_holder, const TMatrix &tm, const BBox3 &bbox,
  dag::ConstSpan<plane3f> planes, const Predicate &pred, const Filter &filter = grid_default_filter<Object>)
{
  struct ObjectsIterator
  {
    __forceinline bool checkBoxBoundingInside(bbox3f, bbox3f) const
    {
      return false; // the convex test still has to run per object
    }
    __forceinline bool checkBoxBounding(bbox3f bbox, bbox3f query_box) const { return v_bbox3_test_box_intersect(bbox, query_box); }
    __forceinline bool checkObjectBounding(vec4f wbsph, bbox3f query_bbox) const
    {
      vec4f objRad = v_splat_w(wbsph);
      return DAGOR_UNLIKELY(v_bbox3_test_sph_intersect(query_bbox, wbsph, v_mul_x(objRad, objRad))) && convex.testSphere(wbsph);
    }
    const Predicate &predFunc;
    const Filter &filterFunc;
    GridConvexPlanesSoA convex;
  };

  mat44f mat44;
  bbox3f wsbbox;
  v_mat44_make_from_43cu_unsafe(mat44, tm.array);
  v_bbox3_init(wsbbox, mat44, v_ldu_bbox3(bbox));
  ObjectsIterator objectsIterator{pred, filter};
  objectsIterator.convex.build(planes, mat44);
  return grid_holder.getBoxIterator(wsbbox, extend_by_bounding::YES).foreach(objectsIterator);
}

// Closest hit. best_t must point at the caller's current hit distance: pred traces and updates it,
// the grid reads it back to shorten the ray, which prunes the rest of the walk. Returns the object
// that produced the closest hit, so pred does not have to report the handle itself.
//
// A caller that already holds a hit passes it in and gets only something nearer, or null. A pred
// whose trace misses leaves best_t unchanged and strips nothing; with no confirmed hit at all the
// walk visits exactly what the find-first ray query would.
template <typename Object, typename Holder, typename Predicate, typename Filter = decltype(grid_default_filter<Object>)>
__forceinline auto grid_find_closest_ray_intersection_impl(const Holder &grid_holder, vec3f from, vec3f dir, vec4f len,
  const float *best_t, const Predicate &pred, const Filter &filter = grid_default_filter<Object>)
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
    const Predicate &predFunc;
    const Filter &filterFunc;
    const float *bestT;
  };

  // The walk starts at the caller's distance, not the query length. ray->len is what the win test
  // reads as "the distance to beat", so seeding it with the query length would score every object
  // that failed to improve a nearer caller distance as a win. Clamping here also prunes from the
  // first cell rather than only after the first hit.
  const vec4f startLen = v_min(len, v_splats(*best_t));
  return grid_holder.getClosestRayIterator(from, dir, startLen, v_zero(), extend_by_bounding::YES)
    .foreach(ObjectsIterator{pred, filter, best_t});
}

template <typename Object, typename Holder, typename Predicate, typename Filter = decltype(grid_default_filter<Object>)>
__forceinline auto grid_find_ray_intersections_impl(const Holder &grid_holder, vec3f from, vec3f dir, vec4f len, const Predicate &pred,
  const Filter &filter = grid_default_filter<Object>)
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
      return v_truemask(v_cmp_le(distSq, v_sqr(objMat.col3)));
    }
    const Predicate &predFunc;
    const Filter &filterFunc;
  };

  return grid_holder.getRayIterator(from, dir, len, v_zero(), extend_by_bounding::YES).foreach(ObjectsIterator{pred, filter});
}
