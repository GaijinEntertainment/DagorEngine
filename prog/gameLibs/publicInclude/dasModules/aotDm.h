//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
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
#include <damageModel/damageEffects.h>
#include <damageModel/damageModelData.h>
#include <damageModel/damageModelParams.h>
#include <damageModel/damagePart.h>
#include <damageModel/collisionData.h>
#include <damageModel/splashDamage.h>
#include <damageModel/syntheticShatterDamage.h>
#include <damageModel/criticalDamageTester.h>
#include <damageModel/damagePartUtils.h>
#include <damageModel/explosive.h>
#include <damageModel/metaParts.h>
#include <damageModel/blkUtils.h>
#include <ecs/game/dm/partId.h>
#include <ecs/game/dm/damageModel.h>

typedef dag::Vector<dm::DamagePartProps> DamageModelDataPartProps;
typedef dag::Vector<dm::DamagePart> DamageModelDataParts;

using MetaPartPartIds = dag::VectorSet<dm::PartId>;

MAKE_TYPE_FACTORY(DamageToPartEvent, dm::DamageToPartEvent)
MAKE_TYPE_FACTORY(ExplosiveProps, dm::ExplosiveProperties);
MAKE_TYPE_FACTORY(ExplosiveMassToSplash, dm::ExplosiveMassToSplash)
MAKE_TYPE_FACTORY(MetaPart, dm::MetaPart);
MAKE_TYPE_FACTORY(CollisionData, dm::CollisionData);
MAKE_TYPE_FACTORY(DamageModelData, dm::DamageModelData);
MAKE_TYPE_FACTORY(DamageModel, dm::DamageModel);
MAKE_TYPE_FACTORY(HitData, dm::HitData);
MAKE_TYPE_FACTORY(DamagePart, dm::DamagePart);
MAKE_TYPE_FACTORY(DamagePartProps, dm::DamagePartProps);
DAS_BIND_VECTOR(DamageModelDataPartProps, DamageModelDataPartProps, dm::DamagePartProps, "DamageModelDataPartProps");
DAS_BIND_VECTOR(DamageModelDataParts, DamageModelDataParts, dm::DamagePart, "DamageModelDataParts");
MAKE_TYPE_FACTORY(PartId, dm::PartId);
DAS_BIND_VECTOR(PartIdList, dm::PartIdList, dm::PartId, "::dm::PartIdList");
MAKE_TYPE_FACTORY(SplashParams, dm::splash::Params);
MAKE_TYPE_FACTORY(DamageModelParams, dm::DamageModelParams);
MAKE_TYPE_FACTORY(DamageEffectPreset, dm::effect::Preset);
MAKE_TYPE_FACTORY(DmEffectPresetList, dm::effect::PresetList);
MAKE_TYPE_FACTORY(MetaPartProp, dm::MetaPartProp);
MAKE_TYPE_FACTORY(DamageEffectActionCluster, dm::effect::ActionCluster);
MAKE_TYPE_FACTORY(SplashProps, dm::splash::Properties);

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
inline int dm_read_overrided_preset(dm::effect::PresetList &presets, const DataBlock &blk, const char *preset_param_name,
  int default_preset_id, const dm::DamageModelData &dm_data)
{
  dm::DataBlockRWHelper dataBlockRWHelper(dm::get_damage_types(), dm::get_part_names_maps());
  return dm::effect::read_overrided_preset_ex(presets, blk, preset_param_name, default_preset_id, &dm_data, nullptr,
    dataBlockRWHelper);
}

inline uint16_t get_rel_hp_fixed(const dm::DamageModelData &dm_data, int part_id)
{
  return get_part_hp_fixed(dm_data, dm::PartId(part_id, -1));
}

inline const char *get_meta_part_prop_name(const dm::MetaPartProp &prop) { return prop.name.c_str(); }

inline dm::PartId find_part_id(const dm::DamageModelData &dm_data, const char *part_name)
{
  return find_part_id(dm_data, dm::get_part_names_maps().partNames, part_name);
}

inline int find_part_id_by_name(const dm::DamageModel &damage_model, const char *part_name)
{
  return find_part_id(damage_model.dmData, dm::get_part_names_maps().partNames, part_name).id;
}

