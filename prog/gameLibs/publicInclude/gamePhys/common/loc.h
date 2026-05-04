//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <math/dag_Point3.h>
#include <math/dag_TMatrix.h>
#include <math/dag_mathAng.h>
#include <math/dag_mathUtils.h>
#include <util/dag_globDef.h>
#include <util/dag_bitwise_cast.h>

namespace danet
{
class BitStream;
}

namespace gamephys
{
struct Orient
{
public:
  Orient()
  {
    quat.identity();
    G_STATIC_ASSERT(sizeof(Orient) == 16);
  }

  Orient(const Quat &q) : quat(q) {}

  bool operator==(const Orient &rhs) const { return quat == rhs.quat; }

  Point3 getYawPitchRoll() const
  {
    Point3 ypr;
    quat_to_euler(quat, ypr.x, ypr.y, ypr.z);
    ypr *= -RAD_TO_DEG;
    return ypr;
  }
  float getYaw() const { return getYawPitchRoll().x; }
  float getAzimuth() const { return 360.f - getYaw(); }
  float getPitch() const { return getYawPitchRoll().y; }
  float getTangage() const { return -getPitch(); }
  float getRoll() const { return getYawPitchRoll().z; }
  const Quat &getQuat() const { return quat; }
  Point4 getQuatAsP4() const { return dag::bit_cast<Point4>(quat); }

  void serialize(danet::BitStream &bs) const;
  bool deserialize(const danet::BitStream &bs);

  void setYPR(float yaw_angle, float pitch_angle, float roll_angle);
  void setYP0(float yaw, float pitch);
  void setYP0(const Point3 &dir);
  void setY00(float y) { return setYP0(y, 0); }
  void set0P0(float p) { return setYP0(0, p); }
  void set00R(float r) { return setYPR(0, 0, r); }
  void setQuat(const Quat &qi) { quat = qi; }

  void setQuatToIdentity() { quat.identity(); }

  void add(const Orient &rhs) { quat = normalize(quat * rhs.quat); }
  void increment(float azimut, float tangage, float roll_angle);
  void transform(Point3 &t) const { t = quat * t; }
  void transform(DPoint3 &t) const { t = quat * t; }
  void transform(const Point3 &in, Point3 &out) const { out = quat * in; }
  void transform(const DPoint3 &in, DPoint3 &out) const { out = quat * in; }

  void transformInv(Point3 &t) const { t = inverse(quat) * t; }
  void transformInv(DPoint3 &t) const { t = inverse(quat) * t; }
  void transformInv(const Point3 &in, Point3 &out) const { out = inverse(quat) * in; }
  void transformInv(const DPoint3 &in, DPoint3 &out) const { out = inverse(quat) * in; }

  void fromTM(const TMatrix &tm) { quat = Quat(tm); }

  void toTM(TMatrix &tm) const { tm.makeTM(quat); }
  TMatrix makeTM() const { return ::makeTM(quat); }

  void toAlternativeTangageRoll(float &out_tangage, float &out_roll) const;

protected:
  Quat quat;
};

struct Loc // Point and orientation (quaternion)
{
  alignas(8) DPoint3 P; // Explicit align for bin compat 64/32 bits
  Orient O;

public:
  Loc() : P(0.0, 0.0, 0.0) { G_STATIC_ASSERT(sizeof(Loc) == 40); }
  Loc(const Loc &loc) = default;
  Loc(Loc &&) = default;
  Loc(float x, float y, float z, float a, float t, float k) : P(x, y, z) { O.setYPR(-a, -t, k); }
  Loc &operator=(const Loc &) = default;
  Loc &operator=(Loc &&) = default;

  bool operator==(const Loc &rhs) const { return P == rhs.P && O == rhs.O; }

  void serialize(danet::BitStream &bs) const;
  bool deserialize(const danet::BitStream &bs);

  Point3 getFwd() const { return O.getQuat().getForward(); }

  void interpolate(const Loc &lhs, const Loc &rhs, float alpha)
  {
    O.setQuat(normalize(qinterp(lhs.O.getQuat(), rhs.O.getQuat(), alpha)));
    P = lerp(lhs.P, rhs.P, alpha);
  }

  void substract(const Loc &lhs, const Loc &rhs)
  {
    P = lhs.P - rhs.P;
    O.setQuat(inverse(rhs.O.getQuat()) * lhs.O.getQuat());
  }

  void add(const Loc &lhs, const Loc &rhs)
  {
    P = lhs.P + rhs.P;
    O.setQuat(lhs.O.getQuat() * rhs.O.getQuat());
  }

  void transform(Point3 &pos) const { pos = Point3::xyz(P) + O.getQuat() * pos; }

  void fromTM(const TMatrix &tm)
  {
    O.fromTM(tm);
    P.set_xyz(tm.getcol(3));
  }
  void toTM(TMatrix &tm) const
  {
    O.toTM(tm);
    tm.m[3][0] = P.x;
    tm.m[3][1] = P.y;
    tm.m[3][2] = P.z;
  }
  TMatrix makeTM() const
  {
    TMatrix tm = O.makeTM();
    tm.m[3][0] = P.x;
    tm.m[3][1] = P.y;
    tm.m[3][2] = P.z;
    return tm;
  }
  void resetLoc()
  {
    O.setQuatToIdentity();
    P.zero();
  }
};

}; // namespace gamephys
