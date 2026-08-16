// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <grid/spatialHashGridImpl.h>
#include <grid/gridImpl.h>
#include <ADT/spatialHash.h>

DAGOR_NOINLINE const GridObject *grid_find_in_box_by_pos(const GridHolder &grid_holder, const BBox3 &bbox, const GridObjPred &pred)
{
  return grid_find_in_box_by_pos_impl<GridObject>(grid_holder, v_ldu_bbox3(bbox), pred);
}

DAGOR_NOINLINE const GridObject *grid_find_in_box_by_bounding(const GridHolder &grid_holder, const BBox3 &bbox,
  const GridObjPred &pred)
{
  return grid_find_in_box_by_bounding_impl<GridObject>(grid_holder, v_ldu_bbox3(bbox), pred);
}

DAGOR_NOINLINE const GridObject *grid_find_in_sphere_by_pos(const GridHolder &grid_holder, const Point3 &center, float radius,
  const GridObjPred &pred)
{
  return grid_find_in_sphere_by_pos_impl<GridObject>(grid_holder, center, radius, pred);
}

DAGOR_NOINLINE const GridObject *grid_find_in_sphere_by_bounding(const GridHolder &grid_holder, const Point3 &center, float radius,
  const GridObjPred &pred)
{
  return grid_find_in_sphere_by_bounding_impl<GridObject>(grid_holder, center, radius, pred);
}

DAGOR_NOINLINE const GridObject *grid_find_in_capsule_by_pos(const GridHolder &grid_holder, const Point3 &from, const Point3 &dir,
  float len, float radius, const GridObjPred &pred)
{
  return grid_find_in_capsule_by_pos_impl<GridObject>(grid_holder, v_ldu(&from.x), v_ldu(&dir.x), v_splats(len), v_splats(radius),
    pred);
}

DAGOR_NOINLINE const GridObject *grid_find_in_capsule_by_bounding(const GridHolder &grid_holder, const Point3 &from, const Point3 &dir,
  float len, float radius, const GridObjPred &pred)
{
  return grid_find_in_capsule_by_bounding_impl<GridObject>(grid_holder, v_ldu(&from.x), v_ldu(&dir.x), v_splats(len), v_splats(radius),
    pred);
}

DAGOR_NOINLINE const GridObject *grid_find_in_transformed_box_by_pos(const GridHolder &grid_holder, const TMatrix &tm,
  const BBox3 &bbox, const GridObjPred &pred)
{
  return grid_find_in_transformed_box_by_pos_impl<GridObject>(grid_holder, tm, bbox, pred);
}

DAGOR_NOINLINE const GridObject *grid_find_in_transformed_box_by_bounding(const GridHolder &grid_holder, const TMatrix &tm,
  const BBox3 &bbox, const GridObjPred &pred)
{
  return grid_find_in_transformed_box_by_bounding_impl<GridObject>(grid_holder, tm, bbox, pred);
}

// The plane test is fixed at 6 planes. Anything else - a convex collision node carries one plane per
// exported face, or a k-dop's plane set - narrows by the oriented bbox alone: fewer candidates than
// no query at all, and callers run their own exact test afterwards regardless.
DAGOR_NOINLINE const GridObject *grid_find_in_convex_by_pos(const GridHolder &grid_holder, const TMatrix &tm, const BBox3 &bbox,
  dag::ConstSpan<plane3f> planes, const GridObjPred &pred)
{
  if (planes.size() != GridConvexPlanesSoA::PLANE_COUNT)
    return grid_find_in_transformed_box_by_pos_impl<GridObject>(grid_holder, tm, bbox, pred);
  return grid_find_in_convex_by_pos_impl<GridObject>(grid_holder, tm, bbox, planes, pred);
}

DAGOR_NOINLINE const GridObject *grid_find_in_convex_by_bounding(const GridHolder &grid_holder, const TMatrix &tm, const BBox3 &bbox,
  dag::ConstSpan<plane3f> planes, const GridObjPred &pred)
{
  if (planes.size() != GridConvexPlanesSoA::PLANE_COUNT)
    return grid_find_in_transformed_box_by_bounding_impl<GridObject>(grid_holder, tm, bbox, pred);
  return grid_find_in_convex_by_bounding_impl<GridObject>(grid_holder, tm, bbox, planes, pred);
}
