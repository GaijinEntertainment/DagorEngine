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
  euler_to_quat(-DegToRad(yaw_angle), -DegToRad(pitch_angle), DegToRad(roll_angle), quat);
}

void Orient::setYP0(float yaw_angle, float pitch_angle)
{
  G_ASSERTF(!check_nan(yaw_angle) && !check_nan(pitch_angle), "NAN detected: yaw=%f, pitch=%f", yaw_angle, pitch_angle);
  euler_heading_attitude_to_quat(-DegToRad(yaw_angle), -DegToRad(pitch_angle), quat);
}

void Orient::setYP0(const Point3 &dir)
{
  G_ASSERTF(!check_nan(dir.x) && !check_nan(dir.y) && !check_nan(dir.z), "NAN detected: %f, %f, %f", dir.x, dir.y, dir.z);
  float y = -RadToDeg(safe_atan2(dir.z, dir.x));
  float horDist = sqrtf(sqr(dir.x) + sqr(dir.z));
  float p = RadToDeg(safe_atan2(dir.y, horDist));
  setYP0(y, p);
}

void Orient::increment(float azimut, float tangage, float roll_angle)
{
  Quat deltaQuat;
  euler_to_quat(DegToRad(azimut), DegToRad(tangage), -DegToRad(roll_angle), deltaQuat);
  quat = normalize(quat * deltaQuat);
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
