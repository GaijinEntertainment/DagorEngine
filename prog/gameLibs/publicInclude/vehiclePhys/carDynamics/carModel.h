//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include "engineModel.h"
#include "gearboxModel.h"
#include "drivelineModel.h"
#include "differentialModel.h"
#include "wheelModel.h"
#include "espSensors.h"


class CarDynamicsModel
{
public:
  static constexpr unsigned NUM_WHEELS = 4;
  static constexpr unsigned NUM_DIFFERENTIALS = 3; // Enough for a 4WD Jeep

protected:
  // Objects that make the car a car
  CarWheelState *wheel[NUM_WHEELS];
  int numWheels;

  // Steering wheel, etc
  CarEngineModel *engine;
  CarGearBox *gearbox;
  CarDifferential *diff[NUM_DIFFERENTIALS];
  int diffCount;
  CarDriveline *driveline;

public:
  real curSpeedKmh;
  EspSensors espSens;

public:
  CarDynamicsModel(int num_wheels);
  ~CarDynamicsModel();

  CarEngineModel *getEngine() { return engine; }
  CarDriveline *getDriveline() { return driveline; }
  CarGearBox *getGearbox() { return gearbox; }
  CarWheelState *getWheelState(int n) { return wheel[n]; }
  int getNumWheels() { return numWheels; }
  CarDifferential *getDiff(int n) { return diff[n]; }
  int getDiffCount() { return diffCount; }

  void addWheelDifferential(int wheel0, int wheel1, const PhysCarDifferentialParams &params);
  // void addMasterDifferential(const DataBlock *diff_blk);

  bool load(const DataBlock *blk_eng, const PhysCarGearBoxParams &gbox_params);
  void prepareDriveline();

  void update(float dt);
  void updateAfterSimulate(float at_time);
};
