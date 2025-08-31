// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <gamePhys/common/mass.h>

#include <ioSys/dag_dataBlock.h>
#include <ioSys/dag_dataBlockUtils.h>
#include <math/dag_mathUtils.h>
#include <generic/dag_sort.h>
#include <generic/dag_staticTab.h>
#include <gameRes/dag_collisionResource.h>
#include <memory/dag_framemem.h>
#include <util/dag_string.h>
#include <EASTL/vector_set.h>
#include <cstring>
#include <generic/dag_carray.h>
#include <EASTL/algorithm.h>
#include <perfMon/dag_statDrv.h>

using namespace gamephys;


void gamephys::reset(PartMass &part_mass)
{
  part_mass.mass = 0.f;
  part_mass.eqArea = 0.f;
  part_mass.momentOfInertia.zero();
  part_mass.center.zero();
  part_mass.fuelTankNum = PartMass::INVALID_TANK_NUM;
  part_mass.volumetric = false;
  part_mass.fuelSystemNum = 0;
}

void gamephys::set_mass(PartMass &part_mass, const Point3 &c, float m)
{
  reset(part_mass);
  part_mass.center = c;
  part_mass.mass = m;
}

Point3 gamephys::compute_moment_of_inertia(const PartMass &part_mass, const Point3 &summary_center_of_mass, float additional_mass)
{
  Point3 distToNewCenter = part_mass.center - summary_center_of_mass;
  return Point3(part_mass.momentOfInertia.x + lengthSq(Point2::yz(distToNewCenter)),
           part_mass.momentOfInertia.y + lengthSq(Point2::xz(distToNewCenter)),
           part_mass.momentOfInertia.z + lengthSq(Point2::xy(distToNewCenter))) *
         (part_mass.mass + additional_mass);
}

void gamephys::reset(FuelTankState &state)
{
  state.currentFuel = 0.f;
  state.available = true;
}

void gamephys::reset(FuelTankProps &props)
{
  props.capacity = 0.f;
  props.fuelSystemNum = 0;
  props.priority = 0;
  props.external = false;
}

void gamephys::reset(FuelSystemProps &props)
{
  props.maxFuel = 0.0f;
  props.maxFuelExternal = 0.0f;
  props.fuelAccumulatorCapacity = 0.0f;
  props.priorityNum = 1;
  props.minimalLoadFactor = -VERY_BIG_NUMBER;
  props.fuelAccumulatorFlowRate = 1.0e6f;
  props.fuelEngineFlowRate = 1.0e6f;
}

void gamephys::reset(FuelSystemState &state)
{
  state.fuel = 0.0f;
  state.fuelExternal = 0.0f;
  state.fuelAccumulated = 0.0f;
}

void gamephys::reset(MassInput &input)
{
  input.dumpFuelAmount = 0.f;
  input.consumeNitroAmount = 0.f;
  input.parts.clear();
  input.payloadCogMult = 1.0;
  input.payloadImMult = 1.0;
  input.partsPresenceFlags = 0ll;
  eastl::fill(input.consumeFuel.begin(), input.consumeFuel.end(), 0.f);
  eastl::fill(input.consumeFuelUnlimited.begin(), input.consumeFuelUnlimited.end(), 0.f);
  eastl::fill(input.leakFuelAmount.begin(), input.leakFuelAmount.end(), 0.f);
  input.loadFactor = 0.f;
}

void gamephys::reset(MassOutput &output)
{
  output.dumpedFuelAmount = 0.f;
  output.leakedFuelAmount = 0.f;
  output.consumedFuelAmount = 0.f;
  eastl::fill(output.availableFuelAmount.begin(), output.availableFuelAmount.end(), 0.f);
  output.availableNitroAmount = 0.f;
}

void gamephys::reset(MassProps &props)
{
  props.maxWeight = 0.f;
  props.massEmpty = 0.f;
  props.numFuelSystems = 1;
  props.separateFuelTanks = false;
  props.numTanks = 0;
  props.crewMass = 0.f;
  props.oilMass = 0.f;
  props.maxNitro = 1.f;
  props.hasFuelDumping = false;
  props.fuelDumpingRate = 0.f;
  props.initialCenterOfGravity.zero();
  props.centerOfGravityClampY.set(-VERY_BIG_NUMBER, VERY_BIG_NUMBER);
  props.momentOfInertiaNorm.zero();
  props.advancedMass = false;
  props.advancedMasses = Tab<PartMass>(midmem);
  props.doesPayloadAffectCOG = false;

  eastl::for_each(props.fuelSystemProps.begin(), props.fuelSystemProps.end(), [](auto &props) { reset(props); });
  eastl::for_each(props.fuelTankProps.begin(), props.fuelTankProps.end(), [](auto &props) { reset(props); });
}

void gamephys::reset(MassState &state)
{
  state.mass = 0.f;
  state.payloadMass = 0.f;
  state.payloadMomentOfInertia.zero();
  state.nitro = 0.f;
  state.centerOfGravity.zero();
  state.momentOfInertiaNormMult.set(1.0, 1.0, 1.0);
  state.momentOfInertia.zero();

  eastl::for_each(state.fuelSystemStates.begin(), state.fuelSystemStates.end(), [](auto &state) { reset(state); });
  eastl::for_each(state.fuelTankStates.begin(), state.fuelTankStates.end(), [](auto &state) { reset(state); });
}

struct MassData
{
  Point3 center;
  float area;
  Point3 momentOfInertia;

  MassData(const Point3 &c, float a, const Point3 &inertia) : center(c), area(a), momentOfInertia(inertia) {}
};

static int sort_int(const int *lhs, const int *rhs) { return *lhs - *rhs; }

void gamephys::set_fuel_custom(const MassProps &props, MassState &state, float amount, bool internal, bool external,
  int fuel_system_num, bool limit)
{
  FuelSystemState &fss = state.fuelSystemStates[fuel_system_num];
  const FuelSystemProps &fsp = props.fuelSystemProps[fuel_system_num];
  fss.fuel = 0.0f;
  fss.fuelExternal = 0.0f;
  float amountToDistribute;
  if (internal)
  {
    fss.fuelAccumulated = min(amount, fsp.fuelAccumulatorCapacity);
    amountToDistribute = amount - fss.fuelAccumulated;
  }
  else
    amountToDistribute = amount;
  for (int p = fsp.priorityNum - 1; p >= 0; --p)
  {
    float maxFuel = 0.0f;
    if (internal)
      maxFuel += fsp.maxFuel - fsp.maxFuelExternal;
    if (external)
      maxFuel += fsp.maxFuelExternal;
    if (fsp.priorityNum > 1)
    {
      maxFuel = 0.0f;
      for (int i = 0; i < props.numTanks; ++i)
      {
        const FuelTankState &fts = state.fuelTankStates[i];
        const FuelTankProps &ftp = props.fuelTankProps[i];
        if (ftp.fuelSystemNum == fuel_system_num && ftp.priority == p && fts.available && (ftp.external ? external : internal))
          maxFuel += ftp.capacity;
      }
    }
    float percentage = safediv(amountToDistribute, maxFuel);
    if (limit || p > 0)
    {
      percentage = clamp(percentage, 0.f, 1.f);
      amountToDistribute = max(0.0f, amountToDistribute - maxFuel);
    }
    else
      amountToDistribute = 0.0f;
    for (int i = 0; i < props.numTanks; ++i)
    {
      FuelTankState &fts = state.fuelTankStates[i];
      const FuelTankProps &ftp = props.fuelTankProps[i];
      if (ftp.fuelSystemNum == fuel_system_num && ftp.priority == p && fts.available)
      {
        if (ftp.external ? external : internal)
          fts.currentFuel = percentage * ftp.capacity;
        fss.fuel += fts.currentFuel;
        if (ftp.external)
          fss.fuelExternal += fts.currentFuel;
      }
    }
  }
}

