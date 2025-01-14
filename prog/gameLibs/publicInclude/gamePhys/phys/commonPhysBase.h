//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <EASTL/utility.h>
#include <EASTL/unique_ptr.h>
#include <math/dag_mathAng.h>
#include <dag/dag_vector.h>
#include <generic/dag_tab.h>
#include <daNet/bitStream.h>
#include <gamePhys/phys/physBase.h>
#include <gamePhys/common/loc.h>
#include <gamePhys/collision/collisionObject.h>
#include <gamePhys/phys/utils.h>
#include <gamePhys/phys/physActor.h>

namespace dacoll
{
extern void set_obj_motion(CollisionObject obj, const TMatrix &tm, const Point3 &vel, const Point3 &omega);
}

class ICustomPhysStateSyncer
{
  template <typename P, typename C, typename PA>
  friend class PhysicsBase;
  ICustomPhysStateSyncer *next = nullptr; // Link list
public:
  virtual void saveHistoryState(int tick) = 0;
  virtual void saveAuthState(int tick) = 0;
  virtual bool isHistoryStateEqualAuth(int htick, int atick) = 0;
  virtual void serializeAuthState(danet::BitStream &bs, int tick) const = 0;
  virtual bool deserializeAuthState(const danet::BitStream &bs, int tick) = 0;
  virtual void eraseHistoryStateTail(int tick) = 0;
  virtual void eraseHistoryStateHead(int tick) = 0;
  virtual void applyAuthState(int at_tick) = 0;
  virtual void clear() = 0;
};

class ExtrapolatedPhysState
{
public:
  gamephys::Loc location; // tm
  float atTime;
  DPoint3 velocity;     // Vwld  // Vector of speed in the world
  DPoint3 acceleration; // Accel
  DPoint3 omega;        // W
  DPoint3 alpha;        // AW

  ExtrapolatedPhysState()
  {
    atTime = 0.f;

    // location has it's own constructor
    velocity.zero();
    acceleration.zero();
    omega.zero();
    alpha.zero();
  }

  template <typename PhysState>
  void copyFrom(const PhysState &state, float time)
  {
    location = state.location;
    atTime = time;
    velocity = state.velocity;
    acceleration = state.acceleration;
    omega = state.omega;
    alpha = state.alpha;
  }
};

extern void write_controls_tick_delta(danet::BitStream &bs, int tick, int ctrlTick);
extern bool read_controls_tick_delta(const danet::BitStream &bs, int tick, int &ctrlTick);

struct CommonPhysPartialState
{
  gamephys::Loc location;
  DPoint3 velocity;
  int atTick = -1;
  int lastAppliedControlsForTick = -2; // < atTick
  uint8_t unitVersion;
  void setUnitVersion(uint8_t v) { unitVersion = v; }
  uint8_t getUnitVersion() const { return unitVersion; }
  bool isProcessed;

  CommonPhysPartialState()
  {
    velocity.zero();
    atTick = -1;
    unitVersion = -1;
    isProcessed = true;
  }

  void serialize(danet::BitStream &bs) const;
  bool deserialize(const danet::BitStream &bs, IPhysBase &);
};

struct ResyncState
{
  bool isCorrectedStateObtained = false;
  bool isNewCorrrectionRequired = false;

  int32_t needUpdateToTick = 0;
  int maxGraceTick = 0;
  float stateTimeBefore = 0.f;

  ExtrapolatedPhysState correctedExtrapolatedState;
};

struct PhysDesyncStats
{
  uint32_t processed{};
  uint32_t unmatched{};       // No state to calculate difference found
  uint32_t untestable{};      // At least one of differing states is not marked as canBeCheckedForSync
  uint32_t smallDifference{}; // Small (unnoticeable) desync is detected, while sync is expected
  uint32_t largeDifference{}; // Large (visible) desync is detected, while sync is expected
  uint32_t dups{};            // Duplicate states
  gamephys::Loc lastPosDifference{};

  void reset() { *this = PhysDesyncStats{}; }
  uint32_t desyncs() const { return untestable + smallDifference + largeDifference; }
};

#define DEFAULT_EXTRAPOLATION_TIME_MULT 2.0f

#define ALL_COLLISION_OBJECTS ~0ull

