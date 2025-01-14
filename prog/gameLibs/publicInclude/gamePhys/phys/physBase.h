//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

namespace gamephys
{
struct Loc;
struct Orient;
struct CollisionContactData;
struct SeqImpulseInfo;
} // namespace gamephys
namespace danet
{
class BitStream;
}
namespace daphys
{
struct SolverBodyInfo;
}
class DataBlock;
class IPhysActor;
class CollisionResource;
struct CollisionObject;
class PhysVars;
struct TraceMeshFaces;


class IPhysBase
{
public:
  virtual void validateTraceCache() = 0;
  virtual void updatePhys(float at_time, float dt, bool is_for_real) = 0;
  virtual int applyUnapprovedCTAsAt(int32_t tick, bool do_remove_old, int starting_at_index, uint8_t unit_version) = 0;

  virtual void applyOffset(const Point3 &offset) = 0;
  virtual void applyVelOmegaDelta(const DPoint3 &add_vel, const DPoint3 &add_omega) = 0;
  virtual void applyPseudoVelOmegaDelta(const DPoint3 &add_pos, const DPoint3 &add_ori) = 0;

  virtual float getMass() const = 0;
  virtual DPoint3 calcInertia() const = 0;

  virtual TraceMeshFaces *getTraceHandle() const = 0;
  virtual IPhysActor *getActor() const = 0;

  virtual dag::ConstSpan<CollisionObject> getCollisionObjects() const = 0;
  virtual uint64_t getActiveCollisionObjectsBitMask() const = 0;
  virtual TMatrix getCollisionObjectsMatrix() const = 0;
  virtual dag::Span<CollisionObject> getMutableCollisionObjects() const = 0;
  virtual void prepareCollisions(daphys::SolverBodyInfo &body1, daphys::SolverBodyInfo &body2, bool first_body, float friction,
    dag::Span<gamephys::CollisionContactData> contacts, dag::Span<gamephys::SeqImpulseInfo> collisions) const = 0;
  virtual const gamephys::Loc &getVisualStateLoc() const = 0;
  virtual const gamephys::Loc &getCurrentStateLoc() const = 0;
  virtual const DPoint3 &getCurrentStatePosition() const = 0;
  virtual DPoint3 getCurrentStateVelocity() const = 0;
  virtual DPoint3 getCurrentStateAccel() const = 0;
  virtual DPoint3 getCurrentStateOmega() const = 0;
  virtual DPoint3 getPreviousStateVelocity() const = 0;
  virtual Point3 getCenterOfMass() const = 0;
  virtual void addOverallShockImpulse(float val) = 0;
  virtual float getOverallShockImpulse() const = 0;
  virtual void addSoundShockImpulse(float val) = 0;
  virtual float getSoundShockImpulse() const = 0;
  virtual void addVolumetricDamage(const Point3 &local_point, float impulse, bool collision, void *user_ptr, int offender_id = -1) = 0;

  virtual void setPositionRough(const Point3 &position) = 0;
  virtual void setOrientationRough(gamephys::Orient orientation) = 0;
  virtual void stopRotationRough() = 0;
  virtual void setTmRough(TMatrix tm) = 0;
  virtual void setTmSoft(TMatrix tm) = 0;
  virtual void setVelocityRough(Point3 velocity) = 0;
  virtual void saveAllStates(int32_t state_tick) = 0;
  virtual void interpolateVisualPosition(float at_time) = 0;
  virtual void calcPosVelAtTime(double time, DPoint3 &out_pos, DPoint3 &out_vel) const = 0;
  virtual void calcQuatOmegaAtTime(double at_time, Quat &out_quat, DPoint3 &out_omega) const = 0;

  virtual void setCurrentTick(int32_t at_tick) = 0;
  virtual int32_t getCurrentTick() const = 0;
  virtual int32_t getPreviousTick() const = 0;
  virtual int32_t getLastAppliedControlsForTick() const = 0;

  virtual float getTimeStep() const = 0;
  virtual void setTimeStep(float ts) = 0;
  virtual float getMaxTimeDeferredControls() const = 0;
  virtual void setMaxTimeDeferredControls(float time) = 0;

  virtual void resetProducedCt() = 0;
  virtual void setLocationFromTm(const TMatrix &tm) = 0;
  virtual void reset() = 0;
  virtual void repair() = 0;

  virtual bool receiveAuthorityApprovedState(const danet::BitStream &bs, uint8_t unit_version, float from_time) = 0;
  virtual bool receivePartialAuthorityApprovedState(const danet::BitStream &bs) = 0;
  virtual void rescheduleAuthorityApprovedSend(int ticks_threshold = 4) = 0;
  virtual bool receiveControlsPacket(const danet::BitStream &bs, int max_queue_size, int32_t &out_anomaly_flags) = 0;

  virtual void reportDeserializationError(int err) = 0;

  virtual bool isAsleep() const = 0;
  virtual void wakeUp() = 0;
  virtual void putToSleep() = 0;

  virtual void setCurrentMinimalState(const gamephys::Loc &loc, const DPoint3 &vel, const DPoint3 &omega) = 0;
  virtual float getFriction() const = 0;
  virtual void setFriction(float value) = 0;
  virtual float getBouncing() const = 0;
  virtual void setBouncing(float value) = 0;

  // virtual const PhysVars* getPhysVars() const = 0;
};
