//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <daScript/daScript.h>
#include <dasModules/aotDagorMath.h>
#include <dasModules/aotEcs.h>
#include <dasModules/dasManagedTab.h>
#include <dasModules/dasDataBlock.h>
#include <dasModules/aotProps.h>
#include <dag/dag_vector.h>
#include <dag/dag_vectorSet.h>
#include <ecs/game/dm/damageModel.h>
#include <damageModel/damageEffects.h>
#include <damageModel/damageModelData.h>
#include <damageModel/damageModelParams.h>
#include <damageModel/damagePart.h>
#include <damageModel/propsRegistryUtils.h>
#include <damageModel/collisionData.h>
#include <damageModel/splashDamage.h>
#include <damageModel/criticalDamageTester.h>
#include <ecs/game/dm/partId.h>
#include <damageModel/damagePartUtils.h>

typedef dag::Vector<dm::DamagePart> DamageModelDataParts;

using MetaPartPartIds = dag::VectorSet<dm::PartId>;

MAKE_TYPE_FACTORY(DamageToPartEvent, dm::DamageToPartEvent)
MAKE_TYPE_FACTORY(PowderProps, dm::damage::PowderProps)
MAKE_TYPE_FACTORY(ExplosiveProps, dm::ExplosiveProps)
MAKE_TYPE_FACTORY(ExplosiveMassToSplash, dm::ExplosiveMassToSplash)
MAKE_TYPE_FACTORY(MetaPart, dm::MetaPart);
MAKE_TYPE_FACTORY(CollisionData, dm::CollisionData);
MAKE_TYPE_FACTORY(DamageModelData, dm::DamageModelData);
MAKE_TYPE_FACTORY(DamageModel, dm::DamageModel);
MAKE_TYPE_FACTORY(HitData, dm::HitData);
MAKE_TYPE_FACTORY(DamagePartProps, dm::DamagePartProps);
MAKE_TYPE_FACTORY(DamagePart, dm::DamagePart);
DAS_BIND_VECTOR(DamageModelDataParts, DamageModelDataParts, dm::DamagePart, "DamageModelDataParts");
MAKE_TYPE_FACTORY(PartId, dm::PartId);
DAS_BIND_VECTOR(PartIdList, dm::PartIdList, dm::PartId, "::dm::PartIdList");
MAKE_TYPE_FACTORY(SplashParams, dm::splash::Params);
MAKE_TYPE_FACTORY(DamageModelParams, dm::DamageModelParams);
MAKE_TYPE_FACTORY(DamageEffectPreset, dm::effect::Preset);
MAKE_TYPE_FACTORY(DmEffectPresetList, dm::effect::PresetList);
MAKE_TYPE_FACTORY(MetaPartProp, dm::MetaPartProp);
MAKE_TYPE_FACTORY(PenetrationTableProps, dm::kinetic::PenetrationTableProps);
MAKE_TYPE_FACTORY(DamageTableProps, dm::kinetic::DamageTableProps);
MAKE_TYPE_FACTORY(EffectsProbabilityMultiplierProps, dm::kinetic::EffectsProbabilityMultiplierProps);
MAKE_TYPE_FACTORY(DamageEffectActionCluster, dm::effect::ActionCluster);

DAS_BIND_VECTOR_SET(MetaPartPartIds, MetaPartPartIds, dm::PartId, " ::MetaPartPartIds")
DAS_BIND_VECTOR(MetaPartPropsVector, dm::MetaPartPropsVector, dm::MetaPartProp, " ::dm::MetaPartPropsVector")
DAS_BIND_VECTOR(DamageEffectPresetsVector, dag::Vector<dm::effect::Preset>, dm::effect::Preset, "::dag::Vector<dm::effect::Preset>")

DAS_BIND_ENUM_CAST_98_IN_NAMESPACE(::dm::effect::ComplexAction::Type, DamageEffectType);
DAS_BIND_ENUM_CAST_98_IN_NAMESPACE(::dm::effect::ComplexAction::TypeMask, DamageEffectsMask);
DAS_BIND_ENUM_CAST_98_IN_NAMESPACE(::dm::DamagePartState, DamagePartState);

DAS_BASE_BIND_ENUM_CAST(dm::ObjectDescriptor::Type, ObjectDescriptorType);
DAS_BASE_BIND_ENUM(dm::ObjectDescriptor::Type, ObjectDescriptorType, WATER, LANDSCAPE, STATIC, COLLISION, CARCASS, RENDINST, TREE,
  PROJECTILE)

DAS_BIND_ENUM_CAST(dm::crit_test::TestResult)
DAS_BASE_BIND_ENUM(dm::crit_test::TestResult, TestResult, EMPTY, RICOCHET, NOT_PENETRATE, INEFFECTIVE, POSSIBLE_EFFECTIVE, EFFECTIVE);


