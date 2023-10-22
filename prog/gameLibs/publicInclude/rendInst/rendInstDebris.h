//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

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


class AcesEffect;

namespace rendinst
{

using ri_damage_effect_cb = void (*)(int type, const TMatrix &emitter_tm, const TMatrix &fx_tm, int pool_idx, bool is_player,
  AcesEffect **locked_fx, const char *effect_template);

bool destroyRIGenWithBullets(const Point3 &from, const Point3 &dir, float &dist, Point3 &norm, bool killBuildings, unsigned frameNo,
  ri_damage_effect_cb effect_cb);
bool destroyRIGenInSegment(const Point3 &p0, const Point3 &p1, bool trees, bool buildings, Point4 &contactPt, bool doKill,
  unsigned frameNo, ri_damage_effect_cb effect_cb);


using damage_effect_cb = void (*)(int type, const TMatrix &emitter_tm, const TMatrix &fx_tm, int pool_idx, bool is_player,
  AcesEffect **locked_fx, const char *effect_template);

inline bool destroyRIInSegment(const Point3 &p0, const Point3 &p1, bool trees, bool buildings, Point4 &contactPt, bool doKill,
  uint32_t frameNo, damage_effect_cb effect_cb)
{
  return destroyRIGenInSegment(p0, p1, trees, buildings, contactPt, doKill, frameNo, effect_cb);
}
inline bool destroyRIWithBullets(const Point3 &from, const Point3 &dir, float &dist, Point3 &norm, bool killBuildings,
  uint32_t frameNo, damage_effect_cb effect_cb)
{
  return destroyRIGenWithBullets(from, dir, dist, norm, killBuildings, frameNo, effect_cb);
}


void doRIGenDamage(const BSphere3 &sphere, unsigned frameNo, ri_damage_effect_cb effect_cb, const Point3 &axis = Point3(0.f, 0.f, 0.f),
  bool create_debris = true);
void doRIGenDamage(const BBox3 &box, unsigned frameNo, ri_damage_effect_cb effect_cb, const Point3 &axis = Point3(0.f, 0.f, 0.f),
  bool create_debris = true);
void doRIGenDamage(const BBox3 &box, unsigned frameNo, ri_damage_effect_cb effect_cb, const TMatrix &check_itm,
  const Point3 &axis = Point3(0.f, 0.f, 0.f), bool create_debris = true);
void doRIGenDamage(const Point3 &pos, unsigned frameNo, ri_damage_effect_cb effect_cb, const Point3 &axis = Point3(0.f, 0.f, 0.f),
  float dmg_pts = 1000, bool create_debris = true);

struct RendInstBufferData
{
  mat44f tm;
  carray<int16_t, 12> data;
};

DynamicPhysObjectData *doRIGenDestr(const RendInstDesc &desc, RendInstBufferData &out_buffer, RendInstDesc &out_desc, float dmg_pts,
  ri_damage_effect_cb effect_cb = nullptr, riex_handle_t *out_gen_riex = nullptr, bool restorable = false, int32_t user_data = -1,
  const Point3 *coll_point = nullptr, bool *ri_removed = nullptr,
  DestrOptionFlags destroy_flags = DestrOptionFlag::AddDestroyedRi | DestrOptionFlag::ForceDestroy);

// This one ignores subcells and doesn't return buffer (as we use it when we don't need to restore it)
// As well it doesn't updateVb, as it's used in batches, so you'll updateVb only once, when you need it
DynamicPhysObjectData *doRIGenDestrEx(const RendInstDesc &desc, float dmg_pts, ri_damage_effect_cb effect_cb = nullptr,
  int32_t user_data = -1);
DynamicPhysObjectData *doRIExGenDestrEx(rendinst::riex_handle_t riex_handle, ri_damage_effect_cb effect_cb = nullptr);

inline void doRendinstDebris(const BBox3 &, bool, uint32_t, damage_effect_cb)
{
  // fixme: not implemented!
}

inline void doRendinstDebris(const BSphere3 &, bool, uint32_t, damage_effect_cb)
{
  // fixme: not implemented!
}

bool restoreRiGen(const RendInstDesc &desc, const RendInstBufferData &buffer);
riex_handle_t restoreRiGenDestr(const RendInstDesc &desc, const RendInstBufferData &buffer);


struct DestroyedRi
{
  vec3f axis = {};
  vec3f normal = {};
  unsigned frameAdded = 0;
  float timeToDamage = 0;
  int fxType = 0;
  riex_handle_t riHandle = RIEX_HANDLE_NULL;
  bool shouldUpdate = false;
  float accumulatedPower = 0;
  float rotationPower = 0;

  RendInstBufferData savedData = {};

  DestroyedRi() = default;
  ~DestroyedRi();
};

DestroyedRi *doRIGenExternalControl(const RendInstDesc &desc, bool rem_rendinst = true);
bool returnRIGenExternalControl(const RendInstDesc &desc, DestroyedRi *ri);
bool removeRIGenExternalControl(const RendInstDesc &desc, DestroyedRi *ri);

void play_riextra_dmg_fx(rendinst::riex_handle_t id, const Point3 &pos, ri_damage_effect_cb effect_cb);

bool getDestrCellData(int layer_idx, const eastl::fixed_function<64, bool(const Tab<DestroyedCellData> &)> &callback);

} // namespace rendinst