template <typename PhysState, typename ControlState, typename PartialState>
class PhysicsBase : public IPhysBase
{
public:
  typedef PhysState PhysStateBase;
  typedef PartialState PhysPartialState;
  ptrdiff_t physActorOffset; // Note: offset from this (see `getActor`)

  PhysStateBase previousState;
  PhysStateBase currentState;

  dag::Vector<PhysStateBase> historyStates;

  gamephys::Loc visualLocation;
  gamephys::Loc visualLocationError;
  float visualLocationErrorProductionTime;

  ControlState appliedCT;
  ControlState producedCT;
  Tab<ControlState> unapprovedCT;
  int lastCTSequenceNumber;
  unsigned numProcessedCT = 0;
  unsigned numForgedCT = 0;

  danet::BitStream *fmsyncPreviousData;
  danet::BitStream *fmsyncCurrentData;

  // On server - allocated on first attempt to send it
  // On client - allocated on first receive
  eastl::unique_ptr<PhysStateBase> authorityApprovedState;
  ICustomPhysStateSyncer *customPhysStateSyncer = nullptr;
  ExtrapolatedPhysState extrapolatedState;

  bool isAuthorityApprovedStateProcessed = true;
  PhysPartialState authorityApprovedPartialState;
  uint8_t authorityApprovedStateUnitVersion;
  float authorityApprovedStateDumpFromTime;
  int32_t lastProcessedAuthorityApprovedStateAtTick;

  float physicsTimeToSendState;
  float physicsTimeToSendPartialState;
  int32_t lastAuthorityApprovedStateSentAtTick;
  float timeStep;
  float maxTimeDeferredControls = 0.350f;
  const float extrapolationTimeMult;

  PhysDesyncStats desyncStats;
  PhysDesyncStats partialStateDesyncStats;

  PhysicsBase(ptrdiff_t physactor_offset, PhysVars * /*phys_vars*/, float time_step,
    float extrapolation_time_mult = DEFAULT_EXTRAPOLATION_TIME_MULT);
  ~PhysicsBase();

  void validateTraceCache() override {}

  int applyUnapprovedCTAsAt(int32_t tick, bool doRemoveOld, int startingAtIndex, uint8_t unitVersion)
  {
    startingAtIndex = max(0, startingAtIndex);
    for (int i = startingAtIndex + 1; i < unapprovedCT.size(); ++i)
    {
      if (unapprovedCT[i].producedAtTick > tick)
      {
        int lastSkippedIndex = i - 1;
        if (unapprovedCT[lastSkippedIndex].getUnitVersion() == unitVersion)
          appliedCT = unapprovedCT[lastSkippedIndex];
        if (doRemoveOld && lastSkippedIndex > 1)
        {
          ++numProcessedCT;
          erase_items(unapprovedCT, 0, lastSkippedIndex - 1); // -1 to keep previous controls before applied one (some physics might
                                                              // need them)
          return 1;
        }
        return lastSkippedIndex;
      }
    }
    if (startingAtIndex < unapprovedCT.size())
    {
      ControlState *ctrlst = unapprovedCT.data() + startingAtIndex;
      if (ctrlst->producedAtTick >= tick)
      {
        if (ctrlst->getUnitVersion() == unitVersion)
          appliedCT = *ctrlst;
        if (doRemoveOld && startingAtIndex > 1)
        {
          ++numProcessedCT;
          erase_items(unapprovedCT, 0, startingAtIndex - 1);
          return 1;
        }
        return startingAtIndex;
      }
      else
      {
        G_ASSERT(!unapprovedCT.empty());
        ControlState &last = unapprovedCT.back();
        if (last.getUnitVersion() == unitVersion)
          appliedCT = last;
        if (doRemoveOld && unapprovedCT.size() > 2)
        {
          ++numProcessedCT;
          erase_items(unapprovedCT, 0, unapprovedCT.size() - 2);
          return 1;
        }
        return (int)unapprovedCT.size() - 1;
      }
    }
    return -1;
  }


  virtual const gamephys::Loc &getVisualStateLoc() const final override { return visualLocation; }
  virtual const gamephys::Loc &getCurrentStateLoc() const final override { return currentState.location; }
  virtual const DPoint3 &getCurrentStatePosition() const final override { return currentState.location.P; }
  virtual DPoint3 getCurrentStateVelocity() const final override { return DPoint3::xyz(currentState.velocity); }
  virtual DPoint3 getCurrentStateAccel() const final override { return DPoint3::xyz(currentState.acceleration); }
  virtual DPoint3 getCurrentStateOmega() const final override { return DPoint3::xyz(currentState.omega); }
  virtual DPoint3 getPreviousStateVelocity() const final override { return DPoint3::xyz(previousState.velocity); }

