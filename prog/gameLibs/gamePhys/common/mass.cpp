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

using namespace gamephys;

PartMass::PartMass() { clear(); }

void PartMass::clear()
{
  mass = 0.f;
  eqArea = 0.f;
  momentOfInertia.zero();
  center.zero();
  fuelTankNum = INVALID_TANK_NUM;
  volumetric = false;
  fuelSystemNum = 0;
}

void PartMass::setMass(const Point3 &c, float m)
{
  clear();
  center = c;
  mass = m;
}

Point3 PartMass::computeMomentOfInertia(const Point3 &summary_center_of_mass, float additional_mass) const
{
  Point3 distToNewCenter = center - summary_center_of_mass;
  return Point3(momentOfInertia.x + lengthSq(Point2::yz(distToNewCenter)), momentOfInertia.y + lengthSq(Point2::xz(distToNewCenter)),
           momentOfInertia.z + lengthSq(Point2::xy(distToNewCenter))) *
         (mass + additional_mass);
}

FuelTank::FuelTank() { clear(); }

void FuelTank::clear()
{
  capacity = 0.f;
  currentFuel = 0.f;
  fuelSystemNum = 0;
  priority = 0;
  external = false;
  available = true;
}

Mass::FuelSystem::FuelSystem() { clear(); }

void Mass::FuelSystem::clear()
{
  fuel = maxFuel = 0.0f;
  fuelExternal = maxFuelExternal = 0.0f;
  fuelAccumulatorCapacity = fuelAccumulated = 0.0f;
  priorityNum = 1;
  minimalLoadFactor = -VERY_BIG_NUMBER;
  fuelAccumulatorFlowRate = 1.0e6f;
  fuelEngineFlowRate = 1.0e6f;
}

float Mass::calcConsumptionLimit(int fuel_system_num, float load_factor, float dt) const
{
  float amount = 0.0f;
  const FuelSystem &fuelSystem = fuelSystems[fuel_system_num];
  const float fuelToEngine = fuelSystem.fuelEngineFlowRate * dt;
  float remains = fuelToEngine - fuelSystem.fuelAccumulated;
  if (remains < 0.0f)
    amount = fuelToEngine;
  else
  {
    amount += fuelSystem.fuelAccumulated;
    if (load_factor >= fuelSystem.minimalLoadFactor)
    {
      const float fuelToAccumulator = fuelSystem.fuelAccumulatorFlowRate * dt;
      amount += min(remains, min(fuelSystem.fuel - fuelSystem.fuelAccumulated, fuelToAccumulator));
    }
  }
  return amount;
}

void Mass::computeFullMass()
{
  const float fuelMass = getFuelMassCurrent();
  mass = massEmpty + oilMass + crewMass + fuelMass + nitro + payloadMass;
}

Mass::Mass() : advancedMasses(midmem) { init(); }

void Mass::init()
{
  massEmpty = mass = 0.f;
  maxWeight = 0.f;
  payloadMass = 0.f;
  payloadMomentOfInertia.zero();

  for (int i = 0; i < MAX_FUEL_SYSTEMS; ++i)
    fuelSystems[i].clear();
  numFuelSystems = 1;
  separateFuelTanks = false;
  numTanks = 0;

  oilMass = 0.f;
  crewMass = 0.f;

  nitro = 0.f;
  maxNitro = 1.f;

  hasFuelDumping = false;
  fuelDumpingRate = 0.0f;

  advancedMass = false;
  doesPayloadAffectCOG = false;

  centerOfGravity.zero();
  initialCenterOfGravity.zero();
  centerOfGravityClampY.set(-VERY_BIG_NUMBER, VERY_BIG_NUMBER);

  momentOfInertiaNorm.zero();
  momentOfInertiaNormMult.set(1.0f, 1.0f, 1.0f);
  momentOfInertia.zero();
}

struct MassData
{
  Point3 center;
  float area;
  Point3 momentOfInertia;

  MassData(const Point3 &c, float a, const Point3 &inertia) : center(c), area(a), momentOfInertia(inertia) {}
};

static int sort_int(const int *lhs, const int *rhs) { return *lhs - *rhs; }

