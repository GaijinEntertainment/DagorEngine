// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <math/dag_math3d.h>
inline void view_matrix_from_look(const Point3 &look, TMatrix &tm, int cub5_face = 0)
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
  if (cub5_face != 0)
  {
    Point3 xa = tm.getcol(0);
    Point3 ya = tm.getcol(1);
    Point3 za = tm.getcol(2);
    switch (cub5_face)
    {
      case 1:
        tm.setcol(0, -xa);
        tm.setcol(1, za);
        tm.setcol(2, ya);
        break;
      case 2:
        tm.setcol(0, ya);
        tm.setcol(1, za);
        tm.setcol(2, xa);
        break;
      case 3:
        tm.setcol(0, xa);
        tm.setcol(1, za);
        tm.setcol(2, -ya);
        break;
      case 4:
        tm.setcol(0, -ya);
        tm.setcol(1, za);
        tm.setcol(2, -xa);
        break;
      case 5:
        tm.setcol(0, xa);
        tm.setcol(1, -ya);
        tm.setcol(2, -za);
        break;
    }
    if ((tm.getcol(0) % tm.getcol(1)) * tm.getcol(2) < 0)
      tm.setcol(0, -tm.getcol(0));
  }
}
