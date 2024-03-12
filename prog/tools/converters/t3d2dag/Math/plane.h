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


#ifndef _PLANE_H_
#define _PLANE_H_

#include "vertex.h"

enum RELATION
{
  BACK = -1,
  PLANAR = 0,
  FRONT = 1,
  CUTS = 2
};

struct Plane
{
  Vertex normal;
  float offset;

  Plane() {}
  Plane(const Vertex &Normal, const float Offset);
  Plane(const float a, const float b, const float c, const float d);
  Plane(const Vertex &v0, const Vertex &v1, const Vertex &v2);

  void normalize();
  float distance(const Vertex &v) const;
  float getPlaneHitInterpolationConstant(const Vertex &v0, const Vertex &v1) const;
  RELATION getVertexRelation(const Vertex &v, float planarDiff = 0.03f) const;
};

#endif
