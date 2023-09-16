//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <util/dag_string.h>
#include <vehiclePhys/physCarParams.h>
#include <vehiclePhys/carDynamics/drivelineElement.h>

class DataBlock;
class CarDriveline;
class CarDynamicsModel;
class CarEngineModel;

// Driveline component base class
class CarDrivelineElement : public DrivelineElementState
{
protected:
  // Administration
  CarDrivelineElement *parent, *child[2]; // Bi-directional tree
  CarDriveline *driveline;                // Part of which driveline

public:
  CarDrivelineElement(CarDriveline *dl);
  virtual ~CarDrivelineElement();

  // Attribs
  float getInertia() { return inertia; }
  void setInertia(float i) { inertia = i; }

  float getRatio() { return ratio; }
  void setRatio(float r);

  float getInvRatio() { return invRatio; }
  float getEffInertia() { return effectiveInertiaDownStream; }
  float getFullRatio() { return cumulativeRatio; }

  CarDrivelineElement *getParent() { return parent; }
  CarDrivelineElement *getChild(int n);
  const CarDriveline *getDriveline() const { return driveline; }

  float getReactionTorque() { return tReaction; }
  float getBrakingTorque() { return tBraking; }

  float getRotVel() { return rotV; }
  void setRotVel(float v) { rotV = v; }
  void setRotAcc(float a) { rotA = a; }
  float calcRotVel(); // More expensive tree search

  // Attaching to other components
  void addChild(CarDrivelineElement *comp);
  void setParent(CarDrivelineElement *comp);

  // Reset for new use
  virtual void resetData();

  // Precalculation
  void calcEffInertia();
  void calcFullRatio();

  // Physics
  void calcReactionForces();
  virtual void calcForces();
  virtual void calcAcc();
  virtual void integrate(float dt);
};

struct CarDrivelineState
{
  // Clutch
  float clutchApplication = 0.f,   // 0..1 (how much clutch is applied)
    clutchApplicationLinear = 0.f, // Linear value of clutch application
    clutchLinearity = 0.98f,       // A linear clutch was too easy to stall
    clutchMaxTorque = 100.f,       // Clutch maximum generated torque (Nm)
    clutchCurrentTorque = 0.f;     // Clutch current torque (app*max)
  bool autoClutch = false;         // Assist takes over clutch?
  float forcedClutchAppValue = -1.f;

  // Semi-static driveline data (changes when gear is changed)
  float preClutchInertia = 0.f, // Total inertia before clutch (engine)
    postClutchInertia = 0.f,    // After clutch (gearbox, diffs)
    totalInertia = 0.f;         // Pre and post clutch inertia

  // Dynamic
  bool prepostLocked = true; // Engine is locked to rest of drivetrain?
  float tClutch = 0.f;       // Directed clutch torque (+/-clutchCurT)
};


// A complete driveline
// Includes the clutch
class CarDriveline : public CarDrivelineState
{
public:
  CarDriveline(CarDynamicsModel *car);
  ~CarDriveline();

  // Attribs
  void setEngine(CarEngineModel *comp);
  void setGearbox(CarDrivelineElement *c) { gearbox = c; }

  float getClutchTorque() { return tClutch; }

  // Precalculation
  void calcFullRatios();
  void calcEffInertia();
  void calcInertiaBeforeClutch();
  void calcInertiaAfterClutch();

  // Clutch
  float getClutchApp() { return clutchApplication; }
  float getClutchAppLinear() { return clutchApplicationLinear; }
  // float GetClutchMaxTorque()         { return clutchMaxTorque; }
  void setClutchApp(float app);
  bool isAutoClutchActive() { return autoClutch; }
  void enableAutoClutch(bool en) { autoClutch = en; }
  void forceClutchApplication(float value)
  {
    forcedClutchAppValue = -1;
    setClutchApp(value);
    forcedClutchAppValue = value;

    unlockClutch();
    setClutchApp(1.0);
  }

  float getForcedClutch() { return forcedClutchAppValue; }
  const CarDynamicsModel *getCar() const { return car; }

  // Definition
  bool load(const DataBlock *blk);
  void loadCenterDiff(const DataBlock &blk);
  void loadCenterDiff(const PhysCarCenterDiffParams &src);

  // Physics
  void resetData();
  void calcForces(float dt);
  void calcAcc();
  void integrate(float dt);

  bool isClutchLocked() { return prepostLocked; }

public:
  PhysCarCenterDiffParams centerDiff;
  float centerDiffPowerRatio;
  float centerDiffTorque;

private:
  void lockClutch() { prepostLocked = true; }
  void unlockClutch() { prepostLocked = false; }

private:
  CarEngineModel *root;         // Root of driveline tree; the engine
  CarDrivelineElement *gearbox; // The gearbox is always there
  CarDynamicsModel *car;
};
