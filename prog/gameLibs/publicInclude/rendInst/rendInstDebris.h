//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <vecmath/dag_vecMathDecl.h>
#include <math/dag_TMatrix.h>
#include <math/dag_Point3.h>
#include <phys/dag_physResource.h>
#include <util/dag_bitFlagsMask.h>
#include <util/dag_stdint.h>
#include <generic/dag_carray.h>
#include <EASTL/fixed_function.h>

#include <rendInst/rendInstDesc.h>
#include <rendInst/constants.h>
#include <rendInst/rendInstGenDamageInfo.h>
#include <rendInst/treeDestr.h>


class AcesEffect;

namespace rendinst
{

using ri_damage_effect_cb = void (*)(int type, const TMatrix &emitter_tm, const TMatrix &fx_tm, int pool_idx, bool is_player,
  AcesEffect **locked_fx, const char *effect_template);

using damage_effect_cb = void (*)(int type, const TMatrix &emitter_tm, const TMatrix &fx_tm, int pool_idx, bool is_player,
  AcesEffect **locked_fx, const char *effect_template);

void doRIGenDamage(const BSphere3 &sphere, unsigned frameNo, const Point3 &axis = Point3(0.f, 0.f, 0.f), bool create_debris = true);
void doRIGenDamage(const BBox3 &box, unsigned frameNo, const Point3 &axis = Point3(0.f, 0.f, 0.f), bool create_debris = true);
void doRIGenDamage(const BBox3 &box, unsigned frameNo, const TMatrix &check_itm, const Point3 &axis = Point3(0.f, 0.f, 0.f),
  bool create_debris = true);
void doRIGenDamage(const Point3 &pos, unsigned frameNo, ri_damage_effect_cb effect_cb, const Point3 &axis = Point3(0.f, 0.f, 0.f),
  float dmg_pts = 1000, bool create_debris = true);

struct RendInstBufferData
{
  mat44f tm;
  carray<int16_t, 12> data;
};

DynamicPhysObjectData *doRIGenDestr(const RendInstDesc &desc, RendInstBufferData &out_buffer, bool create_destr_effects,
  ri_damage_effect_cb effect_cb, riex_handle_t &out_destroyed_riex_handle, int32_t user_data = -1, const Point3 *coll_point = nullptr,
  bool *ri_removed = nullptr, DestrOptionFlags destroy_flags = DestrOptionFlag::AddDestroyedRi | DestrOptionFlag::ForceDestroy,
  const Point3 &impulse = Point3::ZERO, const Point3 &impulse_pos = Point3::ZERO);

// This one ignores subcells and doesn't return buffer (as we use it when we don't need to restore it)
// As well it doesn't updateVb, as it's used in batches, so you'll updateVb only once, when you need it
DynamicPhysObjectData *doRIGenDestrEx(const RendInstDesc &desc, bool create_destr_effects, ri_damage_effect_cb effect_cb = nullptr,
  int32_t user_data = -1);
DynamicPhysObjectData *doRIExGenDestrEx(rendinst::riex_handle_t riex_handle, ri_damage_effect_cb effect_cb = nullptr);

bool restoreRiGen(const RendInstDesc &desc, const RendInstBufferData &buffer);
riex_handle_t restoreRiGenDestr(const RendInstDesc &desc, const RendInstBufferData &buffer);

struct TreeInstData
{
  float timer = 0.0f;
  float disappearStartTime = -1.0f;
  float maxDistanceSq = 0.0f;
  int rndSeed = 0;
  Point2 impactXZ = Point2(0.0f, 0.0f);
  rendinstdestr::TreeDestr::BranchDestr branchDestr;
};

struct TreeInstDebugData
{
  rendinstdestr::TreeDestr::BranchDestr branchDestrFromDamage;
  rendinstdestr::TreeDestr::BranchDestr branchDestrOther;
  float timer_offset = 0.0f;
  bool last_object_was_from_damage = false;
};

struct DestroyedRi
{
  vec3f axis = {};
  vec3f normal = {};
  unsigned frameAdded = 0;
  float timeToDamage = 0;
  int fxType = 0;
  riex_handle_t riHandle = RIEX_HANDLE_NULL;
  int16_t propsId;   // Index in `riDebrisMap`. Negaive if doesn't need update
  uint16_t debrisNo; // Index in `riDebris`
  bool shouldUpdate = false;
  float accumulatedPower = 0;
  float rotationPower = 0;

  RendInstBufferData savedData = {};

  DestroyedRi() = default;
  DestroyedRi(const DestroyedRi &) = delete;
  ~DestroyedRi();
};

DestroyedRi *doRIGenExternalControl(const RendInstDesc &desc, bool rem_rendinst = true);
bool fillTreeInstData(const RendInstDesc &desc, const Point2 &impact_velocity_xz, bool from_damage, TreeInstData &out_data);
void updateTreeDestrRenderData(const TMatrix &original_tm, riex_handle_t ri_handle, TreeInstData &tree_inst_data,
  const TreeInstDebugData *tree_inst_debug_data = nullptr);

bool should_clear_external_controls();

bool returnRIGenExternalControl(const RendInstDesc &desc, DestroyedRi *ri);
bool removeRIGenExternalControl(const RendInstDesc &desc, DestroyedRi *ri, bool lock = true);

void play_riextra_dmg_fx(rendinst::riex_handle_t id, const Point3 &pos, ri_damage_effect_cb effect_cb);

bool getDestrCellData(int layer_idx, const eastl::fixed_function<64, bool(const Tab<DestroyedCellData> &)> &callback);

} // namespace rendinst