//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include "drivelineModel.h"

class PhysCarGearBoxParams;

struct CarGearBoxState
{
  // State (dynamic output)
  int curGear = 0;          // Selected gear
  float autoShiftStart = 0; // Time of auto shift initiation
  float timeToShift = 0;    // Freeshifting; time before gear set
  int futureGear = 0;       // Gear to shift to
  bool shiftUpEnabled = true;
};

class CarGearBox : public CarGearBoxState, public CarDrivelineElement
{
public:
  enum Flags
  {
    AUTOMATIC = 1,
    FORCE_REVERSE = 2
  };
  enum GearNames
  {
    GEAR_NEUTRAL,
    GEAR_REVERSE,
    GEAR_1,
  };

public:
  CarGearBox(CarDynamicsModel *car);
  ~CarGearBox();

  void setParams(const PhysCarGearBoxParams &gbox_params);
  void getParams(PhysCarGearBoxParams &gbox_params);

  void resetData();

  int getNumGears();
  int getCurGear() { return curGear; }
  int getFutureGear() { return futureGear; }
  void setGear(int n);
  void requestGear(int n, bool ignore_cur_shift_timeout = false); // we should deprecate this function, as we cannot do a request of
                                                                  // gear without concrete time
  void requestGear(int n, float at_time, bool ignore_cur_shift_timeout = false);

  bool isNeutral() { return (curGear == GEAR_NEUTRAL); }
  bool isAutomatic() { return flags & AUTOMATIC; }
  void setAutomatic(bool on);
  void enableShiftUp(bool en) { shiftUpEnabled = en; }

  // Physics
  void integrate(float dt);
  void updateAfterSimulate(float at_time);

private:
  void performGearSwitch(float at_time);

private:
  CarDynamicsModel *car; // The car to which we belong

  PhysCarGearBoxParams *params;

  // Physical attributes
  int flags;
};
