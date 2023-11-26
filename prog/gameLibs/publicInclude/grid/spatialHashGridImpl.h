//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <EASTL/intrusive_list.h>
#include <EASTL/fixed_function.h>
#include <vecmath/dag_vecMathDecl.h>
#include <math/dag_bounds3.h>

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
