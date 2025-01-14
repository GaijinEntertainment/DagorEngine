//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <daNet/mpi.h>
#include <rendInst/rendInstDebris.h>
#include <rendInst/rendInstCollision.h>
#include <gamePhys/phys/rendinstSound.h>
#include <gamePhys/phys/destructableObject.h>
#include <EASTL/functional.h>
#include <gameMath/quantization.h>

namespace rendinst
{
struct RendInstDesc;
};

struct ApexDmgInfo;
struct CollisionObject;
struct CachedCollisionObjectInfo;
struct Frustum;

namespace rendinstdestr
{
enum RestorableRendinstState
{
  RRS_CREATED,
  RRS_RESTORED,
  RRS_DESTROYED
};

typedef rendinst::RendInstDesc (*create_tree_rend_inst_destr_cb)(const rendinst::RendInstDesc &desc, bool add_restorable,
  const Point3 &impactPos, const Point3 &impulse, bool create_phys, bool constrained_phys, float omega, float at_time,
  const rendinst::CollisionInfo *coll_info, bool create_destr, bool from_damage);
typedef void (*remove_tree_rendinst_destr_cb)(const rendinst::RendInstDesc &desc);
typedef void (*remove_physx_collision_object_callback)(const rendinst::RendInstDesc &desc);
typedef int (*create_apex_actors_callback)(const char *name, const TMatrix &normalized_tm, const Point3 &scale, const Point3 &pos,
  const Point3 &impulse, int index, ApexDmgInfo *apex_dmg_info, int ri_idx, const BBox3 &ri_bbox);
typedef void (*apex_force_remove_actor_callback)(const int);
typedef void (*on_destr_changed_callback)(const rendinst::RendInstDesc &desc, const TMatrix &ri_tm, const Point3 &pos,
  const Point3 &impulse);
typedef void (*on_rendinst_destroyed_callback)(rendinst::riex_handle_t riex_handle, const TMatrix &tm, const BBox3 &box);
typedef eastl::function<void(const rendinst::riex_handle_t)> on_riextra_destroyed_callback;
typedef eastl::function<void(const rendinst::RendInstDesc &)> on_destr_callback;
typedef eastl::function<bool(rendinst::riex_handle_t)> riextra_should_damage;
typedef bool (*restorable_rendinst_callback)(const rendinst::RendInstDesc &desc, RestorableRendinstState state);
typedef eastl::function<Point3()> get_camera_pos;

struct QuantizedDestrImpulsePosScale
{
  constexpr operator float() const { return 8.f; }
};
struct QuantizedDestrImpulseVecScale
{
  constexpr operator float() const { return 256.f; }
};
typedef gamemath::QuantizedPos<8, 8, 8, QuantizedDestrImpulsePosScale, QuantizedDestrImpulsePosScale, QuantizedDestrImpulsePosScale>
  QuantizedDestrImpulsePos;
typedef gamemath::QuantizedPos<8, 8, 8, QuantizedDestrImpulseVecScale, QuantizedDestrImpulseVecScale, QuantizedDestrImpulseVecScale>
  QuantizedDestrImpulseVec;

struct DestrSettings
{
  bool isNetClient = false;
  bool createDestr = true;
  bool hasSound = false;
  bool hasStaticShadowsInvalidationCallback = false;

  float rendInstMaxLifeTime = 25.f;
  float maxTreeImpulseDamage = 1000.0f;
  float hitPointsToDestrImpulseMult = 3000.f;
  float destrImpulseHitPointsMult = 1.f / 3000.f;
  float destrImpulseSendDist = 128.f;
  bool destrImpulseSendByDefault = true;
  uint32_t destrMaxUpdateSize = 140;
  int riMaxSimultaneousDestrs = 64;
  bool destrNewUpdateMessage = false;
};

DestrSettings &get_mutable_destr_settings();
const DestrSettings &get_destr_settings();

void init_ex(on_destr_changed_callback on_destr_cb, create_tree_rend_inst_destr_cb create_tree_cb,
  remove_tree_rendinst_destr_cb rem_tree_cb, ri_tree_sound_cb tree_sound_cb, get_camera_pos get_current_camera_pos_,
  remove_physx_collision_object_callback remove_physx_obj_cb = NULL, create_apex_actors_callback create_apex_actors_cb = NULL,
  apex_force_remove_actor_callback apex_remove_actor_cb = NULL);
// apply_pending - apply destrs received before level load
void init(on_destr_changed_callback on_destr_cb, bool apply_pending, create_tree_rend_inst_destr_cb create_tree_destr_cb = nullptr,
  remove_tree_rendinst_destr_cb rem_tree_destr_cb = nullptr, ri_tree_sound_cb tree_sound_cb = nullptr,
  get_camera_pos get_camera_pos_cb = nullptr);
void clear();
void shutdown();

struct DestrUpdateDesc
{
  // Don't change order of first 3 members (for fast uint64_t sort)
  union
  {
    struct
    {
      uint32_t offs; // desc.offs for riGen, riUniqueData.offset for riExtra
      uint16_t poolIdx;
      int16_t cellIdx;
    };
    uint64_t cmpKey;
  };
  Point3 riPos{};
  QuantizedDestrImpulsePos serializedLocalPos;
  QuantizedDestrImpulseVec serializedImpulse;

