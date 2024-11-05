// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

// ############################################################################
// ##                                                                        ##
// ##  RECT.H                                                                ##
// ##                                                                        ##
// ##  Misc. Support structure.  Defines an AABB bounding region.            ##
// ##                                                                        ##
// ##  OpenSourced 12/5/2000 by John W. Ratcliff                             ##
// ##                                                                        ##
// ##  No warranty expressed or implied.                                     ##
// ##                                                                        ##
// ##  Part of the Q3BSP project, which converts a Quake 3 BSP file into a   ##
// ##  polygon mesh.                                                         ##
// ############################################################################
// ##                                                                        ##
// ##  Contact John W. Ratcliff at jratcliff@verant.com                      ##
// ############################################################################


#include "vector.h"

template <class Type>
class Rect3d
{
public:
  Rect3d(void)
  {
    r1.Set(0, 0, 0);
    r2.Set(0, 0, 0);
  };

  Rect3d(const Vector3d<Type> &v1, const Vector3d<Type> &v2)
  {
    r1 = v1;
    r2 = v2;
  };

  Rect3d(Type x1, Type y1, Type z1, Type x2, Type y2, Type z2)
  {
    r1.Set(x1, y1, z1);
    r2.Set(x2, y2, z2);
  };

  void Set(Type x1, Type y1, Type z1, Type x2, Type y2, Type z2)
  {

    Type temp;

    if (x1 > x2)
    {
      temp = x2;
      x2 = x1;
      x1 = temp;
    }

    if (y1 > y2)
    {
      temp = y2;
      y2 = y1;
      y1 = temp;
    }

    if (z1 > z2)
    {
      temp = z2;
      z2 = z1;
      z1 = temp;
    }

    r1.Set(x1, y1, z1);
    r2.Set(x2, y2, z2);
  };

  void InitMinMax(void)
  {
    r1.Set(1E9, 1E9, 1E9);
    r2.Set(-1E9, -1E9, -1E9);
  };

  void MinMax(const Vector3d<Type> &pos) { MinMax(pos.GetX(), pos.GetY(), pos.GetZ()); };

  void MinMax(Type x, Type y, Type z)
  {
    if (x < r1.GetX())
      r1.SetX(x);
    if (y < r1.GetY())
      r1.SetY(y);
    if (z < r1.GetZ())
      r1.SetZ(z);

    if (x > r2.GetX())
      r2.SetX(x);
    if (y > r2.GetY())
      r2.SetY(y);
    if (z > r2.GetZ())
      r2.SetZ(z);
  };

  Vector3d<Type> r1;
  Vector3d<Type> r2;
};