inline int get_damage_part_id(const dm::DamagePart &dm_part) { return dm_part.partId.id; }

inline dm::PartId make_part_id(int id, int globalId) { return dm::PartId(static_cast<uint8_t>(id), globalId); }

inline dm::PartId unpack_part_id(uint32_t packed) { return dm::PartId(packed); }

inline int collisiondata_getGeomNodeIdByCollNode(const dm::CollisionData &c, int n) { return (int)c.collNodeIdToGeomNodeId[n]; }

inline int get_part_id_by_coll_node_id(const dm::DamageModelData &dm_data, int coll_node_id)
{
  return dm::get_part_id_by_coll_node_id(dm_data, coll_node_id).id;
}
inline void dm_effect_on_part_kill(const dm::effect::ActionCluster &action_cluster, int &dm_effects, int &rand_seed,
  bool &out_is_critical)
{
  dm::effect::on_part_kill(action_cluster, dm_effects, rand_seed, out_is_critical);
}

inline void dm_effect_on_part_hit(const dm::effect::ActionCluster &action_cluster, const dm::DamageModelParams &, int &dm_effects,
  int &rand_seed, float, float, uint16_t, bool &out_is_critical)
{
  dm::effect::on_part_hit(action_cluster, dm_effects, rand_seed, out_is_critical, nullptr);
}

inline int get_collision_node_id(const dm::DamageModelData &dm_data, int part_id)
{
  return get_collision_node_id(dm_data, dm::PartId(part_id, -1));
}

inline float get_max_hp(const dm::DamageModelData &dm_data, int part_id)
{
  return get_part_hp_total(dm_data, dm::PartId(part_id, -1));
}

inline bool is_part_inner(const dm::DamageModelData &dm_data, int part_id)
{
  const dm::DamagePartProps *props = get_part_props(dm_data, dm::PartId(part_id, -1));
  return props && props->testFlag(dm::DamagePartProps::Flag::INNER);
}

inline int get_part_physmat_id(const dm::DamageModelData &dm_data, int part_id)
{
  const dm::DamagePartProps *props = dm::get_part_props(dm_data, dm::PartId(part_id, -1));
  return props ? props->physMaterialId : -1;
}

inline dm::splash::Params calc_splash_params(const dm::ExplosiveProperties &explosive_props,
  const dm::splash::Properties &splash_properties, bool underwater)
{
  dm::splash::Params params;
  if (explosive_props.mass > 0.f && splash_properties.type != dm::splash::Properties::Type::PREDEFINED)
    dm::splash::calc_explosive_params(explosive_props, 1.f, params.explosiveParams);
  dm::splash::calc_params(splash_properties, underwater ? dm::PhysEnvironment::WATER : dm::PhysEnvironment::AIR,
    dm::get_explosive_settings().massToSplash, params);
  return params;
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

inline float get_part_hp_prop_value(const dm::DamageModelData &dm_data, const dm::PartId &part_id)
{
  const dm::DamagePartProps *props = dm::get_part_props(dm_data, part_id);
  return props ? props->hp : 0.f;
}

inline const char *das_get_part_name(const dm::DamagePartProps &props)
{
  return dm::get_part_names_maps().partNames.getName(props.partNameId);
}

inline bool das_is_coll_node_traceable(const dm::DamageModelData &dm_data, int coll_node_id, bool reverse)
{
  return dm::is_coll_node_traceable(dm_data, coll_node_id, nullptr, reverse);
}

inline void das_load_dm_part_props(dm::DamagePartProps &props, const char *blk_name)
{
  DataBlock blk(blk_name);
  dm::DataBlockRWHelper dataBlockRWHelper(dm::get_damage_types(), dm::get_part_names_maps());
  props.load(blk, dataBlockRWHelper, true);
}

inline void splash_props_load(dm::splash::Properties &props, const DataBlock &blk)
{
  dm::DataBlockRWHelper dataBlockRWHelper(dm::get_damage_types(), dm::get_part_names_maps());
  props.load(blk, dataBlockRWHelper);
}

} // namespace bind_dascript
