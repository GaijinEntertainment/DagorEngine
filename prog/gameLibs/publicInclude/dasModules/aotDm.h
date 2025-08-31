//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <daScript/daScriptBind.h>
#include <dasModules/aotDagorMath.h>
#include <dasModules/aotEcs.h>
#include <dasModules/dasManagedTab.h>
#include <dasModules/dasDataBlock.h>
#include <dasModules/aotProps.h>
#include <dag/dag_vector.h>
#include <dag/dag_vectorSet.h>
#include <damageModel/damageEffects.h>
#include <damageModel/damageModelParams.h>
#include <damageModel/damagePart.h>
#include <ecs/game/dm/collisionData.h>
#include <damageModel/splashDamage.h>
#include <damageModel/syntheticShatterDamage.h>
#include <damageModel/criticalDamageTester.h>
#include <damageModel/damagePartUtils.h>
#include <damageModel/explosive.h>
#include <damageModel/metaParts.h>
#include <damageModel/blkUtils.h>
#include <damageModel/fireDamage.h>
#include <damageModel/damageTypes.h>
#include <damageModel/damagePart.h>
#include <damageModel/damagePartProps.h>
#include <damageModel/damageToPartEvent.h>
#include <damageModel/dmObjectUtils.h>
#include <damageModel/partNameMaps.h>
#include <ecs/game/dm/partId.h>
#include <ecs/game/dm/damageModel.h>
#include <ecs/game/dm/damageModelData.h>

typedef dag::Vector<dm::DamagePartProps> DamageModelDataPartProps;
typedef dag::Vector<dm::DamagePart> DamageModelDataParts;

using MetaPartPartIds = dag::VectorSet<dm::PartId>;

DAS_TYPE_DECL(DamageToPartEvent, dm::DamageToPartEvent);
DAS_TYPE_DECL(ExplosiveProps, dm::ExplosiveProperties);
DAS_TYPE_DECL(ExplosiveMassToSplash, dm::ExplosiveMassToSplash);
DAS_TYPE_DECL(MetaPart, dm::MetaPart);
DAS_TYPE_DECL(CollisionData, dm::CollisionData);
DAS_TYPE_DECL(DamageModelData, dm::DamageModelData);
DAS_TYPE_DECL(DamageModel, dm::DamageModel);
DAS_TYPE_DECL(HitData, dm::HitData);
DAS_TYPE_DECL(DamagePart, dm::DamagePart);
DAS_TYPE_DECL(DamagePartProps, dm::DamagePartProps);
DAS_BIND_VECTOR(DamageModelDataPartProps, DamageModelDataPartProps, dm::DamagePartProps, "DamageModelDataPartProps");
DAS_BIND_VECTOR(DamageModelDataParts, DamageModelDataParts, dm::DamagePart, "DamageModelDataParts");

DAS_TYPE_DECL_BEGIN(PartId, dm::PartId)
bool isLocal() const override { return true; }
bool isRawPod() const override { return true; }
DAS_TYPE_DECL_END;
DAS_BIND_VECTOR(PartIdList, dm::PartIdList, dm::PartId, "::dm::PartIdList");

DAS_TYPE_DECL_BEGIN(SplashParams, dm::splash::Params)
bool isLocal() const override { return true; }
DAS_TYPE_DECL_END;
DAS_TYPE_DECL(DamageModelParams, dm::DamageModelParams);
DAS_TYPE_DECL(DamageEffectPreset, dm::effect::Preset);
DAS_TYPE_DECL(DmEffectPresetList, dm::effect::PresetList);
DAS_TYPE_DECL(MetaPartProp, dm::MetaPartProp);
DAS_TYPE_DECL(DamageEffectActionCluster, dm::effect::ActionCluster);
DAS_TYPE_DECL(SplashProps, dm::splash::Properties);

