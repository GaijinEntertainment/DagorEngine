// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "dasPhysObj.h"
#include <gamePhys/collision/collisionLib.h>

void DasPhysObjState::reset()
{
  lastAppliedControlsForTick = -1; // invalidate prev controls
  velocity.zero();
  acceleration.zero();
  omega.zero();
}

void DasPhysObjState::applyPartialState(const CommonPhysPartialState &state)
{
  location = state.location;
  velocity = state.velocity;
}

void DasPhysObjState::applyDesyncedState(const DasPhysObjState & /*state*/) {}

void DasPhysObjState::serialize(danet::BitStream &bs) const
{
  bs.Write(*static_cast<const DasPhysObjStateBase *>(this));
  write_controls_tick_delta(bs, atTick, lastAppliedControlsForTick);
  bs.WriteCompressed(extStateSerSz);
  bs.Write((const char *)extState, extStateSerSz);
}

bool DasPhysObjState::deserialize(const danet::BitStream &bs, IPhysBase &phys_base)
{
  auto &phys = static_cast<DasPhysObj &>(phys_base);
  G_ASSERTF_RETURN(phys.physStatesPool.isInited(), false, "Not inited physStates pool. allocPhysState wasn't called?");

  bool isOk = bs.Read(static_cast<DasPhysObjStateBase &>(*this));
  isOk &= read_controls_tick_delta(bs, atTick, lastAppliedControlsForTick);
  isOk &= bs.ReadCompressed(extStateSerSz);
  isOk &= !extStateSerSz || (extStateSerSz + sizeof(void *)) <= phys.physStatesPool.getBlockSize();
  if (extStateSerSz && isOk)
  {
    void *est = phys.allocPhysStateEx(*this, extStateSerSz, phys.physStatesPool.getBlockSize() - sizeof(void *));
    isOk &= bs.Read((char *)est, extStateSerSz);
  }
  return isOk;
}

void DasPhysObjControlsState::serialize(danet::BitStream &bs) const
{
  bs.Write(producedAtTick);
  bs.Write(sequenceNumber);
  bs.WriteCompressed(extStateSerSz);
  bs.Write((const char *)extState, extStateSerSz);
}

bool DasPhysObjControlsState::deserialize(const danet::BitStream &bs, IPhysBase &phys_base, int32_t &)
{
  auto &phys = static_cast<DasPhysObj &>(phys_base);
  G_ASSERTF_RETURN(phys.controlsPool.isInited(), false, "Not inited controls pool. allocControls wasn't called?");

  bool isOk = true;
  isOk &= bs.Read(producedAtTick);
  isOk &= bs.Read(sequenceNumber);
  isOk &= bs.ReadCompressed(extStateSerSz);
  isOk &= !extStateSerSz || (extStateSerSz + sizeof(void *)) <= phys.controlsPool.getBlockSize();
  if (isOk && extStateSerSz)
  {
    void *est = phys.allocControlsEx(*this, extStateSerSz, phys.controlsPool.getBlockSize() - sizeof(void *));
    isOk &= bs.Read((char *)est, extStateSerSz);
  }

  return isOk;
}

void DasPhysObjControlsState::reset() {}

DasPhysObj::DasPhysObj(ptrdiff_t physactor_offset, PhysVars *phys_vars, float time_step) :
  PhysicsBase<DasPhysObjState, DasPhysObjControlsState, CommonPhysPartialState>(physactor_offset, phys_vars, time_step)
{}

DasPhysObj::~DasPhysObj()
{
  if (collObj)
    dacoll::destroy_dynamic_collision(collObj);
  appliedCT.freeExtState();
  producedCT.freeExtState();
  clear_and_shrink(unapprovedCT);
  previousState.freeExtState();
  currentState.freeExtState();
  if (authorityApprovedState)
    authorityApprovedState->freeExtState();
  clear_and_shrink(historyStates);
}

void DasPhysObj::loadFromBlk(
  const DataBlock * /*blk*/, const CollisionResource * /*collision*/, const char * /*unit_name*/, bool /*is_player*/)
{
  // collObj = add_dynamic_cylinder_collision(collRad, 1.f, true);
}

void DasPhysObj::updatePhys(float at_time, float dt, bool /*is_for_real*/)
{
  int curTick = gamephys::nearestPhysicsTickNumber(at_time + dt, timeStep);
  currentState.atTick = curTick;
  currentState.lastAppliedControlsForTick = appliedCT.producedAtTick;
}

void DasPhysObj::reset() { currentState.reset(); }

dag::ConstSpan<CollisionObject> DasPhysObj::getCollisionObjects() const { return dag::ConstSpan<CollisionObject>(&collObj, 1); }
