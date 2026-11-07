// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "phys_kind/physKind.h"

template class PhysicsBase<PhysKindState, PhysKindControls, CommonPhysPartialState>;

void write_phys_kind_ext(danet::BitStream &, const uint8_t *, unsigned, unsigned) { G_ASSERT(0); }
int read_phys_kind_ext(const danet::BitStream &, uint8_t *, unsigned) { G_ASSERT_RETURN(false, -1); }
void PhysKindState::serialize(danet::BitStream &) const { G_ASSERT(0); }
bool PhysKindState::deserialize(const danet::BitStream &) { G_ASSERT_RETURN(false, false); }
void PhysKindState::applyPartialState(const CommonPhysPartialState &) { G_ASSERT(0); }
bool PhysKindState::operator==(const PhysKindState &) const { G_ASSERT_RETURN(false, false); }
bool PhysKindState::hasLargeDifferenceWith(const PhysKindState &) const { G_ASSERT_RETURN(false, false); }
void PhysKindControls::serialize(danet::BitStream &) const { G_ASSERT(0); }
bool PhysKindControls::deserialize(const danet::BitStream &) { G_ASSERT_RETURN(false, false); }
bool PhysKindControls::deserialize(const danet::BitStream &, IPhysBase &, int32_t &) { G_ASSERT_RETURN(false, false); }
void PhysKindControls::interpolate(const PhysKindControls &, const PhysKindControls &, float, int) { G_ASSERT(0); }
void PhysKindControls::serializeMinimalState(danet::BitStream &, const PhysKindControls &) const { G_ASSERT(0); }
bool PhysKindControls::deserializeMinimalState(const danet::BitStream &, const PhysKindControls &) { G_ASSERT_RETURN(false, false); }
void PhysKindControls::applyMinimalState(const PhysKindControls &) { G_ASSERT(0); }
void PhysKindControls::reset() { G_ASSERT(0); }
void PhysKind::addCollisionBox(const TMatrix &, const Point3 &) { G_ASSERT(0); }
void PhysKind::addCollisionToWorld() { G_ASSERT(0); }
void PhysKind::putToSleep() { G_ASSERT(0); }
void PhysKind::setCurrentMinimalState(const gamephys::Loc &, const DPoint3 &, const DPoint3 &) { G_ASSERT(0); }
void PhysKind::reset() { G_ASSERT(0); }
bool PhysKind::setWire(dag::ConstSpan<PhysKindWireField>, dag::ConstSpan<PhysKindWireField>) { G_ASSERT_RETURN(false, false); }