  virtual void addOverallShockImpulse(float /*val*/) {}
  virtual float getOverallShockImpulse() const { return 0.f; }
  virtual void addSoundShockImpulse(float /*val*/) {}
  virtual float getSoundShockImpulse() const { return 0.f; }
  virtual void addVolumetricDamage(const Point3 & /*local_point*/, float /*impulse*/, bool /*collision*/, void * /*user_ptr*/,
    int /*offender_id = -1*/)
  {}
  void applyVelOmegaDelta(const DPoint3 &add_vel, const DPoint3 &add_omega) override final
  {
    currentState.velocity += add_vel;
    currentState.omega += add_omega;
  }
  void __forceinline applyPseudoVelOmegaDeltaImpl(const DPoint3 &add_pos, const DPoint3 &add_ori, DPoint3 *out_ccd_add = nullptr)
  {
    G_ASSERT(lengthSq(add_pos) < sqr(1000.f));
    DPoint3 centerOfGravityBefore = dpoint3(currentState.location.O.getQuat() * getCenterOfMass());
    Quat orientInc = Quat(add_ori, length(add_ori));
    currentState.location.O.setQuat(currentState.location.O.getQuat() * orientInc);
    DPoint3 centerOfGravityAfter = dpoint3(currentState.location.O.getQuat() * getCenterOfMass());

    DPoint3 add = add_pos + centerOfGravityBefore - centerOfGravityAfter;
    currentState.location.P += add;
    if (out_ccd_add)
      *out_ccd_add += add;
  }
  void applyPseudoVelOmegaDelta(const DPoint3 &add_pos, const DPoint3 &add_ori) override
  {
    applyPseudoVelOmegaDeltaImpl(add_pos, add_ori);
  }

  TraceMeshFaces *getTraceHandle() const override { return nullptr; }
  virtual IPhysActor *getActor() const override final
  {
    return physActorOffset ? (IPhysActor *)((uint8_t *)this - physActorOffset) : nullptr;
  }

