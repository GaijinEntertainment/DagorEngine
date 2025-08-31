//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <vecmath/dag_vecMath.h>
#include <Jolt/Math/Vec4.h>

inline JPH::Vec3 to_jVec3(const Point3 &p) { return JPH::Vec3(v_ldu_p3(&p.x)); }
inline Point3 to_point3(const JPH::Vec3 &v)
{
  Point3 p;
  v_stu_p3(&p.x, v.mValue);
  return p;
}

JPH::Quat to_jQuat(const TMatrix &tm);

inline JPH::Mat44 to_jMat44(const mat44f &tm)
{
  JPH::Mat44 res;
  res.SetColumn4(0, tm.col0);
  res.SetColumn4(1, tm.col1);
  res.SetColumn4(2, tm.col2);
  res.SetColumn4(3, tm.col3);
  return res;
}
inline JPH::Mat44 to_jMat44(const TMatrix &m)
{
  mat44f mat44;
  v_mat44_make_from_43cu(mat44, m.array);
  return to_jMat44(mat44);
}
inline mat44f to_mat44f(const JPH::Mat44 &m)
{
  mat44f mat44;
  mat44.col0 = m.GetColumn4(0).mValue;
  mat44.col1 = m.GetColumn4(1).mValue;
  mat44.col2 = m.GetColumn4(2).mValue;
  mat44.col3 = m.GetColumn4(3).mValue;
  return mat44;
}
inline TMatrix to_tmatrix(const JPH::Mat44 &m)
{
  TMatrix res;
  v_mat_43cu_from_mat44(res.array, to_mat44f(m));
  return res;
}

// 16-bit ObjectLayer is composed of BroadPhaseLayer(4b):groupMask(6b):layerMask(6b)
inline JPH::ObjectLayer make_objlayer(JPH::BroadPhaseLayer::Type bphl, unsigned short gm, unsigned short lm)
{
  return ((bphl << 12) & 0xF000) | ((gm << 6) & 0x0FC0) | (lm & 0x003F);
}
inline JPH::BroadPhaseLayer::Type objlayer_to_bphlayer(JPH::ObjectLayer l) { return l >> 12; }
inline unsigned short objlayer_to_group_mask(JPH::ObjectLayer l) { return 0x3F & (l >> 6); }
inline unsigned short objlayer_to_layer_mask(JPH::ObjectLayer l) { return 0x3F & l; }