void gamephys::load_masses(MassProps &props, const DataBlock *parts_masses_blk, const DataBlock *surface_parts_masses_blk,
  const CollisionResource *collision, const NameMap &fuel_tank_names, const DataBlock &additional_masses)
{
  clear_and_shrink(props.advancedMasses);
  clear_and_shrink(props.collisionNodesMasses);
  props.numTanks = 0;

  if (collision)
  {
    dag::ConstSpan<CollisionNode> allCollisionNodes = collision->getAllNodes();
    NameMap nameMap;
    float totalArea = 0.f;
    float totalKnownMass = 0.f;
    const DataBlock *additionalParts = additional_masses.getBlockByNameEx("parts");
    for (int i = 0; i < additionalParts->blockCount(); ++i)
    {
      const DataBlock *partBlk = additionalParts->getBlock(i);
      nameMap.addNameId(partBlk->getBlockName());
      PartMass &currentMass = props.advancedMasses.push_back();
      reset(currentMass);
      currentMass.mass = partBlk->getReal("mass", 0.f);
      currentMass.center = partBlk->getPoint3("pos", Point3(0.f, 0.f, 0.f));
      currentMass.volumetric = true;
      totalKnownMass += currentMass.mass;
    }
    carray<float, FuelTankProps::MAX_SYSTEMS> knownFuel;
    mem_set_0(knownFuel);
    carray<float, FuelTankProps::MAX_SYSTEMS> unknownFuel;
    mem_set_0(unknownFuel);

    carray<int, FuelTankProps::MAX_TANKS> fuelTanksNumbers;
    mem_set_ff(fuelTanksNumbers);
    int numFuelTanks = 0;
    for (int i = 0; i < allCollisionNodes.size() && numFuelTanks < FuelTankProps::MAX_TANKS; ++i)
    {
      const CollisionNode &node = allCollisionNodes[i];
      if (node.type != COLLISION_NODE_TYPE_POINTS)
      {
        int tankNameNum = fuel_tank_names.getNameId(allCollisionNodes[i].name.str());
        if (tankNameNum >= 0)
        {
          fuelTanksNumbers[numFuelTanks] = tankNameNum;
          ++numFuelTanks;
        }
      }
    }
    if (numFuelTanks < FuelTankProps::MAX_TANKS)
      sort(fuelTanksNumbers, 0, numFuelTanks, &sort_int);
    else
      sort(fuelTanksNumbers, &sort_int);
#if DAGOR_DBGLEVEL > 0
    for (int i = 1; i < numFuelTanks; ++i)
      G_ASSERTF(fuelTanksNumbers[i] == fuelTanksNumbers[i - 1] + 1, "tank%d_dm follows tank%d_dm", fuelTanksNumbers[i],
        fuelTanksNumbers[i - 1]);
#endif
    const DataBlock *knownAdditionalMasses = additional_masses.getBlockByNameEx("masses");
    for (int i = 0; i < allCollisionNodes.size(); ++i)
    {
      const CollisionNode &node = allCollisionNodes[i];
      if (node.type != COLLISION_NODE_TYPE_POINTS)
      {
        int nid = nameMap.getNameId(node.name.str());
        if (nid < 0)
        {
          nameMap.addNameId(node.name.str());
          PartMass &currentMass = props.advancedMasses.push_back();
          reset(currentMass);

          float knownMass = parts_masses_blk->getReal(node.name.str(), -1.f);
          float knownSurfaceMass = surface_parts_masses_blk->getReal(node.name.str(), -1.f);
          if (knownMass >= 0.0f && knownSurfaceMass >= 0.0f)
            knownMass = -1.0f;
          else if (knownSurfaceMass >= 0.0f)
            knownMass = knownSurfaceMass;
          if (knownMass < 0.f && knownAdditionalMasses)
            knownMass = knownAdditionalMasses->getReal(node.name.str(), -1.f);

          int tankNameNum = fuel_tank_names.getNameId(node.name.str());
          int tankNum = tankNameNum >= 0 ? find_value_idx(fuelTanksNumbers, tankNameNum) : -1;
          int fuelSystemNum =
            props.separateFuelTanks ? parts_masses_blk->getInt(String(128, "tank%d_system", tankNum + 1).str(), 0) : 0;
          if (fuelSystemNum < props.numFuelSystems && tankNum >= 0 && tankNum < FuelTankProps::MAX_TANKS)
          {
            bool external =
              props.separateFuelTanks ? parts_masses_blk->getBool(String(128, "tank%d_external", tankNum + 1).str(), false) : 0;
            if (!external)
            {
              FuelSystemProps &fsp = props.fuelSystemProps[fuelSystemNum];
              currentMass.fuelSystemNum = fuelSystemNum;
              currentMass.fuelTankNum = tankNum;
              FuelTankProps &ftp = props.fuelTankProps[tankNum];
              reset(ftp);

              ftp.external = false;
              ftp.fuelSystemNum = fuelSystemNum;
              ftp.capacity =
                props.separateFuelTanks ? parts_masses_blk->getReal(String(128, "tank%d_capacity", tankNum + 1).str(), 0.f) : 0;
              knownFuel[fuelSystemNum] += ftp.capacity;
              if (ftp.capacity == 0.f)
                ++unknownFuel[fuelSystemNum];
              ++props.numTanks;
              ftp.priority =
                props.separateFuelTanks ? parts_masses_blk->getInt(String(128, "tank%d_priority", tankNum + 1).str(), 0) : 0;
              fsp.priorityNum = max(fsp.priorityNum, ftp.priority + 1);
            }
          }
          else
          {
            CollisionNodeMass &collisionNodeMass = props.collisionNodesMasses.push_back();
            collisionNodeMass.massIndex = props.advancedMasses.size() - 1;
            collisionNodeMass.nodeIndex = i;
          }
          bool isVolumetric = knownMass >= 0.f && knownSurfaceMass < 0.0f;
          currentMass.volumetric = isVolumetric;
          currentMass.mass = knownMass;
          if (isVolumetric)
          {
            totalKnownMass += knownMass;
            BBox3 localBox;
            localBox.setempty();
            if (node.type == COLLISION_NODE_TYPE_MESH)
            {
              for (int j = 0; j < node.indices.size(); ++j)
                localBox += node.tm % node.vertices[node.indices[j]]; // rotated, but not translated vertex position
            }
            else
              localBox = node.modelBBox;
            Point3 width = localBox.width();
            currentMass.momentOfInertia.set(lengthSq(Point2::yz(width)) / 12.f, lengthSq(Point2::xz(width)) / 12.f,
              lengthSq(Point2::xy(width)) / 12.f);
            currentMass.center = node.tm.getcol(3) + localBox.center(); // we already have rotated box, so no need to do it twice, just
                                                                        // translate it.
          }
          else if (node.type == COLLISION_NODE_TYPE_MESH)
          {
            Point3 centerOfMass(0.f, 0.f, 0.f);
            Tab<MassData> centers(framemem_ptr());
            centers.reserve(node.indices.size() / 3);
            for (int j = 0; j < node.indices.size(); j += 3)
            {
              Point3 v0 = node.tm % node.vertices[node.indices[j + 0]];
              Point3 v1 = node.tm % node.vertices[node.indices[j + 1]];
              Point3 v2 = node.tm % node.vertices[node.indices[j + 2]];
              Point3 center = (v0 + v1 + v2) / 3.f;
              Point3 p0 = v0 - center;
              Point3 p1 = v1 - center;
              Point3 p2 = v2 - center;
              float inertiaAboutNormal =
                safediv(length(p1 % p0) * (p1 * p1 + p1 * p0 + p0 * p0) + length(p2 % p1) * (p2 * p2 + p2 * p1 + p1 * p1),
                  (6.f * length(p1 % p0) + length(p2 % p1)));
              Point3 inertia = (p1 - p0) % (p2 - p1);
              normalizeDef(inertia, Point3(0.f, 1.f, 0.));
              inertia *= inertiaAboutNormal;
              float area = length((v1 - v0) % (v2 - v0)) * 0.5f;
              centerOfMass += area * center;
              currentMass.eqArea += area;
              currentMass.momentOfInertia = inertia * area;
              centers.push_back(MassData(center, area, inertia));
            }
            float invArea = safeinv(currentMass.eqArea);
            centerOfMass *= invArea;
            for (int j = 0; j < centers.size(); ++j)
            {
              Point3 rFromCenter = Point3::xyz(centers[j].center) - centerOfMass;
              currentMass.momentOfInertia +=
                Point3(lengthSq(Point2::yz(rFromCenter)), lengthSq(Point2::xz(rFromCenter)), lengthSq(Point2::xy(rFromCenter))) *
                centers[j].area;
            }
            currentMass.momentOfInertia *= invArea;
            currentMass.center = node.tm.getcol(3) + centerOfMass;
            totalArea += currentMass.eqArea;
          }
        }
      }
    }
    float invTotalArea = safeinv(totalArea);
    for (int i = 0; i < props.advancedMasses.size(); ++i)
    {
      if (props.advancedMasses[i].mass > 0.f)
        continue;
      props.advancedMasses[i].mass = max(props.massEmpty - totalKnownMass, 0.0f) * props.advancedMasses[i].eqArea * invTotalArea;
    }

    for (int i = 0; i < props.numTanks; ++i)
    {
      FuelTankProps &ftp = props.fuelTankProps[i];
      const FuelSystemProps &fsp = props.fuelSystemProps[ftp.fuelSystemNum];
      if (ftp.capacity == 0.f)
        ftp.capacity = (fsp.maxFuel - knownFuel[ftp.fuelSystemNum]) * safeinv(float(unknownFuel[ftp.fuelSystemNum]));
    }

    // External fuel tanks

    for (int tankNum = 0; tankNum < FuelTankProps::MAX_TANKS; ++tankNum)
    {
      float capacity = parts_masses_blk->getReal(String(128, "tank%d_capacity", tankNum + 1).str(), 0.f);
      int fuelSystemNum = parts_masses_blk->getInt(String(128, "tank%d_system", tankNum + 1).str(), 0);
      bool external = parts_masses_blk->getBool(String(128, "tank%d_external", tankNum + 1).str(), false);
      if (capacity > 0.0f && fuelSystemNum < props.numFuelSystems && external)
      {
        FuelTankProps &ftp = props.fuelTankProps[tankNum];
        ftp.capacity = capacity;
        ftp.fuelSystemNum = fuelSystemNum;
        ftp.priority = parts_masses_blk->getInt(String(128, "tank%d_priority", tankNum + 1).str(), 0);
        FuelSystemProps &fsp = props.fuelSystemProps[ftp.fuelSystemNum];
        fsp.priorityNum = max(fsp.priorityNum, ftp.priority + 1);
        ftp.external = true;
        ++props.numTanks;
      }
    }

    if (props.numTanks == 0)
    {
      props.numTanks = 1;
      FuelTankProps &ftp = props.fuelTankProps[FuelTankProps::MAIN_SYSTEM];
      ftp.capacity = props.fuelSystemProps[FuelTankProps::MAIN_SYSTEM].maxFuel;
    }

    // Ensure that fuel system capacity is a summ of the connected fuel tanks capcities
    for (int i = 0; i < props.numFuelSystems; ++i)
      props.fuelSystemProps[i].maxFuel = props.fuelSystemProps[i].maxFuelExternal = 0.0f;
    for (int i = 0; i < props.numTanks; ++i)
    {
      const FuelTankProps &ftp = props.fuelTankProps[i];
      FuelSystemProps &fsp = props.fuelSystemProps[ftp.fuelSystemNum];
      fsp.maxFuel += ftp.capacity;
      if (ftp.external)
      {
        fsp.maxFuelExternal += ftp.capacity;
      }
    }
  }
}

