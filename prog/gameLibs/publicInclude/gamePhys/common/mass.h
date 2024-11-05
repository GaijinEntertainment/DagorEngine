//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <math/dag_Point3.h>
#include <math/dag_Point2.h>
#include <generic/dag_span.h>
#include <generic/dag_tab.h>
#include <generic/dag_carray.h>
#include <util/dag_oaHashNameMap.h>

class DataBlock;
class CollisionResource;

namespace gamephys
{

struct PartMass
{
  static constexpr int INVALID_TANK_NUM = 0xff;
  float mass;
  float eqArea;
  Point3 momentOfInertia;
  Point3 center;
  uint8_t fuelSystemNum;
  uint8_t fuelTankNum;
  bool volumetric;

  PartMass();
  void clear();
  void setMass(const Point3 &c, float m);
  Point3 computeMomentOfInertia(const Point3 &summary_center_of_mass, float additional_mass) const;
};

struct FuelTank
{
  static constexpr const int MAX_SYSTEMS = 16;
  static constexpr const int MAX_TANKS = 16;
  static constexpr const int MAIN_SYSTEM = 0;

  float capacity;
  float currentFuel;
  int fuelSystemNum;
  int priority;
  bool external;
  bool available;

  FuelTank();
  void clear();
};

struct EngineMass
{
  float mass;
  float propMass;
  Point3 propPos;
};

typedef uint64_t PartsPresenceFlags;

struct Mass
{
public:
  static constexpr const int MAX_FUEL_SYSTEMS = FuelTank::MAX_SYSTEMS;
  static constexpr const int MAX_FUEL_TANKS = FuelTank::MAX_TANKS;
  static constexpr const int MAIN_FUEL_SYSTEM = FuelTank::MAIN_SYSTEM;

  float massEmpty;               // empty Mass (Includes Crew and Oil)
  float mass;                    // current Mass
  float maxWeight;               // max T/O Weight (for Calculations Only)
  float payloadMass;             // current payload mass
  Point3 payloadMomentOfInertia; // current moment of inertia inflicted by the payload masses

  struct FuelSystem
  {
    float fuel;                    // current Fuel Load
    float fuelExternal;            // current Fuel Load External
    float maxFuel;                 // fuel Capacity
    float maxFuelExternal;         // fuel Capacity External
    float fuelAccumulatorCapacity; // capacity of Fuel Accumulator
    float fuelAccumulated;         // current Level of Fuel in Accumulator
    int priorityNum;               // number of priorities
    float minimalLoadFactor;       // minimal load factor to feed the fuel accumulator
    float fuelAccumulatorFlowRate; // flow rate from fuel tanks to fuel accumulator
    float fuelEngineFlowRate;      // flow rate from fuel accumulator to engines

    FuelSystem();
    void clear();
  };
  // fuel Systems
  carray<FuelSystem, MAX_FUEL_SYSTEMS> fuelSystems;
  int numFuelSystems;
  // all Fuel  Tanks
  bool separateFuelTanks;
  carray<FuelTank, MAX_FUEL_TANKS> fuelTanks;
  int numTanks;

  float crewMass;
  float oilMass;

  float nitro;    // current afterburner Feed Load
  float maxNitro; // afterburner Feed Capacity

  bool hasFuelDumping;
  float fuelDumpingRate;

  Point3 centerOfGravity;
  Point3 initialCenterOfGravity;
  Point2 centerOfGravityClampY;

  DPoint3 momentOfInertiaNorm;     // moment of inertia of empty aircraft normed to its mass
  DPoint3 momentOfInertiaNormMult; // multiplier to moment of inertia (due the damage for example)
  DPoint3 momentOfInertia;         // current total J

  bool advancedMass;
  Tab<PartMass> advancedMasses;
  bool doesPayloadAffectCOG;
  Tab<PartMass> dynamicMasses;

  struct CollisionNodeMass
  {
    int massIndex;
    int nodeIndex;
  };
  Tab<CollisionNodeMass> collisionNodesMasses;

public:
  // utils

  float calcConsumptionLimit(int fuel_system_num, float load_factor, float dt) const;

  void computeFullMass();

  Mass();

  // life cycle

  // initialization

  void init();

  void loadMasses(const DataBlock *parts_masses_blk, const DataBlock *surface_parts_masses_blk, const CollisionResource *collision,
    const NameMap &fuel_tank_names, const DataBlock &additional_masses);

  void baseLoad(const DataBlock *mass_blk, int crew_num);

  void load(const DataBlock &settings_blk, const CollisionResource *collision, const NameMap &fuel_tank_names,
    const DataBlock &additional_masses);

  void load(const DataBlock *mass_blk, const int crewNum, const CollisionResource *collision, const NameMap &fuel_tank_names,
    const DataBlock &additional_masses);

  void loadSaveOverride(DataBlock *mass_blk, bool load, const CollisionResource *collision, const NameMap &fuel_tank_names,
    const DataBlock &additional_masses);

  void repair();

  // simulate

  bool consumeFuel(float amount, bool limited_fuel, int fuel_system_num, float load_factor, float dt);

  float consumeFuelAmount(float amount, bool limited_fuel, int fuel_system_num, float load_factor, float dt);

  float leakFuel(float amount, int tank_num);

  float dumpFuel(float amount);

  bool consumeNitro(float amount);

  void computeMasses(dag::ConstSpan<PartMass> parts, float payload_cog_mult, float payload_im_mult,
    PartsPresenceFlags parts_presence_flags);

  // state set

  void setFuelCustom(float amount, bool internal, bool external, int fuel_system_num, bool limit);

  void setFuel(float amount, int fuel_system_num = MAIN_FUEL_SYSTEM, bool limit = true);

  inline void setFuelInternal(float amount, int fuel_system_num, bool limit)
  {
    setFuelCustom(amount, true, false, fuel_system_num, limit);
  }

  inline void setFuelExternal(float amount, int fuel_system_num, bool limit)
  {
    setFuelCustom(amount, false, true, fuel_system_num, limit);
  }

  void clearFuel();

  // state get

  bool hasFuel(float amount, int fuel_system_num) const;

  inline float getFuelMassInTank(int tank_num) const { return uint8_t(tank_num) < numTanks ? fuelTanks[tank_num].currentFuel : 0.f; }

  float getFuelMassMax() const;

  float getFuelMassMax(int fuel_system_num) const;

  float getFuelMassCurrent() const;

  float getFuelMassCurrent(int fuel_system_num) const;

  inline bool hasNitro() const { return nitro > 0.f; }

  inline float getFullMass() const { return mass; }

  inline const DPoint3 &getMomentOfInertia() const { return momentOfInertia; }
};

} // namespace gamephys
