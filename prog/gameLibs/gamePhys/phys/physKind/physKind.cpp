// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "physKind_api.h"
#include <gamePhys/collision/collisionLib.h>
#include <ioSys/dag_dataBlock.h>
#include <debug/dag_log.h>
#include <string.h>

template class PhysicsBase<PhysKindState, PhysKindControls, CommonPhysPartialState>;

PhysKind::~PhysKind()
{
  for (CollisionObject &co : collision)
    dacoll::destroy_dynamic_collision(co);
}

void PhysKind::loadFromBlk(const DataBlock *blk, const CollisionResource *, const char *, bool)
{
  mass = blk->getReal("mass", mass);
  centerOfMass = blk->getPoint3("centerOfMass", centerOfMass);
  momentOfInertia = blk->getPoint3("momentOfInertia", momentOfInertia);
  const DataBlock *collisionBlk = blk->getBlockByNameEx("collision");
  for (int i = 0; i < collisionBlk->blockCount(); ++i)
    addCollision(*collisionBlk->getBlock(i));
}

void PhysKind::addCollision(const DataBlock &props)
{
  const TMatrix tm = currentState.location.makeTM();
  CollisionObject co = dacoll::add_dynamic_collision_with_mask(props, nullptr, /*is_player*/ false, /*add_to_world*/ false,
    /*auto_mask*/ false, dacoll::DEFAULT_DYN_COLLISION_MASK, &tm);
  if (!co.isValid())
    return;
  if (collisionInWorld)
    dacoll::set_obj_active(co, true);
  collision.push_back(co);
}

void PhysKind::addCollisionBox(const TMatrix &tm, const Point3 &width)
{
  DataBlock props;
  DataBlock *shape = props.addNewBlock("box");
  shape->addTm("tm", tm);
  shape->addPoint3("width", width);
  addCollision(props);
}

void PhysKind::addCollisionToWorld()
{
  collisionInWorld = true;
  for (CollisionObject &co : collision)
    dacoll::set_obj_active(co, true);
}

void PhysKind::updatePhys(double at_time, float dt, bool is_for_real)
{
  if (host && !currentState.isSleep())
    host->step(*this, gamephys::nearestPhysicsTickNumber(at_time, timeStep), dt, is_for_real);
  currentState.atTick = gamephys::nearestPhysicsTickNumber(at_time + dt, timeStep);
  currentState.lastAppliedControlsForTick = (appliedCT.producedAtTick > 0) ? appliedCT.producedAtTick : currentState.atTick - 1;
}

void PhysKind::applyState(bool keep_body_motion)
{
  if (host)
    host->applyState(*this, keep_body_motion);
}

void PhysKind::putToSleep()
{
  currentState.flags |= PhysKindState::FLAG_SLEEP;
  currentState.velocity.zero();
  currentState.omega.zero();
}

void PhysKind::setCurrentMinimalState(const gamephys::Loc &loc, const DPoint3 &vel, const DPoint3 &omega)
{
  currentState.location = loc;
  currentState.velocity = vel;
  currentState.omega = omega;
}

bool PhysKind::setWire(dag::ConstSpan<PhysKindWireField> state_fields, dag::ConstSpan<PhysKindWireField> controls_fields)
{
  const PhysKindWire *state = state_fields.empty() ? nullptr : phys_kind_intern_wire(state_fields, PHYS_KIND_STATE_EXT_MAX);
  const PhysKindWire *controls = controls_fields.empty() ? nullptr : phys_kind_intern_wire(controls_fields, PHYS_KIND_CTRL_EXT_MAX);
  if ((!state && !state_fields.empty()) || (!controls && !controls_fields.empty()))
    return false;
  if (state)
    for (int lod = 0; lod < (int)PhysKindLod::Num; ++lod)
      if (state->maxSnapBits[lod] > PHYS_KIND_SNAP_EXT_MAX * CHAR_BIT)
      {
        logerr("phys_kind wire: a snapshot at LOD %d can take %d bits of state, more than %d", lod, state->maxSnapBits[lod],
          PHYS_KIND_SNAP_EXT_MAX * CHAR_BIT);
        return false;
      }
  stateWire = state;
  controlsWire = producedCT.wire = appliedCT.wire = controls;
  for (PhysKindControls &ct : unapprovedCT)
    ct.wire = controlsWire;
  return true;
}

bool PhysKindControls::deserialize(const danet::BitStream &bs, IPhysBase &phys, int32_t &)
{
  wire = static_cast<PhysKind &>(phys).controlsWire;
  return deserialize(bs);
}

void PhysKind::reset()
{
  currentState.flags = 0;
  memset(currentState.ext, 0, sizeof(currentState.ext));
  applyState();
}
