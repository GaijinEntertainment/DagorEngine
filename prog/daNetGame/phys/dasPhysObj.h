// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/unique_ptr.h>
#include <EASTL/numeric_limits.h>
#include "netPhys.h"
#include <daECS/core/entityComponent.h>
#include <memory/dag_fixedBlockAllocator.h>

struct DasPhysObjExtState
{
  uint16_t extStateSerSz = 0; // size of serializeable part (<= extStateMemSz)
  uint16_t extStateMemSz = 0;
  void *extState = nullptr;

  DasPhysObjExtState() = default;
  DasPhysObjExtState(const DasPhysObjExtState &other)
  {
    memcpy(this, &other, sizeof(other));
    if (other.extState)
    {
      extState = allocExtState(extStateMemSz, *(FixedBlockAllocator **)((char *)extState + extStateMemSz));
      memcpy(extState, other.extState, extStateMemSz);
    }
  }
  DasPhysObjExtState(DasPhysObjExtState &&other)
  {
    memcpy(this, &other, sizeof(other));
    other.extState = nullptr;
  }
  DasPhysObjExtState &operator=(const DasPhysObjExtState &other)
  {
    if (this != &other)
    {
      freeExtState();
      memcpy(this, &other, sizeof(other));
      if (other.extState)
      {
        extState = allocExtState(extStateMemSz, *(FixedBlockAllocator **)((char *)extState + extStateMemSz));
        memcpy(extState, other.extState, extStateMemSz);
      }
    }
    return *this;
  }
  DasPhysObjExtState &operator=(DasPhysObjExtState &&other)
  {
    eastl::swap(extStateSerSz, other.extStateSerSz);
    eastl::swap(extStateMemSz, other.extStateMemSz);
    eastl::swap(extState, other.extState);
    return *this;
  }
  ~DasPhysObjExtState() { freeExtState(); }

private:
  friend class DasPhysObj;
  friend struct DasPhysObjControlsState;
  friend struct DasPhysObjState;
  void *newExtState(int ser_sz, int mem_sz, FixedBlockAllocator &pool)
  {
    int sz = (eastl::max(ser_sz, mem_sz) + sizeof(void *) - 1) & ~(sizeof(void *) - 1);
    G_ASSERTF(sz <= eastl::numeric_limits<decltype(extStateMemSz)>::max(), "%d", sz);
    freeExtState();
    if (sz)
      extState = allocExtState(sz, &pool);
    extStateSerSz = ser_sz;
    extStateMemSz = sz;
    // Note: intentionally not zeroed for sake of perfomance
    return extState;
  }
  void *allocExtState(int sz, FixedBlockAllocator *pool)
  {
    G_FAST_ASSERT(sz);
    G_FAST_ASSERT(pool);
    if (uint32_t blksz = pool->getBlockSize())
      G_ASSERTF((sz + sizeof(void *)) == blksz, "%d != %d", sz + sizeof(void *), blksz);
    else
      pool->init(sz + sizeof(void *), /*min_chunk_size_in_blocks*/ 16); // TODO: tune this const according to real usage
    void *ret = pool->allocateOneBlock();
    *(FixedBlockAllocator **)((char *)ret + sz) = pool;
    return ret;
  }
  void freeExtState()
  {
    if (extState)
    {
      G_FAST_ASSERT(extStateMemSz);
      (*(FixedBlockAllocator **)((char *)extState + extStateMemSz))->freeOneBlock(extState);
      extState = nullptr;
    }
  }
};

struct DasPhysObjStateBase
{
  int32_t atTick = -1;
  int32_t lastAppliedControlsForTick = -2; // < atTick

  gamephys::Loc location;
  Point3 velocity = Point3(0.f, 0.f, 0.f);
  Point3 omega = Point3(0.f, 0.f, 0.f);
  Point3 alpha = Point3(0.f, 0.f, 0.f);
  Point3 acceleration = Point3(0.f, 0.f, 0.f);

private:
  friend class DasPhysObj;
};

struct DasPhysObjState : public DasPhysObjStateBase, public DasPhysObjExtState
{
  bool canBeCheckedForSync = true;

  void *physStateRaw() { return extState; }
  const void *physStateRawRO() const { return extState; }

  void reset();

  void serialize(danet::BitStream &bs) const;
  bool deserialize(const danet::BitStream &bs, IPhysBase &);

  void applyAlternativeHistoryState(const DasPhysObjState & /*state*/) {}
  void applyPartialState(const CommonPhysPartialState &state);
  void applyDesyncedState(const DasPhysObjState & /*state*/);

