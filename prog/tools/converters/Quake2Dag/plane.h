// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

// ############################################################################
// ##                                                                        ##
// ##  PLANE.H                                                               ##
// ##                                                                        ##
// ##  Support structure, defines a plane in 3space.                         ##
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

#include "stl.h"
#include "vector.h"

#define PLANE_EPSILON 0.0001f

class Plane
{
public:
  Vector3d<float> N; // Ax By Cz
  float D;           // D defines plane equation.
  float InverseZ;    // pre-compued -1.0/NormalZ to speed up intersection computation
  Plane(void)
  {
    N.Set(0, 0, 1);
    InverseZ = 1;
    D = 0;
  };

  Plane(const float *p)
  {
    N.x = p[0];
    N.y = p[1];
    N.z = p[2];
    D = p[3];
    InverseZ = 1.0f / N.z;
  }

  bool operator==(const Plane &a) const
  {
    float dx = (float)fabs(N.x - a.N.x);
    if (dx > PLANE_EPSILON)
      return false;

    float dy = (float)fabs(N.y - a.N.y);
    if (dy > PLANE_EPSILON)
      return false;

    float dz = (float)fabs(N.z - a.N.z);
    if (dz > PLANE_EPSILON)
      return false;

    float dd = (float)fabs(D - a.D);
    if (dd > PLANE_EPSILON)
      return false;

    return true;
  };


  // distance from this point to the plane.
  float DistToPt(const Vector3d<float> &pos) const { return (pos.x * N.x + pos.y * N.y + pos.z * N.z) + D; };

  bool Intersect(const Vector3d<float> &p1, const Vector3d<float> &p2, Vector3d<float> &sect)
  {
    float dp1 = DistToPt(p1);
    float dp2 = DistToPt(p2);

    if (dp1 <= 0)
    {
      if (dp2 <= 0)
        return false;
    }
    else
    {
      if (dp2 > 0)
        return false;
    }


    Vector3d<float> dir = p2 - p1;
    float dot1 = dir.Dot(N);
    float dot2 = dp1 - D;
    float t = -(D + dot2) / dot1;
    sect.x = (dir.x * t) + p1.x;
    sect.y = (dir.y * t) + p1.y;
    sect.z = (dir.z * t) + p1.z;
    return true;
  };

  void PlaneTest(const Vector3d<float> &pos, int &code, int bit)
  {
    float dist = DistToPt(pos);
    if (dist > 0)
      code |= bit;
  };

  bool Inside(const Vector3d<float> &pos) const
  {
    float dist = DistToPt(pos);
    if (dist <= 0)
      return true;
    return false;
  };

  // test inclusion/exclusion/intersection between plane and box
  bool Exclude(bool &isect, const Vector3d<float> &bMin, const Vector3d<float> &bMax)
  {

    Vector3d<float> neg, pos;

    if (N.x > 0.0f)
    {
      if (N.y > 0.0f)
      {
        if (N.z > 0.0f)
        {
          pos.x = bMax.x;
          pos.y = bMax.y;
          pos.z = bMax.z;
          neg.x = bMin.x;
          neg.y = bMin.y;
          neg.z = bMin.z;
        }
        else
        {
          pos.x = bMax.x;
          pos.y = bMax.y;
          pos.z = bMin.z;
          neg.x = bMin.x;
          neg.y = bMin.y;
          neg.z = bMax.z;
        }
      }
      else
      {
        if (N.z > 0.0f)
        {
          pos.x = bMax.x;
          pos.y = bMin.y;
          pos.z = bMax.z;
          neg.x = bMin.x;
          neg.y = bMax.y;
          neg.z = bMin.z;
        }
        else
        {
          pos.x = bMax.x;
          pos.y = bMin.y;
          pos.z = bMin.z;
          neg.x = bMin.x;
          neg.y = bMax.y;
          neg.z = bMax.z;
        }
      }
    }
    else
    {
      if (N.y > 0.0f)
      {
        if (N.z > 0.0f)
        {
          pos.x = bMin.x;
          pos.y = bMax.y;
          pos.z = bMax.z;
          neg.x = bMax.x;
          neg.y = bMin.y;
          neg.z = bMin.z;
        }
        else
        {
          pos.x = bMin.x;
          pos.y = bMax.y;
          pos.z = bMin.z;
          neg.x = bMax.x;
          neg.y = bMin.y;
          neg.z = bMax.z;
        }
      }
      else
      {
        if (N.z > 0.0f)
        {
          pos.x = bMin.x;
          pos.y = bMin.y;
          pos.z = bMax.z;
          neg.x = bMax.x;
          neg.y = bMax.y;
          neg.z = bMin.z;
        }
        else
        {
          pos.x = bMin.x;
          pos.y = bMin.y;
          pos.z = bMin.z;
          neg.x = bMax.x;
          neg.y = bMax.y;
          neg.z = bMax.z;
        }
      }
    }

    if (DistToPt(neg) > 0)
      return true; // is excluded!
    else if (DistToPt(pos) > 0)
      isect = true; // does intersect plane!

    return false; // not completely excluded yet.
  }

  void Print(const char *name) const { printf("%s N=(%.2f,%.2f,%.2f) D=(%.2f)\n", name, N.x, N.y, N.z, D); };

  void Print(FILE *fph, const char *name) const { fprintf(fph, "%s N=(%.2f,%.2f,%.2f) D=(%.2f)\n", name, N.x, N.y, N.z, D); };

  void Compute(const Vector3d<float> &A, const Vector3d<float> &B, const Vector3d<float> &C)
  {
    float vx, vy, vz, wx, wy, wz, vw_x, vw_y, vw_z, mag;

    vx = (B.x - C.x);
    vy = (B.y - C.y);
    vz = (B.z - C.z);

    wx = (A.x - B.x);
    wy = (A.y - B.y);
    wz = (A.z - B.z);

    vw_x = vy * wz - vz * wy;
    vw_y = vz * wx - vx * wz;
    vw_z = vx * wy - vy * wx;

    mag = sqrtf((vw_x * vw_x) + (vw_y * vw_y) + (vw_z * vw_z));

    if (mag < 0.000001f)
    {
      mag = 0;
    }
    else
    {
      mag = 1.0f / mag;
    }

    N.x = vw_x * mag;
    N.y = vw_y * mag;
    N.z = vw_z * mag;

    D = 0.0f - ((N.x * A.x) + (N.y * A.y) + (N.z * A.z));

    if (N.z != 0.0f)
      InverseZ = -1.0f / N.z; // compute inverse Z
    else
      InverseZ = 0;
  };


  float SolveZ(float x, float y) const
  {
    float z = (N.y * y + N.x * x + D) * InverseZ;
    return z;
  };
};

typedef std::vector<Plane> PlaneVector;
