//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <daScript/daScript.h>
#include <dasModules/aotDaWeaponProps.h>
#include <dasModules/aotDaWeapons.h>
#include <dasModules/dasModulesCommon.h>
#include <daScript/simulate/runtime_matrices.h>
#include <ecs/game/weapons/gunES.h>
#include <ecs/game/weapons/gunDeviation.h>
#include <ecs/game/weapons/mountedGun.h>
#include <daWeapons/gunShootNodeData.h>

MAKE_TYPE_FACTORY(GunShootNodeData, daweap::GunShootNodeData);
MAKE_TYPE_FACTORY(Gun, daweap::Gun);
MAKE_TYPE_FACTORY(GunState, daweap::GunState);
MAKE_TYPE_FACTORY(GunProps, daweap::GunProps);
MAKE_TYPE_FACTORY(FiringMode, daweap::FiringMode);
MAKE_TYPE_FACTORY(GunLocation, daweap::GunLocation);
MAKE_TYPE_FACTORY(GunDeviationInput, daweap::GunDeviationInput);
MAKE_TYPE_FACTORY(GunDeviation, daweap::GunDeviation);
MAKE_TYPE_FACTORY(MountedGun, MountedGun)

DAS_BIND_ENUM_CAST_98_IN_NAMESPACE(::daweap::EFiringModeType, EFiringModeType);

namespace bind_dascript
{

inline void gun_shoot_node_data_calc_shoot_tm(const ::daweap::GunShootNodeData &data, const Point3 &dir, TMatrix &tm)
{
  tm = data.calcShootTm(dir);
}

inline TMatrix gun_shoot_node_data_calc_shoot_tm2(const ::daweap::GunShootNodeData &data, const Point3 &gun_dir, const TMatrix &vis_tm,
  const TMatrix &phys_tm)
{
  return data.calcShootTm(gun_dir, vis_tm, phys_tm);
}

inline void gun_force_next_shot_time(daweap::Gun &gun, const float at_time) { gun.forceNextShotTime(at_time); }

inline void gun_calculate_shoot_tm(const daweap::Gun &gun, const Point3 &gun_dir, const Point3 *gunPos, const das::float3x4 *vis_tm,
  const das::float3x4 *phys_tm, das::float3x4 &out_tm, das::Context *context, das::LineInfoArg *line_info)
{
  TMatrix &result = reinterpret_cast<TMatrix &>(out_tm);
  if (gunPos)
    result = gun.shootNodeData.calcShootTm(gun_dir, *gunPos);
  else
  {
    if (!vis_tm)
      context->throw_error_at(line_info, "vis_tm is null");
    if (!phys_tm)
      context->throw_error_at(line_info, "phys_tm is null");
    const TMatrix *visM = reinterpret_cast<const TMatrix *>(vis_tm);
    const TMatrix *physM = reinterpret_cast<const TMatrix *>(phys_tm);
    result = gun.shootNodeData.calcShootTm(gun_dir, *visM, *physM);
  }
}

inline void gun_deviation_getAppliedCT(daweap::GunDeviation &gun_deviation,
  const das::TBlock<void, das::TTemporary<daweap::GunDeviationInput>> &block, das::Context *context, das::LineInfoArg *at)
{
  daweap::GunDeviationInput &ct = gun_deviation.getAppliedCT();
  vec4f arg = das::cast<daweap::GunDeviationInput &>::from(ct);
  context->invoke(block, &arg, nullptr, at);
}

inline void gun_launch(daweap::Gun &gun, const daweap::LaunchDesc &ld, ecs::EntityId gun_eid, ecs::EntityId offender_eid)
{
  gun.launch(ld, gun_eid, offender_eid);
}
inline void gun_loadShootNode(daweap::Gun &gun, int shoot_node_idx, const GeomNodeTree &tree)
{
  gun.loadShootNode(dag::Index16(shoot_node_idx), tree);
}
inline void gun_getSpreadSettings(daweap::Gun const &gun, const daweap::GunState &state, SpreadSettings &out)
{
  out = gun.getSpreadSettings(state);
}

} // namespace bind_dascript
