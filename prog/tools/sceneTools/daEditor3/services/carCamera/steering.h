#pragma once

#include <math/dag_Point2.h>

class DataBlock;


class SteeringHelper
{
public:
  SteeringHelper(const DataBlock *blk);
  void reset();

  float update(float cur_wheel_ang, const Point3 &vel, bool kl, bool kr, float dt);
  float getActualMaxAngle(float maxAng, const Point3 &vel);

private:
  float tLow, tHigh;
  float returnSpeed;
  Point2 carVel;
  Point2 speedLow, speedHigh;

  float tPress;
  int press;
  float getSpeed(float t, float carVel);
};

class HandBrakeHelper
{
public:
  HandBrakeHelper(const DataBlock *blk);

  float update(float dt, bool set);

private:
  float tLow, tHigh;
  float timer;
  float value;
};
