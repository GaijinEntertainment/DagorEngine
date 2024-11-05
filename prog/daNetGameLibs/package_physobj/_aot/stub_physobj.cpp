// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "package_physobj/physObj.h"
void PhysObjState::reset() { G_ASSERT(0); }
void PhysObjState::serialize(danet::BitStream &) const { G_ASSERT(0); }
bool PhysObjState::deserialize(const danet::BitStream &, IPhysBase &) { G_ASSERT_RETURN(false, false); }
void PhysObjState::applyPartialState(const CommonPhysPartialState &) { G_ASSERT(0); }
void PhysObjState::applyDesyncedState(const PhysObjState &) { G_ASSERT(0); }
void PhysObjControlState::serialize(danet::BitStream &) const { G_ASSERT(0); }
bool PhysObjControlState::deserialize(const danet::BitStream &, IPhysBase &, int32_t &) { G_ASSERT_RETURN(false, false); }
void PhysObjControlState::interpolate(const PhysObjControlState &, const PhysObjControlState &, float, int) { G_ASSERT(0); }
void PhysObjControlState::serializeMinimalState(danet::BitStream &, const PhysObjControlState &) const { G_ASSERT(0); }
bool PhysObjControlState::deserializeMinimalState(const danet::BitStream &, const PhysObjControlState &)
{
  G_ASSERT_RETURN(false, false);
}
void PhysObjControlState::applyMinimalState(const PhysObjControlState &) { G_ASSERT(0); }
void PhysObjControlState::reset() { G_ASSERT(0); }
void PhysObj::addForce(const Point3 &arm, const Point3 &force) { G_ASSERT(0); }
void PhysObj::updatePhys(float, float, bool) { G_ASSERT(0); }
void PhysObj::applyOffset(Point3 const &) { G_ASSERT(0); }
TMatrix PhysObj::getCollisionObjectsMatrix() const { G_ASSERT_RETURN(false, TMatrix::IDENT); }
bool PhysObj::isCollisionValid() const { G_ASSERT_RETURN(false, false); }
void PhysObj::addSoundShockImpulse(float) { G_ASSERT(0); }
float PhysObj::getSoundShockImpulse() const { G_ASSERT_RETURN(false, 0.0f); }
void PhysObj::reset() { G_ASSERT(0); }
void PhysObj::updatePhysInWorld(TMatrix const &) { G_ASSERT(0); }
