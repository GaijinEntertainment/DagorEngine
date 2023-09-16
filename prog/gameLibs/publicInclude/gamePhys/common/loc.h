//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <math/dag_Point3.h>
#include <math/dag_TMatrix.h>
#include <math/dag_mathAng.h>
#include <math/dag_mathUtils.h>
#include <util/dag_globDef.h>

namespace danet
{
class BitStream;
}

namespace gamephys
{
struct Orient
{
public:
  Orient() : yaw(0.f), pitch(0.f), roll(0.f)
  {
    quat.identity();
    G_STATIC_ASSERT(sizeof(Orient) == 28);
  }

  bool operator==(const Orient &rhs) const { return quat == rhs.quat; }

  float getAzimuth() const { return 360.f - yaw; }
  float getPitch() const { return pitch; }
  float getTangage() const { return -pitch; }
  float getRoll() const { return roll; }
  float getYaw() const { return yaw; }
  const Point3 &getYPR() const { return ypr; }
  const Quat &getQuat() const { return quat; }

  void serialize(danet::BitStream &bs) const;
  bool deserialize(const danet::BitStream &bs);

  void setYPR(float yaw_angle, float pitch_angle, float roll_angle);
  void setY00(float yaw_angle);
  void set0P0(float pitch_angle);
  void set00R(float roll_angle);
  void setYP0(float yaw, float pitch);
  void setYP0(const Point3 &dir);
  void setQuat(const Quat &quat_in);

  void setQuatToIdentity()
  {
    quat.identity();
    yaw = pitch = roll = 0;
  }

  void wrap();

  void add(const Orient &rhs);
  void increment(float azimut, float tangage, float roll_angle);
  void transform(Point3 &t) const { t = quat * t; }
  void transform(DPoint3 &t) const { t = quat * t; }
  void transform(const Point3 &in, Point3 &out) const { out = quat * in; }
  void transform(const DPoint3 &in, DPoint3 &out) const { out = quat * in; }

  void transformInv(Point3 &t) const { t = inverse(quat) * t; }
  void transformInv(DPoint3 &t) const { t = inverse(quat) * t; }
  void transformInv(const Point3 &in, Point3 &out) const { out = inverse(quat) * in; }
  void transformInv(const DPoint3 &in, DPoint3 &out) const { out = inverse(quat) * in; }

  void fromTM(const TMatrix &tm)
  {
    quat = Quat(tm);
    wrap();
  }

  void toTM(TMatrix &tm) const { tm.makeTM(quat); }
  TMatrix makeTM() const { return ::makeTM(quat); }

  void toAlternativeTangageRoll(float &out_tangage, float &out_roll) const;

protected:
  void setYawPitchRoll(float yaw_angle, float pitch_angle, float roll_angle)
  {
    yaw = yaw_angle;
    pitch = pitch_angle;
    roll = roll_angle;
  }

  Quat quat;
  union
  {
    struct
    {
      float yaw;
      float pitch;
      float roll;
    };
    Point3 ypr;
  };
};

struct SimpleLoc;

struct Loc // point and orientation (quaternion with angles)
{
  DPoint3 P;
  Orient O;

public:
  Loc() : P(0.0, 0.0, 0.0) { G_STATIC_ASSERT(sizeof(Loc) == 56); }
  Loc(const Loc &loc) = default;
  Loc(const SimpleLoc &loc);
  Loc(float x, float y, float z, float a, float t, float k) : P(x, y, z) { O.setYPR(-a, -t, k); }

  bool operator==(const Loc &rhs) const { return P == rhs.P && O == rhs.O; }

  void serialize(danet::BitStream &bs) const;
  bool deserialize(const danet::BitStream &bs);

  Point3 getFwd() const { return O.getQuat().getForward(); }

  void interpolate(const Loc &lhs, const Loc &rhs, const float alpha);
  void substract(const Loc &lhs, const Loc &rhs);
  void add(const Loc &lhs, const Loc &rhs);

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

struct SimpleLoc // point and quaternion without angles
{
  struct LocalOrient
  {
    LocalOrient(const Quat &q) : quat(q) {}

    LocalOrient(const LocalOrient &loc) = default;

    LocalOrient(const Orient &orient) : quat(orient.getQuat()) {}

    LocalOrient()
    {
      quat.identity();
      G_STATIC_ASSERT(sizeof(LocalOrient) == 16);
    }

    void fromTM(const TMatrix &tm) { quat = Quat(tm); }

    void toTM(TMatrix &tm, bool = false) const { tm.makeTM(quat); }

    TMatrix makeTM() const { return ::makeTM(quat); }

    Quat getQuat() const { return quat; }

    void setQuat(const Quat &q) { quat = q; }

    Point3 getYPR() const;

    Quat quat;
  };

  DPoint3 P;
  LocalOrient O;

public:
  SimpleLoc() : P(0, 0, 0) { G_STATIC_ASSERT(sizeof(SimpleLoc) == 40); }
  SimpleLoc(const SimpleLoc &loc) = default;
  SimpleLoc(const Loc &loc) : P(loc.P), O(loc.O.getQuat()) {}

  Point3 getFwd() const { return O.quat.getForward(); }

  void interpolate(const SimpleLoc &lhs, const SimpleLoc &rhs, const float alpha);

  void transform(Point3 &pos) const { pos = Point3::xyz(P) + O.quat * pos; }

  void fromTM(const TMatrix &tm)
  {
    O.quat = Quat(tm);
    P.set_xyz(tm.getcol(3));
  }
  void toTM(TMatrix &tm) const
  {
    tm.makeTM(O.quat);
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
    O.quat.identity();
    P.zero();
  }
};
}; // namespace gamephys
