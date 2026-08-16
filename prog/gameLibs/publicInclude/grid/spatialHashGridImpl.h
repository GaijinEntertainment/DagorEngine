//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <EASTL/intrusive_list.h>
#include <EASTL/fixed_function.h>
#include <vecmath/dag_vecMathDecl.h>
#include <math/dag_bounds3.h>
#include <generic/dag_span.h>

struct GridObject : public eastl::intrusive_list_node
{
  vec4f wbsph; // wpos|r

  vec4f getWBSph() const { return wbsph; }
};

template <typename CellType, unsigned gridSize>
class SpatialHash2D;
typedef SpatialHash2D<eastl::intrusive_list<GridObject>, 32> GridHolder;
typedef eastl::fixed_function<sizeof(intptr_t) * 4, bool(const GridObject *)> GridObjPred;

VECTORCALL const GridObject *grid_find_in_box_by_pos(const GridHolder &grid_holder, const BBox3 &bbox, const GridObjPred &pred);
VECTORCALL const GridObject *grid_find_in_box_by_bounding(const GridHolder &grid_holder, const BBox3 &bbox, const GridObjPred &pred);
VECTORCALL const GridObject *grid_find_in_sphere_by_pos(const GridHolder &grid_holder, const Point3 &center, float radius,
  const GridObjPred &pred);
VECTORCALL const GridObject *grid_find_in_sphere_by_bounding(const GridHolder &grid_holder, const Point3 &center, float radius,
  const GridObjPred &pred);
VECTORCALL const GridObject *grid_find_in_capsule_by_pos(const GridHolder &grid_holder, const Point3 &from, const Point3 &dir,
  float len, float radius, const GridObjPred &pred);
VECTORCALL const GridObject *grid_find_in_capsule_by_bounding(const GridHolder &grid_holder, const Point3 &from, const Point3 &dir,
  float len, float radius, const GridObjPred &pred);
VECTORCALL const GridObject *grid_find_in_transformed_box_by_pos(const GridHolder &grid_holder, const TMatrix &tm, const BBox3 &bbox,
  const GridObjPred &pred);
VECTORCALL const GridObject *grid_find_in_transformed_box_by_bounding(const GridHolder &grid_holder, const TMatrix &tm,
  const BBox3 &bbox, const GridObjPred &pred);
// bbox and planes are both in the volume's local space, oriented by tm.
// Exactly 6 planes are tested (what the gamemath convex builders emit). ANY OTHER COUNT falls back to
// the oriented bbox query and ignores the planes, so the result is a superset of the convex - callers
// passing an asset-driven plane set must apply their own exact test.
// bbox must contain the convex; it is what the cells are walked with, so a bbox smaller than the
// planes silently loses objects. Unvalidated, since checking it needs the convex's vertices.
// tm may rotate, translate and uniformly scale. NON-UNIFORM SCALE IS NOT SUPPORTED: the normals are
// rotated as vectors, not by the inverse transpose, so both flavours then misclassify. The sibling
// grid_find_in_transformed_box_* is exact for any scale, so a volume moving between the two must not
// carry one.
// The gamemath builders bake the tm they are given into the planes they emit, so hand them the
// volume-local tm and keep the world placement for the tm here - passing the same tm twice transforms
// the planes twice, and the same holds for a bbox measured in world space.
VECTORCALL const GridObject *grid_find_in_convex_by_pos(const GridHolder &grid_holder, const TMatrix &tm, const BBox3 &bbox,
  dag::ConstSpan<plane3f> planes, const GridObjPred &pred);
VECTORCALL const GridObject *grid_find_in_convex_by_bounding(const GridHolder &grid_holder, const TMatrix &tm, const BBox3 &bbox,
  dag::ConstSpan<plane3f> planes, const GridObjPred &pred);