void Mass::loadMasses(const DataBlock *parts_masses_blk, const DataBlock *surface_parts_masses_blk, const CollisionResource *collision,
  const NameMap &fuel_tank_names, const DataBlock &additional_masses)
{
  clear_and_shrink(advancedMasses);
  clear_and_shrink(collisionNodesMasses);
  const int numTanksOld = numTanks;
  numTanks = 0;

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
      PartMass &currentMass = advancedMasses.push_back();
      currentMass.clear();
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
          PartMass &currentMass = advancedMasses.push_back();
          currentMass.clear();

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
          int fuelSystemNum = separateFuelTanks ? parts_masses_blk->getInt(String(128, "tank%d_system", tankNum + 1).str(), 0) : 0;
          if (fuelSystemNum < numFuelSystems && tankNum >= 0 && tankNum < MAX_FUEL_TANKS)
          {
            bool external =
              separateFuelTanks ? parts_masses_blk->getBool(String(128, "tank%d_external", tankNum + 1).str(), false) : 0;
            if (!external)
            {
              FuelSystem &fuelSystem = fuelSystems[fuelSystemNum];
              currentMass.fuelSystemNum = fuelSystemNum;
              currentMass.fuelTankNum = tankNum;
              FuelTank &ft = fuelTanks[tankNum];
              ft.clear();
              ft.fuelSystemNum = fuelSystemNum;
              ft.capacity = separateFuelTanks ? parts_masses_blk->getReal(String(128, "tank%d_capacity", tankNum + 1).str(), 0.f) : 0;
              knownFuel[fuelSystemNum] += ft.capacity;
              if (ft.capacity == 0.f)
                ++unknownFuel[fuelSystemNum];
              ++numTanks;
              ft.priority = separateFuelTanks ? parts_masses_blk->getInt(String(128, "tank%d_priority", tankNum + 1).str(), 0) : 0;
              fuelSystem.priorityNum = max(fuelSystem.priorityNum, ft.priority + 1);
            }
          }
          else
          {
            CollisionNodeMass &collisionNodeMass = collisionNodesMasses.push_back();
            collisionNodeMass.massIndex = advancedMasses.size() - 1;
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
    for (int i = 0; i < advancedMasses.size(); ++i)
    {
      if (advancedMasses[i].mass > 0.f)
        continue;
      advancedMasses[i].mass = max(massEmpty - totalKnownMass, 0.0f) * advancedMasses[i].eqArea * invTotalArea;
    }

    for (int i = 0; i < numTanks; ++i)
    {
      FuelTank &fuelTank = fuelTanks[i];
      FuelSystem &fuelSystem = fuelSystems[fuelTank.fuelSystemNum];
      if (fuelTank.capacity == 0.f)
        fuelTank.capacity =
          (fuelSystem.maxFuel - knownFuel[fuelTank.fuelSystemNum]) * safeinv(float(unknownFuel[fuelTank.fuelSystemNum]));
      fuelTank.currentFuel = min(fuelTank.currentFuel, fuelTank.capacity);
    }

    // External fuel tanks

    for (int tankNum = 0; tankNum < MAX_FUEL_TANKS; ++tankNum)
    {
      float capacity = parts_masses_blk->getReal(String(128, "tank%d_capacity", tankNum + 1).str(), 0.f);
      int fuelSystemNum = parts_masses_blk->getInt(String(128, "tank%d_system", tankNum + 1).str(), 0);
      bool external = parts_masses_blk->getBool(String(128, "tank%d_external", tankNum + 1).str(), false);
      if (capacity > 0.0f && fuelSystemNum < numFuelSystems && external)
      {
        FuelTank &fuelTank = fuelTanks[tankNum];
        fuelTank.capacity = capacity;
        fuelTank.currentFuel = min(fuelTank.currentFuel, capacity);
        fuelTank.fuelSystemNum = fuelSystemNum;
        fuelTank.priority = parts_masses_blk->getInt(String(128, "tank%d_priority", tankNum + 1).str(), 0);
        FuelSystem &fuelSystem = fuelSystems[fuelTank.fuelSystemNum];
        fuelSystem.priorityNum = max(fuelSystem.priorityNum, fuelTank.priority + 1);
        fuelTank.external = true;
        ++numTanks;
        if (numTanks > numTanksOld)
          fuelTank.available = false;
      }
    }

    // Ensure that fuel system capacity is a summ of the connected fuel tanks capcities
    for (int i = 0; i < numFuelSystems; ++i)
      fuelSystems[i].maxFuel = fuelSystems[i].maxFuelExternal = fuelSystems[i].fuel = fuelSystems[i].fuelExternal = 0.0f;
    for (int i = 0; i < numTanks; ++i)
    {
      const FuelTank &fuelTank = fuelTanks[i];
      FuelSystem &fuelSystem = fuelSystems[fuelTank.fuelSystemNum];
      fuelSystem.maxFuel += fuelTank.capacity;
      fuelSystem.fuel += fuelTank.currentFuel;
      if (fuelTank.external)
      {
        fuelSystem.maxFuelExternal += fuelTank.capacity;
        fuelSystem.fuelExternal += fuelTank.currentFuel;
      }
    }
  }
}

