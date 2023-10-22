//
// DaEditorX
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <math/dag_plane3.h>


void generateBox(const TMatrix &m, Plane3 box[6], BBox3 &b)
{
  float bverts[8][3] = {{-0.5, -0.5, 0.5}, {-0.5, 0.5, 0.5}, {0.5, 0.5, 0.5}, {0.5, -0.5, 0.5},

    {-0.5, -0.5, -0.5}, {-0.5, 0.5, -0.5}, {0.5, 0.5, -0.5}, {0.5, -0.5, -0.5}};

  int bfaces[6][3] = {
    {0, 1, 2},

    {4, 7, 6},

    {4, 0, 3},

    {7, 3, 2},

    {6, 2, 1},

    {5, 1, 0},
  };

  b.setempty();

  Point3 v[8];
  for (int i = 0; i < 8; ++i)
  {
    Point3 bv(bverts[i][0], bverts[i][1], bverts[i][2]);
    Point3 mv;
    mv = m * bv;
    v[i] = mv;
    b += mv;
  }

  for (int i = 0; i < 6; ++i)
  {
    box[i] = Plane3(v[bfaces[i][0]], v[bfaces[i][1]], v[bfaces[i][2]]);
    box[i].normalize();
  }
}

bool isInside(Plane3 box[6], Point3 &p)
{
  for (int i = 0; i < 6; ++i)
  {
    if (box[i].distance(p) < 0)
      return false;
  }

  return true;
}
