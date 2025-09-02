// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ecs/core/entityManager.h>
#include <ecs/core/attributeEx.h>
#include <daECS/core/coreEvents.h>
#include <daECS/core/baseIo.h>
#include <daECS/core/sharedComponent.h>
#include <daECS/core/componentTypes.h>

#include <ecs/phys/physVars.h>
#include <ecs/anim/anim.h>
#include <ecs/phys/turretControl.h>

#include <math/dag_mathUtils.h>
#include <math/dag_mathAng.h>
#include <math/random/dag_random.h>
#include <math/dag_geomTree.h>
#include <gamePhys/phys/utils.h>
#include <ecs/delayedAct/actInThread.h>

#include <EASTL/fixed_string.h>

ECS_UNICAST_EVENT_TYPE(EventOnTurretControlCreated);

ECS_REGISTER_EVENT(EventOnGunCreated);
ECS_REGISTER_EVENT(EventOnGunPayloadCreated);
ECS_REGISTER_EVENT(EventOnGunsPayloadDestroyed);
ECS_REGISTER_EVENT(EventOnGunsPayloadAmmoUpdate);

ECS_REGISTER_EVENT(EventOnTurretControlCreated);

static struct TurretAimDrivesMultSerializer final : public ecs::ComponentSerializer
{
  void serialize(ecs::SerializerCb &cb, const void *data, size_t, ecs::component_type_t hint) override
  {
    G_ASSERT(hint == ecs::ComponentTypeInfo<TurretAimDrivesMult>::type);
    G_UNUSED(hint);

    const TurretAimDrivesMult *state = (const TurretAimDrivesMult *)data;
    cb.write(&state->packed, sizeof(state->packed) * CHAR_BIT, 0);
  }

  bool deserialize(const ecs::DeserializerCb &cb, void *data, size_t, ecs::component_type_t hint) override
  {
    G_ASSERT(hint == ecs::ComponentTypeInfo<TurretAimDrivesMult>::type);
    G_UNUSED(hint);

    TurretAimDrivesMult *state = (TurretAimDrivesMult *)data;
    const bool isOk = cb.read(&state->packed, sizeof(state->packed) * CHAR_BIT, 0);
    state->aimDrivesMult.x = TurretAimMultYawPacked::unpackUnsigned(state->getAimMultYawBits());
    state->aimDrivesMult.y = TurretAimMultPitchPacked::unpackUnsigned(state->getAimMultPitchBits());
    return isOk;
  }
} turret_aim_drives_mult_serializer;

static struct TurretStateSerializer final : public ecs::ComponentSerializer
{
  void serialize(ecs::SerializerCb &cb, const void *data, size_t, ecs::component_type_t hint) override
  {
    G_ASSERT(hint == ecs::ComponentTypeInfo<TurretState>::type);
    G_UNUSED(hint);

    const TurretState *state = (const TurretState *)data;
    cb.write(&state->remote.packed, TurretState::RemoteState::USED_BITS, 0);
  }

  bool deserialize(const ecs::DeserializerCb &cb, void *data, size_t, ecs::component_type_t hint) override
  {
    G_ASSERT(hint == ecs::ComponentTypeInfo<TurretState>::type);
    G_UNUSED(hint);

    TurretState *state = (TurretState *)data;
    state->remote.packed = 0;
    const bool isOk = cb.read(&state->remote.packed, TurretState::RemoteState::USED_BITS, 0);
    state->remote.wishDirection = TurretWishDirPacked(state->remote.getWishDirectionBits()).unpack();
    return isOk;
  }
} turret_state_serializer;

ECS_REGISTER_RELOCATABLE_TYPE(TurretState, &turret_state_serializer);
ECS_REGISTER_RELOCATABLE_TYPE(TurretAimDrivesMult, &turret_aim_drives_mult_serializer);
ECS_AUTO_REGISTER_COMPONENT(TurretState, "turret_state", nullptr, 0);
ECS_AUTO_REGISTER_COMPONENT(TurretState, "turret_state_remote", nullptr, 0);
ECS_AUTO_REGISTER_COMPONENT(TurretAimDrivesMult, "turret_aim_drives_mult", nullptr, 0);