void gamephys::post_load_masses(const MassProps &props, MassState &state, const CollisionResource *collision, int num_tanks_old)
{
  if (collision)
  {
    // Limit tanks
    for (int i = 0; i < props.numTanks; ++i)
    {
      FuelTankState &fts = state.fuelTankStates[i];
      const FuelTankProps &ftp = props.fuelTankProps[i];
      fts.currentFuel = min(fts.currentFuel, ftp.capacity);
    }
    // External fuel tanks
    for (int tankNum = 0; tankNum < FuelTankProps::MAX_TANKS; ++tankNum)
    {
      const FuelTankProps &ftp = props.fuelTankProps[tankNum];
      if (ftp.capacity > 0.0f && ftp.fuelSystemNum < props.numFuelSystems && ftp.external)
      {
        FuelTankState &fts = state.fuelTankStates[tankNum];
        fts.currentFuel = min(fts.currentFuel, ftp.capacity);
        if (props.numTanks > num_tanks_old)
          fts.available = false;
      }
    }
    // Ensure that fuel system capacity is a sum of the connected fuel tanks capacities
    for (int i = 0; i < props.numFuelSystems; ++i)
      state.fuelSystemStates[i].fuel = state.fuelSystemStates[i].fuelExternal = 0.0f;
    for (int i = 0; i < props.numTanks; ++i)
    {
      const FuelTankState &fts = state.fuelTankStates[i];
      const FuelTankProps &ftp = props.fuelTankProps[i];
      FuelSystemState &fss = state.fuelSystemStates[ftp.fuelSystemNum];
      fss.fuel += fts.currentFuel;
      if (ftp.external)
        fss.fuelExternal += fts.currentFuel;
    }
  }
}

