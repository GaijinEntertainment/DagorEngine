//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <generic/dag_staticTab.h>
#include <math/dag_Point2.h>
#include <math/dag_Point3.h>
#include <math/dag_TMatrix.h>

#include <daECS/core/entityComponent.h>
#include <daECS/core/entityId.h>
#include <daECS/core/event.h>
#include <daECS/core/componentTypes.h>

#include <gameMath/quantization.h>

struct TurretAimDrivesMult
{
  enum : uint32_t
  {
    AIM_MULT_YAW_SHIFT = 0,
    AIM_MULT_YAW_BITS = 4,
    AIM_MULT_YAW_MASK = (1 << AIM_MULT_YAW_BITS) - 1,

    AIM_MULT_PITCH_SHIFT = AIM_MULT_YAW_SHIFT + AIM_MULT_YAW_BITS,
    AIM_MULT_PITCH_BITS = 4,
    AIM_MULT_PITCH_MASK = (1 << AIM_MULT_PITCH_BITS) - 1,
  };

  bool operator==(const TurretAimDrivesMult &rhs) const;

  void setAimMultYaw(float mult);
  void setAimMultPitch(float mult);

  void setAimDrivesMultRaw(const Point2 &mult) { aimDrivesMult = mult; }

  const Point2 &getAimDrivesMult() const { return aimDrivesMult; }
  uint8_t getPackedState() const { return packed; }

private:
  friend struct TurretAimDrivesMultSerializer;

  void setAimMultYawBits(uint32_t v);
  void setAimMultPitchBits(uint32_t v);

  uint32_t getAimMultYawBits() const;
  uint32_t getAimMultPitchBits() const;

  Point2 aimDrivesMult = Point2(1.f, 1.f);
  uint8_t packed = -1;
};

struct TurretState
{
  struct ShootState
  {
    Point3 dir = Point3(1.f, 0.f, 0.f);
    Point3 pos = Point3(0.f, 0.f, 0.f);
    Point3 vel = Point3(0.f, 0.f, 0.f);
  };

  struct RemoteState
  {
    enum : uint32_t
    {
      WISH_DIR_SHIFT = 0,
      WISH_DIR_BITS = 24,
      WISH_DIR_MASK = (1 << WISH_DIR_BITS) - 1,

      USED_BITS = WISH_DIR_BITS
    };

    bool operator==(const RemoteState &rhs) const;

    void setWishDirection(const Point3 &dir);
    void setWishDirectionRaw(const Point3 &dir) { wishDirection = dir; }

    const Point3 &getWishDirection() const { return wishDirection; }
    uint32_t getPackedState() const { return packed; }

  private:
    friend struct TurretStateSerializer;

    void setWishDirectionBits(uint32_t v);

    uint32_t getWishDirectionBits() const;

    uint32_t packed = 0;

    Point3 wishDirection = Point3(1.f, 0.f, 0.f);
  };

  ShootState shoot;
  RemoteState remote;
  Point2 wishAngles = ZERO<Point2>();
  Point2 angles = ZERO<Point2>();
  Point2 speed = ZERO<Point2>();

  bool operator==(const TurretState &rhs) const;
};

ECS_DECLARE_RELOCATABLE_TYPE(TurretState);
ECS_DECLARE_RELOCATABLE_TYPE(TurretAimDrivesMult);

using TurretWishDirPacked = gamemath::UnitVecPacked<uint32_t, TurretState::RemoteState::WISH_DIR_BITS>;
using TurretAimMultYawPacked = gamemath::FValQuantizer<TurretAimDrivesMult::AIM_MULT_YAW_BITS, gamemath::NoScale>;
using TurretAimMultPitchPacked = gamemath::FValQuantizer<TurretAimDrivesMult::AIM_MULT_PITCH_BITS, gamemath::NoScale>;
static constexpr uint32_t TURRETS_BITS = 5;

ECS_UNICAST_EVENT_TYPE(EventOnGunCreated, ecs::EntityId);

ECS_UNICAST_EVENT_TYPE(EventOnGunPayloadCreated, ecs::EntityId, int, Point3, float, float, int);
ECS_UNICAST_EVENT_TYPE(EventOnGunsPayloadDestroyed, ecs::EntityId);
ECS_UNICAST_EVENT_TYPE(EventOnGunsPayloadAmmoUpdate, ecs::EntityId, ecs::IntList);
ECS_UNICAST_EVENT_TYPE(CmdInitPlaneSight);