  virtual void setPositionRough(const Point3 &position)
  {
    if (getActor()->isAuthority() && authorityApprovedState)
      authorityApprovedState->location.P = position;

    previousState.location.P = position;
    currentState.location.P = position;
    visualLocation.P = position;

    visualLocationError.P.zero();
    currentState.canBeCheckedForSync = false;

    validateTraceCache();
  }
  virtual void setOrientationRough(gamephys::Orient orient)
  {
    if (getActor()->isAuthority() && authorityApprovedState)
      authorityApprovedState->location.O = orient;
    previousState.location.O = orient;
    currentState.location.O = orient;
    visualLocation.O = orient;

    visualLocationError.O.setQuatToIdentity();
    currentState.canBeCheckedForSync = false;

    validateTraceCache();
  }
  virtual void stopRotationRough()
  {
    if (getActor()->isAuthority() && authorityApprovedState)
    {
      authorityApprovedState->omega.zero();
      authorityApprovedState->alpha.zero();
    }
    previousState.omega.zero();
    previousState.alpha.zero();
    currentState.omega.zero();
    currentState.alpha.zero();

    visualLocationError.O.setQuatToIdentity();
    currentState.canBeCheckedForSync = false;

    validateTraceCache();
  }
  virtual void setTmRough(TMatrix tm)
  {
    if (getActor()->isAuthority() && authorityApprovedState)
      authorityApprovedState->location.O.fromTM(tm);
    previousState.location.fromTM(tm);
    currentState.location.fromTM(tm);
    visualLocation.fromTM(tm);

    visualLocationError.resetLoc();
    currentState.canBeCheckedForSync = false;

    validateTraceCache();
  }
  virtual void setTmSoft(TMatrix tm) { visualLocation.fromTM(tm); }
  virtual void setVelocityRough(Point3 velocity)
  {
    if (getActor()->isAuthority() && authorityApprovedState)
    {
      authorityApprovedState->velocity = velocity;
      authorityApprovedState->acceleration.zero();
    }
    previousState.velocity = velocity;
    previousState.acceleration.zero();
    currentState.velocity = velocity;
    currentState.acceleration.zero();

    visualLocationError.P.zero();
    currentState.canBeCheckedForSync = false;
  }
  virtual void saveAllStates(int32_t state_tick)
  {
    saveCurrentStateTo(currentState, state_tick);
    saveCurrentStateTo(previousState, state_tick);
    clear_and_shrink(historyStates);
    if (getActor()->isShadow())
      historyStates.push_back(previousState);
    else if (getActor()->isAuthority() && authorityApprovedState)
      saveCurrentStateTo(*authorityApprovedState, state_tick);
  }
  virtual void saveCurrentStateTo(PhysStateBase &state, int32_t state_tick) const
  {
    state = currentState;
    state.atTick = state_tick;
  }
  virtual void savePartialStateTo(PhysPartialState &state, uint8_t unit_version) const
  {
    state.location = currentState.location;
    state.velocity = currentState.velocity;
    state.atTick = currentState.atTick;
    state.lastAppliedControlsForTick = currentState.lastAppliedControlsForTick;
    state.unitVersion = unit_version;
  }
  virtual void setCurrentState(const PhysStateBase &state) { currentState = state; }
  virtual void setCurrentMinimalState(const gamephys::Loc & /* loc */, const DPoint3 & /* vel */, const DPoint3 & /* omega */) override
  {}
  void saveCurrentStateToHistory()
  {
    if (DAGOR_UNLIKELY(!historyStates.capacity()))
      historyStates.reserve(1.5f / timeStep + 0.5f); // Note: 45 on tickrate 30
    saveCurrentStateTo(historyStates.push_back(), currentState.atTick);
  }
  virtual void applyProducedState() { appliedCT = producedCT; }
  void extrapolateMinimalState(const PhysStateBase &state, float time, ExtrapolatedPhysState &newState) const
  {
    float dt = time - float(state.atTick) * timeStep;

    // Position extrapolation
    Point3 position0 = state.location.P;
    Point3 velocity0 = state.velocity;
    Point3 acceleration0 = state.acceleration;

    Point3 velocity1 = velocity0 + acceleration0 * dt;
    Point3 position1 = position0 + (velocity0 + velocity1) * 0.5f * dt;

    // Orientation extrapolation
    float orientationDt = min(dt, timeStep * extrapolationTimeMult);

    Quat orientationQuat0 = normalize(state.location.O.getQuat());
    Point3 omega0 = state.omega;
    Point3 alpha0 = state.alpha;
    Point3 omega1 = omega0 + alpha0 * orientationDt;
    Point3 angleTravelled = omega0 * orientationDt + 0.5f * alpha0 * sqr(orientationDt);

    float angleTravelledLen = length(angleTravelled);
    Quat orientationQuat1;
    if (angleTravelledLen > VERY_SMALL_NUMBER)
    {
      Quat angleTravelledQuat(angleTravelled, angleTravelledLen);
      orientationQuat1 = orientationQuat0 * angleTravelledQuat;
    }
    else
      orientationQuat1 = orientationQuat0;

    // Save results to the new state
    newState.location.P = position1;
    newState.location.O.setQuat(orientationQuat1);

    newState.velocity = velocity1;
    newState.acceleration = acceleration0;
    newState.omega = omega1;
    newState.alpha = state.alpha;
    newState.atTime = time;
  }

  static gamephys::Loc __forceinline lerpVisualLocImpl(const PhysStateBase &prevState, const PhysStateBase &curState, float time_step,
    float at_time, bool lerp_vel = true)
  {
    G_FAST_ASSERT(curState.atTick != prevState.atTick); // delta can't be 0
    float delta = (curState.atTick - prevState.atTick) * time_step;
    float dt = clamp(at_time - float(prevState.atTick) * time_step, 0.f, delta);

    gamephys::Loc visLoc;
    visLoc.interpolate(prevState.location, curState.location, dt / delta);

    if (lerp_vel && curState.velocity * prevState.velocity > 0.f)
    {
      DPoint3 noAccelerationPos = prevState.location.P + dpoint3(prevState.velocity * delta);
      DPoint3 posDiff = curState.location.P - noAccelerationPos;
      DPoint3 fullDisplacement = curState.location.P - prevState.location.P;
      if (fullDisplacement.lengthSq() > posDiff.lengthSq())
      {
        DPoint3 acceleration = posDiff * 2.f / sqr(delta);
        DPoint3 newPos = prevState.location.P + dpoint3(prevState.velocity * dt + acceleration * sqr(dt) * 0.5f);
        visLoc.P = newPos;
      }
    }

    return visLoc;
  }

