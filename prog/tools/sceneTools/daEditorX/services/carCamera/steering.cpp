// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "steering.h"

#include <ioSys/dag_dataBlock.h>
#include <util/dag_globDef.h>
#include <startup/dag_globalSettings.h>
// #include "../game/GameSettings.h"
// #include "../utils/mathUtils.h"

#include <math/dag_math3d.h>
#include <math/dag_mathBase.h>


//===============================================
//
//===============================================

SteeringHelper::SteeringHelper(const DataBlock *blk)
{
  tLow = blk->getReal("tLow", 0.1);
  tHigh = blk->getReal("tHigh", 0.15);
  carVel = blk->getPoint2("carVel", Point2(40, 100)) / 3.6f;
  speedLow = blk->getPoint2("speedLow", Point2(80, 40)) * DEG_TO_RAD;
  speedHigh = blk->getPoint2("speedHigh", Point2(200, 100)) * DEG_TO_RAD;
  returnSpeed = blk->getReal("returnSpeed", 260) * DEG_TO_RAD;

  reset();
}

void SteeringHelper::reset()
{
  press = 0;
  tPress = 0.0f;
}

float SteeringHelper::getSpeed(float t, float carVelR)
{
  real speedLowR, speedHighR;

  if (carVelR <= carVel[0])
  {
    speedLowR = speedLow[0];
    speedHighR = speedHigh[0];
  }
  else if (carVelR >= carVel[1])
  {
    speedLowR = speedLow[1];
    speedHighR = speedHigh[1];
  }
  else
  {
    real t = (carVelR - carVel[0]) / (carVel[1] - carVel[0]);
    speedLowR = speedLow[0] + t * (speedLow[1] - speedLow[0]);
    speedHighR = speedHigh[0] + t * (speedHigh[1] - speedHigh[0]);
  }
  speedLowR *= 0.5f;
  speedHighR *= 0.5f;

  float speed = 0.0;
  if (t < tLow)
    speed = speedLowR;
  else if (t > tHigh)
    speed = speedHighR;
  else
    speed = speedLowR + (speedHighR - speedLowR) * (t - tLow) / (tHigh - tLow);

  return (press > 0) ? (speed) : ((-speed));
}

float SteeringHelper::update(float curAngle, const Point3 &vel, bool kl, bool kr, float dt)
{
  int newPress = (kl == kr) ? 0 : (kr ? 1 : -1);
  if (newPress && ((newPress == press) || (press == 0)))
    tPress += dt;
  else
    tPress = 0.0f;
  press = newPress;

  if (press)
  {
    real speed = getSpeed(tPress, vel.length());
    if ((curAngle > returnSpeed * dt) && (speed < 0))
      curAngle += (speed - returnSpeed) * dt;
    else if ((curAngle < -returnSpeed * dt) && (speed > 0))
      curAngle += (speed + returnSpeed) * dt;
    else
      curAngle += speed * dt;
  }
  else
  {
    if (fabsf(curAngle) < 2.0f * DEG_TO_RAD)
      curAngle = 0.0f;
    else
    {
      float newAngle = (curAngle > 0) ? (curAngle - returnSpeed * dt) : (curAngle + returnSpeed * dt);
      if (newAngle * curAngle < 0)
        curAngle = 0.0f;
      else
        curAngle = newAngle;
    }
  }
  return curAngle;
}

float SteeringHelper::getActualMaxAngle(float maxAng, const Point3 &vel) { return maxAng; }

HandBrakeHelper::HandBrakeHelper(const DataBlock *blk)
{
  tLow = blk->getReal("tLow", 0.1);
  tHigh = blk->getReal("tHigh", 0.15);
  timer = 0;
  value = -1.0f;
}

float HandBrakeHelper::update(float dt, bool set)
{
  if (set)
  {
    if (timer <= tLow)
      value = 0.0;
    else if (timer >= tHigh)
      value = 1.0;
    else
      value = (timer - tLow) / (tHigh - tLow);

    timer += dt;
  }
  else
  {
    timer = 0;
    value = -1.0f;
  }
  return value;
}
