//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
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

struct DestructableCreationParams
{
  DynamicPhysObjectData *physObjData = nullptr;
  TMatrix tm = TMatrix::IDENT;
  // set out-of-bounds position, so if camera pos is not set,
  // it would count as being very far
  Point3 camPos = Point3(1e6f, 1e6f, 1e6f);
  float scaleDt = -1.0f;

  int resIdx = -1;
  uint32_t hashVal = 0;

  float timeToLive = -1.0f;
  float defaultTimeToLive = -1.0f;
  float timeToKinematic = -1.0f;
};
} // namespace destructables

namespace gamephys
{
struct DestructableObjectAddImpulse;
class DestructableObject
{
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
  void doAddImpulse(const Point3 &pos, const Point3 &impulse, float speedLimit, float omegaLimit);

public:
  static inline int numFloatable = 0;
  eastl::unique_ptr<DynamicPhysObjectClass<PhysWorld>> physObj;
  eastl::unique_ptr<destructables::DestrRendData, destructables::DestrRendDataDeleter> rendData;
  int resIdx;
  Point4 intialTmAndHash[4]; // row0-1 - initialTm(43), row3 = hash

  DestructableObject(const destructables::DestructableCreationParams &params, PhysWorld *phys_world, float scale_dt);
  bool update(float dt, bool force_update_ttl); // return false if it need to be destroyed

  destructables::id_t getId() const { return (destructables::id_t)this; }

  int getNumActiveBodies() const;
  bool hasInteractableBodies() const;

  void addImpulse(PhysWorld &pw, const Point3 &pos, const Point3 &impulse, float speedLimit = 7.f, float omegaLimit = 5.f);

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

id_t addDestructable(gamephys::DestructableObject **out_destr, const DestructableCreationParams &params, PhysWorld *phys_world);
id_t addDestructable(gamephys::DestructableObject **out_destr, DynamicPhysObjectData *po_data, const TMatrix &tm,
  PhysWorld *phys_world, const Point3 &cam_pos, int res_idx = -1, uint32_t hash_val = 0, const DataBlock *blk = nullptr);
void clear();
void removeDestructableById(id_t id);

void update(float dt);
dag::ConstSpan<gamephys::DestructableObject *> getDestructableObjects();
}; // namespace destructables
