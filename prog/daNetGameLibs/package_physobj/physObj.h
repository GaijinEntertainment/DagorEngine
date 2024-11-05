// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/utility.h>
#include <EASTL/vector_map.h>
#include <gamePhys/phys/physObj/physObj.h>
#include "phys/netPhys.h"
#include <daECS/core/entityComponent.h>
#include <generic/dag_smallTab.h>

namespace ecs
{
template <typename T>
class List;
}

struct PhysObjSnapshotState
{
  eastl::vector<eastl::pair<ecs::EntityId, Point3 /*remoteWishDir*/>> turrets;
};

struct PhysObjSnapshot : public BasePhysSnapshot, public PhysObjSnapshotState
{};

typedef CachedQuantizedInfoT<QuantizedWorldLocSmall, gamemath::UnitVecPacked24> BasePhysObjCachedQuantizedInfo;
struct PhysObjCachedQuantizedInfo : public BasePhysObjCachedQuantizedInfo
{
  SmallTab<eastl::pair<ecs::EntityId, unsigned>> turrets;

  void update(const PhysObj &phys, ecs::EntityId eid, const ecs::List<ecs::EntityId> *turret_control__gunEids = nullptr);
};

#define PHYSOBJ_ACTOR PhysActor<PhysObj, PhysObjCustomPhys, PhysType::PHYSOBJ>

struct PhysObjCustomPhys : public DummyCustomPhys
{
  typedef PhysObjSnapshot SnapshotType;
  typedef PhysObjCachedQuantizedInfo CachedQuantizedInfo;

  typedef eastl::vector_map<uint16_t /*netPhysId*/, SnapshotType> SnapshotCacheType;
  static SnapshotCacheType snapshotCache;

  PHYSOBJ_ACTOR &getPhysObj();
  const PHYSOBJ_ACTOR &getPhysObj() const { return const_cast<PhysObjCustomPhys *>(this)->getPhysObj(); }

  static void onPostInit(ecs::EntityId /*eid*/, BasePhysActor &);
  static void onSnapshotCacheReset() { SnapshotCacheType().swap(snapshotCache); }
  void onPostUpdate(float at_time, const BasePhysActor::UpdateContext &uctx);
};

template <>
void PHYSOBJ_ACTOR::serializePhysSnapshotImpl(danet::BitStream &bs, PhysSnapSerializeType pst) const;
template <>
bool PHYSOBJ_ACTOR::deserializePhysSnapshot(
  const danet::BitStream &from, PhysSnapSerializeType pst, BasePhysSnapshot &snap, net::IConnection &);
template <>
void PHYSOBJ_ACTOR::processSnapshotNoEntity(uint16_t netPhysId, BasePhysSnapshot &&basesnap);

template <>
void PHYSOBJ_ACTOR::updateRemoteShadow(float remote_time, float dt, const BasePhysActor::UpdateContext &uctx);

extern template class PHYSOBJ_ACTOR;
typedef PHYSOBJ_ACTOR PhysObjActor;
#undef PHYSOBJ_ACTOR

ECS_DECLARE_PHYS_ACTOR_TYPE(PhysObjActor);
