//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <gamePhys/phys/unitPhysBase.h>
#include <gamePhys/collision/collisionObject.h>
#include <gamePhys/collision/contactData.h>
#include <gamePhys/collision/collisionInfo.h>
#include <generic/dag_staticTab.h>

constexpr int MAX_CACHED_CONTACTS = 8;

namespace rendinst
{
struct RendInstDesc;
}

struct PhysObjState
{
  int32_t atTick = -1;
  int32_t lastAppliedControlsForTick = -2; // < atTick
  bool canBeCheckedForSync = true;

  gamephys::Loc location;
  Point3 velocity = Point3(0.f, 0.f, 0.f);
  Point3 omega = Point3(0.f, 0.f, 0.f);
  Point3 alpha = Point3(0.f, 0.f, 0.f);
  Point3 acceleration = Point3(0.f, 0.f, 0.f);
  Point3 addForce = ZERO<Point3>();
  Point3 addMoment = ZERO<Point3>();
  Point3 contactPoint = ZERO<Point3>();
  Point3 gravDirection = Point3(0, -1, 0);
  bool logCCD = false;
  bool isSleep = false;
  float sleepTimer = 0.f;
  bool hadContact = false;
  float ignoreGameObjsUntilTime = -1.0f; // -1 - not initialized, 0 - ignore forever

  StaticTab<gamephys::CachedContact, MAX_CACHED_CONTACTS> cachedContacts;

  void reset();

  void serialize(danet::BitStream &bs) const;
  bool deserialize(const danet::BitStream &bs, IPhysBase &);

  void applyAlternativeHistoryState(const PhysObjState & /*state*/) {}
  void applyPartialState(const CommonPhysPartialState &state);
  void applyDesyncedState(const PhysObjState & /*state*/);

  bool operator==(const PhysObjState &a) const
  {
    static constexpr float posThresholdSq = 1e-4f;
    static constexpr float velThresholdSq = 0.01f;
    static constexpr float omegaThresholdSq = 1e-4f;
    static constexpr float cosDirThreshold = 0.998629534754f; // cos of 3.0 degrees
    static constexpr float yprThreshold = 0.05f;

    if (lengthSq(location.P - a.location.P) > posThresholdSq)
      return false;

    const float velSq = velocity.lengthSq();
    const float aVelSq = a.velocity.lengthSq();

    if (fabsf(velSq - aVelSq) > velThresholdSq)
      return false;
    if (velSq > 1e-12f && (velocity / sqrtf(velSq)) * (a.velocity * safeinv(sqrtf(aVelSq))) < cosDirThreshold)
      return false;

    const Point3 &ypr = location.O.getYPR();
    const Point3 &aYpr = a.location.O.getYPR();

    bool equals = lengthSq(omega - a.omega) < omegaThresholdSq && fabsf(ypr[0] - aYpr[0]) < yprThreshold &&
                  fabsf(ypr[1] - aYpr[1]) < yprThreshold && fabsf(ypr[2] - aYpr[2]) < yprThreshold && addForce == a.addForce &&
                  addMoment == a.addMoment;
    return equals;
  }

  bool hasLargeDifferenceWith(const PhysObjState &a) const
  {
    const Point3 &ypr = location.O.getYPR();
    const Point3 &aYpr = a.location.O.getYPR();

    const float ANGLE_THRESHOLD = 0.002f;

    bool isLargeDifference = lengthSq(location.P - a.location.P) > 0.01f || fabsf(ypr[0] - aYpr[0]) > ANGLE_THRESHOLD ||
                             fabsf(ypr[1] - aYpr[1]) > ANGLE_THRESHOLD || fabsf(ypr[2] - aYpr[2]) > ANGLE_THRESHOLD ||
                             lengthSq(velocity - a.velocity) > 0.1f || lengthSq(omega - a.omega) > 0.01f ||
                             lengthSq(alpha - a.alpha) > 0.1f || lengthSq(acceleration - a.acceleration) > 0.1f;
    return isLargeDifference;
  }
};

struct PhysObjControlState
{
  int32_t producedAtTick = -1;
  int sequenceNumber = 0;
  int getSequenceNumber() const { return sequenceNumber; }
  void setSequenceNumber(int seq) { sequenceNumber = seq; }
  uint8_t unitVersion = 0;
  bool isForged = false;
  void setControlsForged(bool val) { isForged = val; }
  bool isControlsForged() const { return isForged; }
  // TODO: add any external forces here (which should be checked of course)

  danet::BitStream customControls;

  DEFINE_COMMON_PHYS_CONTROLS_METHODS(PhysObjControlState);
  void init() { reset(); }
  void stepHistory() {}
};

struct CCDCheck
{
  Point3 from;
  Point3 to;
  Point3 offset;
  bool res = false;

  CCDCheck(const Point3 &in_from, const Point3 &in_to, const Point3 &in_offset) :
    from(in_from), to(in_to), offset(in_offset), res(lengthSq(in_offset) > 0.f)
  {}
};

