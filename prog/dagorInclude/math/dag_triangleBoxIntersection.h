//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <math/dag_Point3.h>
#include <math/dag_bounds3.h>


bool test_triangle_box_intersection(const Point3 &point0, const Point3 &point1, const Point3 &point2, const BBox3 &bbox);