DAS_BIND_ENUM_CAST(dm::KillEffect)
DAS_BASE_BIND_ENUM(dm::KillEffect, KillEffect, KILL_EFF_NONE, KILL_EFF_INACTIVE, KILL_EFF_DAMAGED, KILL_EFF_SCORCHED, KILL_EFF_SMOKE,
  KILL_EFF_FIRE, KILL_EFF_EXPLOSION, KILL_EFF_DROWN, KILL_EFF_DESTRUCTION, KILL_EFF_WRECKED, KILL_EFF_HULL_BREAK);

namespace bind_dascript
{
template <typename Preset, typename Params>
int dm_read_overrided_preset(PropertyPresets<Preset> &presets, const DataBlock &blk, const char *preset_param_name,
  int default_preset_id, const Params &params)
{
  return read_overrided_preset(presets, blk, preset_param_name, default_preset_id, &params);
}

inline int dm_register_props_ex(const char *name, const DataBlock &blk, const char *class_name)
{
  return dm::register_props_ex(name, &blk, class_name);
}

inline uint16_t get_rel_hp_fixed(const dm::DamageModelData &dm_data, int part_id)
{
  return get_part_hp_fixed(dm_data, dm::PartId(part_id, -1));
}

inline const char *get_meta_part_prop_name(const dm::MetaPartProp &prop) { return prop.name.c_str(); }

inline int find_part_id_by_name(const dm::DamageModel &damage_model, const char *part_name)
{
  return find_part_id(damage_model.dmData, part_name).id;
}

inline int get_damage_part_id(const dm::DamagePart &dm_part) { return dm_part.partId.id; }

inline dm::PartId make_part_id(int id, int globalId) { return dm::PartId(static_cast<uint8_t>(id), globalId); }

inline dm::PartId unpack_part_id(uint32_t packed) { return dm::PartId(packed); }

inline int collisiondata_getGeomNodeIdByCollNode(const dm::CollisionData &c, int n) { return (int)c.collNodeIdToGeomNodeId[n]; }

inline int get_part_id_by_coll_node_id(const dm::DamageModelData &dm_data, int coll_node_id)
{
  return dm::get_part_id_by_coll_node_id(dm_data, coll_node_id).id;
}
inline void dm_effect_on_part_kill(const dm::effect::ActionCluster &action, int &dm_effects, int &rand_seed, bool &out_is_critical,
  const das::TBlock<void, dm::PartId, bool> &block, das::Context *context, das::LineInfoArg *at)
{
  dm::effect::on_part_kill(
    action,
    [&](dm::PartId partId, bool canCut) {
      vec4f args[] = {das::cast<const dm::PartId &>::from(partId), das::cast<bool>::from(canCut)};
      context->invoke(block, args, nullptr, at);
    },
    dm_effects, rand_seed, out_is_critical);
}

inline void dm_effect_on_part_hit(const dm::effect::ActionCluster &action, const dm::DamageModelParams &params, int &dm_effects,
  int &rand_seed, float chance_mult, uint16_t part_hp_fixed, bool &out_is_critical,
  const das::TBlock<float, int, dm::PartId> &calc_chance_multiplier, const das::TBlock<void, dm::PartId, bool> &block,
  das::Context *context, das::LineInfoArg *at)
{
  dm::effect::on_part_hit(
    action, params,
    [&](int type, dm::PartId partId) {
      vec4f args[] = {
        das::cast<bool>::from(type),
        das::cast<const dm::PartId &>::from(partId),
      };
      return das::cast<float>::to(context->invoke(calc_chance_multiplier, args, nullptr, at));
    },
    [&](dm::PartId partId, bool canCut) {
      vec4f args[] = {das::cast<const dm::PartId &>::from(partId), das::cast<bool>::from(canCut)};
      context->invoke(block, args, nullptr, at);
    },
    dm_effects, rand_seed, chance_mult, part_hp_fixed, out_is_critical);
}

inline int get_collision_node_id(const dm::DamageModelData &dm_data, int part_id)
{
  return get_collision_node_id(dm_data, dm::PartId(part_id, -1));
}

inline float get_max_hp(const dm::DamageModelData &dm_data, int part_id) { return get_part_max_hp(dm_data, dm::PartId(part_id, -1)); }

inline dm::splash::Params calc_splash_params(int damage_props_id, bool underwater)
{
  return dm::splash::calc_params(damage_props_id, underwater ? dm::PhysEnvironment::WATER : dm::PhysEnvironment::AIR);
}

inline const dm::effect::ActionCluster *get_damage_effect_action_cluster(const dm::effect::Preset &preset, bool on_kill,
  int damage_type_id, float damage)
{
  dm::effect::Context context;
  context.onKill = on_kill;
  context.damage = damage;
  context.damageTypeId = damage_type_id;
  return preset.getActionCluster(context);
}

} // namespace bind_dascript