void gamephys::load_save_override(MassProps &props, DataBlock *mass_blk, bool load, const CollisionResource *collision,
  const NameMap &fuel_tank_names, const DataBlock &additional_masses)
{
  blkutil::loadSaveBlk(mass_blk, "EmptyMass", props.massEmpty, 0.0f, load);

  if (load)
  {
    props.fuelSystemProps[FuelTankProps::MAIN_SYSTEM].maxFuel = mass_blk->getReal("MaxFuelMass", 0.0f);
    props.fuelSystemProps[FuelTankProps::MAIN_SYSTEM].maxFuelExternal = mass_blk->getReal("MaxFuelMassExternal", 0.0f);
    props.numFuelSystems = 1;
  }
  for (int i = 0; i < (load ? FuelTankProps::MAX_SYSTEMS : props.numFuelSystems); ++i)
  {
    blkutil::loadSaveBlk(mass_blk, String(32, "MaxFuelMass%d", i).str(), props.fuelSystemProps[i].maxFuel,
      i == FuelTankProps::MAIN_SYSTEM ? props.fuelSystemProps[i].maxFuel : 0.0f, load);
    blkutil::loadSaveBlk(mass_blk, String(32, "MaxFuelMassExternal%d", i).str(), props.fuelSystemProps[i].maxFuelExternal,
      i == FuelTankProps::MAIN_SYSTEM ? props.fuelSystemProps[i].maxFuelExternal : 0.0f, load);
    if (load && props.fuelSystemProps[i].maxFuel > 0.0f && i > 0)
      ++props.numFuelSystems;
  }

  float minimalLoadFactor = -1.0e6f;
  float fuelAccumulatorFlowRate = 1.0e6f;
  float fuelEngineFlowRate = 1.0e6f;
  if (load)
  {
    minimalLoadFactor = mass_blk->getReal("MinimalLoadFactor", minimalLoadFactor);
    fuelAccumulatorFlowRate = mass_blk->getReal("FuelAccumulatorFlowRate", fuelAccumulatorFlowRate);
    fuelEngineFlowRate = mass_blk->getReal("FuelEngineFlowRate", fuelEngineFlowRate);
  }
  for (int i = 0; i < props.numFuelSystems; ++i)
  {
    blkutil::loadSaveBlk(mass_blk, String(32, "FuelAccumulatorCapacity%d", i).str(), props.fuelSystemProps[i].fuelAccumulatorCapacity,
      0.0f, load);
    blkutil::loadSaveBlk(mass_blk, String(32, "MinimalLoadFactor%d", i).str(), props.fuelSystemProps[i].minimalLoadFactor,
      minimalLoadFactor, load);
    blkutil::loadSaveBlk(mass_blk, String(32, "FuelAccumulatorFlowRate%d", i).str(), props.fuelSystemProps[i].fuelAccumulatorFlowRate,
      fuelAccumulatorFlowRate, load);
    blkutil::loadSaveBlk(mass_blk, String(32, "FuelEngineFlowRate%d", i).str(), props.fuelSystemProps[i].fuelEngineFlowRate,
      fuelEngineFlowRate, load);
  }

  blkutil::loadSaveBlk(mass_blk, "hasFuelDumping", props.hasFuelDumping, false, load);
  blkutil::loadSaveBlk(mass_blk, "fuelDumpingRate", props.fuelDumpingRate, -1.0f, load);

  blkutil::loadSaveBlk(mass_blk, "MaxNitro", props.maxNitro, 0.0f, load);
  blkutil::loadSaveBlk(mass_blk, "OilMass", props.oilMass, 0.0f, load);
  blkutil::loadSaveBlk(mass_blk, "AdvancedMass", props.advancedMass, false, load);
  blkutil::loadSaveBlk(mass_blk, "doesPayloadAffectCOG", props.doesPayloadAffectCOG, false, load);
  blkutil::loadSaveBlk(mass_blk, "SeparateFuelTanks", props.separateFuelTanks, false, load);
  blkutil::loadSaveBlk(mass_blk, "Takeoff", props.maxWeight, 0.0f, load);
  blkutil::loadSaveBlk(mass_blk, "CenterOfGravity", props.initialCenterOfGravity, Point3(0.f, 0.f, 0.f), load);

  if (load)
  {
    clear_and_shrink(props.dynamicMasses);
    props.dynamicMasses.clear();

    const int partBlockNameId = mass_blk->getNameId("Part");
    int partBlockNum = -1;
    while ((partBlockNum = mass_blk->findBlock(partBlockNameId, partBlockNum)) >= 0)
    {
      const DataBlock *partBlk = mass_blk->getBlock(partBlockNum);
      PartMass &dynamicMass = props.dynamicMasses.push_back();
      reset(dynamicMass);
      dynamicMass.mass = partBlk->getReal("mass", 0.0f);
      dynamicMass.center = partBlk->getPoint3("center", ZERO<Point3>());
      dynamicMass.momentOfInertia = partBlk->getPoint3("inertiaMoment", ZERO<Point3>()) * safeinv(dynamicMass.mass);
    }
  }
  else
  {
    for (const auto &dynamicMass : props.dynamicMasses)
    {
      DataBlock *partBlk = mass_blk->addBlock("Part");
      partBlk->getReal("mass", dynamicMass.mass);
      partBlk->setPoint3("center", dynamicMass.center);
      partBlk->setPoint3("inertiaMoment", dynamicMass.momentOfInertia * dynamicMass.mass);
    }
  }

  DataBlock *partsMassesBlk = mass_blk->addBlock("Parts");
  DataBlock *partsWithSurfaceMassesBlk = mass_blk->addBlock("PartsWithSurface");
  if (load)
  {
    load_masses(props, partsMassesBlk, partsWithSurfaceMassesBlk, collision, fuel_tank_names, additional_masses);
    for (int i = props.numFuelSystems; i < FuelTankProps::MAX_SYSTEMS; ++i)
      reset(props.fuelSystemProps[i]);
  }
  else
  {
    if (collision != NULL)
    {
      if (props.separateFuelTanks)
      {
        for (int i = 0; i < props.numTanks; ++i)
        {
          FuelTankProps &ftp = props.fuelTankProps[i];
          if (ftp.capacity != 0.f)
            partsMassesBlk->setReal(String(128, "tank%d_capacity", i + 1).str(), ftp.capacity);
          partsMassesBlk->setInt(String(128, "tank%d_system", i + 1).str(), ftp.fuelSystemNum);
          partsMassesBlk->setBool(String(128, "tank%d_external", i + 1).str(), ftp.external);
          partsMassesBlk->setInt(String(128, "tank%d_priority", i + 1).str(), ftp.priority);
        }
      }
      dag::ConstSpan<CollisionNode> allCollisionNodes = collision->getAllNodes();
      for (int i = 0; i < props.collisionNodesMasses.size(); ++i)
      {
        const CollisionNodeMass &collisionNodeMass = props.collisionNodesMasses[i];
        const PartMass &partMass = props.advancedMasses[collisionNodeMass.massIndex];
        DataBlock *blk = partMass.volumetric ? partsMassesBlk : partsWithSurfaceMassesBlk;
        blk->setReal(allCollisionNodes[collisionNodeMass.nodeIndex].name.str(), partMass.mass);
      }
    }
  }
}

float Mass::calcConsumptionLimit(int fuel_system_num, float load_factor, float dt) const
{
  float amount = 0.0f;
  const FuelSystemProps &fsp = props.fuelSystemProps[fuel_system_num];
  const FuelSystemState &fss = state.fuelSystemStates[fuel_system_num];
  const float fuelToEngine = fsp.fuelEngineFlowRate * dt;
  float remains = fuelToEngine - fss.fuelAccumulated;
  if (remains < 0.0f)
    amount = fuelToEngine;
  else
  {
    amount += fss.fuelAccumulated;
    if (load_factor >= fsp.minimalLoadFactor)
    {
      const float fuelToAccumulator = fsp.fuelAccumulatorFlowRate * dt;
      amount += min(remains, min(fss.fuel - fss.fuelAccumulated, fuelToAccumulator));
    }
  }
  return amount;
}

void Mass::computeFullMass()
{
  const float fuelMass = getFuelMassCurrent();
  state.mass = props.massEmpty + props.oilMass + props.crewMass + fuelMass + state.nitro + state.payloadMass;
}

Mass::Mass() { init(); }

void Mass::init()
{
  reset(props);
  reset(state);
  reset(output);
}

