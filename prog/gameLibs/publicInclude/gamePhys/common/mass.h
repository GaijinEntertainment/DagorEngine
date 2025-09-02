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
#include <generic/dag_staticTab.h>
#include <debug/dag_assert.h>

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
};
void reset(PartMass &part_mass);
void set_mass(PartMass &part_mass, const Point3 &c, float m);
Point3 compute_moment_of_inertia(const PartMass &part_mass, const Point3 &summary_center_of_mass, float additional_mass);

struct CollisionNodeMass
{
  int massIndex;
  int nodeIndex;
};

struct FuelTankState
{
  float currentFuel;
  bool available;
};
void reset(FuelTankState &state);

struct FuelTankProps
{
  static constexpr int MAX_SYSTEMS = 16;
  static constexpr int MAX_TANKS = 16;
  static constexpr int MAIN_SYSTEM = 0;

  float capacity;
  int fuelSystemNum;
  int priority;
  bool external;
};
void reset(FuelTankProps &props);

struct EngineMass
{
  float mass;
  float propMass;
  Point3 propPos;
};

struct FuelSystemProps
{
  float maxFuel;                 // Fuel capacity
  float maxFuelExternal;         // External fuel capacity
  float fuelAccumulatorCapacity; // Fuel accumulator capacity
  int priorityNum;               // Number of priorities
  float minimalLoadFactor;       // Minimal load for feeding fuel accumulator
  float fuelAccumulatorFlowRate; // Flow rate from fuel tanks to accumulator
  float fuelEngineFlowRate;      // Flow rate from accumulator to engines
};
void reset(FuelSystemProps &props);

struct FuelSystemState
{
  float fuel;            // Current fuel load
  float fuelExternal;    // Current external fuel load
  float fuelAccumulated; // Current fuel level in accumulator
};
void reset(FuelSystemState &state);

typedef uint64_t PartsPresenceFlags;

struct MassInput
{
  float dumpFuelAmount;
  float consumeNitroAmount;
  Tab<PartMass> parts;
  float payloadCogMult;
  float payloadImMult;
  PartsPresenceFlags partsPresenceFlags;
  carray<float, FuelTankProps::MAX_SYSTEMS> consumeFuel;
  carray<float, FuelTankProps::MAX_SYSTEMS> consumeFuelUnlimited;
  carray<float, FuelTankProps::MAX_TANKS> leakFuelAmount;
  float loadFactor;
  MassInput() = default;
  MassInput(IMemAlloc *a) : parts(a) {} //-V730
};
void reset(MassInput &input);

struct MassOutput
{
  float dumpedFuelAmount;
  float leakedFuelAmount;
  float consumedFuelAmount;
  carray<float, FuelTankProps::MAX_SYSTEMS> availableFuelAmount;
  float availableNitroAmount;
};
void reset(MassOutput &output);

struct MassProps
{
  float massEmpty; // Empty mass (includes crew and oil)
  float maxWeight; // Max T/O weight (for calculations only)
  int numFuelSystems;
  bool separateFuelTanks;
  int numTanks;
  float crewMass;
  float oilMass;
  float maxNitro; // Afterburner feed capacity
  bool hasFuelDumping;
  float fuelDumpingRate;
  Point3 initialCenterOfGravity;
  Point2 centerOfGravityClampY;
  DPoint3 momentOfInertiaNorm; // Moment of inertia of empty aircraft normed to its mass
  bool advancedMass;
  Tab<PartMass> advancedMasses;
  bool doesPayloadAffectCOG;
  Tab<PartMass> dynamicMasses;
  Tab<CollisionNodeMass> collisionNodesMasses;

  carray<FuelTankProps, FuelTankProps::MAX_TANKS> fuelTankProps;
  carray<FuelSystemProps, FuelTankProps::MAX_SYSTEMS> fuelSystemProps;
};

struct MassState
{
  float mass;                    // Current mass
  float payloadMass;             // Current payload mass
  Point3 payloadMomentOfInertia; // Current moment of inertia caused by the payload masses
  float nitro;                   // Current afterburner feed load
  Point3 centerOfGravity;
  DPoint3 momentOfInertiaNormMult; // Moment of inertia multiplier, caused by damage for example
  DPoint3 momentOfInertia;         // Current total J


  carray<FuelTankState, FuelTankProps::MAX_TANKS> fuelTankStates;
  carray<FuelSystemState, FuelTankProps::MAX_SYSTEMS> fuelSystemStates;
};
void reset(MassProps &props);
void reset(MassState &state);

void set_fuel_custom(const MassProps &props, MassState &state, float amount, bool internal, bool external, int fuel_system_num,
  bool limit);

inline void set_fuel_internal(const MassProps &props, MassState &state, float amount, int fuel_system_num, bool limit)
{
  set_fuel_custom(props, state, amount, true, false, fuel_system_num, limit);
}

inline void set_fuel_external(const MassProps &props, MassState &state, float amount, int fuel_system_num, bool limit)
{
  set_fuel_custom(props, state, amount, false, true, fuel_system_num, limit);
}

