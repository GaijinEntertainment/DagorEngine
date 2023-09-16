//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <math/dag_Point2.h>
#include <math/dag_Point3.h>

inline void calculate_dudv(const Point3 &v0, const Point3 &v1, const Point3 &v2, const Point2 &uv0, const Point2 &uv1,
  const Point2 &uv2, Point3 &out_du, Point3 &out_dv)
{
  Point3 edge1 = v1 - v0;
  Point3 edge2 = v2 - v0;

  float du1 = uv1.x - uv0.x;
  float dv1 = uv1.y - uv0.y;
  float du2 = uv2.x - uv0.x;
  float dv2 = uv2.y - uv0.y;

  out_du = dv2 * edge1 - dv1 * edge2;
  out_dv = du1 * edge2 - du2 * edge1;

  out_du.normalize();
  out_dv.normalize();

  Point3 faceNormal = edge1 % edge2;
  Point3 textureNormal = out_du % out_dv;
  if (textureNormal * faceNormal < 0.f)
  {
    out_du = -out_du;
    out_dv = -out_dv;
  }
}