void Mass::loadMasses(const DataBlock *parts_masses_blk, const DataBlock *surface_parts_masses_blk, const CollisionResource *collision,
  const NameMap &fuel_tank_names, const DataBlock &additional_masses)
{
  clear_and_shrink(props.advancedMasses);
  clear_and_shrink(props.collisionNodesMasses);
  const int numTanksOld = props.numTanks;
  props.numTanks = 0;

  if (collision)
  {
    dag::ConstSpan<CollisionNode> allCollisionNodes = collision->getAllNodes();
    NameMap nameMap;
    float totalArea = 0.f;
    float totalKnownMass = 0.f;
    const DataBlock *additionalParts = additional_masses.getBlockByNameEx("parts");
    for (int i = 0; i < additionalParts->blockCount(); ++i)
    {
      const DataBlock *partBlk = additionalParts->getBlock(i);
      nameMap.addNameId(partBlk->getBlockName());
      PartMass &currentMass = props.advancedMasses.push_back();
      reset(currentMass);
      currentMass.mass = partBlk->getReal("mass", 0.f);
      currentMass.center = partBlk->getPoint3("pos", Point3(0.f, 0.f, 0.f));
      currentMass.volumetric = true;
      totalKnownMass += currentMass.mass;
    }
    carray<float, MAX_FUEL_SYSTEMS> knownFuel;
    mem_set_0(knownFuel);
    carray<float, MAX_FUEL_SYSTEMS> unknownFuel;
    mem_set_0(unknownFuel);

    carray<int, MAX_FUEL_TANKS> fuelTanksNumbers;
    mem_set_ff(fuelTanksNumbers);
    int numFuelTanks = 0;
    for (int i = 0; i < allCollisionNodes.size() && numFuelTanks < MAX_FUEL_TANKS; ++i)
    {
      const CollisionNode &node = allCollisionNodes[i];
      if (node.type != COLLISION_NODE_TYPE_POINTS)
      {
        int tankNameNum = fuel_tank_names.getNameId(allCollisionNodes[i].name.str());
        if (tankNameNum >= 0)
        {
          fuelTanksNumbers[numFuelTanks] = tankNameNum;
          ++numFuelTanks;
        }
      }
    }
    if (numFuelTanks < MAX_FUEL_TANKS)
      sort(fuelTanksNumbers, 0, numFuelTanks, &sort_int);
    else
      sort(fuelTanksNumbers, &sort_int);
#if DAGOR_DBGLEVEL > 0
    for (int i = 1; i < numFuelTanks; ++i)
      G_ASSERTF(fuelTanksNumbers[i] == fuelTanksNumbers[i - 1] + 1, "tank%d_dm follows tank%d_dm", fuelTanksNumbers[i],
        fuelTanksNumbers[i - 1]);
#endif
    const DataBlock *knownAdditionalMasses = additional_masses.getBlockByNameEx("masses");
    for (int i = 0; i < allCollisionNodes.size(); ++i)
    {
      const CollisionNode &node = allCollisionNodes[i];
      if (node.type != COLLISION_NODE_TYPE_POINTS)
      {
        int nid = nameMap.getNameId(node.name.str());
        if (nid < 0)
        {
          nameMap.addNameId(node.name.str());
          PartMass &currentMass = props.advancedMasses.push_back();
          reset(currentMass);

          float knownMass = parts_masses_blk->getReal(node.name.str(), -1.f);
          float knownSurfaceMass = surface_parts_masses_blk->getReal(node.name.str(), -1.f);
          if (knownMass >= 0.0f && knownSurfaceMass >= 0.0f)
            knownMass = -1.0f;
          else if (knownSurfaceMass >= 0.0f)
            knownMass = knownSurfaceMass;
          if (knownMass < 0.f && knownAdditionalMasses)
            knownMass = knownAdditionalMasses->getReal(node.name.str(), -1.f);

          int tankNameNum = fuel_tank_names.getNameId(node.name.str());
          int tankNum = tankNameNum >= 0 ? find_value_idx(fuelTanksNumbers, tankNameNum) : -1;
          int fuelSystemNum =
            props.separateFuelTanks ? parts_masses_blk->getInt(String(128, "tank%d_system", tankNum + 1).str(), 0) : 0;
          if (fuelSystemNum < props.numFuelSystems && tankNum >= 0 && tankNum < MAX_FUEL_TANKS)
          {
            bool external =
              props.separateFuelTanks ? parts_masses_blk->getBool(String(128, "tank%d_external", tankNum + 1).str(), false) : 0;
            if (!external)
            {
              FuelSystemProps &fsp = props.fuelSystemProps[fuelSystemNum];
              currentMass.fuelSystemNum = fuelSystemNum;
              currentMass.fuelTankNum = tankNum;
              FuelTankState &fts = state.fuelTankStates[tankNum];
              reset(fts);
              FuelTankProps &ftp = props.fuelTankProps[tankNum];
              reset(ftp);

              ftp.external = false;
              ftp.fuelSystemNum = fuelSystemNum;
              ftp.capacity =
                props.separateFuelTanks ? parts_masses_blk->getReal(String(128, "tank%d_capacity", tankNum + 1).str(), 0.f) : 0;
              knownFuel[fuelSystemNum] += ftp.capacity;
              if (ftp.capacity == 0.f)
                ++unknownFuel[fuelSystemNum];
              ++props.numTanks;
              ftp.priority =
                props.separateFuelTanks ? parts_masses_blk->getInt(String(128, "tank%d_priority", tankNum + 1).str(), 0) : 0;
              fsp.priorityNum = max(fsp.priorityNum, ftp.priority + 1);
            }
          }
          else
          {
            CollisionNodeMass &collisionNodeMass = props.collisionNodesMasses.push_back();
            collisionNodeMass.massIndex = props.advancedMasses.size() - 1;
            collisionNodeMass.nodeIndex = i;
          }
          bool isVolumetric = knownMass >= 0.f && knownSurfaceMass < 0.0f;
          currentMass.volumetric = isVolumetric;
          currentMass.mass = knownMass;
          if (isVolumetric)
          {
            totalKnownMass += knownMass;
            BBox3 localBox;
            localBox.setempty();
            if (node.type == COLLISION_NODE_TYPE_MESH)
            {
              for (int j = 0; j < node.indices.size(); ++j)
                localBox += node.tm % node.vertices[node.indices[j]]; // rotated, but not translated vertex position
            }
            else
              localBox = node.modelBBox;
            Point3 width = localBox.width();
            currentMass.momentOfInertia.set(lengthSq(Point2::yz(width)) / 12.f, lengthSq(Point2::xz(width)) / 12.f,
              lengthSq(Point2::xy(width)) / 12.f);
            currentMass.center = node.tm.getcol(3) + localBox.center(); // we already have rotated box, so no need to do it twice, just
                                                                        // translate it.
          }
          else if (node.type == COLLISION_NODE_TYPE_MESH)
          {
            Point3 centerOfMass(0.f, 0.f, 0.f);
            Tab<MassData> centers(framemem_ptr());
            centers.reserve(node.indices.size() / 3);
            for (int j = 0; j < node.indices.size(); j += 3)
            {
              Point3 v0 = node.tm % node.vertices[node.indices[j + 0]];
              Point3 v1 = node.tm % node.vertices[node.indices[j + 1]];
              Point3 v2 = node.tm % node.vertices[node.indices[j + 2]];
              Point3 center = (v0 + v1 + v2) / 3.f;
              Point3 p0 = v0 - center;
              Point3 p1 = v1 - center;
              Point3 p2 = v2 - center;
              float inertiaAboutNormal =
                safediv(length(p1 % p0) * (p1 * p1 + p1 * p0 + p0 * p0) + length(p2 % p1) * (p2 * p2 + p2 * p1 + p1 * p1),
                  (6.f * length(p1 % p0) + length(p2 % p1)));
              Point3 inertia = (p1 - p0) % (p2 - p1);
              normalizeDef(inertia, Point3(0.f, 1.f, 0.));
              inertia *= inertiaAboutNormal;
              float area = length((v1 - v0) % (v2 - v0)) * 0.5f;
              centerOfMass += area * center;
              currentMass.eqArea += area;
              currentMass.momentOfInertia = inertia * area;
              centers.push_back(MassData(center, area, inertia));
            }
            float invArea = safeinv(currentMass.eqArea);
            centerOfMass *= invArea;
            for (int j = 0; j < centers.size(); ++j)
            {
              Point3 rFromCenter = Point3::xyz(centers[j].center) - centerOfMass;
              currentMass.momentOfInertia +=
                Point3(lengthSq(Point2::yz(rFromCenter)), lengthSq(Point2::xz(rFromCenter)), lengthSq(Point2::xy(rFromCenter))) *
                centers[j].area;
            }
            currentMass.momentOfInertia *= invArea;
            currentMass.center = node.tm.getcol(3) + centerOfMass;
            totalArea += currentMass.eqArea;
          }
        }
      }
    }
    float invTotalArea = safeinv(totalArea);
    for (int i = 0; i < props.advancedMasses.size(); ++i)
    {
      if (props.advancedMasses[i].mass > 0.f)
        continue;
      props.advancedMasses[i].mass = max(props.massEmpty - totalKnownMass, 0.0f) * props.advancedMasses[i].eqArea * invTotalArea;
    }

    for (int i = 0; i < props.numTanks; ++i)
    {
      FuelTankState &fts = state.fuelTankStates[i];
      FuelTankProps &ftp = props.fuelTankProps[i];
      const FuelSystemProps &fsp = props.fuelSystemProps[ftp.fuelSystemNum];
      if (ftp.capacity == 0.f)
        ftp.capacity = (fsp.maxFuel - knownFuel[ftp.fuelSystemNum]) * safeinv(float(unknownFuel[ftp.fuelSystemNum]));
      fts.currentFuel = min(fts.currentFuel, ftp.capacity);
    }

    // External fuel tanks

    for (int tankNum = 0; tankNum < MAX_FUEL_TANKS; ++tankNum)
    {
      float capacity = parts_masses_blk->getReal(String(128, "tank%d_capacity", tankNum + 1).str(), 0.f);
      int fuelSystemNum = parts_masses_blk->getInt(String(128, "tank%d_system", tankNum + 1).str(), 0);
      bool external = parts_masses_blk->getBool(String(128, "tank%d_external", tankNum + 1).str(), false);
      if (capacity > 0.0f && fuelSystemNum < props.numFuelSystems && external)
      {
        FuelTankState &fts = state.fuelTankStates[tankNum];
        FuelTankProps &ftp = props.fuelTankProps[tankNum];
        ftp.capacity = capacity;
        fts.currentFuel = min(fts.currentFuel, capacity);
        ftp.fuelSystemNum = fuelSystemNum;
        ftp.priority = parts_masses_blk->getInt(String(128, "tank%d_priority", tankNum + 1).str(), 0);
        FuelSystemProps &fsp = props.fuelSystemProps[ftp.fuelSystemNum];
        fsp.priorityNum = max(fsp.priorityNum, ftp.priority + 1);
        ftp.external = true;
        ++props.numTanks;
        if (props.numTanks > numTanksOld)
          fts.available = false;
      }
    }

    if (props.numTanks == 0)
    {
      props.numTanks = 1;
      FuelTankProps &ftp = props.fuelTankProps[gamephys::FuelTankProps::MAIN_SYSTEM];
      ftp.capacity = props.fuelSystemProps[gamephys::FuelTankProps::MAIN_SYSTEM].maxFuel;
    }

    // Ensure that fuel system capacity is a summ of the connected fuel tanks capcities
    for (int i = 0; i < props.numFuelSystems; ++i)
      props.fuelSystemProps[i].maxFuel = props.fuelSystemProps[i].maxFuelExternal = state.fuelSystemStates[i].fuel =
        state.fuelSystemStates[i].fuelExternal = 0.0f;
    for (int i = 0; i < props.numTanks; ++i)
    {
      const FuelTankState &fts = state.fuelTankStates[i];
      const FuelTankProps &ftp = props.fuelTankProps[i];
      FuelSystemProps &fsp = props.fuelSystemProps[ftp.fuelSystemNum];
      FuelSystemState &fss = state.fuelSystemStates[ftp.fuelSystemNum];
      fsp.maxFuel += ftp.capacity;
      fss.fuel += fts.currentFuel;
      if (ftp.external)
      {
        fsp.maxFuelExternal += ftp.capacity;
        fss.fuelExternal += fts.currentFuel;
      }
    }
  }
}

