// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <vecmath/dag_vecMathDecl.h>
#include <math/dag_bounds3.h>
#include <riGen/riGenExtra.h>

struct RiGridObject
{
  rendinst::riex_handle_t handle;

  RiGridObject() {} //-V730
  RiGridObject(rendinst::riex_handle_t h) : handle(h) {}
  bool operator==(RiGridObject rhs) const { return handle == rhs.handle; }
  bool operator!=(RiGridObject rhs) const { return handle != rhs.handle; }
  bool operator<(RiGridObject rhs) const { return handle < rhs.handle; }
  vec4f getWBSph() const // wpos|r
  {
    uint32_t riType = rendinst::handle_to_ri_type(handle);
    uint32_t riInstance = rendinst::handle_to_ri_inst(handle);
    return rendinst::riExtra.data()[riType].riXYZR.data()[riInstance];
  }
  bbox3f getWBBox() const
  {
    uint32_t riType = rendinst::handle_to_ri_type(handle);
    uint32_t riInstance = rendinst::handle_to_ri_inst(handle);
    const rendinst::RiExtraPool &pool = rendinst::riExtra.data()[riType];
    mat44f tm;
    v_mat43_transpose_to_mat44(tm, pool.riTm.data()[riInstance]);
    bbox3f bbox;
    v_bbox3_init(bbox, tm, pool.collBb);
    // optimize by world sphere. box from sphere can be smaller for rotated objects.
    vec4f wbsph = getWBSph();
    bbox3f sphBox;
    v_bbox3_init_by_bsph(sphBox, wbsph, v_bsph_radius(wbsph));
    return v_bbox3_get_box_intersection(sphBox, bbox);
  }
  uint64_t getHandle() const { return handle; }
  static RiGridObject null() { return rendinst::RIEX_HANDLE_NULL; }
  const char *getDebugName() const;
};

template <typename ObjectType>
union LinearGridLeaf;
template <typename ObjectType>
struct LinearGridSubCell;
template <typename ObjectType>
struct LinearGridMainCell;
template <typename CellType>
class LinearGrid;
typedef LinearGrid<RiGridObject> RiGrid;
typedef eastl::fixed_function<256, bool(RiGridObject)> RiGridObjPred;

DAG_DECLARE_RELOCATABLE(RiGridObject);
DAG_DECLARE_RELOCATABLE(LinearGridLeaf<RiGridObject>);
DAG_DECLARE_RELOCATABLE(LinearGridSubCell<RiGridObject>);
DAG_DECLARE_RELOCATABLE(LinearGridMainCell<RiGridObject>);

VECTORCALL RiGridObject rigrid_find_in_box_by_pos(const RiGrid &grid_holder, const bbox3f &bbox, const RiGridObjPred &pred);
VECTORCALL RiGridObject rigrid_find_in_box_by_bounding(const RiGrid &grid_holder, const bbox3f &bbox, const RiGridObjPred &pred);
VECTORCALL RiGridObject rigrid_find_in_box_by_bounding_min(const RiGrid &grid_holder, const bbox3f &bbox, const RiGridObjPred &pred,
  float min_radius);
VECTORCALL RiGridObject rigrid_find_in_box_by_bounding_max(const RiGrid &grid_holder, const bbox3f &bbox, const RiGridObjPred &pred,
  float max_radius);
VECTORCALL RiGridObject rigrid_find_in_box_by_bounding_pool(const RiGrid &grid_holder, const bbox3f &bbox, uint32_t pool,
  const RiGridObjPred &pred);
VECTORCALL RiGridObject rigrid_find_in_sphere_by_pos(const RiGrid &grid_holder, const Point3 &bsphere_c, float radius,
  const RiGridObjPred &pred);
VECTORCALL RiGridObject rigrid_find_in_sphere_by_bounding(const RiGrid &grid_holder, const Point3 &bsphere_c, float radius,
  const RiGridObjPred &pred);
VECTORCALL RiGridObject rigrid_find_in_sphere_by_bounding_min(const RiGrid &grid_holder, const Point3 &center, float radius,
  const RiGridObjPred &pred, float min_radius);
VECTORCALL RiGridObject rigrid_find_in_sphere_by_bounding_max(const RiGrid &grid_holder, const Point3 &center, float radius,
  const RiGridObjPred &pred, float max_radius);
VECTORCALL RiGridObject rigrid_find_in_sphere_by_bounding_pool(const RiGrid &grid_holder, const Point3 &bsphere_c, float radius,
  uint32_t pool, const RiGridObjPred &pred);
VECTORCALL RiGridObject rigrid_find_in_capsule_by_pos(const RiGrid &grid_holder, const Point3 &from, const Point3 &dir, float len,
  float radius, const RiGridObjPred &pred);
VECTORCALL RiGridObject rigrid_find_in_capsule_by_bounding(const RiGrid &grid_holder, const Point3 &from, const Point3 &dir, float len,
  float radius, const RiGridObjPred &pred);
VECTORCALL RiGridObject rigrid_find_in_transformed_box_by_pos(const RiGrid &grid_holder, const TMatrix &tm, const BBox3 &bbox,
  const RiGridObjPred &pred);
VECTORCALL RiGridObject rigrid_find_in_transformed_box_by_bounding(const RiGrid &grid_holder, const TMatrix &tm, const BBox3 &bbox,
  const RiGridObjPred &pred);
VECTORCALL RiGridObject rigrid_find_ray_intersections(const RiGrid &grid_holder, const Point3 &from, const Point3 &dir, float len,
  const RiGridObjPred &pred);
// Closest hit. best_t points at the caller's current hit distance: pred traces and updates it, the
// grid shortens the ray to match and skips whatever can no longer be closer. Returns the winner.
// An in-out limit, so a caller holding a hit passes it in and gets only something nearer, or null;
// pass len for "no hit yet". pred's return value is ignored here, it cannot abort the walk.
// Only that write-back shortens the ray: a pred whose trace misses leaves best_t alone and strips
// nothing, so the bounding prefilter can never truncate the ray by itself. For "does anything
// block" use rigrid_find_ray_intersections above instead - there pred confirms and returns true to
// stop at the first hit; this query is for when the nearest hit itself is needed.
VECTORCALL RiGridObject rigrid_find_closest_ray_intersection(const RiGrid &grid_holder, const Point3 &from, const Point3 &dir,
  float len, const float *best_t, const RiGridObjPred &pred);
void rigrid_debug_pos(const RiGrid &grid_holder, const Point3 &pos);

namespace rigrid_dump
{
struct GridConfig;
}
// Reads the live riExtraGrid config, so a dump records what it was actually built with instead of
// re-deriving it from gameparams and risking a different set of defaults.
void rigrid_get_dump_config(rigrid_dump::GridConfig &out);
