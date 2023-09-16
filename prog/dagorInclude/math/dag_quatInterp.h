//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <math/dag_TMatrix.h>
#include <math/dag_Quat.h>
#include <math/dag_mathBase.h>

namespace AnimV20Math
{

__forceinline TMatrix makeTM(Point3 p, Quat q, Point3 s)
{
  TMatrix m;

  m.m[0][0] = (q.x * q.x + q.w * q.w - 0.5f) * 2 * s.x;
  m.m[1][0] = (q.x * q.y - q.z * q.w) * 2 * s.y;
  m.m[2][0] = (q.x * q.z + q.y * q.w) * 2 * s.z;

  m.m[0][1] = (q.y * q.x + q.z * q.w) * 2 * s.x;
  m.m[1][1] = (q.y * q.y + q.w * q.w - 0.5f) * 2 * s.y;
  m.m[2][1] = (q.y * q.z - q.x * q.w) * 2 * s.z;

  m.m[0][2] = (q.z * q.x - q.y * q.w) * 2 * s.x;
  m.m[1][2] = (q.z * q.y + q.x * q.w) * 2 * s.y;
  m.m[2][2] = (q.z * q.z + q.w * q.w - 0.5f) * 2 * s.z;

  m.m[3][0] = p.x;
  m.m[3][1] = p.y;
  m.m[3][2] = p.z;

  return m;
}


__forceinline float dot(const Quat &l, const Quat &r) { return l.x * r.x + l.y * r.y + l.z * r.z + l.w * r.w; }
__forceinline Quat lerp(const Quat &l, Quat r, float lt, float rt)
{
  return {l.x * lt + r.x * rt, l.y * lt + r.y * rt, l.z * lt + r.z * rt, l.w * lt + r.w * rt};
}
inline Quat fnlerp(const Quat &l, const Quat &r, float t)
{
  float ca = dot(l, r);

  float d = fabsf(ca);
  float k = 0.931872f + d * (-1.25654f + d * 0.331442f);
  float ot = t + t * (t - 0.5f) * (t - 1) * k;

  float lt = 1 - ot;
  float rt = ca > 0 ? ot : -ot;

  return normalize(lerp(l, r, lt, rt));
}

inline Quat onlerp(const Quat &l, const Quat &r, float t)
{
  float ca = dot(l, r);

  float d = fabsf(ca);
  float A = 1.0904f + d * (-3.2452f + d * (3.55645f - d * 1.43519f));
  float B = 0.848013f + d * (-1.06021f + d * 0.215638f);
  float k = A * (t - 0.5f) * (t - 0.5f) + B;
  float ot = t + t * (t - 0.5f) * (t - 1) * k;

  float lt = 1 - ot;
  float rt = ca > 0 ? ot : -ot;

  return normalize(lerp(l, r, lt, rt));
}

inline Quat fast_qinterp(const Quat &q1, const Quat &q2, float t) { return onlerp(q1, q2, t); }

__forceinline Quat qinterp(const Quat &q1, const Quat &q2, float t) { return normalize(::qinterp(q1, q2, t)); }

} // end of namespace AnimV20Math