void Mass::baseLoad(const DataBlock *mass_blk, int crew_num)
{
  props.massEmpty = mass_blk->getReal("Empty", 0.f);
  props.oilMass = mass_blk->getReal("Oil", 0.f);
  props.crewMass = crew_num * 90.f;
  props.maxWeight = mass_blk->getReal("TakeOff", 0.f);

  eastl::for_each(props.fuelSystemProps.begin(), props.fuelSystemProps.end(), [](FuelSystemProps &p) { reset(p); });
  eastl::for_each(state.fuelSystemStates.begin(), state.fuelSystemStates.end(), [](FuelSystemState &s) { reset(s); });

  props.fuelSystemProps[MAIN_FUEL_SYSTEM].maxFuel = mass_blk->getReal("Fuel", 0.f);
  props.fuelSystemProps[MAIN_FUEL_SYSTEM].maxFuelExternal = 0.0f;
  props.numFuelSystems = 1;
  for (int i = 0; i < MAX_FUEL_SYSTEMS; ++i)
  {
    props.fuelSystemProps[i].maxFuel =
      mass_blk->getReal(String(16, "Fuel%d", i).str(), i == MAIN_FUEL_SYSTEM ? props.fuelSystemProps[i].maxFuel : 0.0f);
    props.fuelSystemProps[i].maxFuelExternal = 0.0f;
    if (props.fuelSystemProps[i].maxFuel > 0.0f && i > MAIN_FUEL_SYSTEM)
    {
      ++props.numFuelSystems;
      state.fuelSystemStates[i].fuel = props.fuelSystemProps[i].maxFuel;
      state.fuelSystemStates[i].fuelExternal = props.fuelSystemProps[i].maxFuelExternal;
    }
  }

  props.advancedMass = mass_blk->getBool("AdvancedMass", false);
  props.doesPayloadAffectCOG = mass_blk->getBool("doesPayloadAffectCOG", false);
  props.separateFuelTanks = mass_blk->getBool("SeparateFuelTanks", false);
  props.initialCenterOfGravity = mass_blk->getPoint3("CenterOfGravity", Point3(0.f, 0.f, 0.f));
  state.centerOfGravity = props.initialCenterOfGravity;
  props.centerOfGravityClampY = mass_blk->getPoint2("CenterOfGravityClampY", Point2(-VERY_BIG_NUMBER, VERY_BIG_NUMBER));

  props.hasFuelDumping = mass_blk->getBool("hasFuelDumping", false);
  props.fuelDumpingRate = mass_blk->getReal("fuelDumpingRate", -1.0f);

  state.mass = props.massEmpty;
  props.maxNitro = state.nitro = mass_blk->getReal("Nitro", 0.0f);
}

void Mass::load(const DataBlock &settings_blk, const CollisionResource *collision, const NameMap &fuel_tank_names,
  const DataBlock &additional_masses)
{
  const DataBlock *massBlk = settings_blk.getBlockByNameEx("Mass");
  const DataBlock *aircraftBlk = settings_blk.getBlockByNameEx("Aircraft");
  const int crewNum = aircraftBlk->getInt("crew", 0);

  baseLoad(massBlk, crewNum);

  DataBlock emptyBlk;
  loadMasses(massBlk, &emptyBlk, collision, fuel_tank_names, additional_masses);
}

void Mass::load(const DataBlock *mass_blk, const int crewNum, const CollisionResource *collision, const NameMap &fuel_tank_names,
  const DataBlock &additional_masses)
{
  baseLoad(mass_blk, crewNum);

  DataBlock emptyBlk;
  loadMasses(mass_blk, &emptyBlk, collision, fuel_tank_names, additional_masses);
}

void Mass::loadSaveOverride(DataBlock *mass_blk, bool load, const CollisionResource *collision, const NameMap &fuel_tank_names,
  const DataBlock &additional_masses)
{
  carray<float, FuelTankProps::MAX_SYSTEMS> fuel, fuelExternal;
  for (int i = 0; i < FuelTankProps::MAX_SYSTEMS; ++i)
  {
    fuel[i] = props.numFuelSystems <= FuelTankProps::MAX_SYSTEMS ? state.fuelSystemStates[i].fuel : 0.0f;
    fuelExternal[i] = props.numFuelSystems <= FuelTankProps::MAX_SYSTEMS ? state.fuelSystemStates[i].fuelExternal : 0.0f;
  }
  // props loading
  load_save_override(props, mass_blk, load, collision, fuel_tank_names, additional_masses);

  // state setting
  if (load)
  {
    for (int i = 0; i < props.numTanks; ++i)
    {
      if (!props.fuelTankProps[i].external)
        reset(state.fuelTankStates[i]);
    }
    for (int i = 0; i < props.numFuelSystems; ++i)
    {
      state.fuelSystemStates[i].fuelAccumulated =
        min(state.fuelSystemStates[i].fuelAccumulated, props.fuelSystemProps[i].fuelAccumulatorCapacity);
    }
  }
  state.nitro = props.maxNitro;
  state.centerOfGravity = props.initialCenterOfGravity;
  if (load)
  {
    for (int i = 0; i < props.numFuelSystems; ++i)
    {
      set_fuel_internal(props, state,
        min(fuel[i] - fuelExternal[i], props.fuelSystemProps[i].maxFuel - props.fuelSystemProps[i].maxFuelExternal), i, true);
      set_fuel_external(props, state, min(fuelExternal[i], props.fuelSystemProps[i].maxFuelExternal), i, true);
    }
    for (int i = props.numFuelSystems; i < FuelTankProps::MAX_SYSTEMS; ++i)
      reset(state.fuelSystemStates[i]);
  }
  int numTanksOld = props.numTanks;
  post_load_masses(props, state, collision, numTanksOld);
}

