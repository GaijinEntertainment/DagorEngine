//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <math/dag_Point3.h>
#include <vecmath/dag_vecMathDecl.h>

// 6 ray vs. triangles tests
bool test_triangle_triangle_intersection(const Point3 &p1, const Point3 &q1, const Point3 &r1, const Point3 &p2, const Point3 &q2,
  const Point3 &r2);

// fast, but seems not to work always
bool test_triangle_triangle_intersection_mueller(const Point3 &p1, const Point3 &q1, const Point3 &r1, const Point3 &p2,
  const Point3 &q2, const Point3 &r2);

bool VECTORCALL v_test_triangle_triangle_intersection(vec3f v0, vec3f v1, vec3f v2, vec3f u0, vec3f u1, vec3f u2);