  virtual gamephys::Loc lerpVisualLoc(const PhysStateBase &prevState, const PhysStateBase &curState, float time_step, float at_time)
  {
    return lerpVisualLocImpl(prevState, curState, time_step, at_time);
  }

  void interpolateVisualPosition(float at_time) override final
  {
    const float tsErr = timeStep / 100.f;
    if (currentState.atTick > previousState.atTick && at_time >= (previousState.atTick * timeStep - tsErr) &&
        at_time <= (currentState.atTick * timeStep + tsErr))
      visualLocation = lerpVisualLoc(previousState, currentState, timeStep, at_time);
    else
      visualLocation = currentState.location;
  }
  virtual void calcPosVelAtTime(double at_time, DPoint3 &out_pos, DPoint3 &out_vel) const
  {
    double currentStateTime = currentState.atTick * (double)timeStep;
    if (at_time > currentStateTime || currentState.atTick == previousState.atTick)
    {
      ExtrapolatedPhysState extrapolatedPhysState;
      extrapolateMinimalState(currentState, at_time, extrapolatedPhysState);
      out_pos = extrapolatedPhysState.location.P;
      out_vel = extrapolatedPhysState.velocity;
    }
    else
      gamephys::calc_pos_vel_at_time(at_time, previousState.location.P, dpoint3(previousState.velocity),
        previousState.atTick * (double)timeStep, currentState.location.P, dpoint3(currentState.velocity), currentStateTime, out_pos,
        out_vel);
  }
  virtual void calcQuatOmegaAtTime(double at_time, Quat &out_quat, DPoint3 &out_omega) const
  {
    double currentStateTime = currentState.atTick * (double)timeStep;
    if (at_time > currentStateTime || currentState.atTick == previousState.atTick)
    {
      ExtrapolatedPhysState extrapolatedPhysState;
      extrapolateMinimalState(currentState, at_time, extrapolatedPhysState);
      out_quat = extrapolatedPhysState.location.O.getQuat();
      out_omega = extrapolatedPhysState.omega;
    }
    else
      gamephys::calc_quat_omega_at_time(at_time, previousState.location.O.getQuat(), dpoint3(previousState.omega),
        previousState.atTick * (double)timeStep, currentState.location.O.getQuat(), dpoint3(currentState.omega), currentStateTime,
        out_quat, out_omega);
  }
  void calculateCurrentVisualLocationError(float current_time, gamephys::Loc &out_current_visual_location_error) const
  {
    const float errorFixingDuration = 0.33f;
    if (current_time - visualLocationErrorProductionTime < errorFixingDuration)
    {
      float timeFixing = max(0.f, current_time - visualLocationErrorProductionTime);
      float fixPercent = timeFixing / errorFixingDuration;
      gamephys::Loc zeroLoc;
      zeroLoc.resetLoc(); // To consider: use static var instead?
      out_current_visual_location_error.interpolate(visualLocationError, zeroLoc, fixPercent);
    }
    else
      out_current_visual_location_error.resetLoc();
  }
  void rescheduleAuthorityApprovedSend(int ticks_threshold = 4) override final
  {
    if (ticks_threshold <= 0 || abs(currentState.atTick - lastAuthorityApprovedStateSentAtTick) > ticks_threshold)
    {
      physicsTimeToSendState = ticks_threshold >= 0 ? 0.f : -1.f;
      physicsTimeToSendPartialState = ticks_threshold >= 0 ? 0.f : -1.f;
      lastAuthorityApprovedStateSentAtTick = currentState.atTick;
    }
    else
      physicsTimeToSendState = min(physicsTimeToSendState, float(ticks_threshold) * timeStep);
  }
  typedef void (*custom_resync_cb_t)(IPhysBase *self, const PhysStateBase & /*prev_desynced_state*/,
    const PhysStateBase & /*desynced_state*/, const PhysStateBase & /*incoming_state*/, const PhysStateBase * /*matching_state*/);
  virtual custom_resync_cb_t getCustomResyncCb() const { return nullptr; }
  virtual void resetProducedCt() { producedCT.reset(); }
  virtual void setCurrentTick(int32_t at_tick) final override { currentState.atTick = at_tick; }
  virtual int32_t getCurrentTick() const final override { return currentState.atTick; }
  virtual int32_t getPreviousTick() const final override { return previousState.atTick; }
  virtual int32_t getLastAppliedControlsForTick() const final override { return currentState.lastAppliedControlsForTick; }
  virtual float getTimeStep() const final override { return timeStep; }
  virtual void setTimeStep(float ts) final override { timeStep = ts; }
  virtual float getMaxTimeDeferredControls() const final override { return maxTimeDeferredControls; }
  virtual void setMaxTimeDeferredControls(float time) final override { maxTimeDeferredControls = time; }
  virtual void reset() {}
  virtual void repair() {}

