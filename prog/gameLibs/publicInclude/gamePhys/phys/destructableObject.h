//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <EASTL/unique_ptr.h>
#include <generic/dag_smallTab.h>
#include <dag/dag_vector.h>
#include <math/dag_Point4.h>
#include <gamePhys/collision/physLayers.h>
#include <gamePhys/phys/destructableRendObject.h>


// Fwd declaration
template <typename T>
class DynamicPhysObjectClass;
class PhysWorld;
class DynamicPhysObjectData;
class DataBlock;

namespace destructables
{
typedef void *id_t;
static constexpr id_t INVALID_ID = nullptr;
static constexpr int DESTRUCTABLES_DELETE_MAX_PER_FRAME = 10;
} // namespace destructables

namespace gamephys
{
struct DestructableObjectAddImpulse;
class DestructableObject
{
  PhysWorld *physWorld;
  SmallTab<float, MidmemAlloc> ttlBodies;
  float timeToFloat = -1.f;

public:
  float ttl = 65;
  int gen; // Note: intentionally not inited

private:
  friend struct gamephys::DestructableObjectAddImpulse;
  float timeToKinematic = 2;
  float defaultTimeToLive = 3;
  float scaleDt;
  void doAddImpulse(const Point3 &pos, const Point3 &impulse, float speedLimit);

public:
  static inline int numFloatable = 0;
  eastl::unique_ptr<DynamicPhysObjectClass<PhysWorld>> physObj;
  eastl::unique_ptr<destructables::DestrRendData, destructables::DestrRendDataDeleter> rendData;
  int resIdx;
  Point4 intialTmAndHash[4]; // row0-1 - initialTm(43), row3 = hash

  DestructableObject(DynamicPhysObjectData *po_data, float scale_dt, const TMatrix &tm, PhysWorld *phys_world, int res_idx,
    uint32_t hash_val, const DataBlock *blk);
  bool update(float dt, bool force_update_ttl); // return false if it need to be destroyed

  destructables::id_t getId() const { return (destructables::id_t)this; }

  int getNumActiveBodies() const;
  bool hasInteractableBodies() const;

  void addImpulse(const Point3 &pos, const Point3 &impulse, float speedLimit = 3.f);

  bool isFloatable() const { return timeToFloat >= 0.f; }
  void keepFloatable(float dt, float at_time);
  void setTimeToFloat(float time) { timeToFloat = time; }
  bool isAlive() const { return ttl > 0; }
  void markForDelete() { ttl = 0.f; }

  DynamicPhysObjectClass<PhysWorld> *getPhysObj() { return physObj.get(); }
};
}; // namespace gamephys

namespace destructables
{
extern int maxNumberOfDestructableBodies;
extern float minDestrRadiusSq;

void init(const DataBlock *blk, int fgroup = dacoll::EPL_DEBRIS);

id_t addDestructable(gamephys::DestructableObject **out_destr, DynamicPhysObjectData *po_data, const TMatrix &tm,
  PhysWorld *phys_world, const Point3 &cam_pos, int res_idx = -1, uint32_t hash_val = 0, const DataBlock *blk = nullptr);
void clear();
void removeDestructableById(id_t id);

void update(float dt);
dag::ConstSpan<gamephys::DestructableObject *> getDestructableObjects();
}; // namespace destructables
