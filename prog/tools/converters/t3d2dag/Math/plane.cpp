// Copyright (C) Gaijin Games KFT.  All rights reserved.

/***********      .---.         .-"-.      *******************\
* -------- *     /   ._.       / ´ ` \     * ---------------- *
* Author's *     \_  (__\      \_°v°_/     * humus@rogers.com *
*   note   *     //   \\       //   \\     * ICQ #47010716    *
* -------- *    ((     ))     ((     ))    * ---------------- *
*          ****--""---""-------""---""--****                  ********\
* This file is a part of the work done by Humus. You are free to use  *
* the code in any way you like, modified, unmodified or copy'n'pasted *
* into your own work. However, I expect you to respect these points:  *
*  @ If you use this file and its contents unmodified, or use a major *
*    part of this file, please credit the author and leave this note. *
*  @ For use in anything commercial, please request my approval.      *
*  @ Share your work and ideas too as much as you can.                *
\*********************************************************************/


#include "plane.h"

Plane::Plane(const Vertex &Normal, const float Offset)
{
  normal = Normal;
  offset = Offset;
}

Plane::Plane(const float a, const float b, const float c, const float d)
{
  normal.x = a;
  normal.y = b;
  normal.z = c;
  offset = d;
}

Plane::Plane(const Vertex &v0, const Vertex &v1, const Vertex &v2)
{
  normal = cross(v1 - v0, v2 - v0);
  normal.normalize();
  offset = -(normal * v0);
}

void Plane::normalize()
{
  float invLen = 1.0f / length(normal);
  normal *= invLen;
  offset *= invLen;
}

float Plane::distance(const Vertex &v) const { return (v * normal + offset); }

float Plane::getPlaneHitInterpolationConstant(const Vertex &v0, const Vertex &v1) const
{
  return (distance(v0)) / (normal * (v0 - v1));
}

RELATION Plane::getVertexRelation(const Vertex &v, float planarDiff) const
{
  float rel = distance(v);

  if (fabsf(rel) < planarDiff)
    return PLANAR;
  if (rel > 0)
    return FRONT;
  else
    return BACK;
}
