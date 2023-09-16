#include <gamePhys/common/kinematics.h>

#include <math/dag_Quat.h>
#include <math/dag_mathUtils.h>

static const float minVel = 1.0e-3f;

static const float minAccel = 1.0e-3f;

// Extrapolating position and velocity in circular motion
void gamephys::extrapolate_circular(const Point3 &pos, // current position abs
  const Point3 &vel,                                   // current velocity abs
  const Point3 &acc,                                   // current acceleration abs,
                     // but it is assumed that the acceleration is constant in aircraft veloicty frame reference
  float time,      // extrapolation time
  Point3 &out_pos, // position at current time + time
  Point3 &out_vel) // velocity at current time + time
{
  const float velLen = vel.length();
  if (velLen < minVel)
  {
    out_pos = pos + acc * sqr(time) * 0.5f;
    out_vel = acc * time;
    return;
  }
  const float accLen = acc.length();
  if (accLen < minAccel)
  {
    out_pos = pos + vel * time;
    out_vel = vel;
    return;
  }
  const Point3 velNorm = vel / velLen;
  const Point3 accNorm = acc / accLen;

  const Point3 axis = velNorm % accNorm;
  Point3 accTurnNorm = axis % velNorm;
  const float accTurnLen = accTurnNorm * acc;
  if (rabs(accTurnLen) > minAccel)
  {
    const float accLnLen = acc * velNorm;
    const float angle = rabs(accLnLen) > minAccel
                          ? accTurnLen / accLnLen * (log(max(rabs(velLen + accLnLen * time), minVel)) - log(velLen))
                          : accTurnLen / velLen * time;
    const Quat quat(axis, angle);
    out_vel = (quat * vel) * (velLen + accLnLen * time) / velLen;

    const float turnRadius = sqr(velLen + accLnLen * time) / accTurnLen;
    if (turnRadius < 1.0e4f)
    {
      const Point3 dirFromTurnCenter = -accTurnNorm * turnRadius;
      const Point3 turnCenter = pos - dirFromTurnCenter;
      out_pos = turnCenter + quat * dirFromTurnCenter;
    }
    else
      out_pos = pos + vel * time + acc * sqr(time) * 0.5f;
  }
  else
  {
    out_vel = vel + acc * time;
    out_pos = pos + vel * time + acc * sqr(time) / 2;
  }
}