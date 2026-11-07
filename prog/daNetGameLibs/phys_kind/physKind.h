// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <gamePhys/phys/physKind/physKind_api.h>
#include <daECS/core/event.h>
#include "phys/netPhys.h"

struct PhysKindSnapshot : public BasePhysSnapshot
{
  uint8_t lod = 0;
  uint8_t stateTickLow = 0; // state tick low byte, decoded within 128
  bool shadowLoaded = false;
  uint8_t extBits = 0;
  uint8_t ext[PHYS_KIND_SNAP_EXT_MAX] = {}; // as the wire had it, the actor decodes it
};

// Snapshot bodies of the last update, one per LOD: each receiver gets a bit copy
struct PhysKindCachedQuantizedInfo : public CachedQuantizedInfoT<>
{
  static constexpr unsigned BODY_MAX_BYTES = (1 + PHYS_KIND_SNAP_POSE_MAX_BITS + 8 * (1 + PHYS_KIND_SNAP_EXT_MAX) + 7) / 8;
  uint8_t body[(int)PhysKindLod::Num][BODY_MAX_BYTES];
  uint16_t bodyBits[(int)PhysKindLod::Num] = {};

  void update(const PhysKind &phys, ecs::EntityId);
};

struct PhysKindCustomPhys : public DummyCustomPhys
{
  typedef PhysKindSnapshot SnapshotType;
  typedef PhysKindCachedQuantizedInfo CachedQuantizedInfo;

  int lastShadowSnapTick = -1;
  int lastOwnerSnapTick = -1;
  int lastCorrectedOwnerTick = -1;
  int serverTickLag = -1;
  int lastRole = -1;
  double createdAt = 0.0;
  float snapTimeStep = 0.f;
  bool ownControlsArrived = false;

  static void onPreInit(ecs::EntityId eid, BasePhysActor &actor);
};

#define PHYS_KIND_ACTOR PhysActor<PhysKind, PhysKindCustomPhys, PhysType::CUSTOM8>

template <>
void PHYS_KIND_ACTOR::serializePhysSnapshotImpl(danet::BitStream &bs, PhysSnapSerializeType pst) const;
template <>
bool PHYS_KIND_ACTOR::deserializePhysSnapshot(
  const danet::BitStream &from, PhysSnapSerializeType pst, BasePhysSnapshot &snap, net::IConnection &);
template <>
void PHYS_KIND_ACTOR::updateRemoteShadow(double remote_time, float dt, const BasePhysActor::UpdateContext &uctx);
template <>
void PHYS_KIND_ACTOR::teleportTo(const TMatrix &tm, bool hard_teleport);
template <>
bool PHYS_KIND_ACTOR::beforeEnqueueCT(double at_time);

extern template class PHYS_KIND_ACTOR;
typedef PHYS_KIND_ACTOR PhysKindActor;
#undef PHYS_KIND_ACTOR

ECS_DECLARE_PHYS_ACTOR_TYPE(PhysKindActor);

ECS_UNICAST_PROFILE_EVENT_TYPE(CmdPhysKindStep, int32_t /*tick*/, float /*dt*/, bool /*is_for_real*/, bool /*use_controls*/);
ECS_UNICAST_PROFILE_EVENT_TYPE(CmdPhysKindApplyState, bool /*keep_body_motion*/);
