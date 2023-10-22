//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <daNet/mpi.h>
#include <rendInst/rendInstDebris.h>
#include <rendInst/rendInstCollision.h>
#include <gamePhys/phys/rendinstSound.h>
#include <gamePhys/phys/destructableObject.h>
#include <EASTL/functional.h>

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

typedef void (*create_tree_rend_inst_destr_cb)(const rendinst::RendInstDesc &desc, bool add_restorable, const Point3 &impulse,
  bool create_phys, bool constrained_phys, float omega, float at_time, const rendinst::CollisionInfo *coll_info, bool create_destr);
typedef void (*remove_tree_rendinst_destr_cb)(const rendinst::RendInstDesc &desc);
typedef void (*remove_physx_collision_object_callback)(const rendinst::RendInstDesc &desc);
typedef int (*create_apex_actors_callback)(const char *name, const TMatrix &normalized_tm, const Point3 &scale, const Point3 &pos,
  const Point3 &impulse, int index, ApexDmgInfo *apex_dmg_info, int ri_idx);
typedef void (*apex_force_remove_actor_callback)(const int);
typedef void (*on_destr_changed_callback)(const rendinst::RendInstDesc &desc);
typedef void (*on_rendinst_destroyed_callback)(rendinst::riex_handle_t riex_handle, const TMatrix &tm, const BBox3 &box);
typedef eastl::function<void(const rendinst::riex_handle_t)> on_riextra_destroyed_callback;
typedef eastl::function<void(const rendinst::RendInstDesc &)> on_destr_callback;
typedef eastl::function<bool(rendinst::riex_handle_t)> riextra_should_damage;
typedef bool (*restorable_rendinst_callback)(const rendinst::RendInstDesc &desc, RestorableRendinstState state);
typedef eastl::function<Point3()> get_camera_pos;

struct DestrSettings
{
  bool isClient = false;
  bool createDestr = true;
  bool hasSound = false;
  bool hasStaticShadowsInvalidationCallback = false;

  float rendInstMaxLifeTime = 25.f;
  float hitPointsToDestrImpulseMult = 3000.f;
  float destrImpulseHitPointsMult = 1.f / 3000.f;
};

DestrSettings &get_mutable_destr_settings();
const DestrSettings &get_destr_settings();

void init_ex(mpi::ObjectID oid, on_destr_changed_callback on_destr_cb, create_tree_rend_inst_destr_cb create_tree_cb,
  remove_tree_rendinst_destr_cb rem_tree_cb, ri_tree_sound_cb tree_sound_cb, get_camera_pos get_current_camera_pos_,
  remove_physx_collision_object_callback remove_physx_obj_cb = NULL, create_apex_actors_callback create_apex_actors_cb = NULL,
  apex_force_remove_actor_callback apex_remove_actor_cb = NULL);
void init(on_destr_changed_callback on_destr_cb, bool apply_cache, create_tree_rend_inst_destr_cb create_tree_destr_cb = nullptr,
  remove_tree_rendinst_destr_cb rem_tree_destr_cb = nullptr, ri_tree_sound_cb tree_sound_cb = nullptr);
void clear();
void shutdown();

bool serialize_destr_data(danet::BitStream &bs); // return true if there is some destruction data
void deserialize_destr_data(const danet::BitStream &bs, int apply_flags = 0, int max_simultaneous_destrs = 20);

void startSession(void *phys_wld);
void endSession();
void setApexEnabled(bool enabled);
void resetApexDestructedRIList();

mpi::IObject *getReflectionObject();

void remove_restorable_by_destructable_id(destructables::id_t id);
void testObjToRestorablesIntersection(const BBox3 &obj_box, const TMatrix &obj_tm, rendinst::RendInstCollisionCB *coll_cb,
  float at_time);

rendinst::RendInstDesc destroyRendinst(rendinst::RendInstDesc desc, bool add_restorable, const Point3 &pos, const Point3 &impulse,
  float at_time, const rendinst::CollisionInfo *coll_info, bool create_destr, rendinst::ri_damage_effect_cb effect_cb = NULL,
  bool is_client = false, ApexDmgInfo *apex_dmg_info = NULL, int destroy_neighbour_recursive_depth = 1,
  float impulse_mult_for_child = 1.f, on_destr_callback on_destr_cb = nullptr,
  rendinst::DestrOptionFlags flags = rendinst::DestrOptionFlag::AddDestroyedRi | rendinst::DestrOptionFlag::ForceDestroy);
