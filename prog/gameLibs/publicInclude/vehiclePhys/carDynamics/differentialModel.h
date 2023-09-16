//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include "drivelineModel.h"

class CarWheelState;
class PhysCarDifferentialParams;

// A diff that has 3 parts; 1 'input', and 2 'outputs'.
// Actually, all parts work together in deciding what happens.
class CarDifferential : public CarDrivelineElement
{
public:
  // Output members
  CarWheelState *wheel[2];

public:
  CarDifferential(CarDriveline *dl);
  ~CarDifferential();

  void resetData();
  void setParams(const PhysCarDifferentialParams &p);

  float getAccIn() const { return accIn; }
  float getAccOut(int n) const { return accOut[n]; }

  float calcLockTorque(float torqueOut[], float mIn, float mt);
  void calc1DiffForces(float dt, float torqueIn, float inertiaIn);
  void integrate(float dt);
  void lock(int diff_side);

protected:
  PhysCarDifferentialParams *params;

  // State
  float velASymmetric; // Difference in wheel rotation speed

  // Resulting accelerations
  float accIn, accASymmetric;
  float accOut[2];
  int locked = 0;
};