  virtual void setLocationFromTm(const TMatrix &tm) { currentState.location.fromTM(tm); }

  void registerCustomPhysStateSyncer(ICustomPhysStateSyncer &syncer)
  {
#if DAGOR_DBGLEVEL > 0
    for (auto ss = customPhysStateSyncer; ss; ss = ss->next)
      G_ASSERTF(ss != &syncer, "%p already registered", &syncer);
#endif
    syncer.next = customPhysStateSyncer;
    customPhysStateSyncer = &syncer;
  }

  bool unregisterCustomPhysStateSyncer(ICustomPhysStateSyncer &syncer)
  {
    for (ICustomPhysStateSyncer **pss = &customPhysStateSyncer; *pss; pss = &(*pss)->next)
      if (*pss == &syncer)
      {
        *pss = syncer.next;
        return true;
      }
    return false;
  }

  template <typename F, typename... Args>
  auto forEachCustomStateSyncer(F cb, Args &&...args)
  {
    if constexpr (eastl::is_same_v<decltype(cb(customPhysStateSyncer)), bool>)
    {
      for (auto ss = customPhysStateSyncer; ss; ss = ss->next)
        if (!cb(ss, eastl::forward<Args>(args)...))
          return false;
      return true;
    }
    else
      for (auto ss = customPhysStateSyncer; ss; ss = ss->next)
        cb(ss, eastl::forward<Args>(args)...);
  }

  bool receiveAuthorityApprovedState(const danet::BitStream &bs, uint8_t unit_version, float from_time) override final
  {
    PhysState incomingState;
    if (!incomingState.deserialize(bs, *this))
      return false;
    bool ok = true;
    if (!authorityApprovedState || incomingState.atTick > authorityApprovedState->atTick)
    {
      ok = forEachCustomStateSyncer([&](auto ss) { return ss->deserializeAuthState(bs, incomingState.atTick); });
      if (DAGOR_UNLIKELY(!authorityApprovedState))
      {
        auto paas = eastl::make_unique<PhysStateBase>();
        authorityApprovedState.swap(paas);
        paas.release(); // -V530 Force compiler to avoid gen dtor call here
      }
      *authorityApprovedState = eastl::move(incomingState);
      isAuthorityApprovedStateProcessed = false;
      authorityApprovedStateUnitVersion = unit_version;
      authorityApprovedStateDumpFromTime = from_time;
    }
    else
      ; // Otherwise assume that it's out-of-order packet and ignore it
    return ok;
  }
  bool receivePartialAuthorityApprovedState(const danet::BitStream &bs) override final
  {
    PhysPartialState incomingPartialState;
    if (!incomingPartialState.deserialize(bs, *this))
      return false;
    if (incomingPartialState.atTick > authorityApprovedPartialState.atTick)
    {
      authorityApprovedPartialState = eastl::move(incomingPartialState);
      authorityApprovedPartialState.isProcessed = false;
    }
    else
      ; // Otherwise assume that it's out-of-order packet and ignore it
    return true;
  }