void Mass::repair()
{
  state.nitro = props.maxNitro;
  state.momentOfInertiaNormMult.set(1.0, 1.0, 1.0);
}

const MassOutput &gamephys::Mass::simulate(const MassInput &input, float dt)
{
  TIME_PROFILE_DEV(mass_simulate)
  reset(output);
  static constexpr int dumpFuelSystem = 0;
  eastl::vector_set<eastl::pair<float, int>, eastl::less<eastl::pair<float, int>>, framemem_allocator> tanksToDumpFrom;
  tanksToDumpFrom.reserve(props.numTanks);

  for (int i = 0; i < props.numTanks; ++i)
  {
    FuelTankState &fts = state.fuelTankStates[i];
    // Leak
    if (input.leakFuelAmount[i] > 0.0f)
    {
      const float leakedFuel = min(input.leakFuelAmount[i], fts.currentFuel);
      fts.currentFuel -= leakedFuel;
      state.fuelSystemStates[i].fuel -= leakedFuel;
      output.leakedFuelAmount += leakedFuel;
    }
  }
  // Dump
  if (input.dumpFuelAmount > 0.0f)
  {
    for (int i = 0; i < props.numTanks; ++i)
    {
      const FuelTankState &fts = state.fuelTankStates[i];
      const FuelTankProps &ftp = props.fuelTankProps[i];
      if (ftp.fuelSystemNum == dumpFuelSystem && fts.available && fts.currentFuel > 0.0f)
      {
        tanksToDumpFrom.insert({fts.currentFuel, i});
      }
    }

    float fuelLeftToDump = input.dumpFuelAmount;
    while (!tanksToDumpFrom.empty())
    {
      auto curIt = tanksToDumpFrom.begin();

      FuelTankState &fts = state.fuelTankStates[curIt->second];

      const float curAmountToDump = fuelLeftToDump / static_cast<float>(tanksToDumpFrom.size());

      if (fts.currentFuel > curAmountToDump)
      {
        fts.currentFuel -= curAmountToDump;
        fuelLeftToDump -= curAmountToDump;
      }
      else
      {
        fuelLeftToDump -= fts.currentFuel;
        fts.currentFuel = 0.f;
      }

      tanksToDumpFrom.erase(curIt);
    }
    const float fuelDumped = input.dumpFuelAmount - fuelLeftToDump;
    state.fuelSystemStates[dumpFuelSystem].fuel -= fuelDumped;
    output.dumpedFuelAmount = fuelDumped;
  }
  // Nitro
  if (input.consumeNitroAmount > 0.f)
  {
    state.nitro = max(state.nitro - input.consumeNitroAmount, 0.f);
    output.availableNitroAmount = state.nitro;
  }
  // Consume
  for (int i = 0; i < props.numFuelSystems; i++)
  {
    if (input.consumeFuel[i] > 0.f)
      output.consumedFuelAmount += consumeFuelAmount(input.consumeFuel[i], true, i, input.loadFactor, dt);
    if (input.consumeFuelUnlimited[i] > 0.f)
      output.consumedFuelAmount += consumeFuelAmount(input.consumeFuelUnlimited[i], false, i, input.loadFactor, dt);
  }
  computeMasses(input.parts, input.payloadCogMult, input.payloadImMult, input.partsPresenceFlags);
  for (int i = Mass::MAIN_FUEL_SYSTEM; i < props.numFuelSystems; i++)
    output.availableFuelAmount[i] = calcConsumptionLimit(i, input.loadFactor, dt);
  computeFullMass();
  return output;
}

float Mass::consumeFuelAmount(float amount, bool limited_fuel, int fuel_system_num, float load_factor, float dt)
{
  // Consumption fuel from the fuel accumulator
  const FuelSystemProps &fsp = props.fuelSystemProps[fuel_system_num];
  FuelSystemState &fss = state.fuelSystemStates[fuel_system_num];
  fss.fuelAccumulated -= min(amount, fsp.fuelEngineFlowRate * dt);

  // Recharging the fuel accumulator
  float amountToAccumulate = min(fsp.fuelAccumulatorCapacity - fss.fuelAccumulated, fsp.fuelAccumulatorFlowRate * dt);
  float fuelConsumed = 0.f;
  fss.fuel = 0.0f;
  fss.fuelExternal = 0.0f;

  for (int p = 0; p < fsp.priorityNum; ++p)
  {
    float oldFuel = 0.f;
    for (int i = 0; i < props.numTanks; ++i)
    {
      const FuelTankState &fts = state.fuelTankStates[i];
      const FuelTankProps &ftp = props.fuelTankProps[i];
      if (ftp.fuelSystemNum == fuel_system_num && ftp.priority == p && fts.available)
        oldFuel += fts.currentFuel;
    }
    const float oldFuelInv = safeinv(oldFuel);
    for (int i = 0; i < props.numTanks; ++i)
    {
      FuelTankState &fts = state.fuelTankStates[i];
      const FuelTankProps &ftp = props.fuelTankProps[i];
      if (ftp.fuelSystemNum == fuel_system_num && ftp.priority == p && fts.currentFuel > 0.f && fts.available)
      {
        if (fuelConsumed < amountToAccumulate)
        {
          const float fuelToConsume =
            load_factor >= fsp.minimalLoadFactor ? min(amountToAccumulate * fts.currentFuel * oldFuelInv, fts.currentFuel) : 0.0f;
          fss.fuelAccumulated += fuelToConsume;
          fuelConsumed += fuelToConsume;
          if (limited_fuel)
            fts.currentFuel -= fuelToConsume;
        }
        fss.fuel += fts.currentFuel;
        if (ftp.external)
          fss.fuelExternal += fts.currentFuel;
      }
    }
    if (fuelConsumed < amountToAccumulate && fss.fuel > 0.f)
    {
      for (int i = 0; i < props.numTanks; ++i)
      {
        FuelTankState &fts = state.fuelTankStates[i];
        const FuelTankProps &ftp = props.fuelTankProps[i];
        if (ftp.fuelSystemNum == fuel_system_num && ftp.priority == p && fts.currentFuel > 0.f && fts.available)
        {
          float fuelToConsume = load_factor >= fsp.minimalLoadFactor ? min(amountToAccumulate - fuelConsumed, fts.currentFuel) : 0.0f;
          fss.fuelAccumulated += fuelToConsume;
          fuelConsumed += fuelToConsume;
          if (limited_fuel)
          {
            fts.currentFuel -= fuelToConsume;
            fss.fuel = max(0.0f, fss.fuel - fuelToConsume);
            if (ftp.external)
              fss.fuelExternal += fts.currentFuel;
          }
        }
        if (fuelConsumed >= amountToAccumulate)
          break;
      }
    }
  }
  fss.fuelAccumulated = max(fss.fuelAccumulated, 0.0f);
  fss.fuel += fss.fuelAccumulated;

  return fuelConsumed;
}