void load_masses(MassProps &props, const DataBlock *parts_masses_blk, const DataBlock *surface_parts_masses_blk,
  const CollisionResource *collision, const NameMap &fuel_tank_names, const DataBlock &additional_masses);

void post_load_masses(const MassProps &props, MassState &state, const CollisionResource *collision, int num_tanks_old);

void load_save_override(MassProps &props, DataBlock *mass_blk, bool load, const CollisionResource *collision,
  const NameMap &fuel_tank_names, const DataBlock &additional_masses);

struct Mass
{
public:
  static constexpr int MAX_FUEL_SYSTEMS = FuelTankProps::MAX_SYSTEMS;
  static constexpr int MAX_FUEL_TANKS = FuelTankProps::MAX_TANKS;
  static constexpr int MAIN_FUEL_SYSTEM = FuelTankProps::MAIN_SYSTEM;

public:
  // utils
  void computeFullMass();

  float calcConsumptionLimit(int fuel_system_num, float load_factor, float dt) const;

  Mass();

  const MassOutput &getOutput() const { return output; }
  const MassProps &getProps() const { return props; };
  const MassState &getState() const { return state; };

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
  const MassOutput &simulate(const MassInput &input, float dt = 0.f);

private:
  float consumeFuelAmount(float amount, bool limited_fuel, int fuel_system_num, float load_factor, float dt);

  void computeMasses(dag::ConstSpan<PartMass> parts, float payload_cog_mult, float payload_im_mult,
    PartsPresenceFlags parts_presence_flags);

  // state set
public:
  void setFuelCustom(float amount, bool internal, bool external, int fuel_system_num, bool limit)
  {
    set_fuel_custom(props, state, amount, internal, external, fuel_system_num, limit);
  };

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

  void addExtraMass(float extra_mass);

  void setMass(float mass) { state.mass = mass; };


  void setPayloadMass(float payload_mass) { state.payloadMass = payload_mass; };

  void setPayloadMomentOfInteria(const Point3 &moment) { state.payloadMomentOfInertia = moment; };

  void setFuelSystemFuel(int fuel_system_num, float amount) { state.fuelSystemStates[fuel_system_num].fuel = amount; };

  void setFuelSystemFuelAccumulated(int fuel_system_num, float amount)
  {
    state.fuelSystemStates[fuel_system_num].fuelAccumulated = amount;
  };

  void setFuelTankAvailable(int fuel_tank_num, bool available) { state.fuelTankStates[fuel_tank_num].available = available; };

  void setFuelTankCurrentFuel(int fuel_tank_num, float amount) { state.fuelTankStates[fuel_tank_num].currentFuel = amount; };

  void setNitro(float amount) { state.nitro = amount; };

  void setCenterOfGravity(const Point3 &cog) { state.centerOfGravity = cog; };

  void setMomentOfInertiaNormMult(const DPoint3 &moi_norm_mult) { state.momentOfInertiaNormMult = moi_norm_mult; };

  void setMomentOfInertia(const DPoint3 &moi) { state.momentOfInertia = moi; };

  // props set (for lifecycle functions only)

  void setMassEmpty(float mass_empty) { props.massEmpty = mass_empty; }

  void setCrewMass(float crew_mass) { props.crewMass = crew_mass; };

  void setMomentOfInertiaNorm(const DPoint3 &moi_norm) { props.momentOfInertiaNorm = moi_norm; };

  void setAdvancedMass(bool adv_mass) { props.advancedMass = adv_mass; };

  void setDynamicMassFuelTankNum(int idx, int fuel_tank_num) { props.dynamicMasses[idx].fuelTankNum = fuel_tank_num; };

  void setDynamicMassFuelSystemNum(int idx, int fuel_sys_num) { props.dynamicMasses[idx].fuelSystemNum = fuel_sys_num; };

  void setNumTanks(int num_tanks) { props.numTanks = num_tanks; };

  void setFuelTankCapacity(int idx, float capacity) { props.fuelTankProps[idx].capacity = capacity; };

  void setFuelSystemMaxFuel(int idx, float max_fuel) { props.fuelSystemProps[idx].maxFuel = max_fuel; };

  // state get

  bool hasFuel(float amount, int fuel_system_num) const;

  inline float getFuelMassInTank(int tank_num) const
  {
    return uint8_t(tank_num) < props.numTanks ? state.fuelTankStates[tank_num].currentFuel : 0.f;
  }

  float getFuelMassMax() const;

  float getFuelMassMax(int fuel_system_num) const;

  float getFuelMassCurrent() const;

  float getFuelMassCurrent(int fuel_system_num) const;

  inline bool hasNitro() const { return state.nitro > 0.f; }

  inline float getFullMass() const { return state.mass; }

  inline const DPoint3 &getMomentOfInertia() const { return state.momentOfInertia; }

private:
  MassOutput output;
  MassProps props;
  MassState state;
};
} // namespace gamephys