  bool receiveControlsPacket(const danet::BitStream &bs, int max_queue_size, int32_t &out_anomaly_flags)
  {
    alignas(ControlState) int stateBuf[(sizeof(ControlState) + 3) / 4];
#if DAGOR_DBGLEVEL > 0 && defined(_M_IX86_FP) && _M_IX86_FP == 0 // fill with NaNs if compiled for IA32 (in order to catch access to
                                                                 // not inited float members)
    for (int *iptr = stateBuf; iptr < &stateBuf[countof(stateBuf)]; ++iptr)
      *iptr = 0x7fbfffff;
#endif
    ControlState &state = *new (stateBuf, _NEW_INPLACE) ControlState;
    bs.SetReadOffset(0);
    if (!state.deserialize(bs, *this, out_anomaly_flags))
    {
      state.~ControlState();
      return false;
    }

    if (max_queue_size > 0 && unapprovedCT.size() > max_queue_size)
      safe_erase_items(unapprovedCT, 0, unapprovedCT.size() - 1);

    // Ctrls from a far future are abnormal and may break the logic. Can happen briefly when we switch tickrate.
    if (max_queue_size > 0 && state.producedAtTick > currentState.atTick + max_queue_size)
      return true;

    // TODO: count lost controls for controlling player here.
    if (!unapprovedCT.empty())
    {
      const ControlState *latest = &unapprovedCT.back();
      int latestSequenceNumber = latest->getSequenceNumber();
      int sequenceDifference = state.getSequenceNumber() - latestSequenceNumber;
      const int maxInterpolatedSequenceDifference = 32; // at 10 it should occure at 30% loss once in 88 minutes
      if (sequenceDifference > 1)
      {
        uint8_t latestUnitVersion = latest->getUnitVersion();
        if (sequenceDifference < maxInterpolatedSequenceDifference && state.getUnitVersion() == latestUnitVersion)
        {
          int missedCount = sequenceDifference - 1;
          int latestStateIndex = unapprovedCT.size() - 1;
          for (int i = 1; i <= missedCount; i++)
          {
            float interpolationK = cvt(latestSequenceNumber + i, latestSequenceNumber, state.getSequenceNumber(), 0.f, 1.f);
            ControlState &interpolatedState = unapprovedCT.push_back();
            interpolatedState.interpolate(unapprovedCT[latestStateIndex], state, interpolationK, latestSequenceNumber + i);
            interpolatedState.setUnitVersion(latestUnitVersion);
            interpolatedState.setControlsForged(true);
          }
        }
      }
    }
    // Try to patch interpolated controls if we have them
    bool resHistory = receiveControlsHistory(bs, state);
    ControlState *last = !unapprovedCT.empty() ? &unapprovedCT.back() : nullptr;
    if (!last || last->producedAtTick < state.producedAtTick)
      unapprovedCT.push_back(eastl::move(state));
    else if (last->producedAtTick == state.producedAtTick && last->isControlsForged())
      *last = eastl::move(state); // allow overwrite of forged controls
    else
      ; // it's either dup (in which case it should be dropped) or out-of-order packet (which is unlikely but possible) - insert it in
        // proper place (or better yet - use vector_set<>)
    state.~ControlState();
    return resHistory;
  }
  bool receiveControlsHistory(const danet::BitStream &bs, const ControlState &base_state)
  {
    uint8_t numPackets = 0;
    bool isOk = bs.Read(numPackets);
    if (!isOk)
      return false;
    for (int i = 0; i < numPackets; ++i)
    {
      ControlState state = base_state;
      if (!state.deserializeMinimalState(bs, base_state))
        return false;
      // Skip it if we already applied it
      if (currentState.atTick > state.producedAtTick)
        continue;
      ControlState *forgedState = NULL;
      bool alreadyHave = false;
      for (int j = 0; j < unapprovedCT.size(); ++j)
      {
        if (unapprovedCT[j].producedAtTick == state.producedAtTick)
        {
          if (unapprovedCT[j].isControlsForged())
            forgedState = &unapprovedCT[j];
          alreadyHave = true;
          break;
        }
      }
      // Just skip control which we already received but haven't processed
      if (alreadyHave)
      {
        if (forgedState)
          forgedState->applyMinimalState(state); // copy to forged state
        continue;
      }
      unapprovedCT.push_back(eastl::move(state));
    }
    return true;
  }
  void appendControlsHistory(danet::BitStream &bs, int depth) const
  {
    // We don't have base state for some reason, so we can't create history at all (we don't have history)
    if (unapprovedCT.empty())
    {
      bs.Write(uint8_t(0));
      return;
    }
    // Go from bottom to top, but clamp it.
    int startFrom = max((int)unapprovedCT.size() - depth - 1, 0);
    int lastCt = unapprovedCT.size() - 1;
    bs.Write(uint8_t(lastCt - startFrom));
    for (int i = startFrom; i < lastCt; ++i)
      unapprovedCT[i].serializeMinimalState(bs, unapprovedCT.back());
  }

