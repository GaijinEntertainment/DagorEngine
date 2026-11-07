//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <math/dag_Point3.h>
#include <math/dag_mathBase.h>

namespace HumanInput
{

// Gyro bias and gravity of a pad, fed with every motion sample in the frame of JoystickMotionState. The bias is learned
// while the pad rests: by itself after AUTO_REST_SEC when it is small, after CALIBRATE_REST_SEC of any size once
// calibrate() asks. Not thread safe.
class JoystickMotionFilter
{
public:
  static constexpr float REST_GYRO_DEG_S = 1.f; // a pad on a table stays inside, a hand shakes more
  static constexpr float REST_ACCEL_G = 0.03f;
  static constexpr float AUTO_REST_SEC = 1.5f;
  static constexpr float AUTO_MAX_BIAS_DEG_S = 2.f;
  static constexpr float CALIBRATE_REST_SEC = 3.f;

  Point3 biasDegS = Point3(0, 0, 0);
  Point3 up = Point3(0, 0, 0); // unit, zero until a sample reads more than 0.5 g
  float restSec = 0;
  bool calibrating = false;

  void calibrate()
  {
    calibrating = true;
    restSec = 0;
  }

  // one sample, dt in seconds up to about 0.1; returns the angular velocity without the bias
  Point3 update(const Point3 &gyro_deg_s, const Point3 &accel_g, float dt)
  {
    if ((gyro_deg_s - restGyro).lengthSq() > sqr(REST_GYRO_DEG_S) || (accel_g - restAccel).lengthSq() > sqr(REST_ACCEL_G))
    {
      restSec = 0;
      restCount = 0;
    }
    else
      restSec += dt;
    restCount = min(restCount + 1, REST_AVERAGE_SAMPLES);
    restGyro += (gyro_deg_s - restGyro) / float(restCount);
    restAccel += (accel_g - restAccel) / float(restCount);
    if (calibrating ? restSec >= CALIBRATE_REST_SEC : restSec >= AUTO_REST_SEC && restGyro.lengthSq() < sqr(AUTO_MAX_BIAS_DEG_S))
    {
      biasDegS = restGyro;
      calibrating = false;
    }

    const Point3 w = gyro_deg_s - biasDegS;
    up -= (w * (DEG_TO_RAD * dt)) % up;
    if (accel_g.lengthSq() > sqr(0.5f))
      up += (accel_g - up) * min(dt * UP_PULL_PER_SEC, 1.f);
    up.normalize();
    return w;
  }

  // around gravity: the player space of GyroWiki; else, and while gravity is unknown, around the pad's own Y axis
  float yaw(const Point3 &ang_vel, bool around_gravity) const
  {
    if (!around_gravity || up.lengthSq() == 0)
      return ang_vel.y;
    const float yawRoll = sqrtf(sqr(ang_vel.y) + sqr(ang_vel.z));
    return clamp((ang_vel.y * up.y + ang_vel.z * up.z) * PLAYER_SPACE_YAW_RELAX, -yawRoll, yawRoll);
  }

private:
  static constexpr int REST_AVERAGE_SAMPLES = 1024;
  static constexpr float UP_PULL_PER_SEC = 2.f;
  static constexpr float PLAYER_SPACE_YAW_RELAX = 1.41f; // the yawRelaxFactor of GyroWiki

  Point3 restGyro = Point3(0, 0, 0), restAccel = Point3(0, 0, 0);
  int restCount = 0;
};

} // namespace HumanInput