void Mass::computeMasses(dag::ConstSpan<PartMass> parts, float payload_cog_mult, float payload_im_mult,
  PartsPresenceFlags parts_presence_flags)
{
  TIME_PROFILE_DEV(mass_simulate_compute_masses)
  state.payloadMass = 0.f;
  computeFullMass();
  float prePayloadMass = state.mass;

  Point3 payloadCenterOfGravity(0.0f, 0.0f, 0.0f);
  state.payloadMomentOfInertia.zero();
  for (int i = 0; i < parts.size(); i++)
  {
    const Point3 &partCenter = parts[i].center;
    const float partMass = parts[i].mass;
    state.payloadMass += partMass;
    payloadCenterOfGravity += partCenter * partMass;
    state.payloadMomentOfInertia +=
      Point3(lengthSq(Point2::yz(partCenter)), lengthSq(Point2::xz(partCenter)), lengthSq(Point2::xy(partCenter))) * partMass;
  }

  computeFullMass();

  carray<bool, MAX_FUEL_TANKS> tanksFuelAdded;
  mem_set_0(tanksFuelAdded);
  int numTanksFuelAdded = 0;

  if (props.advancedMass)
  {
    state.centerOfGravity = payloadCenterOfGravity;
    const float invMass = safeinv(state.mass);
    for (const auto &part : props.advancedMasses)
    {
      if (part.fuelTankNum != PartMass::INVALID_TANK_NUM)
      {
        if (!tanksFuelAdded[part.fuelTankNum])
          ++numTanksFuelAdded;
        tanksFuelAdded[part.fuelTankNum] = true;
      }
      state.centerOfGravity += part.center * (part.mass + getFuelMassInTank(part.fuelTankNum));
    }
    state.centerOfGravity *= invMass;

    state.momentOfInertia = state.payloadMomentOfInertia;
    for (const auto &part : props.advancedMasses)
      state.momentOfInertia +=
        DPoint3::xyz(compute_moment_of_inertia(part, state.centerOfGravity, getFuelMassInTank(part.fuelTankNum)));
  }
  else
  {
    if (props.doesPayloadAffectCOG)
      state.centerOfGravity = (props.initialCenterOfGravity * prePayloadMass + payloadCenterOfGravity) * safeinv(state.mass);
    else
      state.centerOfGravity = (props.initialCenterOfGravity * prePayloadMass + payloadCenterOfGravity * payload_cog_mult) *
                              safeinv(state.mass + state.payloadMass * (-1.0f + payload_cog_mult));
    state.momentOfInertia =
      state.payloadMomentOfInertia * sqr(payload_im_mult) + DPoint3(props.momentOfInertiaNorm.x * state.momentOfInertiaNormMult.x,
                                                              props.momentOfInertiaNorm.y * state.momentOfInertiaNormMult.y,
                                                              props.momentOfInertiaNorm.z * state.momentOfInertiaNormMult.z) *
                                                              (state.mass - state.payloadMass);
  }

  if (!props.dynamicMasses.empty())
    if (parts_presence_flags < (1ll << props.dynamicMasses.size()) - 1ll || numTanksFuelAdded < props.numTanks)
    {
      float missedPartsMass = 0.0f;
      Point3 missedPartsCenterOfGravity = ZERO<Point3>();
      Point3 fuelCenterOfGravity = ZERO<Point3>();

      for (int i = 0; i < props.dynamicMasses.size(); ++i)
      {
        const PartMass &dynamicMass = props.dynamicMasses[i];
        if (((1ll << i) & parts_presence_flags) == 0)
        {
          const float tankFuelMass =
            dynamicMass.fuelTankNum != PartMass::INVALID_TANK_NUM ? state.fuelTankStates[dynamicMass.fuelTankNum].currentFuel : 0.0f;
          const float dynamicMassMass = dynamicMass.mass + tankFuelMass;
          if (missedPartsMass + dynamicMassMass > props.massEmpty)
            break;
          missedPartsMass += dynamicMassMass;
          missedPartsCenterOfGravity += dynamicMassMass * dynamicMass.center;
        }
        else if (dynamicMass.fuelTankNum != PartMass::INVALID_TANK_NUM && !tanksFuelAdded[dynamicMass.fuelTankNum])
        {
          const float tankFuelMass = state.fuelTankStates[dynamicMass.fuelTankNum].currentFuel;
          fuelCenterOfGravity += tankFuelMass * dynamicMass.center;
        }
      }

      const Point3 centerOfGravityPrev = state.centerOfGravity;
      state.centerOfGravity =
        (state.centerOfGravity * state.mass + fuelCenterOfGravity - missedPartsCenterOfGravity) / (state.mass - missedPartsMass);
      state.mass -= missedPartsMass;

      if (props.advancedMass)
      {
        state.momentOfInertia = state.payloadMomentOfInertia;
        for (const auto &part : props.advancedMasses)
          state.momentOfInertia +=
            DPoint3::xyz(compute_moment_of_inertia(part, state.centerOfGravity, getFuelMassInTank(part.fuelTankNum)));
      }
      else
      {
        Point3 distToNewCenter = state.centerOfGravity - centerOfGravityPrev;
        state.momentOfInertia = Point3(state.momentOfInertia.x + lengthSq(Point2::yz(distToNewCenter)) * state.mass,
          state.momentOfInertia.y + lengthSq(Point2::xz(distToNewCenter)) * state.mass,
          state.momentOfInertia.z + lengthSq(Point2::xy(distToNewCenter)) * state.mass);
      }
      float missedPartsMass2 = 0.0f;
      for (int i = 0; i < props.dynamicMasses.size(); ++i)
      {
        const PartMass &dynamicMass = props.dynamicMasses[i];
        if (((1ll << i) & parts_presence_flags) == 0)
        {
          const float tankFuelMass =
            dynamicMass.fuelTankNum != PartMass::INVALID_TANK_NUM ? state.fuelTankStates[dynamicMass.fuelTankNum].currentFuel : 0.0f;
          const float dynamicMassMass = dynamicMass.mass + tankFuelMass;
          if (missedPartsMass2 + dynamicMassMass > props.massEmpty)
            break;
          missedPartsMass2 += dynamicMassMass;
          const DPoint3 missedMomentOfInertia =
            DPoint3::xyz(compute_moment_of_inertia(dynamicMass, centerOfGravityPrev, tankFuelMass));
          if (state.momentOfInertia.x - missedMomentOfInertia.x < 0.0f || state.momentOfInertia.y - missedMomentOfInertia.y < 0.0f ||
              state.momentOfInertia.z - missedMomentOfInertia.z < 0.0f)
            break;
          state.momentOfInertia -= missedMomentOfInertia;
        }
        else if (dynamicMass.fuelTankNum != PartMass::INVALID_TANK_NUM && !tanksFuelAdded[dynamicMass.fuelTankNum])
        {
          const float tankFuelMass = state.fuelTankStates[dynamicMass.fuelTankNum].currentFuel;
          state.momentOfInertia += DPoint3::xyz(compute_moment_of_inertia(dynamicMass, state.centerOfGravity, tankFuelMass));
        }
      }
    }

  state.centerOfGravity.y = clamp(state.centerOfGravity.y, props.centerOfGravityClampY.x, props.centerOfGravityClampY.y);
}

void Mass::setFuel(float amount, int fuel_system_num, bool limit) { setFuelCustom(amount, true, true, fuel_system_num, limit); }

void Mass::clearFuel()
{
  for (int i = 0; i < props.numFuelSystems; ++i)
    setFuel(0.0f, i, true);
}

void Mass::addExtraMass(float extra_mass)
{
  props.maxWeight += extra_mass;
  props.massEmpty += extra_mass;
  state.mass += extra_mass;
}

bool Mass::hasFuel(float amount, int fuel_system_num) const
{
  float fuel = state.fuelSystemStates[fuel_system_num].fuelAccumulated;
  for (int i = 0; i < props.numTanks; ++i)
    if (props.fuelTankProps[i].fuelSystemNum == fuel_system_num && state.fuelTankStates[i].available)
      fuel += state.fuelTankStates[i].currentFuel;
  return fuel > amount;
}

float Mass::getFuelMassMax() const
{
  float maxFuel = 0.0f;
  for (int i = 0; i < props.numFuelSystems; ++i)
    maxFuel += props.fuelSystemProps[i].maxFuel;
  return maxFuel;
}

float Mass::getFuelMassMax(int fuel_system_num) const { return props.fuelSystemProps[fuel_system_num].maxFuel; }

float Mass::getFuelMassCurrent() const
{
  float fuel = 0.0f;
  for (int i = 0; i < props.numFuelSystems; ++i)
    fuel += state.fuelSystemStates[i].fuel;
  return fuel;
}

float Mass::getFuelMassCurrent(int fuel_system_num) const { return state.fuelSystemStates[fuel_system_num].fuel; }