DAS_BIND_VECTOR_SET(MetaPartPartIds, MetaPartPartIds, dm::PartId, " ::MetaPartPartIds")
DAS_BIND_VECTOR(MetaPartPropsVector, dm::MetaPartPropsVector, dm::MetaPartProp, " ::dm::MetaPartPropsVector")
DAS_BIND_VECTOR(DamageEffectPresetsVector, dag::Vector<dm::effect::Preset>, dm::effect::Preset, "::dag::Vector<dm::effect::Preset>")

DAS_BIND_ENUM_CAST_98_IN_NAMESPACE(::dm::effect::ComplexAction::Type, DamageEffectType);
DAS_BIND_ENUM_CAST_98_IN_NAMESPACE(::dm::effect::ComplexAction::TypeMask, DamageEffectsMask);
DAS_BIND_ENUM_CAST_98_IN_NAMESPACE(::dm::DamagePartState, DamagePartState);

DAS_BIND_ENUM_CAST(dm::crit_test::TestResult)
DAS_BASE_BIND_ENUM(dm::crit_test::TestResult, TestResult, EMPTY, RICOCHET, NOT_PENETRATE, INEFFECTIVE, POSSIBLE_EFFECTIVE, EFFECTIVE);


DAS_BIND_ENUM_CAST(dm::KillEffect)
DAS_BASE_BIND_ENUM(dm::KillEffect, KillEffect, KILL_EFF_NONE, KILL_EFF_INACTIVE, KILL_EFF_DAMAGED, KILL_EFF_SCORCHED, KILL_EFF_SMOKE,
  KILL_EFF_FIRE, KILL_EFF_EXPLOSION, KILL_EFF_DROWN, KILL_EFF_DESTRUCTION, KILL_EFF_WRECKED, KILL_EFF_HULL_BREAK);

DAS_TYPE_DECL(Ray3, Ray3);