  virtual void reportDeserializationError(int /*err*/) {}

  bool isNeedToSaveControlsAt(int tick) const { return unapprovedCT.empty() || tick > unapprovedCT.back().producedAtTick; }

  bool isAsleep() const override { return false; }
  void wakeUp() override {}
  void putToSleep() override { G_ASSERTF(0, "%s() is not supported by this physics type", __FUNCTION__); }

  virtual dag::Span<CollisionObject> getMutableCollisionObjects() const { return {}; }
  virtual uint64_t getActiveCollisionObjectsBitMask() const { return ALL_COLLISION_OBJECTS; }

  virtual void updatePhysInWorld(const TMatrix &tm)
  {
    dag::ConstSpan<CollisionObject> collisionObjects = getCollisionObjects();
    for (const CollisionObject &co : collisionObjects)
      if (co)
        dacoll::set_obj_motion(co, tm, currentState.velocity, currentState.omega);
  }


  virtual void addCollisionToWorld() {}
  virtual void prepareCollisions(daphys::SolverBodyInfo & /*body1*/, daphys::SolverBodyInfo & /*body2*/, bool /*first_body*/,
    float /*friction*/, dag::Span<gamephys::CollisionContactData> /*contacts*/,
    dag::Span<gamephys::SeqImpulseInfo> /*collisions*/) const
  {}

  void saveProducedCTAsUnapproved(int tick)
  {
    producedCT.producedAtTick = tick;
    producedCT.setUnitVersion(getActor()->getAuthorityUnitVersion());
    producedCT.setSequenceNumber(++lastCTSequenceNumber);
    unapprovedCT.push_back() = producedCT;
  }

  virtual float getFriction() const { return 0.f; }
  virtual void setFriction(float) {}

  virtual float getBouncing() const { return 0.f; }
  virtual void setBouncing(float) {}

  TMatrix getCollisionObjectsMatrix() const override { return getVisualStateLoc().makeTM(); }
};

#define DEFINE_COMMON_PHYS_CONTROLS_METHODS(ControlsClass)                                        \
  void serialize(danet::BitStream &bs) const;                                                     \
  bool deserialize(const danet::BitStream &bs, IPhysBase &, int32_t &);                           \
  void interpolate(const ControlsClass &a, const ControlsClass &b, float k, int sequence_number); \
  void serializeMinimalState(danet::BitStream &bs, const ControlsClass &base_state) const;        \
  bool deserializeMinimalState(const danet::BitStream &bs, const ControlsClass &base_state);      \
  void applyMinimalState(const ControlsClass &minimal_state);                                     \
  void reset();                                                                                   \
  int getUnitVersion() const                                                                      \
  {                                                                                               \
    return unitVersion;                                                                           \
  }                                                                                               \
  void setUnitVersion(int version)                                                                \
  {                                                                                               \
    unitVersion = version;                                                                        \
  }

template <typename PhysState, typename ControlState, typename PartialState>
PhysicsBase<PhysState, ControlState, PartialState>::PhysicsBase(ptrdiff_t physactor_offset, PhysVars *, float time_step,
  float extrapolation_time_mult) :
  physActorOffset(physactor_offset),
  fmsyncPreviousData(NULL),
  fmsyncCurrentData(NULL),
  timeStep(time_step),
  extrapolationTimeMult(extrapolation_time_mult)
{
  appliedCT.init();
  producedCT.init();
  authorityApprovedPartialState.isProcessed = true;

  visualLocationErrorProductionTime = -1.f;

  lastCTSequenceNumber = -1;
  isAuthorityApprovedStateProcessed = true;
  authorityApprovedStateUnitVersion = -1;
  authorityApprovedStateDumpFromTime = -1.f;
  lastProcessedAuthorityApprovedStateAtTick = -1;

  physicsTimeToSendState = 0.f;
  physicsTimeToSendPartialState = 0.f;
  lastAuthorityApprovedStateSentAtTick = 0;
}

template <typename PhysState, typename ControlState, typename PartialState>
PhysicsBase<PhysState, ControlState, PartialState>::~PhysicsBase()
{
  del_it(fmsyncPreviousData);
  del_it(fmsyncCurrentData);
}
