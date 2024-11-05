// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <gamePhys/common/loc.h>
#include <math/dag_Quat.h>
#include <math/dag_mathUtils.h>
#include <daNet/bitStream.h>

using namespace gamephys;

void Orient::setYPR(float yaw_angle, float pitch_angle, float roll_angle)
{
  G_ASSERTF(!check_nan(yaw_angle) && !check_nan(pitch_angle) && !check_nan(roll_angle), "NAN detected: %f, %f, %f", yaw_angle,
    pitch_angle, roll_angle);
  setYawPitchRoll(yaw_angle, pitch_angle, roll_angle);
  euler_to_quat(-DegToRad(yaw), -DegToRad(pitch), DegToRad(roll), quat);
}

void Orient::setY00(float yaw_angle)
{
  G_ASSERTF(!check_nan(yaw_angle), "NAN detected: yaw=%f", yaw_angle);
  setYawPitchRoll(yaw_angle, 0.f, 0.f);
  euler_heading_to_quat(-DegToRad(yaw), quat);
}

void Orient::set0P0(float pitch_angle)
{
  G_ASSERTF(!check_nan(pitch_angle), "NAN detected: pitch=%f", pitch_angle);
  setYawPitchRoll(0.f, pitch_angle, 0.f);
  euler_attitude_to_quat(-DegToRad(pitch), quat);
}

void Orient::set00R(float roll_angle)
{
  G_ASSERTF(!check_nan(roll_angle), "NAN detected: roll=%f", roll_angle);
  setYawPitchRoll(0.f, 0.f, roll_angle);
  euler_bank_to_quat(DegToRad(roll), quat);
}

void Orient::setYP0(float yaw_angle, float pitch_angle)
{
  G_ASSERTF(!check_nan(yaw_angle) && !check_nan(pitch_angle), "NAN detected: yaw=%f, pitch=%f", yaw_angle, pitch_angle);
  setYawPitchRoll(yaw_angle, pitch_angle, 0.f);
  euler_heading_attitude_to_quat(-DegToRad(yaw), -DegToRad(pitch), quat);
}

void Orient::setYP0(const Point3 &dir)
{
  G_ASSERTF(!check_nan(dir.x) && !check_nan(dir.y) && !check_nan(dir.z), "NAN detected: %f, %f, %f", dir.x, dir.y, dir.z);
  float y = -RadToDeg(safe_atan2(dir.z, dir.x));
  float horDist = sqrtf(sqr(dir.x) + sqr(dir.z));
  float p = RadToDeg(safe_atan2(dir.y, horDist));
  setYP0(y, p);
}

void Orient::wrap()
{
  float azimut, tangage, bank; // No need to init these locals since quat_to_euler() always overwrites it
  quat_to_euler(quat, azimut, tangage, bank);
  setYawPitchRoll(-RadToDeg(azimut), -RadToDeg(tangage), -RadToDeg(bank));
}

void Orient::setQuat(const Quat &quat_in)
{
  quat = quat_in;
  wrap();
}

void Orient::add(const Orient &rhs)
{
  quat = normalize(quat * rhs.quat);
  wrap();
}

void Orient::increment(float azimut, float tangage, float roll_angle)
{
  Quat deltaQuat;
  euler_to_quat(DegToRad(azimut), DegToRad(tangage), -DegToRad(roll_angle), deltaQuat);
  quat = normalize(quat * deltaQuat);
  wrap();
}

void Orient::serialize(danet::BitStream &bs) const
{
  for (int i = 0; i < 4; ++i) // dear compiler, please unroll this
    bs.Write(quat[i]);
}

bool Orient::deserialize(const danet::BitStream &bs)
{
  Quat quatRead;
  for (int i = 0; i < 4; ++i)
    if (!bs.Read(quatRead[i]))
      return false;
  setQuat(quatRead);
  return true;
}

// Alternative tangage (-270..90) and roll (-90..90)
void Orient::toAlternativeTangageRoll(float &out_tangage, float &out_roll) const
{
  TMatrix tm;
  tm.makeTM(quat);

  Point3 localForward = tm.getcol(0);
  Point3 localLeft = tm.getcol(2);
  float upY = tm.getcol(1).y;

  out_roll = RadToDeg(unsafe_atan2(localLeft.y, sqrtf(sqr(localLeft.x) + sqr(localLeft.z))));
  out_tangage = RadToDeg(unsafe_atan2(localForward.y, sqrtf(sqr(localForward.x) + sqr(localForward.z))));
  if (upY < 0.f)
    out_tangage = -180.f - out_tangage;
}

Loc::Loc(const SimpleLoc &loc) : P(loc.P) { O.setQuat(loc.O.quat); }

void Loc::interpolate(const Loc &lhs, const Loc &rhs, const float alpha)
{
  O.setQuat(normalize(qinterp(lhs.O.getQuat(), rhs.O.getQuat(), alpha)));
  P = lerp(lhs.P, rhs.P, alpha);
}

void SimpleLoc::interpolate(const SimpleLoc &lhs, const SimpleLoc &rhs, const float alpha)
{
  O = normalize(qinterp(lhs.O.quat, rhs.O.quat, alpha));
  P = lerp(lhs.P, rhs.P, alpha);
}

Point3 SimpleLoc::LocalOrient::getYPR() const
{
  float azimut = 0.f;
  float tangage = 0.f;
  float bank = 0.f;
  quat_to_euler(quat, azimut, tangage, bank);
  return Point3(-RadToDeg(azimut), -RadToDeg(tangage), -RadToDeg(bank));
}

void Loc::substract(const Loc &lhs, const Loc &rhs)
{
  P = lhs.P - rhs.P;
  O.setQuat(inverse(rhs.O.getQuat()) * lhs.O.getQuat());
}

void Loc::add(const Loc &lhs, const Loc &rhs)
{
  P = lhs.P + rhs.P;
  O.setQuat(lhs.O.getQuat() * rhs.O.getQuat());
}

void Loc::serialize(danet::BitStream &bs) const
{
  bs.Write(P.x);
  bs.Write(P.y);
  bs.Write(P.z);
  O.serialize(bs);
}

bool Loc::deserialize(const danet::BitStream &bs)
{
#if _TARGET_ANDROID //== to workaround ICE
  double x, y, z;
  if (bs.Read(x) && bs.Read(y) && bs.Read(z))
  {
    P.set(x, y, z);
    return O.deserialize(bs);
  }
  return false;
#else
  return bs.Read(P.x) && bs.Read(P.y) && bs.Read(P.z) && O.deserialize(bs);
#endif
}
