// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <math/dag_Point3.h>
#include <gamePhys/phys/commonPhysBase.h>
#include <daECS/core/entityId.h>
#include "physEvents.h"

// atm only functions needed for net_phys_collision_es_impl are overrided semi-properly, the rest are stubbed

class NavMeshPhysProxy : public IPhysBase
{
  gamephys::Loc loc;
  float mass;
  Point3 velocity;
  Point3 pseudoVelDelta;
  float radius;
  float height;
  float timeStep;

public:
  static void *vtblPtr;

  // Warn: for perfomace reasons dtor isn't called (and ctor might get called multiple times)
  NavMeshPhysProxy(ecs::EntityId target_eid, float dt);
  ~NavMeshPhysProxy() = delete;

  void applyChangesTo(ecs::EntityId eid) const
  {
    g_entity_mgr->sendEventImmediate(eid, CmdNavPhysCollisionApply(&velocity, &pseudoVelDelta));
  }
  void validateTraceCache() override{};
  void updatePhys(float, float, bool) override{};
  int applyUnapprovedCTAsAt(int32_t, bool, int, uint8_t) override { return -1; };

  void applyOffset(const Point3 &) override{};
  void applyVelOmegaDelta(const DPoint3 &add_vel, const DPoint3 &add_omega) override;
  void applyPseudoVelOmegaDelta(const DPoint3 &add_pos, const DPoint3 &add_ori) override;

  float getMass() const override;
  DPoint3 calcInertia() const override;

  TraceMeshFaces *getTraceHandle() const override { return nullptr; };
  IPhysActor *getActor() const override { return nullptr; };

  dag::ConstSpan<CollisionObject> getCollisionObjects() const override;
  uint64_t getActiveCollisionObjectsBitMask() const override;
  TMatrix getCollisionObjectsMatrix() const override;
  dag::Span<CollisionObject> getMutableCollisionObjects() const override { return {}; };
  void prepareCollisions(daphys::SolverBodyInfo &,
    daphys::SolverBodyInfo &,
    bool,
    float,
    dag::Span<gamephys::CollisionContactData>,
    dag::Span<gamephys::SeqImpulseInfo>) const override{};
  const gamephys::Loc &getVisualStateLoc() const override { return loc; };
  const gamephys::Loc &getCurrentStateLoc() const override { return loc; };
  const DPoint3 &getCurrentStatePosition() const override { return loc.P; };
  DPoint3 getCurrentStateAccel() const override { return ZERO<DPoint3>(); };
  DPoint3 getCurrentStateVelocity() const override;
  DPoint3 getCurrentStateOmega() const override { return ZERO<DPoint3>(); };
  DPoint3 getPreviousStateVelocity() const override { return ZERO<DPoint3>(); };
  Point3 getCenterOfMass() const override;
  void addOverallShockImpulse(float) override{};
  float getOverallShockImpulse() const override { return -1; };
  void addSoundShockImpulse(float) override{};
  float getSoundShockImpulse() const override { return -1; };
  void addVolumetricDamage(const Point3 &, float, bool, void *, int) override{};
  void setPositionRough(const Point3 &) override{};
  void setOrientationRough(gamephys::Orient) override{};
  void stopRotationRough() override{};
  void setTmRough(TMatrix) override{};
  void setTmSoft(TMatrix) override{};
  void setVelocityRough(Point3) override{};
  void saveAllStates(int32_t) override{};
  void interpolateVisualPosition(float) override{};
  void calcPosVelAtTime(double, DPoint3 &, DPoint3 &) const override{};
  void calcQuatOmegaAtTime(double, Quat &, DPoint3 &) const override{};

  void setCurrentTick(int32_t) override{};
  int32_t getCurrentTick() const override { return -1; };
  int32_t getPreviousTick() const override { return -1; };
  int32_t getLastAppliedControlsForTick() const override { return -1; };

  float getTimeStep() const override;
  void setTimeStep(float) override{};
  float getMaxTimeDeferredControls() const override { return -1; };
  void setMaxTimeDeferredControls(float) override{};

  void resetProducedCt() override{};
  void setLocationFromTm(const TMatrix &) override{};
  void reset() override{};
  void repair() override{};

  bool receiveAuthorityApprovedState(const danet::BitStream &, uint8_t, float) override { return false; };
  bool receivePartialAuthorityApprovedState(const danet::BitStream &) override { return false; };
  void rescheduleAuthorityApprovedSend(int) override{};
  bool receiveControlsPacket(const danet::BitStream &, int, int32_t &) override { return false; };

  void reportDeserializationError(int) override{};

  bool isAsleep() const override { return false; };
  void wakeUp() override{};
  void putToSleep() override{};

  void setCurrentMinimalState(const gamephys::Loc &, const DPoint3 &, const DPoint3 &) override{};
  float getFriction() const override { return -1; };
  void setFriction(float) override{};
  float getBouncing() const override { return -1; };
  void setBouncing(float) override{};
};
