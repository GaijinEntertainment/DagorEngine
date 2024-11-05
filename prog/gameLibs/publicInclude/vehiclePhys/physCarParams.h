//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <math/dag_Point3.h>

class DataBlock;

struct PhysCarSuspensionParams
{
  enum
  {
    FLG_POWERED = 0x1,
    FLG_STEERED = 0x2,
    FLG_RIGID_AXLE = 0x4, // Wheels are attached to rigid axle at the same angle and perpendicular to the axis
                          // This is used for rigid rear axle (most of old RWD cars)
    FLG_K_AS_W0 = 0x8,    // When set, springK defines not spring stiffness (in N/m), but suspension frequency (in rad/sec)
  };

  Point3 upperPt; //< Highest position that can be reached by wheel upper point
  Point3 springAxis;
  float fixPtOfs; //< Offset between point to which spring is attached (and resulting spring force is applied) and upperPt
                  //  This defines the spring length, and therefore real maximum suspension movement range

  float upTravel;                 //< Maximum distance upwards from stable position
  float maxTravel;                //< Total wheel move range (must be >upTravel)
  float springK;                  //< Spring stiffness [N/m]
  float springKdUp, springKdDown; //< Spring damping [N*sec/m]
  float arbK;                     //< Antirollbar stiffness [N/m]
              // the given value of the anti-rollbar stiffness per meter of the difference in the current positions of the wheel
              // springs connected by this stabilizer;
              // in general, it depends on the angles, but if we take the shoulders of the stabilizer as 30 cm, then with our
              // suspension moves we have sin a ~= a
  float wRad, wMass;
  int flags;

  bool checkFlag(int flg) const { return (flags & flg) ? true : false; }
  void setFlag(int flg, bool set) { set ? flags |= flg : flags &= ~flg; }

  bool powered() const { return checkFlag(FLG_POWERED); }
  bool controlled() const { return checkFlag(FLG_STEERED); }
  bool rigidAxle() const { return checkFlag(FLG_RIGID_AXLE); }
  bool springKAsW0() const { return checkFlag(FLG_K_AS_W0); }

  void setPowered(bool set) { setFlag(FLG_POWERED, set); }
  void setControlled(bool set) { setFlag(FLG_STEERED, set); }
  void setRigidAxle(bool set) { setFlag(FLG_RIGID_AXLE, set); }
  void setSpringKAsW0(bool set) { setFlag(FLG_K_AS_W0, set); }

  void userSave(DataBlock &b) const;
  void userLoad(const DataBlock &b);
};

class PhysCarGearBoxParams
{
public:
  static constexpr int NUM_GEARS_MAX = 16;

  PhysCarGearBoxParams();
  void load(const DataBlock *blk);

  float getRatio(int gear);
  float getInertia(int gear);

  // for user adjustments
  void setRatioValue(int gear, float value) { ratio[gear] = value; }
  float getRatioValue(int gear) { return ratio[gear]; }

  void userSave(DataBlock &b) const;
  void userLoad(const DataBlock &b);

  int numGears;
  float ratioMul;

  int timeToDeclutch, timeToClutch;
  int minTimeToSwitch;

  // Automatic transmissions
  float maxTargetRPM;

private:
  float ratio[NUM_GEARS_MAX];
  float inertia[NUM_GEARS_MAX];
};

class PhysCarDifferentialParams
{
public:
  PhysCarDifferentialParams();

  enum DiffType
  {
    FREE = 0,  // Free/open differential
    VISCOUS,   // Viscous locking differential
    SALISBURY, // Grand Prix Legends type clutch locking
    ELSD,      // electronically controlled LSD
    NUM_TYPES
  };

  static const char *const diff_type_names[NUM_TYPES];

  int type;
  float powerRatio, coastRatio;

  // Coefficient for viscous diff
  float lockingCoeff;

  // Salisbury diff
  float preload;

  // eLSD
  float yawAngleWt, steerAngleWt, yawRateWt;

  const char *getTypeName() { return diff_type_names[type]; }
  void load(const DataBlock &blk);
  void save(DataBlock &blk) const;
};

class PhysCarCenterDiffParams
{
public:
  PhysCarCenterDiffParams();

  enum DiffType
  {
    FREE = 0,
    VISCOUS,
    ELSD,
    NUM_TYPES
  };
  static const char *const diff_type_names[NUM_TYPES];

  float fixedRatio;
  float viscousK;
  float powerRatio, coastRatio;
  int type;

  void load(const DataBlock &blk);
  void save(DataBlock &blk) const;
};

class PhysCarParams
{
public:
  float frontBrakesTorque;
  float rearBrakesTorque;
  float handBrakesTorque;
  float handBrakesTorque2;
  float brakeForceAdjustK;
  float minWheelSelfBrakeTorque;
  float ackermanAngle, toeIn;

  float longForcePtInOfs, longForcePtDownOfs;
  float maxWheelAng;

  PhysCarSuspensionParams frontSusp, rearSusp;

  void setDefaultValues(const PhysCarSuspensionParams &front_susp, const PhysCarSuspensionParams &rear_susp);

  void load(const DataBlock *b, const DataBlock &cars_blk);

  void userSave(DataBlock &b) const;
  void userLoad(const DataBlock &b);

private:
  void loadSuspensionParams(PhysCarSuspensionParams &s, const DataBlock &b);
};
