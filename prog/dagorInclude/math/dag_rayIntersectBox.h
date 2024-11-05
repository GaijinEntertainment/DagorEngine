//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <math/dag_Point3.h>
#include <math/dag_bounds3.h>
#include <math/dag_TMatrix.h>


//! intersect ray (p0,dir) with world aligned bbox located at (0,0,0) with sizes (box_sz)
inline bool ray_intersect_wa_box0(const Point3 &p0, const Point3 &dir, const Point3 &box_sz)
{
#define TEST_PLANES(NX, NY, NZ, X, Y, Z)                              \
  den = dir.x * NX + dir.y * NY + dir.z * NZ;                         \
  if (float_nonzero(den))                                             \
  {                                                                   \
    t = -(p0.x * NX + p0.y * NY + p0.z * NZ) / den;                   \
    if (t > 0)                                                        \
    {                                                                 \
      float Y = p0.Y + dir.Y * t;                                     \
      float Z = p0.Z + dir.Z * t;                                     \
      if (Y >= -1e-3 && Y <= box_sz.Y && Z >= -1e-3 && Z <= box_sz.Z) \
      {                                                               \
        return true;                                                  \
      }                                                               \
    }                                                                 \
    t = -(p0.x * NX + p0.y * NY + p0.z * NZ - box_sz.X) / den;        \
    if (t > 0)                                                        \
    {                                                                 \
      float Y = p0.Y + dir.Y * t;                                     \
      float Z = p0.Z + dir.Z * t;                                     \
      if (Y >= -1e-3 && Y <= box_sz.Y && Z >= -1e-3 && Z <= box_sz.Z) \
      {                                                               \
        return true;                                                  \
      }                                                               \
    }                                                                 \
  }

  float den, t;
  TEST_PLANES(1, 0, 0, x, y, z);
  TEST_PLANES(0, 1, 0, y, x, z);
  TEST_PLANES(0, 0, 1, z, x, y);

#undef TEST_PLANES
  return false;
}

inline bool ray_intersect_wa_box0(const Point3 &p0, const Point3 &dir, const Point3 &box_sz, float &out_t)
{
#define TEST_PLANES(NX, NY, NZ, X, Y, Z)                              \
  den = dir.x * NX + dir.y * NY + dir.z * NZ;                         \
  if (float_nonzero(den))                                             \
  {                                                                   \
    t = -(p0.x * NX + p0.y * NY + p0.z * NZ) / den;                   \
    if (t > 0 && t < minT)                                            \
    {                                                                 \
      float Y = p0.Y + dir.Y * t;                                     \
      float Z = p0.Z + dir.Z * t;                                     \
      if (Y >= -1e-3 && Y <= box_sz.Y && Z >= -1e-3 && Z <= box_sz.Z) \
      {                                                               \
        minT = t;                                                     \
        intersect = true;                                             \
      }                                                               \
    }                                                                 \
    t = -(p0.x * NX + p0.y * NY + p0.z * NZ - box_sz.X) / den;        \
    if (t > 0 && t < minT)                                            \
    {                                                                 \
      float Y = p0.Y + dir.Y * t;                                     \
      float Z = p0.Z + dir.Z * t;                                     \
      if (Y >= -1e-3 && Y <= box_sz.Y && Z >= -1e-3 && Z <= box_sz.Z) \
      {                                                               \
        minT = t;                                                     \
        intersect = true;                                             \
      }                                                               \
    }                                                                 \
  }

  float den, t, minT = FLT_MAX;
  bool intersect = false;
  TEST_PLANES(1, 0, 0, x, y, z);
  TEST_PLANES(0, 1, 0, y, x, z);
  TEST_PLANES(0, 0, 1, z, x, y);

#undef TEST_PLANES
  if (intersect)
  {
    out_t = minT;
    return true;
  }
  return false;
}

//! intersect ray (p0,dir) with bbox transformed by inverse(box_itm)
__forceinline bool ray_intersect_box_i(const Point3 &p0, const Point3 &dir, const BBox3 &box, const TMatrix &box_itm, float &out_t)
{
  Point3 _p0 = box_itm * p0;
  Point3 _dir = box_itm % dir;
  return ray_intersect_wa_box0(_p0 - box[0], _dir, box[1] - box[0], out_t);
}

//! intersect ray (p0,dir) with bbox transformed by inverse(box_itm)
__forceinline bool ray_intersect_box_i(const Point3 &p0, const Point3 &dir, const BBox3 &box, const TMatrix &box_itm)
{
  Point3 _p0 = box_itm * p0;
  Point3 _dir = box_itm % dir;
  return ray_intersect_wa_box0(_p0 - box[0], _dir, box[1] - box[0]);
}

//! intersect ray (p0,dir) with bbox transformed by box_tm
__forceinline bool ray_intersect_box(const Point3 &p0, const Point3 &dir, const BBox3 &box, const TMatrix &box_tm, float &out_t)
{
  float tm_det = box_tm.det();
  if (fabsf(tm_det) < 1e-12)
    return false;
  return ray_intersect_box_i(p0, dir, box, inverse(box_tm, tm_det), out_t);
}

//! intersect ray (p0,dir) with bbox transformed by box_tm
__forceinline bool ray_intersect_box(const Point3 &p0, const Point3 &dir, const BBox3 &box, const TMatrix &box_tm)
{
  float tm_det = box_tm.det();
  if (fabsf(tm_det) < 1e-12)
    return false;
  return ray_intersect_box_i(p0, dir, box, inverse(box_tm, tm_det));
}
