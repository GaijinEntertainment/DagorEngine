//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <rendInst/riexHandle.h>
#include <rendInst/rendInstDesc.h>
#include <rendInst/constants.h>
#include <rendInst/layerFlags.h>

#include <vecmath/dag_vecMath.h>
#include <generic/dag_tab.h>
#include <math/dag_Point3.h>
#include <EASTL/functional.h>
#include <EASTL/fixed_function.h>
#include <memory/dag_framemem.h>
#include <generic/dag_relocatableFixedVector.h>
#include <3d/dag_resPtr.h>


class RenderableInstanceLodsResource;
class CollisionResource;
struct RiGenVisibility;
class DataBlock;
struct Capsule;

namespace rendinst
{

void prepareRiExtraRefs(const DataBlock &riConf);
void setRiExtraTiledSceneWritingThread(int64_t tid);

int addRIGenExtraResIdx(const char *ri_res_name, int ri_pool_ref, int ri_pool_ref_layer, AddRIFlags ri_flags);
void requestRiExResources(const char *ri_res_name, const eastl::fixed_function<sizeof(void *), void(const char *, unsigned)> &res_cb,
  AddRIFlags ri_flags = {});
int getRIGenExtraResIdx(const char *ri_res_name);
int getOrAddMissingRIGenExtraResIdx(const char *ri_res_name, AddRIFlags ri_flags);
void updatePerDrawGatheredData(uint32_t id);
const UniqueBuf &getPerDrawGatheredData();
int cloneRIGenExtraResIdx(const char *source_res_name, const char *new_res_name);
void setRIGenExtraResDynamic(const char *ri_res_name);
bool isRIGenExtraDynamic(int res_idx);
void addRiGenExtraDebris(uint32_t res_idx, int layer_idx);
void reloadRIExtraResources(const char *ri_res_name);
RenderableInstanceLodsResource *getRIGenExtraRes(int res_idx);
CollisionResource *getRIGenExtraCollRes(int res_idx);
const bbox3f *getRIGenExtraCollBb(int res_idx);
const E3DCOLOR *getRIGenExtraColors(int res_idx);
dag::Vector<BBox3, framemem_allocator> getRIGenExtraInstancesWorldBboxesByGrid(int res_idx, Point2 grid_origin, Point2 grid_size,
  IPoint2 grid_cells);
bbox3f getRIGenExtraOverallInstancesWorldBbox(int res_idx);

const char *getRIGenExtraName(uint32_t res_idx);
void iterateRIExtraMap(eastl::fixed_function<sizeof(void *) * 3, void(int, const char *)> cb);
int getRIExtraMapSize();

uint64_t getRIExtraGlobalWorldVersion(bool add, bool del);

bool updateRiExtraReqLod(uint32_t res_idx, unsigned lod);

void applyTiledScenesUpdateForRIGenExtra(int max_quota_usec, int max_maintenance_quota_usec);

riex_handle_t addRIGenExtra43(int res_idx, bool movable, mat43f_cref tm, bool collision, int orig_cell, int orig_offset,
  int add_data_dwords = 0, const int32_t *add_data = nullptr, bool on_loading = false);
riex_handle_t addRIGenExtra44(int res_idx, bool movable, mat44f_cref tm, bool collision, int orig_cell, int orig_offset,
  int add_data_dwords = 0, const int32_t *add_data = nullptr, bool on_loading = false);
void processDelayedGridAdds(int cell_idx);

void moveToOriginalScene(riex_handle_t id);
// Returns true if lock succeed
bool moveRIGenExtra43(riex_handle_t id, mat43f_cref tm, bool moved, bool do_not_wait);
inline bool moveRIGenExtra44(riex_handle_t id, mat44f_cref tm, bool moved, bool do_not_wait)
{
  mat43f m43;
  v_mat44_transpose_to_mat43(m43, tm);
  return moveRIGenExtra43(id, m43, moved, do_not_wait);
}

using riex_render_info_t = uint32_t;
__forceinline rendinst::riex_render_info_t make_riex_render_info_handle(uint32_t scene, uint32_t node_index)
{
  G_ASSERT(scene <= 3 && node_index <= 0x3FFFFFFF);
  return (uint32_t(scene) << uint32_t(30)) | uint32_t(node_index);
}
void setRIGenExtraData(riex_render_info_t id, uint32_t start, const uint32_t *udata, uint32_t cnt);
void setCASRIGenExtraData(riex_render_info_t id, uint32_t start, const uint32_t *wasdata, const uint32_t *newdata,
  uint32_t cnt); // this would set new data only if it was equal to wasdata
void setRIGenExtraNodeShadowsVisibility(riex_render_info_t id, bool visible);
const mat44f &getRIGenExtraTiledSceneNode(rendinst::riex_render_info_t);

bool isRiGenExtraValid(riex_handle_t id);
bool delRIGenExtra(riex_handle_t id); // return true if ri exta was actually deleted and false otherwise
void delRIGenExtraFromCell(riex_handle_t id, int cell_Id, int offset);
void delRIGenExtraFromCell(const RendInstDesc &desc);

using invalidate_handle_cb = void (*)(riex_handle_t invalidate_handle);
void registerRIGenExtraInvalidateHandleCb(invalidate_handle_cb cb);
bool unregisterRIGenExtraInvalidateHandleCb(invalidate_handle_cb cb);
void onRIGenExtraInvalidateHandle(riex_handle_t id);
bool removeRIGenExtraFromGrid(riex_handle_t id);

using ri_destruction_cb = void (*)(riex_handle_t handle, bool is_dynamic, bool create_destr_effects, int32_t user_data,
  const Point3 &impulse, const Point3 &impulse_pos);
void registerRiExtraDestructionCb(ri_destruction_cb cb);
bool unregisterRiExtraDestructionCb(ri_destruction_cb cb);
void onRiExtraDestruction(riex_handle_t id, bool is_dynamic, bool create_destr_effects, int32_t user_data = -1,
  const Point3 &impulse = Point3::ZERO, const Point3 &impulse_pos = Point3::ZERO);

using ri_impulse_cb = void (*)(riex_handle_t handle, float impulse, const Point3 &impulse_dir, const Point3 &impulse_pos,
  const Point3 &collision_normal, int32_t user_data);
void registerRiExtraImpulseCb(ri_impulse_cb cb);
bool unregisterRiExtraImpulseCb(ri_impulse_cb cb);
void onRiExtraImpulse(riex_handle_t id, float impulse, const Point3 &impulse_dir, const Point3 &impulse_pos,
  const Point3 &collision_normal, int32_t user_data);

bbox3f riex_get_lbb(int res_idx);
float riex_get_bsph_rad(int res_idx);
dag::ConstSpan<mat43f> riex_get_instance_matrices(int res_idx);
inline bool riex_is_instance_valid(const mat43f &ri_tm) { return !v_test_all_bits_zeros(ri_tm.row0); }

void setGameClockForRIGenExtraDamage(const float *session_time);
bool applyDamageRIGenExtra(riex_handle_t id, float dmg_pts, float *absorbed_dmg_impulse = nullptr,
  bool ignore_damage_threshold = false);
bool applyDamageRIGenExtra(const RendInstDesc &desc, float dmg_pts, float *absorbed_dmg_impulse = nullptr,
  bool ignore_damage_threshold = false);
bool damageRIGenExtra(riex_handle_t id, float dmg_pts, mat44f *out_destr_tm, riex_handle_t &out_destroyed_riex_handle,
  int *out_destroyed_riex_offset = nullptr, DestrOptionFlags destroy_flags = DestrOptionFlag::AddDestroyedRi);

struct RiExtraPerInstanceItem
{
  uint32_t data[4];
};

uint32_t getRiExtraPerInstanceRenderDataOffset(riex_handle_t id);
void setRiExtraPerInstanceRenderData(riex_handle_t id, const RiExtraPerInstanceItem *data, uint32_t cnt);
void resetPerInstanceRenderDataAfterFrame();

void before_render();

struct RiGenCollidableData;
using riex_collidable_t = dag::RelocatableFixedVector<riex_handle_t, 64, true, framemem_allocator>;
void gatherRIGenExtraCollidable(riex_collidable_t &out_handles, const BBox3 &box, bool read_lock);
void gatherRIGenExtraCollidable(riex_collidable_t &out_handles, const BSphere3 &sphere, bool read_lock);
void gatherRIGenExtraCollidable(riex_collidable_t &out_handles, const Capsule &capsule, bool read_lock);
void gatherRIGenExtraCollidable(riex_collidable_t &out_handles, const TMatrix &tm, const BBox3 &box, bool read_lock);
void gatherRIGenExtraCollidable(riex_collidable_t &out_handles, const Point3 &p0, const Point3 &dir, float len, bool read_lock);
void gatherRIGenExtraCollidableMin(riex_collidable_t &out_handles, bbox3f_cref box, float min_bsph_rad);
void gatherRIGenExtraCollidable(dag::RelocatableFixedVector<RiGenCollidableData, 64, true, framemem_allocator> &out_data,
  const Point3 &pos, float radius, bool read_lock);

// if res_idx == nullptr, would get all, otherwise only those with res_idx in sorted_res_idx array.
// if fast, it will be checked using spheres only
void gatherRIGenExtraRenderable(Tab<riex_render_info_t> &out_handles, const int *sorted_res_idx, int count, bbox3f_cref box, bool fast,
  SceneSelection s);
// if fast, it will be checked using spheres only
void gatherRIGenExtraRenderable(Tab<mat44f> &out_handles, bbox3f_cref box, bool fast, SceneSelection s);

// if fast, it will be checked using spheres only
void gatherRIGenExtraRenderable(Tab<mat44f> &out_handles, const BBox3 &box, bool fast, SceneSelection s);
void gatherRIGenExtraToTestForShadows(Tab<bbox3f> &out_bboxes, mat44f_cref globtm_cull, float static_shadow_texel_size,
  uint32_t usr_data);
void gatherRIGenExtraShadowInvisibleBboxes(Tab<bbox3f> &out_bboxes, bbox3f_cref gather_bbox);


struct pos_and_render_info_t
{
  Point3 pos;
  riex_render_info_t info;
};
typedef dag::RelocatableFixedVector<pos_and_render_info_t, 128, true /*overflow*/> riex_pos_and_ri_tab_t;
void gatherRIGenExtraRenderableNotCollidable(riex_pos_and_ri_tab_t &out, bbox3f_cref box, bool fast, SceneSelection s);
bool gatherRIGenExtraBboxes(const RiGenVisibility *main_visibility, mat44f_cref volume_box,
  eastl::function<void(mat44f_cref, const BBox3 &, const char *)> callback);

uint32_t getRiGenExtraResCount();

enum class HasRIClipmap
{
  UNKNOWN = -1,
  NO,
  YES
};
HasRIClipmap hasRIClipmapPools();

int getRiGenExtraInstances(Tab<riex_handle_t> &out_handles, uint32_t res_idx);
int getRiGenExtraInstances(Tab<riex_handle_t> &out_handles, uint32_t res_idx, const bbox3f &box);

void updateRiExtraBBoxScalesForPrepasses(const DataBlock &blk);
void updateRiExtraForceDisableShadowList(const DataBlock &blk);

bool hasRIExtraOnLayers(const RiGenVisibility *visibility, LayerFlags layer_flags);


void riex_lock_write(const char *locker_nm = nullptr);
void riex_unlock_write();

void riex_lock_read();
void riex_unlock_read();

struct ScopedRIExtraWriteLock
{
  ScopedRIExtraWriteLock(const char *n = "riEx::wr") { riex_lock_write(n); }
  ~ScopedRIExtraWriteLock() { riex_unlock_write(); }
};

struct ScopedRIExtraReadLock
{
  ScopedRIExtraReadLock() { riex_lock_read(); }
  ~ScopedRIExtraReadLock() { riex_unlock_read(); }
};

void rigrid_debug_pos(const Point3 &pos, const Point3 &camera_pos);

} // namespace rendinst