  DestrUpdateDesc()
  {
    offs = 0;
    poolIdx = ~0;
    cellIdx = 0;
  }
  DestrUpdateDesc(rendinstdestr::DestrUpdateDesc &&rhs) { memcpy((void *)this, (void *)&rhs, sizeof(DestrUpdateDesc)); }
  DestrUpdateDesc &operator=(rendinstdestr::DestrUpdateDesc &&rhs)
  {
    memcpy((void *)this, (void *)&rhs, sizeof(DestrUpdateDesc));
    return *this;
  }
  bool operator==(const rendinstdestr::DestrUpdateDesc &rhs) const { return cmpKey == rhs.cmpKey; }
  bool operator<(const rendinstdestr::DestrUpdateDesc &rhs) const { return cmpKey < rhs.cmpKey; }
  static bool lessVerify(const rendinstdestr::DestrUpdateDesc &lhs, const rendinstdestr::DestrUpdateDesc &rhs)
  {
    if (uint16_t(lhs.cellIdx) != uint16_t(rhs.cellIdx)) // use only unsigned cmp
      return uint16_t(lhs.cellIdx) < uint16_t(rhs.cellIdx);
    if (lhs.poolIdx != rhs.poolIdx)
      return lhs.poolIdx < rhs.poolIdx;
    if (lhs.offs != rhs.offs)
      return lhs.offs < rhs.offs;
    return false;
  }
};

bool serialize_destr_data(danet::BitStream &bs); // return true if there is some destruction data
bool deserialize_destr_data(const danet::BitStream &bs, int apply_flags = 0, int max_simultaneous_destrs = 20);
void serialize_destr_update(danet::BitStream &bs, const Point3 *camera_pos, float camera_rad,
  dag::ConstSpan<DestrUpdateDesc> update_data, bool send_by_default, int max_impulses);
bool deserialize_destr_update(const danet::BitStream &bs);

void clear_synced_ri_extra_pools();
void sync_all_ri_extra_pools();
void sync_ri_extra_pool(int pool_id);
bool serialize_synced_ri_extra_pools(danet::BitStream &bs, bool full, bool skip_if_no_data);
bool deserialize_synced_ri_extra_pools(const danet::BitStream &bs);
int get_client_ri_pool_id(int server_pool_id); // server -> client
int get_server_ri_pool_id(int client_pool_id); // client -> server
bool is_server_ri_pool_sync_pending(int server_pool_id);

void startSession(void *phys_wld);
void endSession();
void setApexEnabled(bool enabled);
void resetApexDestructedRIList();

void remove_restorable_by_destructable_id(destructables::id_t id);
void testObjToRestorablesIntersection(const BBox3 &obj_box, const TMatrix &obj_tm, rendinst::RendInstCollisionCB *coll_cb,
  float at_time);

rendinst::RendInstDesc destroyRendinst(rendinst::RendInstDesc desc, bool add_restorable, const Point3 &pos, const Point3 &impulse,
  float at_time, const rendinst::CollisionInfo *coll_info, bool create_destr_effects, ApexDmgInfo *apex_dmg_info = NULL,
  int destroy_neighbour_recursive_depth = 1, float impulse_mult_for_child = 1.f, on_destr_callback on_destr_cb = nullptr,
  rendinst::DestrOptionFlags flags = rendinst::DestrOptionFlag::AddDestroyedRi | rendinst::DestrOptionFlag::ForceDestroy);
void destroyRiExtra(rendinst::riex_handle_t riex_handle, const TMatrix &transform, bool create_destr_effects, const Point3 &impulse,
  const Point3 &impulse_pos, rendinst::DestrOptionFlags flags = {});
void update(float dt, const Frustum *frustum);
void update_paused(const Frustum *frustum);
void fill_ri_destructable_params(destructables::DestructableCreationParams &params, const rendinst::RendInstDesc &desc,
  DynamicPhysObjectData *po_data, const TMatrix &tm, rendinst::DestrOptionFlags flags);

bool apply_damage_to_riextra(rendinst::riex_handle_t handle, float dmg, const Point3 &pos, const Point3 &impulse, float at_time);
void apply_damage_to_ri(const rendinst::RendInstDesc &desc, float dmg, float impulse_to_hp, const Point3 &pos, const Point3 &impulse,
  float at_time, bool create_destr_effects, float impulse_mult_for_child, bool *isDestroyed = nullptr);

void damage_ri_in_sphere(const Point3 &pos, float rad, const Point2 &dmg_near_far, float impulse_to_hp, float at_time,
  bool create_destr_effects, on_riextra_destroyed_callback &&riex_destr_cb, riextra_should_damage &&should_damage);

void set_on_rendinst_destroyed_cb(on_rendinst_destroyed_callback cb);
void call_on_rendinst_destroyed_cb(rendinst::riex_handle_t riex_handle, const TMatrix &tm, const BBox3 &box);
rendinst::ri_damage_effect_cb get_ri_damage_effect_cb();
void set_ri_damage_effect_cb(rendinst::ri_damage_effect_cb effect_cb);
void register_restorable_rendinst_cb(restorable_rendinst_callback cb);
bool unregister_restorable_rendinst_cb(restorable_rendinst_callback cb);

CollisionObject &get_tree_collision();
void clear_tree_collision();

CachedCollisionObjectInfo *get_cached_collision_object(const rendinst::RendInstDesc &ri_desc);
void add_cached_collision_object(CachedCollisionObjectInfo *object);
CachedCollisionObjectInfo *get_or_add_cached_collision_object(const rendinst::RendInstDesc &ri_desc, float at_time);
CachedCollisionObjectInfo *get_or_add_cached_collision_object(const rendinst::RendInstDesc &ri_desc, float at_time,
  const rendinst::CollisionInfo &coll_info);

int test_dynobj_to_ri_phys_collision(const CollisionObject &coA, const TMatrix &tmA, float max_rad);
int test_dynobj_to_ri_phys_collision(const CollisionObject &coA, float max_rad);

void remove_tree_rendinst_destr(const rendinst::RendInstDesc &desc);
rendinst::RendInstDesc create_tree_rend_inst_destr(const rendinst::RendInstDesc &desc, bool add_restorable, const Point3 &impactPos,
  const Point3 &impulse, bool create_phys, bool constrained_phys, float wanted_omega, float at_time,
  const rendinst::CollisionInfo *coll_info, bool create_destr, bool from_damage);

void clear_phys_objs();

bbox3f get_ri_phys_containing_bbox(const bbox3f &intersect_bbox, const Frustum &intersect_frustum, bool only_trees);
void debug_draw_ri_phys();

using calc_expl_damage_cb = eastl::fixed_function<3 * sizeof(void *), float(const rendinst::CollisionInfo &coll_info, float distance)>;

// will do both deferred destruction and try perform delayed sync from server
void perform_delayed_destruction(int quota_usec = -1);

void doRIExtraDamageInBox(const BBox3 &box, float at_time, bool create_destr_effects, const Point3 &view_pos,
  calc_expl_damage_cb calc_expl_dmg_cb, const BSphere3 *check_sphere, const TMatrix *check_itm, rendinst::DestrOptionFlags flags);

inline void doRendinstDamage(const BSphere3 &sphere, bool, uint32_t frameNo, float at_time, bool create_destr_effects,
  const Point3 &view_pos, calc_expl_damage_cb calc_expl_dmg_cb, rendinst::DestrOptionFlags flags)
{
  rendinst::doRIGenDamage(sphere, frameNo, {0.f, 0.f, 0.f}, create_destr_effects);
  doRIExtraDamageInBox(BBox3(sphere), at_time, create_destr_effects, view_pos, calc_expl_dmg_cb, &sphere, nullptr, flags);
}

inline void doRendinstDamage(const BBox3 &box, bool, uint32_t frameNo, float at_time, bool create_destr_effects,
  const Point3 &view_pos, calc_expl_damage_cb calc_expl_dmg_cb, rendinst::DestrOptionFlags flags)
{
  rendinst::doRIGenDamage(box, frameNo, {0.f, 0.f, 0.f}, create_destr_effects);
  doRIExtraDamageInBox(box, at_time, create_destr_effects, view_pos, calc_expl_dmg_cb, nullptr, nullptr, flags);
}

inline void doRendinstDamage(const BBox3 &box, bool, uint32_t frameNo, float at_time, bool create_destr_effects,
  const Point3 &view_pos, calc_expl_damage_cb calc_expl_dmg_cb, const TMatrix &check_itm, rendinst::DestrOptionFlags flags)
{
  rendinst::doRIGenDamage(box, frameNo, check_itm, {0.f, 0.f, 0.f}, create_destr_effects);
  doRIExtraDamageInBox(box, at_time, create_destr_effects, view_pos, calc_expl_dmg_cb, nullptr, &check_itm, flags);
}

void remove_ri_without_collision_in_radius(const Point3 &pos, float radius);
}; // namespace rendinstdestr