  bool operator==(const DasPhysObjState &a) const
  {
    G_ASSERTF(!extStateSerSz != !a.extStateSerSz || extStateSerSz == a.extStateSerSz, "%d %d", extStateSerSz, a.extStateSerSz);
    bool equals = location.P == a.location.P && location.O.getQuat() == a.location.O.getQuat() && velocity == a.velocity &&
                  omega == a.omega && acceleration == a.acceleration && extStateSerSz == a.extStateSerSz &&
                  memcmp(extState, a.extState, extStateSerSz) == 0;
    return equals;
  }

  bool hasLargeDifferenceWith(const DasPhysObjState &a) const
  {
    const Point3 &ypr = location.O.getYPR();
    const Point3 &aYpr = a.location.O.getYPR();

    const float ANGLE_THRESHOLD = 0.002f;

    bool isLargeDifference = lengthSq(location.P - a.location.P) > 0.01f || fabsf(ypr[0] - aYpr[0]) > ANGLE_THRESHOLD ||
                             fabsf(ypr[1] - aYpr[1]) > ANGLE_THRESHOLD || fabsf(ypr[2] - aYpr[2]) > ANGLE_THRESHOLD ||
                             lengthSq(velocity - a.velocity) > 0.1f || lengthSq(omega - a.omega) > 0.01f ||
                             lengthSq(acceleration - a.acceleration) > 0.1f;
    return isLargeDifference;
  }
};

struct DasPhysObjControlsStateBase
{
  int32_t producedAtTick = -1;
  int sequenceNumber = 0;

private:
  friend class DasPhysObj;
};

struct DasPhysObjControlsState : public DasPhysObjControlsStateBase, public DasPhysObjExtState
{
  bool isForged = false;

  void *controlsRaw() { return extState; }
  const void *controlsRawRO() const { return extState; }

  void setControlsForged(bool val) { isForged = val; }
  bool isControlsForged() const { return isForged; }
  int getSequenceNumber() const { return sequenceNumber; }
  void setSequenceNumber(int seq) { sequenceNumber = seq; }

  void serialize(danet::BitStream &bs) const;
  bool deserialize(const danet::BitStream &bs, IPhysBase &, int32_t &);
  void interpolate(const DasPhysObjControlsState &prev, const DasPhysObjControlsState &, float, int) { *this = prev; }
  void serializeMinimalState(danet::BitStream &, const DasPhysObjControlsState &) const
  { /*noop*/
  }
  bool deserializeMinimalState(const danet::BitStream &, const DasPhysObjControlsState &)
  { /*noop*/
    return true;
  }
  void applyMinimalState(const DasPhysObjControlsState &) { G_ASSERT(0); }

  int getUnitVersion() const { return 0; }
  void setUnitVersion(int) {}
  void reset();
  void init() { reset(); }
  void stepHistory() {}
};

// Note: not relocatable since controls/state contains pointer to physics
#define DAS_PHYSOBJ_ACTOR PhysActor<DasPhysObj, DummyCustomPhys, PhysType::DAS_PHYSOBJ>

class DasPhysObj final : public PhysicsBase<DasPhysObjState, DasPhysObjControlsState, CommonPhysPartialState>
{
  FixedBlockAllocator controlsPool, physStatesPool;
  friend struct DasPhysObjControlsState;
  friend struct DasPhysObjState;

public:
  CollisionObject collObj;
  float mass = 80.f;

  void *allocControls(DasPhysObjControlsState &ctrl, int sz) { return ctrl.newExtState(sz, 0, controlsPool); }
  void *allocControlsEx(DasPhysObjControlsState &ctrl, int ser_sz, int mem_sz)
  {
    return ctrl.newExtState(ser_sz, mem_sz, controlsPool);
  }
  void *allocPhysState(DasPhysObjState &phstate, int sz) { return phstate.newExtState(sz, 0, physStatesPool); }
  void *allocPhysStateEx(DasPhysObjState &phstate, int ser_sz, int mem_sz)
  {
    return phstate.newExtState(ser_sz, mem_sz, physStatesPool);
  }

  DasPhysObj(ptrdiff_t physactor_offset, PhysVars *phys_vars, float time_step);
  ~DasPhysObj();

  void loadFromBlk(const DataBlock *blk, const CollisionResource *collision, const char *unit_name, bool is_player);
  void updatePhys(float at_time, float dt, bool is_for_real) override final;
  void reset() override;

  void applyOffset(const Point3 &) override {}
  float getMass() const override { return mass; }
  DPoint3 calcInertia() const override { return DPoint3(2.f, 0.5f, 2.f) * getMass(); }
  dag::ConstSpan<CollisionObject> getCollisionObjects() const override;
  virtual Point3 getCenterOfMass() const override { return Point3(0.f, 0.5f, 0.f); }
};

extern template class DAS_PHYSOBJ_ACTOR;
typedef DAS_PHYSOBJ_ACTOR DasPhysObjActor;
#undef DAS_PHYSOBJ_ACTOR

ECS_DECLARE_PHYS_ACTOR_TYPE(DasPhysObjActor);