class PhysObj final : public PhysicsBase<PhysObjState, PhysObjControlState, CommonPhysPartialState>
{
public:
  float mass = 1.f;
  float objDestroyMass = 1.f;
  float linearDamping = 0.f;
  float angularDamping = 0.f;
  float friction = 0.f;
  float bouncing = 0.f;
  float limitFriction = 0.f;
  float frictionGround = 0.f;
  float bouncingGround = 0.f;
  float minSpeedForBounce = 0.f;
  float boundingRadius = -1.f;
  float soundShockImpulse = 0.f;
  float linearSlop = 0.01f;
  float energyConservation = 1.0f;
  float erp = 1.f;
  float baumgarteBias = 0.f;
  float warmstartingFactor = 0.f;
  float gravityMult = 1.f;
  Point3 momentOfInertia = Point3(1.f, 1.f, 1.f);
  Point3 velocityLimit = Point3(0.f, 0.f, 0.f);
  Point3 velocityLimitDampingMult = Point3(-1.f, -1.f, -1.f);
  Point3 omegaLimit = Point3(0.f, 0.f, 0.f);
  Point3 omegaLimitDampingMult = Point3(-1.f, -1.f, -1.f);
  bool isVelocityLimitAbsolute = false;
  bool isOmegaLimitAbsolute = false;
  bool useFutureContacts = false;
  bool useMovementDamping = true;
  bool hasGroundCollisionPoint = false;
  bool hasRiDestroyingCollision = false;
  bool hasFrtCollision = false;
  bool shouldFallThroughGround = false;
  bool shouldTraceGround = false;
  bool addToWorld = false;
  bool skipUpdateOnSleep = false;
  bool ignoreCollision = false;
  float noSleepAtTheSlopeCos = -1.f; // [-1; 1], where -1 disables the feature
  float sleepDelay = 1.f;
  float omegaMovementDamping = 0.9f;
  float omegaMovementDampingThreshold = 0.1f;
  float movementDampingThreshold = 0.01f;
  int sleepUpdateFrequency = 0;
  int physMatId = -1;
  Point3 centerOfMass = Point3(0.f, 0.f, 0.f);

  Tab<CollisionObject> collision;
  Tab<BSphere3> ccdSpheres;
  Tab<CCDCheck> ccdLog;
  Tab<gamephys::CollisionContactData> contactsLog;

  TMatrix collisionNodeTm = TMatrix::IDENT;
  Tab<int> collisionNodes;

  Point3 groundCollisionPoint = ZERO<Point3>();
  Point3 frtCollisionNormal = ZERO<Point3>();

  Tab<int> ignoreGameObjs;
  float ignoreGameObjsTime = 0.0f; // 0 - ignore forever

  PhysObj(ptrdiff_t physactor_offset, PhysVars *phys_vars, float time_step);
  ~PhysObj();

  void loadFromBlk(const DataBlock *blk, const CollisionResource *collision, const char *unit_name, bool is_player);
  void loadPhysParams(const DataBlock *blk);
  void loadLimits(const DataBlock *blk);
  void loadSleepSettings(const DataBlock *blk);
  void loadSolver(const DataBlock *blk);
  void loadCollisionSettings(const DataBlock *blk, const CollisionResource *coll_res);

  virtual void updatePhys(float at_time, float dt, bool is_for_real) override final;
  virtual dag::ConstSpan<CollisionObject> getCollisionObjects() const { return collision; }
  virtual uint64_t getActiveCollisionObjectsBitMask() const { return ~0ull; }
  virtual dag::Span<CollisionObject> getMutableCollisionObjects() const { return make_span(const_cast<PhysObj *>(this)->collision); }
  virtual void applyOffset(const Point3 &offset);
  virtual void addSoundShockImpulse(float val);
  virtual float getSoundShockImpulse() const;
  void updatePhysInWorld(const TMatrix &tm) override;

  void resolveCollision(float dt, dag::ConstSpan<gamephys::CollisionContactData> contacts);

  virtual bool isAsleep() const override final { return currentState.isSleep; }
  virtual void putToSleep() override final
  {
    currentState.isSleep = true;
    currentState.velocity.zero();
    currentState.omega.zero();
  }
  virtual void wakeUp() override final
  {
    currentState.isSleep = false;
    currentState.sleepTimer = 0.f;
  }

  virtual void setCurrentMinimalState(const gamephys::Loc &loc, const DPoint3 &vel, const DPoint3 &omega) override
  {
    currentState.location = loc;
    currentState.velocity = vel;
    currentState.omega = omega;
  }

  virtual void reset();
  virtual float getMass() const { return mass; }
  virtual DPoint3 calcInertia() const { return dpoint3(momentOfInertia * getMass()); }
  virtual Point3 getCenterOfMass() const { return centerOfMass; }
  void drawDebug();

  void addForce(const Point3 &arm, const Point3 &force);

  virtual float getFriction() const { return friction; }
  virtual void setFriction(float value) { friction = value; }

  virtual float getBouncing() const { return bouncing; }
  virtual void setBouncing(float value) { bouncing = value; }

  TMatrix getCollisionObjectsMatrix() const override;
  bool isCollisionValid() const;
};
