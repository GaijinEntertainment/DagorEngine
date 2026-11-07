// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "physKindState_api.h"
#include <gamePhys/collision/collisionObject.h>
#include <generic/dag_relocatableFixedVector.h>

class PhysKind;
class DataBlock;
class CollisionResource;

class IPhysKindHost
{
public:
  virtual ~IPhysKindHost() = default;
  virtual void step(PhysKind &phys, int32_t tick, float dt, bool is_for_real) = 0;
  // Owner corrections keep motion, others place exactly
  virtual void applyState(PhysKind &phys, bool keep_body_motion) = 0;
};

class PhysKind final : public PhysicsBase<PhysKindState, PhysKindControls, CommonPhysPartialState>
{
public:
  IPhysKindHost *host = nullptr;
  float mass = 1.f;
  Point3 centerOfMass = Point3(0.f, 0.f, 0.f);
  Point3 momentOfInertia = Point3(1.f, 1.f, 1.f);
  bool external = false;
  bool shadowSim = false;
  uint8_t snapExtSize[(int)PhysKindLod::Num] = {
    PHYS_KIND_SNAP_EXT_MAX, PHYS_KIND_SNAP_EXT_MAX, PHYS_KIND_SNAP_EXT_MAX, PHYS_KIND_SNAP_EXT_MAX};
  // A state wire takes the place of snapExtSize; the AAS has the ext as it is
  const PhysKindWire *stateWire = nullptr;
  const PhysKindWire *controlsWire = nullptr;

  using PhysicsBase::PhysicsBase;
  ~PhysKind();

  void loadFromBlk(const DataBlock *blk, const CollisionResource *coll_res, const char *unit_name, bool is_player);
  void addCollision(const DataBlock &props);
  void addCollisionBox(const TMatrix &tm, const Point3 &width);

  void addCollisionToWorld() override;
  void updatePhys(double at_time, float dt, bool is_for_real);
  void applyState(bool keep_body_motion = false);
  // false and a logerr for a bad field or a snapshot over PHYS_KIND_SNAP_EXT_MAX; empty fields clear the layout: raw bytes
  bool setWire(dag::ConstSpan<PhysKindWireField> state_fields, dag::ConstSpan<PhysKindWireField> controls_fields);

  dag::ConstSpan<CollisionObject> getCollisionObjects() const override { return collision; }
  TMatrix getCollisionObjectsMatrix() const override { return currentState.location.makeTM(); }
  void applyOffset(const Point3 &offset) override { currentState.location.P += dpoint3(offset); }
  float getMass() const override { return mass; }
  DPoint3 calcInertia() const override { return dpoint3(momentOfInertia * mass); }
  Point3 getCenterOfMass() const override { return centerOfMass; }

  bool isAsleep() const override { return currentState.isSleep(); }
  void putToSleep() override;
  void wakeUp() override { currentState.flags &= ~PhysKindState::FLAG_SLEEP; }
  void setCurrentMinimalState(const gamephys::Loc &loc, const DPoint3 &vel, const DPoint3 &omega) override;
  void reset() override;

private:
  dag::RelocatableFixedVector<CollisionObject, 1> collision;
  bool collisionInWorld = false;
};

extern template class PhysicsBase<PhysKindState, PhysKindControls, CommonPhysPartialState>;