void Mass::baseLoad(const DataBlock *mass_blk, int crew_num)
{
  massEmpty = mass_blk->getReal("Empty", 0.f);
  oilMass = mass_blk->getReal("Oil", 0.f);
  crewMass = crew_num * 90.f;
  maxWeight = mass_blk->getReal("TakeOff", 0.f);

  for (int i = 0; i < MAX_FUEL_SYSTEMS; ++i)
    fuelSystems[i].clear();
  fuelSystems[MAIN_FUEL_SYSTEM].maxFuel = mass_blk->getReal("Fuel", 0.f);
  fuelSystems[MAIN_FUEL_SYSTEM].maxFuelExternal = 0.0f;
  numFuelSystems = 1;
  for (int i = 0; i < MAX_FUEL_SYSTEMS; ++i)
  {
    fuelSystems[i].maxFuel = mass_blk->getReal(String(16, "Fuel%d", i).str(), i == MAIN_FUEL_SYSTEM ? fuelSystems[i].maxFuel : 0.0f);
    fuelSystems[i].maxFuelExternal = 0.0f;
    if (fuelSystems[i].maxFuel > 0.0f && i > MAIN_FUEL_SYSTEM)
    {
      ++numFuelSystems;
      fuelSystems[i].fuel = fuelSystems[i].maxFuel;
      fuelSystems[i].fuelExternal = fuelSystems[i].maxFuelExternal;
    }
  }

  advancedMass = mass_blk->getBool("AdvancedMass", false);
  doesPayloadAffectCOG = mass_blk->getBool("doesPayloadAffectCOG", false);
  separateFuelTanks = mass_blk->getBool("SeparateFuelTanks", false);
  initialCenterOfGravity = mass_blk->getPoint3("CenterOfGravity", Point3(0.f, 0.f, 0.f));
  centerOfGravity = initialCenterOfGravity;
  centerOfGravityClampY = mass_blk->getPoint2("CenterOfGravityClampY", Point2(-VERY_BIG_NUMBER, VERY_BIG_NUMBER));

  hasFuelDumping = mass_blk->getBool("hasFuelDumping", false);
  fuelDumpingRate = mass_blk->getReal("fuelDumpingRate", -1.0f);

  mass = massEmpty;
  maxNitro = nitro = mass_blk->getReal("Nitro", 0.0f);
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
  carray<float, MAX_FUEL_SYSTEMS> fuel, fuelExternal;
  for (int i = 0; i < MAX_FUEL_SYSTEMS; ++i)
  {
    fuel[i] = numFuelSystems <= MAX_FUEL_SYSTEMS ? fuelSystems[i].fuel : 0.0f;
    fuelExternal[i] = numFuelSystems <= MAX_FUEL_SYSTEMS ? fuelSystems[i].fuelExternal : 0.0f;
  }

  blkutil::loadSaveBlk(mass_blk, "EmptyMass", massEmpty, 0.0f, load);

  if (load)
  {
    fuelSystems[MAIN_FUEL_SYSTEM].maxFuel = mass_blk->getReal("MaxFuelMass", 0.0f);
    fuelSystems[MAIN_FUEL_SYSTEM].maxFuelExternal = mass_blk->getReal("MaxFuelMassExternal", 0.0f);
    numFuelSystems = 1;
  }
  for (int i = 0; i < (load ? MAX_FUEL_SYSTEMS : numFuelSystems); ++i)
  {
    blkutil::loadSaveBlk(mass_blk, String(32, "MaxFuelMass%d", i).str(), fuelSystems[i].maxFuel,
      i == MAIN_FUEL_SYSTEM ? fuelSystems[i].maxFuel : 0.0f, load);
    blkutil::loadSaveBlk(mass_blk, String(32, "MaxFuelMassExternal%d", i).str(), fuelSystems[i].maxFuelExternal,
      i == MAIN_FUEL_SYSTEM ? fuelSystems[i].maxFuelExternal : 0.0f, load);
    if (load && fuelSystems[i].maxFuel > 0.0f && i > 0)
      ++numFuelSystems;
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
  for (int i = 0; i < numFuelSystems; ++i)
  {
    blkutil::loadSaveBlk(mass_blk, String(32, "FuelAccumulatorCapacity%d", i).str(), fuelSystems[i].fuelAccumulatorCapacity, 0.0f,
      load);
    if (load)
      fuelSystems[i].fuelAccumulated = min(fuelSystems[i].fuelAccumulated, fuelSystems[i].fuelAccumulatorCapacity);
    blkutil::loadSaveBlk(mass_blk, String(32, "MinimalLoadFactor%d", i).str(), fuelSystems[i].minimalLoadFactor, minimalLoadFactor,
      load);
    blkutil::loadSaveBlk(mass_blk, String(32, "FuelAccumulatorFlowRate%d", i).str(), fuelSystems[i].fuelAccumulatorFlowRate,
      fuelAccumulatorFlowRate, load);
    blkutil::loadSaveBlk(mass_blk, String(32, "FuelEngineFlowRate%d", i).str(), fuelSystems[i].fuelEngineFlowRate, fuelEngineFlowRate,
      load);
  }

  blkutil::loadSaveBlk(mass_blk, "hasFuelDumping", hasFuelDumping, false, load);
  blkutil::loadSaveBlk(mass_blk, "fuelDumpingRate", fuelDumpingRate, -1.0f, load);

  blkutil::loadSaveBlk(mass_blk, "MaxNitro", maxNitro, 0.0f, load);
  nitro = maxNitro;
  blkutil::loadSaveBlk(mass_blk, "OilMass", oilMass, 0.0f, load);
  blkutil::loadSaveBlk(mass_blk, "AdvancedMass", advancedMass, false, load);
  blkutil::loadSaveBlk(mass_blk, "doesPayloadAffectCOG", doesPayloadAffectCOG, false, load);
  blkutil::loadSaveBlk(mass_blk, "SeparateFuelTanks", separateFuelTanks, false, load);
  blkutil::loadSaveBlk(mass_blk, "Takeoff", maxWeight, 0.0f, load);
  blkutil::loadSaveBlk(mass_blk, "CenterOfGravity", initialCenterOfGravity, Point3(0.f, 0.f, 0.f), load);
  centerOfGravity = initialCenterOfGravity;

  if (load)
  {
    clear_and_shrink(dynamicMasses);
    dynamicMasses.clear();

    const int partBlockNameId = mass_blk->getNameId("Part");
    int partBlockNum = -1;
    while ((partBlockNum = mass_blk->findBlock(partBlockNameId, partBlockNum)) >= 0)
    {
      const DataBlock *partBlk = mass_blk->getBlock(partBlockNum);
      PartMass &dynamicMass = dynamicMasses.push_back();
      dynamicMass.mass = partBlk->getReal("mass", 0.0f);
      dynamicMass.center = partBlk->getPoint3("center", ZERO<Point3>());
      dynamicMass.momentOfInertia = partBlk->getPoint3("inertiaMoment", ZERO<Point3>()) * safeinv(dynamicMass.mass);
    }
  }
  else
  {
    for (const auto &dynamicMass : dynamicMasses)
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
    loadMasses(partsMassesBlk, partsWithSurfaceMassesBlk, collision, fuel_tank_names, additional_masses);
    for (int i = 0; i < numFuelSystems; ++i)
    {
      setFuelInternal(min(fuel[i] - fuelExternal[i], fuelSystems[i].maxFuel - fuelSystems[i].maxFuelExternal), i, true);
      setFuelExternal(min(fuelExternal[i], fuelSystems[i].maxFuelExternal), i, true);
    }
    for (int i = numFuelSystems; i < MAX_FUEL_SYSTEMS; ++i)
      fuelSystems[i].clear();
  }
  else
  {
    if (collision != NULL)
    {
      if (separateFuelTanks)
      {
        for (int i = 0; i < numTanks; ++i)
        {
          FuelTank &fuelTank = fuelTanks[i];
          if (fuelTank.capacity != 0.f)
            partsMassesBlk->setReal(String(128, "tank%d_capacity", i + 1).str(), fuelTank.capacity);
          partsMassesBlk->setInt(String(128, "tank%d_system", i + 1).str(), fuelTank.fuelSystemNum);
          partsMassesBlk->setBool(String(128, "tank%d_external", i + 1).str(), fuelTank.external);
          partsMassesBlk->setInt(String(128, "tank%d_priority", i + 1).str(), fuelTank.priority);
        }
      }
      dag::ConstSpan<CollisionNode> allCollisionNodes = collision->getAllNodes();
      for (int i = 0; i < collisionNodesMasses.size(); ++i)
      {
        const CollisionNodeMass &collisionNodeMass = collisionNodesMasses[i];
        const PartMass &partMass = advancedMasses[collisionNodeMass.massIndex];
        DataBlock *blk = partMass.volumetric ? partsMassesBlk : partsWithSurfaceMassesBlk;
        blk->setReal(allCollisionNodes[collisionNodeMass.nodeIndex].name.str(), partMass.mass);
      }
    }
  }
}

void Mass::repair()
{
  nitro = maxNitro;
  momentOfInertiaNormMult.set(1.0f, 1.0f, 1.0f);
}

bool Mass::consumeFuel(float amount, bool limited_fuel, int fuel_system_num, float load_factor, float dt)
{
  const float fuelConsumed = consumeFuelAmount(amount, limited_fuel, fuel_system_num, load_factor, dt);
  return fuelSystems[fuel_system_num].fuelAccumulated > 0.0f || fuelConsumed >= amount - 1.0e-6f;
}

float Mass::consumeFuelAmount(float amount, bool limited_fuel, int fuel_system_num, float load_factor, float dt)
{
  // Consumption fuel from the fuel accumulator
  FuelSystem &fuelSystem = fuelSystems[fuel_system_num];
  fuelSystem.fuelAccumulated -= min(amount, fuelSystem.fuelEngineFlowRate * dt);

  // Recharging the fuel accumulator
  float amountToAccumulate =
    min(fuelSystem.fuelAccumulatorCapacity - fuelSystem.fuelAccumulated, fuelSystem.fuelAccumulatorFlowRate * dt);
  float fuelConsumed = 0.f;
  fuelSystem.fuel = 0.0f;
  fuelSystem.fuelExternal = 0.0f;

  for (int p = 0; p < fuelSystem.priorityNum; ++p)
  {
    float oldFuel = 0.f;
    for (int i = 0; i < numTanks; ++i)
    {
      const FuelTank &fuelTank = fuelTanks[i];
      if (fuelTank.fuelSystemNum == fuel_system_num && fuelTank.priority == p && fuelTank.available)
        oldFuel += fuelTank.currentFuel;
    }
    const float oldFuelInv = safeinv(oldFuel);
    for (int i = 0; i < numTanks; ++i)
    {
      FuelTank &fuelTank = fuelTanks[i];
      if (fuelTank.fuelSystemNum == fuel_system_num && fuelTank.priority == p && fuelTank.currentFuel > 0.f && fuelTank.available)
      {
        if (fuelConsumed < amountToAccumulate)
        {
          const float fuelToConsume = load_factor >= fuelSystem.minimalLoadFactor
                                        ? min(amountToAccumulate * fuelTank.currentFuel * oldFuelInv, fuelTank.currentFuel)
                                        : 0.0f;
          fuelSystem.fuelAccumulated += fuelToConsume;
          fuelConsumed += fuelToConsume;
          if (limited_fuel)
            fuelTank.currentFuel -= fuelToConsume;
        }
        fuelSystem.fuel += fuelTank.currentFuel;
        if (fuelTank.external)
          fuelSystem.fuelExternal += fuelTank.currentFuel;
      }
    }
    if (fuelConsumed < amountToAccumulate && fuelSystem.fuel > 0.f)
    {
      for (int i = 0; i < numTanks; ++i)
      {
        FuelTank &fuelTank = fuelTanks[i];
        if (fuelTank.fuelSystemNum == fuel_system_num && fuelTank.priority == p && fuelTank.currentFuel > 0.f && fuelTank.available)
        {
          float fuelToConsume =
            load_factor >= fuelSystem.minimalLoadFactor ? min(amountToAccumulate - fuelConsumed, fuelTank.currentFuel) : 0.0f;
          fuelSystem.fuelAccumulated += fuelToConsume;
          fuelConsumed += fuelToConsume;
          if (limited_fuel)
          {
            fuelTank.currentFuel -= fuelToConsume;
            fuelSystem.fuel = max(0.0f, fuelSystem.fuel - fuelToConsume);
            if (fuelTank.external)
              fuelSystem.fuelExternal += fuelTank.currentFuel;
          }
        }
        if (fuelConsumed >= amountToAccumulate)
          break;
      }
    }
  }
  fuelSystem.fuelAccumulated = max(fuelSystem.fuelAccumulated, 0.0f);
  fuelSystem.fuel += fuelSystem.fuelAccumulated;

  computeFullMass();

  return fuelConsumed;
}

float Mass::leakFuel(float amount, int tank_num)
{
  FuelTank &fuelTank = fuelTanks[tank_num];
  FuelSystem &fuelSystem = fuelSystems[fuelTank.fuelSystemNum];

  float consumedFuel = min(amount, fuelTank.currentFuel);
  fuelTank.currentFuel -= consumedFuel;
  float fuel = 0.0f;
  fuelSystem.fuel = 0.0f;
  fuelSystem.fuelExternal = 0.0f;
  for (int i = 0; i < numTanks; ++i)
  {
    if (fuelTanks[i].fuelSystemNum == fuelTank.fuelSystemNum && fuelTanks[i].available)
    {
      fuelSystem.fuel += fuelTanks[i].currentFuel;
      if (fuelTanks[i].external)
        fuelSystem.fuelExternal += fuelTanks[i].currentFuel;
    }
    fuel += fuelTanks[i].currentFuel;
  }
  for (int i = 0; i < numFuelSystems; ++i)
  {
    fuel += fuelSystems[i].fuelAccumulated;
    fuelSystems[i].fuel += fuelSystems[i].fuelAccumulated;
  }
  computeFullMass();
  return consumedFuel;
}


float Mass::dumpFuel(float amount)
{
  static const int dumpFuelSystem = 0;
  eastl::vector_set<eastl::pair<float, int>> tanksToDumpFrom;

  for (int i = 0; i < numTanks; ++i)
  {
    if (fuelTanks[i].fuelSystemNum == dumpFuelSystem && fuelTanks[i].available && fuelTanks[i].currentFuel > 0.0f)
    {
      tanksToDumpFrom.insert({fuelTanks[i].currentFuel, i});
    }
  }

  float fuelLeftToDump = amount;

  while (!tanksToDumpFrom.empty())
  {
    auto curIt = tanksToDumpFrom.begin();

    FuelTank &fuelTank = fuelTanks[curIt->second];

    float curAmountToDump = fuelLeftToDump / tanksToDumpFrom.size();

    if (fuelTank.currentFuel > curAmountToDump)
    {
      fuelTank.currentFuel -= curAmountToDump;
      fuelLeftToDump -= curAmountToDump;
    }
    else
    {
      fuelLeftToDump -= fuelTank.currentFuel;
      fuelTank.currentFuel = 0.0f;
    }

    tanksToDumpFrom.erase(curIt);
  }

  FuelSystem &fuelSystem = fuelSystems[dumpFuelSystem];

  fuelSystem.fuel = 0.0f;
  fuelSystem.fuelExternal = 0.0f;

  for (int i = 0; i < numTanks; ++i)
  {
    if (fuelTanks[i].fuelSystemNum == dumpFuelSystem && fuelTanks[i].available)
    {
      fuelSystem.fuel += fuelTanks[i].currentFuel;
      if (fuelTanks[i].external)
        fuelSystem.fuelExternal += fuelTanks[i].currentFuel;
    }
  }

  computeFullMass();
  return amount - fuelLeftToDump;
}

bool Mass::consumeNitro(float amount)
{
  float fuel = 0.f;
  for (int i = 0; i < numTanks; ++i)
    if (fuelTanks[i].available)
      fuel += fuelTanks[i].currentFuel;
  nitro = max(nitro - amount, 0.f);
  computeFullMass();
  return nitro > 0.f;
}

void Mass::computeMasses(dag::ConstSpan<PartMass> parts, float payload_cog_mult, float payload_im_mult,
  PartsPresenceFlags parts_presence_flags)
{
  payloadMass = 0.f;
  computeFullMass();
  float prePayloadMass = mass;

  Point3 payloadCenterOfGravity(0.0f, 0.0f, 0.0f);
  payloadMomentOfInertia.zero();
  for (int i = 0; i < parts.size(); i++)
  {
    const Point3 &partCenter = parts[i].center;
    const float partMass = parts[i].mass;
    payloadMass += partMass;
    payloadCenterOfGravity += partCenter * partMass;
    payloadMomentOfInertia +=
      Point3(lengthSq(Point2::yz(partCenter)), lengthSq(Point2::xz(partCenter)), lengthSq(Point2::xy(partCenter))) * partMass;
  }

  computeFullMass();

  carray<bool, MAX_FUEL_TANKS> tanksFuelAdded;
  mem_set_0(tanksFuelAdded);
  int numTanksFuelAdded = 0;

  if (advancedMass)
  {
    centerOfGravity = payloadCenterOfGravity;
    const float invMass = safeinv(mass);
    for (const auto &part : advancedMasses)
    {
      if (part.fuelTankNum != PartMass::INVALID_TANK_NUM)
      {
        if (!tanksFuelAdded[part.fuelTankNum])
          ++numTanksFuelAdded;
        tanksFuelAdded[part.fuelTankNum] = true;
      }
      centerOfGravity += part.center * (part.mass + getFuelMassInTank(part.fuelTankNum));
    }
    centerOfGravity *= invMass;

    momentOfInertia = payloadMomentOfInertia;
    for (const auto &part : advancedMasses)
      momentOfInertia += DPoint3::xyz(part.computeMomentOfInertia(centerOfGravity, getFuelMassInTank(part.fuelTankNum)));
  }
  else
  {
    if (doesPayloadAffectCOG)
      centerOfGravity = (initialCenterOfGravity * prePayloadMass + payloadCenterOfGravity) * safeinv(mass);
    else
      centerOfGravity = (initialCenterOfGravity * prePayloadMass + payloadCenterOfGravity * payload_cog_mult) *
                        safeinv(mass + payloadMass * (-1.0f + payload_cog_mult));
    momentOfInertia = payloadMomentOfInertia * sqr(payload_im_mult) + DPoint3(momentOfInertiaNorm.x * momentOfInertiaNormMult.x,
                                                                        momentOfInertiaNorm.y * momentOfInertiaNormMult.y,
                                                                        momentOfInertiaNorm.z * momentOfInertiaNormMult.z) *
                                                                        (mass - payloadMass);
  }

  if (!dynamicMasses.empty())
    if (parts_presence_flags < (1ll << dynamicMasses.size()) - 1ll || numTanksFuelAdded < numTanks)
    {
      float missedPartsMass = 0.0f;
      Point3 missedPartsCenterOfGravity = ZERO<Point3>();
      Point3 fuelCenterOfGravity = ZERO<Point3>();

      for (int i = 0; i < dynamicMasses.size(); ++i)
      {
        const PartMass &dynamicMass = dynamicMasses[i];
        if (((1ll << i) & parts_presence_flags) == 0)
        {
          const float tankFuelMass =
            dynamicMass.fuelTankNum != PartMass::INVALID_TANK_NUM ? fuelTanks[dynamicMass.fuelTankNum].currentFuel : 0.0f;
          const float dynamicMassMass = dynamicMass.mass + tankFuelMass;
          if (missedPartsMass + dynamicMassMass > massEmpty)
            break;
          missedPartsMass += dynamicMassMass;
          missedPartsCenterOfGravity += dynamicMassMass * dynamicMass.center;
        }
        else if (dynamicMass.fuelTankNum != PartMass::INVALID_TANK_NUM && !tanksFuelAdded[dynamicMass.fuelTankNum])
        {
          const float tankFuelMass = fuelTanks[dynamicMass.fuelTankNum].currentFuel;
          fuelCenterOfGravity += tankFuelMass * dynamicMass.center;
        }
      }

      const Point3 centerOfGravityPrev = centerOfGravity;
      centerOfGravity = (centerOfGravity * mass + fuelCenterOfGravity - missedPartsCenterOfGravity) / (mass - missedPartsMass);
      mass -= missedPartsMass;

      if (advancedMass)
      {
        momentOfInertia = payloadMomentOfInertia;
        for (const auto &part : advancedMasses)
          momentOfInertia += DPoint3::xyz(part.computeMomentOfInertia(centerOfGravity, getFuelMassInTank(part.fuelTankNum)));
      }
      else
      {
        Point3 distToNewCenter = centerOfGravity - centerOfGravityPrev;
        momentOfInertia = Point3(momentOfInertia.x + lengthSq(Point2::yz(distToNewCenter)) * mass,
          momentOfInertia.y + lengthSq(Point2::xz(distToNewCenter)) * mass,
          momentOfInertia.z + lengthSq(Point2::xy(distToNewCenter)) * mass);
      }
      float missedPartsMass2 = 0.0f;
      for (int i = 0; i < dynamicMasses.size(); ++i)
      {
        const PartMass &dynamicMass = dynamicMasses[i];
        if (((1ll << i) & parts_presence_flags) == 0)
        {
          const float tankFuelMass =
            dynamicMass.fuelTankNum != PartMass::INVALID_TANK_NUM ? fuelTanks[dynamicMass.fuelTankNum].currentFuel : 0.0f;
          const float dynamicMassMass = dynamicMass.mass + tankFuelMass;
          if (missedPartsMass2 + dynamicMassMass > massEmpty)
            break;
          missedPartsMass2 += dynamicMassMass;
          const DPoint3 missedMomentOfInertia = DPoint3::xyz(dynamicMass.computeMomentOfInertia(centerOfGravityPrev, tankFuelMass));
          if (momentOfInertia.x - missedMomentOfInertia.x < 0.0f || momentOfInertia.y - missedMomentOfInertia.y < 0.0f ||
              momentOfInertia.z - missedMomentOfInertia.z < 0.0f)
            break;
          momentOfInertia -= missedMomentOfInertia;
        }
        else if (dynamicMass.fuelTankNum != PartMass::INVALID_TANK_NUM && !tanksFuelAdded[dynamicMass.fuelTankNum])
        {
          const float tankFuelMass = fuelTanks[dynamicMass.fuelTankNum].currentFuel;
          momentOfInertia += DPoint3::xyz(dynamicMass.computeMomentOfInertia(centerOfGravity, tankFuelMass));
        }
      }
    }

  centerOfGravity.y = clamp(centerOfGravity.y, centerOfGravityClampY.x, centerOfGravityClampY.y);
}

void Mass::setFuelCustom(float amount, bool internal, bool external, int fuel_system_num, bool limit)
{
  FuelSystem &fuelSystem = fuelSystems[fuel_system_num];
  fuelSystem.fuel = 0.0f;
  fuelSystem.fuelExternal = 0.0f;
  float amountToDistribute;
  if (internal)
  {
    fuelSystem.fuelAccumulated = min(amount, fuelSystem.fuelAccumulatorCapacity);
    amountToDistribute = amount - fuelSystem.fuelAccumulated;
  }
  else
    amountToDistribute = amount;
  for (int p = fuelSystem.priorityNum - 1; p >= 0; --p)
  {
    float maxFuel = 0.0f;
    if (internal)
      maxFuel += fuelSystem.maxFuel - fuelSystem.maxFuelExternal;
    if (external)
      maxFuel += fuelSystem.maxFuelExternal;
    if (fuelSystem.priorityNum > 1)
    {
      maxFuel = 0.0f;
      for (int i = 0; i < numTanks; ++i)
      {
        const FuelTank &fuelTank = fuelTanks[i];
        if (fuelTank.fuelSystemNum == fuel_system_num && fuelTank.priority == p && fuelTank.available &&
            (fuelTank.external ? external : internal))
          maxFuel += fuelTank.capacity;
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
    for (int i = 0; i < numTanks; ++i)
    {
      FuelTank &fuelTank = fuelTanks[i];
      if (fuelTank.fuelSystemNum == fuel_system_num && fuelTank.priority == p && fuelTank.available)
      {
        if (fuelTank.external ? external : internal)
          fuelTank.currentFuel = percentage * fuelTank.capacity;
        fuelSystem.fuel += fuelTank.currentFuel;
        if (fuelTank.external)
          fuelSystem.fuelExternal += fuelTank.currentFuel;
      }
    }
  }
}

void Mass::setFuel(float amount, int fuel_system_num, bool limit) { setFuelCustom(amount, true, true, fuel_system_num, limit); }

void Mass::clearFuel()
{
  for (int i = 0; i < numFuelSystems; ++i)
    setFuel(0.0f, i, true);
}

bool Mass::hasFuel(float amount, int fuel_system_num) const
{
  float fuel = fuelSystems[fuel_system_num].fuelAccumulated;
  for (int i = 0; i < numTanks; ++i)
    if (fuelTanks[i].fuelSystemNum == fuel_system_num && fuelTanks[i].available)
      fuel += fuelTanks[i].currentFuel;
  return fuel > amount;
}

float Mass::getFuelMassMax() const
{
  float maxFuel = 0.0f;
  for (int i = 0; i < numFuelSystems; ++i)
    maxFuel += fuelSystems[i].maxFuel;
  return maxFuel;
}

float Mass::getFuelMassMax(int fuel_system_num) const { return fuelSystems[fuel_system_num].maxFuel; }

float Mass::getFuelMassCurrent() const
{
  float fuel = 0.0f;
  for (int i = 0; i < numFuelSystems; ++i)
    fuel += fuelSystems[i].fuel;
  return fuel;
}

float Mass::getFuelMassCurrent(int fuel_system_num) const { return fuelSystems[fuel_system_num].fuel; }
