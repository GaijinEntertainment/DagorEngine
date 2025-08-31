// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "riGen/riGrid.h"

#include <grid/gridImpl.h>
#include <ADT/linearGrid.h>

DAGOR_NOINLINE RiGridObject rigrid_find_in_box_by_pos(const RiGrid &grid_holder, bbox3f bbox, const RiGridObjPred &pred)
{
  return grid_find_in_box_by_pos_impl<RiGridObject>(grid_holder, bbox, pred);
}

DAGOR_NOINLINE RiGridObject rigrid_find_in_box_by_bounding(const RiGrid &grid_holder, bbox3f bbox, const RiGridObjPred &pred)
{
  return grid_find_in_box_by_bounding_impl<RiGridObject>(grid_holder, bbox, pred);
}

DAGOR_NOINLINE RiGridObject rigrid_find_in_box_by_bounding_min(const RiGrid &grid_holder, bbox3f bbox, const RiGridObjPred &pred,
  float min_radius)
{
  return grid_find_in_box_by_bounding_impl<RiGridObject>(grid_holder, bbox, pred,
    [min_radius](auto, vec4f wbsph) { return v_extract_w(wbsph) > min_radius; });
}

DAGOR_NOINLINE RiGridObject rigrid_find_in_box_by_bounding_max(const RiGrid &grid_holder, bbox3f bbox, const RiGridObjPred &pred,
  float max_radius)
{
  return grid_find_in_box_by_bounding_impl<RiGridObject>(grid_holder, bbox, pred,
    [max_radius](auto, vec4f wbsph) { return v_extract_w(wbsph) < max_radius; });
}

DAGOR_NOINLINE RiGridObject rigrid_find_in_box_by_bounding_pool(const RiGrid &grid_holder, bbox3f bbox, uint32_t pool,
  const RiGridObjPred &pred)
{
  return grid_find_in_box_by_bounding_impl<RiGridObject>(grid_holder, bbox, pred,
    [pool](RiGridObject object, vec4f) { return rendinst::handle_to_ri_type(object.getHandle()) == pool; });
}

DAGOR_NOINLINE RiGridObject rigrid_find_in_sphere_by_pos(const RiGrid &grid_holder, const Point3 &center, float radius,
  const RiGridObjPred &pred)
{
  return grid_find_in_sphere_by_pos_impl<RiGridObject>(grid_holder, center, radius, pred);
}

DAGOR_NOINLINE RiGridObject rigrid_find_in_sphere_by_bounding(const RiGrid &grid_holder, const Point3 &center, float radius,
  const RiGridObjPred &pred)
{
  return grid_find_in_sphere_by_bounding_impl<RiGridObject>(grid_holder, center, radius, pred);
}

DAGOR_NOINLINE RiGridObject rigrid_find_in_sphere_by_bounding_min(const RiGrid &grid_holder, const Point3 &center, float radius,
  const RiGridObjPred &pred, float min_radius)
{
  return grid_find_in_sphere_by_bounding_impl<RiGridObject>(grid_holder, center, radius, pred,
    [min_radius](auto, vec4f wbsph) { return v_extract_w(wbsph) > min_radius; });
}

DAGOR_NOINLINE RiGridObject rigrid_find_in_sphere_by_bounding_max(const RiGrid &grid_holder, const Point3 &center, float radius,
  const RiGridObjPred &pred, float max_radius)
{
  return grid_find_in_sphere_by_bounding_impl<RiGridObject>(grid_holder, center, radius, pred,
    [max_radius](auto, vec4f wbsph) { return v_extract_w(wbsph) < max_radius; });
}

DAGOR_NOINLINE RiGridObject rigrid_find_in_sphere_by_bounding_pool(const RiGrid &grid_holder, const Point3 &bsphere_c, float radius,
  uint32_t pool, const RiGridObjPred &pred)
{
  return grid_find_in_sphere_by_bounding_impl<RiGridObject>(grid_holder, bsphere_c, radius, pred,
    [pool](RiGridObject object, vec4f) { return rendinst::handle_to_ri_type(object.getHandle()) == pool; });
}

DAGOR_NOINLINE RiGridObject rigrid_find_in_capsule_by_pos(const RiGrid &grid_holder, const Point3 &from, const Point3 &dir, float len,
  float radius, const RiGridObjPred &pred)
{
  return grid_find_in_capsule_by_pos_impl<RiGridObject>(grid_holder, v_ldu(&from.x), v_ldu(&dir.x), v_splats(len), v_splats(radius),
    pred);
}

DAGOR_NOINLINE RiGridObject rigrid_find_in_capsule_by_bounding(const RiGrid &grid_holder, const Point3 &from, const Point3 &dir,
  float len, float radius, const RiGridObjPred &pred)
{
  return grid_find_in_capsule_by_bounding_impl<RiGridObject>(grid_holder, v_ldu(&from.x), v_ldu(&dir.x), v_splats(len),
    v_splats(radius), pred);
}

DAGOR_NOINLINE RiGridObject rigrid_find_in_transformed_box_by_pos(const RiGrid &grid_holder, const TMatrix &tm, const BBox3 &bbox,
  const RiGridObjPred &pred)
{
  return grid_find_in_transformed_box_by_pos_impl<RiGridObject>(grid_holder, tm, bbox, pred);
}

DAGOR_NOINLINE RiGridObject rigrid_find_in_transformed_box_by_bounding(const RiGrid &grid_holder, const TMatrix &tm, const BBox3 &bbox,
  const RiGridObjPred &pred)
{
  return grid_find_in_transformed_box_by_bounding_impl<RiGridObject>(grid_holder, tm, bbox, pred);
}

DAGOR_NOINLINE RiGridObject rigrid_find_ray_intersections(const RiGrid &grid_holder, const Point3 &from, const Point3 &dir, float len,
  const RiGridObjPred &pred)
{
  return grid_find_ray_intersections_impl<RiGridObject>(grid_holder, v_ldu(&from.x), v_ldu(&dir.x), v_splats(len), pred);
}