namespace bind_dascript
{
static inline int dm_read_overrided_preset(dm::effect::PresetList &presets, const DataBlock &blk, const char *preset_param_name,
  int default_preset_id, const dm::DamageModelData &dm_data)
{
  dm::DataBlockRWHelper dataBlockRWHelper(dm::get_damage_types(), dm::get_part_names_maps());
  return dm::effect::read_overrided_preset_ex(presets, blk, preset_param_name, default_preset_id, dm_data.props.parts, dm_data.parts,
    nullptr, dataBlockRWHelper);
}

static inline uint16_t get_rel_hp_fixed(const dm::DamageModelData &dm_data, int part_id)
{
  return get_part_hp_fixed(dm_data.parts, dm::PartId(part_id, -1));
}

static inline const char *get_meta_part_prop_name(const dm::MetaPartProp &prop) { return prop.name.c_str(); }

static inline dm::PartId find_part_id(const dm::DamageModelData &dm_data, const char *part_name)
{
  return find_part_id(dm_data.props.parts, dm_data.parts, dm::get_part_names_maps().partNames, part_name);
}

static inline int find_part_id_by_name(const dm::DamageModel &damage_model, const char *part_name)
{
  return find_part_id(damage_model.dmData.props.parts, damage_model.dmData.parts, dm::get_part_names_maps().partNames, part_name).id;
}

static inline int get_damage_part_id(const dm::DamagePart &dm_part) { return dm_part.partId.id; }

static inline dm::PartId make_part_id(int id, int globalId) { return dm::PartId(static_cast<uint8_t>(id), globalId); }

static inline dm::PartId unpack_part_id(uint32_t packed) { return dm::PartId(packed); }

static inline int collisiondata_getGeomNodeIdByCollNode(const dm::CollisionData &c, int n) { return (int)c.collNodeIdToGeomNodeId[n]; }

static inline int get_part_id_by_coll_node_id(const dm::DamageModelData &dm_data, int coll_node_id)
{
  return dm::get_part_id_by_coll_node_id(dm_data.collisionNodeIdToPartId, coll_node_id).id;
}

static inline void dm_effect_on_part_kill(const dm::effect::ActionCluster &action_cluster, int &dm_effects, int &rand_seed,
  bool &out_is_critical)
{
  dm::effect::on_part_kill(action_cluster, dm_effects, rand_seed, out_is_critical);
}

static inline void dm_effect_on_part_hit(const dm::effect::ActionCluster &action_cluster, const dm::DamageModelParams &,
  int &dm_effects, int &rand_seed, float, float, uint16_t, bool &out_is_critical)
{
  dm::effect::on_part_hit(action_cluster, dm_effects, rand_seed, out_is_critical, nullptr);
}

static inline int get_collision_node_id(const dm::DamageModelData &dm_data, int part_id)
{
  return get_collision_node_id(dm_data.props.parts, dm::PartId(part_id, -1));
}

static inline float get_max_hp(const dm::DamageModelData &dm_data, int part_id)
{
  return get_part_hp_total(dm_data.props.parts, dm::PartId(part_id, -1));
}

static inline bool is_part_inner(const dm::DamageModelData &dm_data, int part_id)
{
  const dm::DamagePartProps *props = get_part_props(dm_data.props.parts, dm::PartId(part_id, -1));
  return props && props->testFlag(dm::DamagePartProps::Flag::INNER);
}

static inline int get_part_physmat_id(const dm::DamageModelData &dm_data, int part_id)
{
  const dm::DamagePartProps *props = dm::get_part_props(dm_data.props.parts, dm::PartId(part_id, -1));
  return props ? props->physMaterialId : -1;
}

static inline dm::splash::Params calc_splash_params(const dm::ExplosiveProperties &explosive_props,
  const dm::splash::Properties &splash_properties, bool underwater)
{
  dm::splash::Params params;
  if (explosive_props.mass > 0.f && splash_properties.type != dm::splash::Properties::Type::PREDEFINED)
    dm::splash::calc_explosive_params(explosive_props, 1.f, params.explosiveParams);
  dm::splash::calc_params(splash_properties, underwater ? dm::PhysEnvironment::WATER : dm::PhysEnvironment::AIR,
    dm::get_explosive_settings().massToSplash, params);
  return params;
}

static inline const dm::effect::ActionCluster *get_damage_effect_action_cluster(const dm::effect::Preset &preset, bool on_kill,
  int damage_type_id, float damage)
{
  dm::effect::Context context;
  context.onKill = on_kill;
  context.damage = damage;
  context.damageTypeId = dm::make_damage_type_id(damage_type_id);
  return preset.getActionCluster(context);
}

static inline float get_part_hp_prop_value(const dm::DamageModelData &dm_data, const dm::PartId &part_id)
{
  const dm::DamagePartProps *props = dm::get_part_props(dm_data.props.parts, part_id);
  return props ? props->hp : 0.f;
}

static inline const char *das_get_part_name(const dm::DamageModelData &dm_data, int part_id)
{
  return dm::get_part_names_maps().partNames.getName(dm_data.props.parts[part_id].partNameId);
}

static inline bool das_is_coll_node_traceable(const dm::DamageModelData &dm_data, int coll_node_id, bool reverse)
{
  return dm::is_coll_node_traceable(dm_data.props.parts, dm_data.parts, dm_data.decorMask, dm_data.collisionNodeIdToPartId,
    dm_data.collisionNodeExclude, coll_node_id, nullptr, reverse);
}

static inline void das_load_dm_part_props(dm::DamagePartProps &props, const char *blk_name)
{
  DataBlock blk(blk_name);
  dm::DataBlockRWHelper dataBlockRWHelper(dm::get_damage_types(), dm::get_part_names_maps());
  props.load(blk, dataBlockRWHelper, dm::get_fire_presets(), true);
}

static inline void splash_props_load(dm::splash::Properties &props, const DataBlock &blk)
{
  dm::DataBlockRWHelper dataBlockRWHelper(dm::get_damage_types(), dm::get_part_names_maps());
  props.load(blk, dataBlockRWHelper);
}

static inline void explosive_props_load(dm::ExplosiveProperties &props, const DataBlock &blk, float mass_mult)
{
  props.load(blk, dm::get_explosive_settings(), mass_mult);
}

inline int find_damage_type_id_by_name(const char *name) { return dm::get_damage_types().findTypeByName(name).getIdx(); }

static inline void deal_damage_to_part(dm::DamageModel &dmComp, ecs::EntityId eid, int part_id, int damag_type_id,
  int damage_effects_mask, float damage, const dm::HitData &hit_data)
{
  dm::deal_damage_to_part(dmComp, eid, part_id, damag_type_id, damage_effects_mask, damage, hit_data);
}

static inline float calc_kinetic_penetration_shift(const InterpolateTabFloat &table, float residual_penetration, float distance,
  float scale, float shift)
{
  return dm::kinetic::calc_penetration_shift(table, residual_penetration, distance, scale, shift);
}

inline int get_splash_damage_type_id(const dm::splash::Properties &props) { return props.damageTypeId.getIdx(); }

inline int get_damage_part_props_count(const dm::DamageModelData &dm_data) { return dm_data.parts.size(); }

inline const dm::DamagePart &get_damage_part_props(const dm::DamageModelData &dm_data, const dm::PartId &part_id)
{
  return dm_data.parts[part_id.id];
}

inline const dm::DamagePart &get_damage_part_props(const dm::DamageModelData &dm_data, int part_id) { return dm_data.parts[part_id]; }

inline dm::DamagePart &get_damage_part_props_for_modification(dm::DamageModelData &dm_data, const dm::PartId &part_id)
{
  return dm_data.parts[part_id.id];
}

inline dm::DamagePart &get_damage_part_props_for_modification(dm::DamageModelData &dm_data, int part_id)
{
  return dm_data.parts[part_id];
}

inline void damage_part_set_enabled(dm::DamagePart &dp, bool e) { dp.enabled = e ? 1 : 0; }

inline bool is_valid_part_id(const dm::DamageModelData &dm_data, const dm::PartId &part_id)
{
  return is_valid_part_id(dm_data.props.parts, part_id);
}

inline bool is_part_enabled(const dm::DamageModelData &dm_data, const dm::PartId &part_id)
{
  return is_part_enabled(dm_data.parts, part_id);
}

inline dm::DamagePartState get_part_state(const dm::DamageModelData &dm_data, const dm::PartId &part_id)
{
  return get_part_state(dm_data.parts, part_id);
}

inline uint16_t get_part_hp_fixed(const dm::DamageModelData &dm_data, const dm::PartId &part_id)
{
  return get_part_hp_fixed(dm_data.parts, part_id);
}

inline float get_part_hp_rel(const dm::DamageModelData &dm_data, const dm::PartId &part_id)
{
  return get_part_hp_rel(dm_data.parts, part_id);
}

inline float get_part_hp(const dm::DamageModelData &dm_data, const dm::PartId &part_id)
{
  return get_part_hp(dm_data.props.parts, dm_data.parts, part_id);
}

inline bool is_part_alive(const dm::DamageModelData &dm_data, const dm::PartId &part_id)
{
  return is_part_alive(dm_data.parts, part_id);
}

inline bool is_part_damaged(const dm::DamageModelData &dm_data, const dm::PartId &part_id)
{
  return is_part_damaged(dm_data.parts, part_id);
}

inline bool is_part_dead(const dm::DamageModelData &dm_data, const dm::PartId &part_id)
{
  return is_part_dead(dm_data.parts, part_id);
}

inline bool has_damaged_part(const dm::DamageModelData &dm_data) { return has_damaged_part(dm_data.parts); }

} // namespace bind_dascript