void destroyRiExtra(rendinst::riex_handle_t riex_handle, const TMatrix &transform, rendinst::ri_damage_effect_cb effect_cb);
void update(float dt, const Frustum *frustum);

bool apply_damage_to_riextra(rendinst::riex_handle_t handle, float dmg, const Point3 &pos, const Point3 &impulse, float at_time);
void apply_damage_to_ri(const rendinst::RendInstDesc &desc, float dmg, float impulse_to_hp, const Point3 &pos, const Point3 &impulse,
  float at_time, bool create_destr, rendinst::ri_damage_effect_cb effect_cb, bool is_client, float impulse_mult_for_child,
  bool *isDestroyed = nullptr);

void damage_ri_in_sphere(const Point3 &pos, float rad, const Point2 &dmg_near_far, float at_time, bool create_destr,
  rendinst::ri_damage_effect_cb effect_cb, on_riextra_destroyed_callback &&riex_destr_cb, bool is_client,
  riextra_should_damage &&should_damage);

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
void create_tree_rend_inst_destr(const rendinst::RendInstDesc &desc, bool add_restorable, const Point3 &impulse, bool create_phys,
  bool constrained_phys, float wanted_omega, float at_time, const rendinst::CollisionInfo *coll_info, bool create_destr);

void clear_phys_objs();

bbox3f get_ri_phys_containing_bbox(const bbox3f &intersect_bbox, const Frustum &intersect_frustum, bool only_trees);
void debug_draw_ri_phys();

using calc_expl_damage_cb = eastl::fixed_function<3 * sizeof(void *), float(const rendinst::CollisionInfo &coll_info, float distance)>;

void doRIExtraDamageInBox(const BBox3 &box, rendinst::ri_damage_effect_cb effect_cb, float at_time, bool is_client, bool create_destr,
  const Point3 &view_pos, calc_expl_damage_cb calc_expl_dmg_cb, const BSphere3 *check_sphere, const TMatrix *check_itm,
  rendinst::DestrOptionFlags flags);

inline void doRendinstDamage(const BSphere3 &sphere, bool, uint32_t frameNo, rendinst::damage_effect_cb effect_cb, float at_time,
  bool is_client, bool create_destr, const Point3 &view_pos, calc_expl_damage_cb calc_expl_dmg_cb, rendinst::DestrOptionFlags flags)
{
  rendinst::doRIGenDamage(sphere, frameNo, effect_cb, {0.f, 0.f, 0.f}, create_destr);
  doRIExtraDamageInBox(BBox3(sphere), effect_cb, at_time, is_client, create_destr, view_pos, calc_expl_dmg_cb, &sphere, nullptr,
    flags);
}

inline void doRendinstDamage(const BBox3 &box, bool, uint32_t frameNo, rendinst::damage_effect_cb effect_cb, float at_time,
  bool is_client, bool create_destr, const Point3 &view_pos, calc_expl_damage_cb calc_expl_dmg_cb, rendinst::DestrOptionFlags flags)
{
  rendinst::doRIGenDamage(box, frameNo, effect_cb, {0.f, 0.f, 0.f}, create_destr);
  doRIExtraDamageInBox(box, effect_cb, at_time, is_client, create_destr, view_pos, calc_expl_dmg_cb, nullptr, nullptr, flags);
}

inline void doRendinstDamage(const BBox3 &box, bool, uint32_t frameNo, rendinst::damage_effect_cb effect_cb, float at_time,
  bool is_client, bool create_destr, const Point3 &view_pos, calc_expl_damage_cb calc_expl_dmg_cb, const TMatrix &check_itm,
  rendinst::DestrOptionFlags flags)
{
  rendinst::doRIGenDamage(box, frameNo, effect_cb, check_itm, {0.f, 0.f, 0.f}, create_destr);
  doRIExtraDamageInBox(box, effect_cb, at_time, is_client, create_destr, view_pos, calc_expl_dmg_cb, nullptr, &check_itm, flags);
}

void remove_ri_without_collision_in_radius(const Point3 &pos, float radius);
}; // namespace rendinstdestr