void TurretState::RemoteState::setWishDirectionBits(uint32_t v)
{
  packed = (packed & ~(WISH_DIR_MASK << WISH_DIR_SHIFT)) | ((v & WISH_DIR_MASK) << WISH_DIR_SHIFT);
}

void TurretAimDrivesMult::setAimMultYawBits(uint32_t v)
{
  packed = (packed & ~(AIM_MULT_YAW_MASK << AIM_MULT_YAW_SHIFT)) | ((v & AIM_MULT_YAW_MASK) << AIM_MULT_YAW_SHIFT);
}

void TurretAimDrivesMult::setAimMultPitchBits(uint32_t v)
{
  packed = (packed & ~(AIM_MULT_PITCH_MASK << AIM_MULT_PITCH_SHIFT)) | ((v & AIM_MULT_PITCH_MASK) << AIM_MULT_PITCH_SHIFT);
}

uint32_t TurretState::RemoteState::getWishDirectionBits() const { return (packed >> WISH_DIR_SHIFT) & WISH_DIR_MASK; }

uint32_t TurretAimDrivesMult::getAimMultYawBits() const { return (packed >> AIM_MULT_YAW_SHIFT) & AIM_MULT_YAW_MASK; }

uint32_t TurretAimDrivesMult::getAimMultPitchBits() const { return (packed >> AIM_MULT_PITCH_SHIFT) & AIM_MULT_PITCH_MASK; }

void TurretState::RemoteState::setWishDirection(const Point3 &dir)
{
  G_ASSERT(!check_nan(dir));
  G_ASSERT(float_nonzero(dir.lengthSq()));

  wishDirection = normalize(dir);
  TurretWishDirPacked wishDirPacked(wishDirection);
  setWishDirectionBits(wishDirPacked.val);
}

void TurretAimDrivesMult::setAimMultYaw(float mult)
{
  G_ASSERT(!check_nan(mult));

  uint32_t aimMultPacked = TurretAimMultYawPacked::packUnsigned(saturate(mult));
  setAimMultYawBits(aimMultPacked);
  aimDrivesMult.x = TurretAimMultYawPacked::unpackUnsigned(aimMultPacked);
}

void TurretAimDrivesMult::setAimMultPitch(float mult)
{
  G_ASSERT(!check_nan(mult));

  uint32_t aimMultPacked = TurretAimMultPitchPacked::packUnsigned(saturate(mult));
  setAimMultPitchBits(aimMultPacked);
  aimDrivesMult.y = TurretAimMultPitchPacked::unpackUnsigned(aimMultPacked);
}

bool TurretState::RemoteState::operator==(const TurretState::RemoteState &rhs) const { return packed == rhs.packed; }

bool TurretState::operator==(const TurretState &rhs) const { return remote == rhs.remote; }

bool TurretAimDrivesMult::operator==(const TurretAimDrivesMult &rhs) const { return packed == rhs.packed; }


template <typename Callable>
inline void turret_payload_gun_ammo_ecs_query(ecs::EntityManager &mgr, ecs::EntityId eid, Callable c);

ECS_NO_ORDER
ECS_REQUIRE(eastl::true_type turret_control__hasPayload)
void __forceinline update_turret_payload_es(const ParallelUpdateFrameDelayed &, ecs::EntityManager &manager, ecs::EntityId eid,
  const ecs::EidList &turret_control__gunEids)
{
  if (turret_control__gunEids.empty())
    return;

  EventOnGunsPayloadAmmoUpdate evt(eid, ecs::IntList());
  ecs::IntList &ammoList = evt.get<1>();
  ammoList.resize(turret_control__gunEids.size(), 0);
  for (int gunIndex = 0; gunIndex < turret_control__gunEids.size(); ++gunIndex)
    turret_payload_gun_ammo_ecs_query(manager, turret_control__gunEids[gunIndex],
      [&](ECS_REQUIRE(ecs::auto_type gun) ECS_REQUIRE(eastl::true_type gun__isPayload) int gun__ammo) {
        ammoList[gunIndex] = gun__ammo;
      });

  const ecs::EntityId ownerEid = ECS_GET_OR(turret_control__gunEids[0], turret__owner, ecs::INVALID_ENTITY_ID);
  if (ownerEid != ecs::INVALID_ENTITY_ID)
    manager.sendEvent(ownerEid, eastl::move(evt));
}
