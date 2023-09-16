//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#pragma message("obsolete sphere visibility")

#include <math/dag_TMatrix4.h>
#include <math/dag_bounds3.h>

enum
{
  CLIP_LEFT = 0x00000001,
  CLIP_RIGHT = 0x00000002,
  CLIP_TOP = 0x00000004,
  CLIP_BOTTOM = 0x00000008,
  CLIP_FRONT = 0x00000010,
  CLIP_BACK = 0x00000020,
  CLIP_GEN0 = 0x00000040,
  CLIP_GEN1 = 0x00000080,
  CLIP_GEN2 = 0x00000100,
  CLIP_GEN3 = 0x00000200,
  CLIP_GEN4 = 0x00000400,
  CLIP_GEN5 = 0x00000800,

  CLIP_UNIONALL = 0x00000FFF,

  CLIP_ISECTLEFT = 0x00001000,
  CLIP_ISECTRIGHT = 0x00002000,
  CLIP_ISECTTOP = 0x00004000,
  CLIP_ISECTBOTTOM = 0x00008000,
  CLIP_ISECTFRONT = 0x00010000,
  CLIP_ISECTBACK = 0x00020000,
  CLIP_ISECTGEN0 = 0x00040000,
  CLIP_ISECTGEN1 = 0x00080000,
  CLIP_ISECTGEN2 = 0x00100000,
  CLIP_ISECTGEN3 = 0x00200000,
  CLIP_ISECTGEN4 = 0x00400000,
  CLIP_ISECTGEN5 = 0x00800000,
  CLIP_ZNOTVISIBLE = 0x01000000,

  CLIP_ISECTALL = 0x00FFF000,

  CLIP_DEFAULT = CLIP_ISECTALL | CLIP_ZNOTVISIBLE,
};


// get clip flags for sphere with center c and radius r
// if (clipflg&CLIP_DEFAULT)!=0, sphere is not visible
// if clipflg==0, sphere is not clipped at all
// NOTE: user clip planes are NOT used to determine clip flags
// use getglobtm() to get globtm and globrk
__forceinline unsigned sphere_visibility(const Point3 &c, real r, const TMatrix4 &globtm, float * = nullptr)
{
  const Point4 globrk = globtm.getrow(0) + globtm.getrow(1) + globtm.getrow(2);
  real z = c.x * globtm(0, 2) + c.y * globtm(1, 2) + c.z * globtm(2, 2) + globtm(3, 2);
  real zr = globrk[2] * r;
  if (z + zr <= 0)
    return CLIP_ISECTBACK | CLIP_UNIONALL;
  real w = c.x * globtm(0, 3) + c.y * globtm(1, 3) + c.z * globtm(2, 3) + globtm(3, 3);
  real wr = globrk[3] * r;
  if (z - zr > w - wr)
    return CLIP_ISECTFRONT | CLIP_UNIONALL;
  real x = c.x * globtm(0, 0) + c.y * globtm(1, 0) + c.z * globtm(2, 0) + globtm(3, 0);
  real xr = globrk[0] * r;
  if (x + xr <= -w - wr)
    return CLIP_ISECTLEFT | CLIP_UNIONALL;
  if (x - xr > w + wr)
    return CLIP_ISECTRIGHT | CLIP_UNIONALL;
  real y = c.x * globtm(0, 1) + c.y * globtm(1, 1) + c.z * globtm(2, 1) + globtm(3, 1);
  real yr = globrk[1] * r;
  if (y + yr <= -w - wr)
    return CLIP_ISECTBOTTOM | CLIP_UNIONALL;
  if (y - yr > w + wr)
    return CLIP_ISECTTOP | CLIP_UNIONALL;
  if (z - zr > 0 && z + zr <= w + wr && x - xr > -w + wr && x + xr <= w - wr && y - yr > -w + wr && y + yr <= w - wr)
    return 0;
  return CLIP_UNIONALL;
}

inline unsigned sphere_visibility(const BSphere3 &sph, const TMatrix4 &globtm, float * = nullptr)
{
  return sphere_visibility(sph.c, sph.r, globtm);
}
