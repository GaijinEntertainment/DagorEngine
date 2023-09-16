//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <daScript/daScript.h>
#include <daScript/simulate/runtime_matrices.h>
#include <dasModules/aotDaWeaponProps.h>
#include <dasModules/aotBallisticsProps.h>
#include <dasModules/dasModulesCommon.h>
#include <dasModules/dasMacro.h>
#include <daWeapons/projectileBallisticsSight.h>
#include <ecs/game/weapons/gunLaunchEvents.h>

using LaunchDescsTab = Tab<daweap::LaunchDesc>;

MAKE_TYPE_FACTORY(LaunchDesc, daweap::LaunchDesc);
MAKE_TYPE_FACTORY(GunControls, daweap::GunControls);
MAKE_TYPE_FACTORY(GunLauchEvents, daweap::GunLaunchEvents);
MAKE_TYPE_FACTORY(SpreadSettings, SpreadSettings);

DAS_BIND_VECTOR(LaunchDescsTab, LaunchDescsTab, daweap::LaunchDesc, " LaunchDescsTab");

namespace bind_dascript
{
inline bool das_aim_projectile(const ballistics::ProjectileProps &props, const Point3 &shooter_pos, const Point3 &shooter_vel,
  const Point3 &shooter_vel_loc, const Point3 &target_pos, const Point3 &target_vel, const Point3 &target_acc, float muzzle_velocity,
  float precision_sq, bool account_ballistics, bool accounts_air_friction, float dt, float max_time, int inter_max,
  Point3 &out_shooter_dir, float &out_flight_time)
{
  Point3 hitPos;
  return ballistics::aim_projectile(props, shooter_pos, shooter_vel, shooter_vel_loc, target_pos, target_vel, target_acc,
    muzzle_velocity, precision_sq, account_ballistics, accounts_air_friction, dt, max_time, inter_max, out_shooter_dir, hitPos,
    out_flight_time);
}

inline void setInitLaunchDescHint(ecs::ComponentsInitializer &init, const char *key, uint32_t key_hash, const daweap::LaunchDesc &to)
{
  init[ecs::HashedConstString({key, key_hash})] = to;
}
inline void setInitLaunchDesc(ecs::ComponentsInitializer &init, const char *s, const daweap::LaunchDesc &to)
{
  setInitLaunchDescHint(init, s, ECS_HASH_SLOW(s ? s : "").hash, to);
}
inline void fastSetInitLaunchDesc(ecs::ComponentsInitializer &init, const ecs::component_t name, const ecs::FastGetInfo &lt,
  const daweap::LaunchDesc &to, const char *name_str)
{
  init.insert(name, lt, to, name_str);
}

} // namespace bind_dascript
