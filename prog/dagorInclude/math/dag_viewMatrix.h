//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <math/dag_Point3.h>
#include <math/dag_TMatrix.h>
inline void view_matrix_from_look(const Point3 &look, TMatrix &tm)
{
  tm.setcol(0, Point3(0, 1, 0) % look);
  float xAxisLength = length(tm.getcol(0));
  if (xAxisLength <= 1.192092896e-06F)
  {
    tm.setcol(0, normalize(Point3(0, 0, look.y / fabsf(look.y)) % look));
    tm.setcol(1, normalize(look % tm.getcol(0)));
  }
  else
  {
    tm.setcol(0, tm.getcol(0) / xAxisLength);
    tm.setcol(1, normalize(look % tm.getcol(0)));
  }
  tm.setcol(2, look);
}

inline Point3 tangent_to_world(const Point3 &vec, const Point3 &tangentZ)
{
  Point3 up = fabsf(tangentZ.z) < 0.999 ? Point3(0, 0, 1) : Point3(1, 0, 0);
  Point3 tangentX = normalize(cross(up, tangentZ));
  Point3 tangentY = cross(tangentZ, tangentX);
  return tangentX * vec.x + tangentY * vec.y + tangentZ * vec.z;
}

inline void view_matrix_from_tangentZ(const Point3 &tangentZ, TMatrix &tm)
{
  tm.setcol(2, tangentZ);
  Point3 up = fabsf(tangentZ.z) < 0.999 ? Point3(0, 0, 1) : Point3(1, 0, 0);
  tm.setcol(0, normalize(cross(up, tangentZ)));
  tm.setcol(1, cross(tangentZ, tm.getcol(0)));
}
