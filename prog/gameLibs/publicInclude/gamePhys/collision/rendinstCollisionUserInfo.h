//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <rendInst/rendInstCollision.h>
#include <gamePhys/collision/cachedCollisionObject.h>

#if DAGOR_DBGLEVEL > 0
extern void (*ri_coll_damage_log)(const rendinst::CollisionInfo &coll_info, float impulse, float damage);
extern void (*ri_coll_destr_result_log)(const gamephys::CollisionObjectInfo *obj_info, float applied, float absorbed, bool destroyed);
#else
static constexpr void (*ri_coll_damage_log)(const rendinst::CollisionInfo &, float, float) = nullptr;
static constexpr void (*ri_coll_destr_result_log)(const gamephys::CollisionObjectInfo *, float, float, bool) = nullptr;
#endif

class RendinstImpulseThresholdData final : public CachedCollisionObjectInfo
{
  ~RendinstImpulseThresholdData() override;

public:
  const rendinst::CollisionInfo collInfo;
  Point3 finalImpulse;
  Point3 finalPos;

  RendinstImpulseThresholdData(float impulse, const rendinst::RendInstDesc &ri_desc, float at_time,
    const rendinst::CollisionInfo &coll_info);

  float onImpulse(float impulse, const Point3 &dir, const Point3 &pos, float point_vel, const Point3 & /*collision_normal*/,
    uint32_t /*flags*/ = CIF_NONE, int32_t user_data = -1) override;
  float getDestructionImpulse() const override { return thresImpulse; }
  bool isRICollision() const override { return true; }
};

class TreeRendinstImpulseThresholdData final : public CachedCollisionObjectInfo
{
  ~TreeRendinstImpulseThresholdData() override;

public:
  const rendinst::CollisionInfo collInfo;
  Point3 finalImpulse;
  Point3 finalPos;
  float lastPointVel;
  float lastOmega;

  TreeRendinstImpulseThresholdData(float impulse, const rendinst::RendInstDesc &ri_desc, float at_time,
    const rendinst::CollisionInfo &coll_info);

  float onImpulse(float impulse, const Point3 &dir, const Point3 &pos, float point_vel, const Point3 & /*collision_normal*/,
    uint32_t /*flags*/ = CIF_NONE, int32_t user_data = -1) override;
  float getDestructionImpulse() const override { return thresImpulse; }
  bool isTreeCollision() const override { return true; }
};
