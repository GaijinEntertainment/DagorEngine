//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <math/dag_Point3.h>
#include <math/dag_bounds3.h>

BSphere3 mesh_bounding_sphere(const Point3 *point, unsigned point_count);

BSphere3 mesh_fast_bounding_sphere(const Point3 *point, unsigned point_count);

BSphere3 triangle_bounding_sphere(const Point3 &p1, const Point3 &p2, const Point3 &p3);

__forceinline BSphere3 triangle_bounding_sphere(const Point3 *point) { return triangle_bounding_sphere(point[0], point[1], point[2]); }
