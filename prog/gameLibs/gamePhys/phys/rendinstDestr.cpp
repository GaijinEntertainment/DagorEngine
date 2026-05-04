// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <gamePhys/phys/rendinstDestr.h>
#include <rendInst/rendInstExtra.h>
#include <rendInst/rendInstAccess.h>
#include <rendInst/rendInstExtraAccess.h>
#include <rendInst/rendInstGenDamageInfo.h>
#include <rendInst/rendInstGenRender.h>
#include <rendInst/rendInstFx.h>
#include <rendInst/rendInstCollision.h>
#include "../../rendInst/riGen/riGenData.h"
#include <rendInst/riexSync.h>
#include <EASTL/hash_set.h>
#include <EASTL/bitset.h>
#include <EASTL/array.h>
#include <EASTL/vector_set.h>

#include <phys/dag_physDecl.h>
#include <phys/dag_physics.h>

#include <gamePhys/phys/destructableObject.h>
#include <gamePhys/phys/dynamicPhysModel.h>
#include <gamePhys/phys/rendinstPhys.h>
#include <gameRes/dag_collisionResource.h>
#include <gamePhys/collision/collisionLib.h>
#include <gamePhys/collision/rendinstCollision.h>
#include <gamePhys/collision/rendinstCollisionUserInfo.h>
#include <gamePhys/collision/contactResultWrapper.h>
#include <gamePhys/collision/cachedCollisionObject.h>
#include <memory/dag_fixedBlockAllocator.h>
#include <math/dag_vecMathCompatibility.h>
#include <math/random/dag_random.h>
#include <math/dag_TMatrix4.h>
#include <math/dag_mathUtils.h>
#include <osApiWrappers/dag_critSec.h>
#include <osApiWrappers/dag_miscApi.h>
#include <ioSys/dag_dataBlock.h>
#include <memory/dag_framemem.h>
#include <math/dag_math3d.h>
#include <util/dag_convar.h>
#include <util/dag_stlqsort.h>
#include <generic/dag_sort.h>
#include <perfMon/dag_statDrv.h>
#include <perfMon/dag_cpuFreq.h>
#include <debug/dag_debug3d.h>
#include <gui/dag_visualLog.h>
#include <util/dag_string.h>
#include <util/dag_finally.h>
#include <scene/dag_occlusion.h>

#if DAGOR_DBGLEVEL > 0
#define TREE_DESTR_DEBUG 1
#endif

#if TREE_DESTR_DEBUG
#include <imgui/imgui.h>
#include <gui/dag_imgui.h>
#include <util/dag_console.h>

CONSOLE_FLOAT_VAL("ridestr", groundOffsetMulOverride, -1.0f);
CONSOLE_FLOAT_VAL("ridestr", groundDampingOverride, -1.0f);
CONSOLE_FLOAT_VAL("ridestr", groundJointDampingOverride, -1.0f);
CONSOLE_FLOAT_VAL("ridestr", groundJointMoveDistanceOverride, -1.0f);
CONSOLE_FLOAT_VAL("ridestr", groundJointAngleLimitWholeOverride, -1.0f);
CONSOLE_FLOAT_VAL("ridestr", groundJointAngleLimitBottomOverride, -1.0f);
CONSOLE_FLOAT_VAL("ridestr", treeTopTrunkDampingLinOverride, -1.0f);
CONSOLE_FLOAT_VAL("ridestr", treeTopTrunkDampingAngOverride, -1.0f);
CONSOLE_FLOAT_VAL("ridestr", canopyDampingLinOverride, -1.0f);
CONSOLE_FLOAT_VAL("ridestr", canopyDampingAngOverride, -1.0f);
CONSOLE_FLOAT_VAL("ridestr", canopySpringDampingAngOverride, -1.0f);
CONSOLE_FLOAT_VAL("ridestr", canopyJointDampingOverride, -1.0f);
CONSOLE_FLOAT_VAL("ridestr", canopyStiffnessMultOverride, -1.0f);
CONSOLE_FLOAT_VAL("ridestr", canopyJointAngLimitOverride, -1.0f);
CONSOLE_FLOAT_VAL("ridestr", linearLimitOverride, -1.0f);
CONSOLE_FLOAT_VAL("ridestr", angleLimitOverride, -1.0f);
CONSOLE_FLOAT_VAL("ridestr", canopyRadiusMulOverride, -1.0f);
CONSOLE_FLOAT_VAL("ridestr", canopyImpulseMulOverride, -1.0f);
CONSOLE_FLOAT_VAL("ridestr", canopyUnderhangMulOverride, -1.0f);
CONSOLE_FLOAT_VAL("ridestr", velocityThresholdOverride, -1.0f);
CONSOLE_FLOAT_VAL("ridestr", groundJointBreakVelocityCapOverride, -1.0f);
CONSOLE_FLOAT_VAL("ridestr", maxAngularVelocityOverride, -1.0f);
#endif

#if DAGOR_DBGLEVEL > 0
CONSOLE_BOOL_VAL("ridestr", ri_debug_collision, false);
#else
static constexpr bool ri_debug_collision = false;
#endif

#define RIDESTR_DBG_LOG(fmt, ...)                          \
  do                                                       \
  {                                                        \
    if (ri_debug_collision) [[unlikely]]                   \
    {                                                      \
      String msg(256, framemem_ptr(), fmt, ##__VA_ARGS__); \
      debug("%s", msg.c_str());                            \
      visuallog::logmsg(msg.c_str());                      \
    }                                                      \
  } while (0)

static constexpr int INITIAL_REPLICATION = 1;
static const Point3 RI_DESTROY_BBOX_MARGIN_UP = Point3(0, 0.05, 0);
static const float MAX_RI_DESTROY_IMPULSE = 1e8f;

static bool apply_reflection = false;
static bool apply_pending_destrs = false;
static bool is_branch_destruction_supported = true; // ULQ doesn't support it
static rendinstdestr::remove_physx_collision_object_callback rem_physx_collision_obj_cb = NULL;
static rendinstdestr::create_apex_actors_callback create_apex_actors_at_point_cb = NULL;
static rendinstdestr::apex_force_remove_actor_callback apex_force_remove_actor_cb = NULL;
static rendinstdestr::on_destr_changed_callback on_changed_destr_cb = NULL;
static rendinstdestr::on_tree_destr_created_callback on_tree_destr_created_cb = NULL;
static rendinstdestr::get_camera_pos get_current_camera_pos = NULL;
static rendinstdestr::get_current_time_callback g_get_current_time_cb = NULL;
static rendinstdestr::get_occlusion_callback get_occlusion_cb = NULL;
static dag::RelocatableFixedVector<rendinstdestr::on_rendinst_destroyed_callback, 3> on_rendinst_destroyed_callbacks;
static dag::RelocatableFixedVector<rendinstdestr::restorable_rendinst_callback, 2> restorable_rendinst_callbacks;
static ri_tree_sound_cb g_tree_sound_cb = nullptr;
static rendinstdestr::DestrSettings destrSettings;
static rendinst::ri_damage_effect_cb ri_effect_cb = nullptr;
static PhysWorld *phys_world = NULL;
static danet::BitStream pending_initial_destrs;
static danet::BitStream pending_destrs_updates;
static int simultaneous_destrs_count = 0;

// delayed net destr
struct DelayedRendInstNetDestr
{
  rendinst::RendInstDesc desc;
  bool isPoolSyncPending;

  DelayedRendInstNetDestr() = default;
  DelayedRendInstNetDestr(const rendinst::RendInstDesc &desc, bool is_sync_pending) : desc(desc), isPoolSyncPending(is_sync_pending) {}
  bool operator==(const DelayedRendInstNetDestr &rhs) const { return desc == rhs.desc; }
};
struct DelayedRendInstNetDestrList
{
  bool isEnabled = false;
  bool isForcedUpdatePending = false;
  uint64_t lastUpdatedWorldVersion = 0u;
  WinCritSec mutex;
  dag::Vector<DelayedRendInstNetDestr> list;

  void clear()
  {
    WinAutoLock lock(mutex);
    clear_and_shrink(list);
    lastUpdatedWorldVersion = 0u;
  }
};
static DelayedRendInstNetDestrList g_delayed_net_destr_list;

static void on_riex_pool_mapping_change() { g_delayed_net_destr_list.isForcedUpdatePending = true; }

// deferred destr to remove lag spikes
struct DeferredRiDestrData
{
  rendinst::RendInstDesc riDesc;
  Point3 pos;
  Point3 impulse;
  float atTime;
  rendinst::DestrOptionFlags flags;
  bool createDestr;
};
struct DeferredRendInstDestrList
{
  bool isEnabled;
  dag::Vector<DeferredRiDestrData> list;
  void clear() { clear_and_shrink(list); }
};
static DeferredRendInstDestrList g_deferred_ri_destr_list;

static constexpr int RI_DESTR_COLLISION_MAX_QUEUE_CAPACITY = 16;
static constexpr int RI_DESTR_COLLISION_DISABLED = INT_MIN;
static constexpr int RI_DESTR_COLLISION_NO_SESSION = -1;
static BBox3 ri_destr_collision_queue[RI_DESTR_COLLISION_MAX_QUEUE_CAPACITY];
static int ri_destr_collision_queue_count = RI_DESTR_COLLISION_DISABLED; // DISABLED = feature off, NO_SESSION = enabled but inactive,
                                                                         // 0+ = active queue
static Tab<RendInstPhys> riPhys(midmem);
static FixedBlockAllocator cco_alloc(eastl::max(sizeof(RendinstImpulseThresholdData), sizeof(TreeRendinstImpulseThresholdData)), 32);
static Tab<CachedCollisionObjectInfo *> cached_collision_objects(midmem);
static CollisionObject tree_collision;
static void do_delayed_ri_extra_destruction_impl();

static bool useDebugTreeInstData;
static rendinst::TreeInstDebugData debugTreeInstData;
static int currentCutTreeCount = 0;

#if ENABLE_APEX
template <>
struct eastl::hash<rendinst::RendInstDesc>
{
  size_t operator()(const rendinst::RendInstDesc &val) const
  {
    uint32_t result = 2166136261U;
    result = (result * 16777619) ^ val.cellIdx;
    result = (result * 16777619) ^ val.pool;
    result = (result * 16777619) ^ val.offs;
    return (size_t)result;
  }
};
static eastl::hash_set<rendinst::RendInstDesc> apex_destructed_list;
#endif

#define FMT_DESC_STR      "[pool:%i cell:%i index:%i offset:%i]"
#define FMT_DESC_V(X)     int((X).pool), int((X).cellIdx), int((X).idx), int((X).offs)
#define VERBOSE_SYNC(...) // debug("[ri destr sync] " __VA_ARGS__)


static void call_restorable_rendinst_cb(const rendinst::RendInstDesc &desc, rendinstdestr::RestorableRendinstState state)
{
  constexpr const char *STATE_NAMES[3] = {"CREATED", "RESTORED", "DESTROYED"};
  G_UNUSED(STATE_NAMES);
  VERBOSE_SYNC("  restorable callback called desc:" FMT_DESC_STR " state:%s", FMT_DESC_V(desc), STATE_NAMES[int(state)]);
  for (rendinstdestr::restorable_rendinst_callback cb : restorable_rendinst_callbacks)
    cb(desc, state);
}

namespace rendinst
{
extern bool enable_apex;
};

class RestorableRendinst
{
  // To restore
  rendinst::RendInstDesc riDesc;
  rendinst::RendInstBufferData riBuffer;

public:
  bool isRequestSentToServer = false;
  rendinstdestr::QuantizedDestrImpulsePos serializedLocalPos;
  rendinstdestr::QuantizedDestrImpulseVec serializedImpulse;
  destructables::id_t destrId = destructables::INVALID_ID;

private:
  float ttl;
  float atTime;
  rendinst::CollisionInfo collInfo;
  bool confirmedDestruction;
  rendinst::riex_handle_t generatedHandle;
#if ENABLE_APEX
  int apexDestrId;
#endif

public:
  RestorableRendinst(const rendinst::RendInstDesc &desc, const rendinst::RendInstBufferData &buffer, destructables::id_t destr_id,
    float at_time, const rendinst::CollisionInfo &coll_info, rendinst::riex_handle_t generated_handle, int apex_destr_id) :
    riDesc(desc),
    riBuffer(buffer),
    destrId(destr_id),
    ttl(1.f),
    atTime(at_time),
    collInfo(coll_info),
    confirmedDestruction(false),
    generatedHandle(generated_handle)
#if ENABLE_APEX
    ,
    apexDestrId(apex_destr_id)
#endif
  {
    G_UNUSED(apex_destr_id);
  }

  bool isRestoreNeeded() const { return !confirmedDestruction; }

  rendinst::riex_handle_t restore()
  {
    rendinstdestr::remove_tree_rendinst_destr(riDesc);
    rendinst::delRIGenExtra(generatedHandle);
    destructables::removeDestructableById(destrId);
    rendinst::riex_handle_t h = rendinst::restoreRiGenDestr(riDesc, riBuffer);
    VERBOSE_SYNC("restored ri (likely no sync received) desc:" FMT_DESC_STR " restored:%i", FMT_DESC_V(riDesc),
      h != rendinst::RIEX_HANDLE_NULL);
#if ENABLE_APEX
    if (rendinst::enable_apex && apex_force_remove_actor_cb && apexDestrId >= 0)
      apex_force_remove_actor_cb(apexDestrId);
    apex_destructed_list.erase(riDesc);
#endif
    return h;
  }

  bool update(float dt) // return false if need to be destroyed
  {
    ttl -= dt;
    return !confirmedDestruction && ttl > 0.f;
  }

  void confirmDestruction()
  {
    VERBOSE_SYNC("confirmed ri destruction desc:" FMT_DESC_STR, FMT_DESC_V(riDesc));
    confirmedDestruction = true;
  }

  void invalidateHandle(rendinst::riex_handle_t invalidate_handle)
  {
    if (generatedHandle == invalidate_handle)
      generatedHandle = rendinst::RIEX_HANDLE_NULL;
  }

  const rendinst::RendInstDesc &getRiDesc() const { return riDesc; }
  float getAtTime() const { return atTime; }
  float getTtl() const { return ttl; }
  const rendinst::CollisionInfo &getCollInfo() const { return collInfo; }
};
static eastl::vector<RestorableRendinst> restorables;

static void invalidate_handle_cb(rendinst::riex_handle_t invalidate_handle)
{
  for (RestorableRendinst &restr : restorables)
    restr.invalidateHandle(invalidate_handle);
}

rendinstdestr::DestrSettings &rendinstdestr::get_mutable_destr_settings() { return destrSettings; }

const rendinstdestr::DestrSettings &rendinstdestr::get_destr_settings() { return destrSettings; }

void rendinstdestr::init_ex(rendinstdestr::on_destr_changed_callback on_destr_cb, ri_tree_sound_cb tree_sound_cb,
  get_camera_pos get_current_camera_pos_, get_current_time_callback get_current_time_cb, bool enable_branch_destruction,
  remove_physx_collision_object_callback remove_physx_obj_cb, create_apex_actors_callback create_apex_actors_cb,
  apex_force_remove_actor_callback apex_remove_actor_cb)
{
  on_changed_destr_cb = on_destr_cb;
  apply_pending_destrs = true;
  g_delayed_net_destr_list.isEnabled = true;
  g_deferred_ri_destr_list.isEnabled = true;
  rendinst::enable_apex = false;
  g_tree_sound_cb = tree_sound_cb;
  rem_physx_collision_obj_cb = remove_physx_obj_cb;
  create_apex_actors_at_point_cb = create_apex_actors_cb;
  apex_force_remove_actor_cb = apex_remove_actor_cb;
  get_current_camera_pos = get_current_camera_pos_;
  g_get_current_time_cb = get_current_time_cb;
  rendinst::registerRIGenExtraInvalidateHandleCb(invalidate_handle_cb);
  rendinst::do_delayed_ri_extra_destruction = +[] {
    rendinst::ScopedRIExtraReadLock rl;
    do_delayed_ri_extra_destruction_impl();
  };
  debugTreeInstData.branchDestrFromDamage = get_tree_destr().branchDestrFromDamage;
  debugTreeInstData.branchDestrOther = get_tree_destr().branchDestrOther;
  is_branch_destruction_supported = enable_branch_destruction;
  riexsync::pool_mapping_change_callback = on_riex_pool_mapping_change;
  riexsync::sync_active = true;
}

void rendinstdestr::init(rendinstdestr::on_destr_changed_callback on_destr_cb, bool apply_pending, bool enable_branch_destruction,
  ri_tree_sound_cb tree_sound_cb, get_camera_pos get_camera_pos_cb, get_current_time_callback get_current_time_cb)
{
  on_changed_destr_cb = on_destr_cb;
  g_delayed_net_destr_list.isEnabled = true;
  g_deferred_ri_destr_list.isEnabled = true;
  rendinst::enable_apex = false;

  apply_pending_destrs = apply_pending;
  is_branch_destruction_supported = enable_branch_destruction;
  g_tree_sound_cb = tree_sound_cb;
  rem_physx_collision_obj_cb = nullptr;
  create_apex_actors_at_point_cb = nullptr;
  apex_force_remove_actor_cb = nullptr;
  get_current_camera_pos = get_camera_pos_cb;
  g_get_current_time_cb = get_current_time_cb;

  debugTreeInstData.branchDestrFromDamage = get_tree_destr().branchDestrFromDamage;
  debugTreeInstData.branchDestrOther = get_tree_destr().branchDestrOther;
  riexsync::pool_mapping_change_callback = on_riex_pool_mapping_change;
  riexsync::sync_active = true;
}


void rendinstdestr::set_occlusion_callback(get_occlusion_callback cb) { get_occlusion_cb = cb; }

void rendinstdestr::clear()
{
  restorables.clear();
  restorables.shrink_to_fit();
  riexsync::clear_synced_ri_extra_pools();
  clear_tree_collision();
  clear_all_ptr_items_and_shrink(cached_collision_objects);
  G_VERIFY(cco_alloc.clear() == 0);
  clear_phys_objs();
}

void rendinstdestr::shutdown()
{
  clear();
  endSession();
  ri_effect_cb = nullptr;
  rendinst::sweep_rendinst_cb = nullptr;
  on_rendinst_destroyed_callbacks.clear();
}

void rendinstdestr::startSession(void *phys_wld)
{
  phys_world = static_cast<PhysWorld *>(phys_wld);
  apply_reflection = true;
  if (ri_destr_collision_queue_count != RI_DESTR_COLLISION_DISABLED)
    ri_destr_collision_queue_count = 0;

  if (pending_initial_destrs.GetNumberOfUnreadBits() > 0 && apply_pending_destrs)
  {
    deserialize_destr_data(pending_initial_destrs, INITIAL_REPLICATION);
    pending_initial_destrs.Clear();
  }
  if (pending_destrs_updates.GetNumberOfUnreadBits() > 0 && apply_pending_destrs)
  {
    danet::BitStream bs(framemem_ptr());
    while (pending_destrs_updates.GetNumberOfUnreadBits() > 0)
    {
      bs.Reset();
      if (pending_destrs_updates.Read(bs))
      {
        bool done = deserialize_destr_update(bs);
        G_ASSERT(done && bs.GetNumberOfUnreadBits() == 0);
        G_UNUSED(done);
      }
    }
    pending_destrs_updates.Clear();
  }
  simultaneous_destrs_count = rendinstdestr::get_destr_settings().riMaxSimultaneousDestrs;
}

void rendinstdestr::endSession()
{
  clear();
  apply_reflection = false;
  pending_initial_destrs.Clear();
  pending_destrs_updates.Clear();

  g_delayed_net_destr_list.clear();
  g_deferred_ri_destr_list.clear();
  simultaneous_destrs_count = 0;
  if (ri_destr_collision_queue_count != RI_DESTR_COLLISION_DISABLED)
    ri_destr_collision_queue_count = RI_DESTR_COLLISION_NO_SESSION;
}

void rendinstdestr::set_ri_destr_collision_enabled(bool enabled)
{
  ri_destr_collision_queue_count = enabled ? RI_DESTR_COLLISION_NO_SESSION : RI_DESTR_COLLISION_DISABLED;
}

static void queue_ri_collision_for_debris(const mat44f &inst_tm, const bbox3f &local_bbox)
{
  if (!get_current_camera_pos)
    return;

  bbox3f wbb;
  v_bbox3_init(wbb, inst_tm, local_bbox);
  vec4f margin = v_splats(rendinstdestr::get_destr_settings().destrCollisionMargin);
  wbb.bmin = v_sub(wbb.bmin, margin);
  wbb.bmax = v_add(wbb.bmax, margin);
  BBox3 worldBox;
  v_stu_bbox3(worldBox, wbb);

  Point3 camPos = get_current_camera_pos();
  float distSq = v_extract_x(v_distance_sq_to_bbox_x(wbb.bmin, wbb.bmax, v_ldu(&camPos.x)));
  float maxDist = rendinstdestr::get_destr_settings().destrCollisionMaxCameraDist;
  if (distSq > maxDist * maxDist)
    return;

  if (ri_destr_collision_queue_count < 0)
    return;

  // Try to merge into an existing queued box, pick the candidate with smallest merged volume
  const rendinstdestr::DestrSettings &settings = rendinstdestr::get_destr_settings();
  int queueCapacity = min(settings.destrCollisionQueueCapacity, RI_DESTR_COLLISION_MAX_QUEUE_CAPACITY);
  float newVolume = worldBox.volume();
  int bestIdx = -1;
  float bestMergedVolume = FLT_MAX;
  for (int i = 0; i < ri_destr_collision_queue_count; ++i)
  {
    BBox3 merged = ri_destr_collision_queue[i];
    merged += worldBox;
    Point3 mergedSize = merged.width();
    if (mergedSize.x > settings.destrCollisionMaxMergeXZ || mergedSize.z > settings.destrCollisionMaxMergeXZ ||
        mergedSize.y > settings.destrCollisionMaxMergeY)
      continue;
    float mergedVolume = merged.volume();
    float sumVolume = ri_destr_collision_queue[i].volume() + newVolume;
    if (mergedVolume < eastl::min(bestMergedVolume, sumVolume * settings.destrCollisionMergeThreshold))
    {
      bestIdx = i;
      bestMergedVolume = mergedVolume;
    }
  }
  if (bestIdx >= 0) // good candidate for merge found, merge with it
  {
    ri_destr_collision_queue[bestIdx] += worldBox;
    if (ri_debug_collision) [[unlikely]]
    {
      RIDESTR_DBG_LOG("ridestr: merged box " FMT_P3_02F "-" FMT_P3_02F " into queue[%d], new size " FMT_P3_02F "-" FMT_P3_02F
                      ", queue size=%d",
        P3D(worldBox.lim[0]), P3D(worldBox.lim[1]), bestIdx, P3D(ri_destr_collision_queue[bestIdx].lim[0]),
        P3D(ri_destr_collision_queue[bestIdx].lim[1]), ri_destr_collision_queue_count);
      draw_debug_box_buffered(worldBox, E3DCOLOR_MAKE(0, 255, 255, 255), 3000);
    }
  }
  else if (ri_destr_collision_queue_count >= queueCapacity) // queue is full, attempt to replace an existing bbox
  {
    int furthestIdx = 0;
    float furthestDistSq = 0.f;
    for (int i = 0; i < ri_destr_collision_queue_count; ++i)
    {
      float d = (ri_destr_collision_queue[i].center() - camPos).lengthSq();
      if (d > furthestDistSq)
      {
        furthestDistSq = d;
        furthestIdx = i;
      }
    }
    if (distSq >= furthestDistSq)
    {
      if (ri_debug_collision) [[unlikely]]
      {
        RIDESTR_DBG_LOG("ridestr: queue full (%d), new box not closer than furthest", queueCapacity);
        draw_debug_box_buffered(worldBox, E3DCOLOR_MAKE(255, 0, 0, 255), 3000);
      }
    }
    else
    {
      if (ri_debug_collision) [[unlikely]]
      {
        RIDESTR_DBG_LOG("ridestr: queue full (%d), replacing queue[%d] (dist %.1f) with closer box (dist %.1f)", queueCapacity,
          furthestIdx, sqrtf(furthestDistSq), sqrtf(distSq));
        draw_debug_box_buffered(worldBox, E3DCOLOR_MAKE(255, 255, 0, 255), 3000);
      }
      ri_destr_collision_queue[furthestIdx] = worldBox;
    }
  }
  else // merge didn't happen, and there is place in queue
  {
    ri_destr_collision_queue[ri_destr_collision_queue_count++] = worldBox;
    if (ri_debug_collision) [[unlikely]]
    {
      RIDESTR_DBG_LOG("ridestr: queued box " FMT_P3_02F "-" FMT_P3_02F ", queue size=%d", P3D(worldBox.lim[0]), P3D(worldBox.lim[1]),
        ri_destr_collision_queue_count);
      draw_debug_box_buffered(worldBox, E3DCOLOR_MAKE(0, 255, 0, 255), 3000);
    }
  }
}

int rendinstdestr::process_ri_collision_queue()
{
  if (ri_destr_collision_queue_count <= 0)
    return 0;

  int count = min(ri_destr_collision_queue_count, get_destr_settings().destrCollisionPerFrameLimit);
  if (count <= 0)
    return 0;

  TIME_PROFILE(ridestr_process_ri_collision_queue);

  float prevTtl = dacoll::exchange_ttl_for_collision_instances(get_destr_settings().destrCollisionTTL);
  FINALLY([&] { dacoll::exchange_ttl_for_collision_instances(prevTtl); });
  RIDESTR_DBG_LOG("ridestr: process_ri_collision_queue: count=%d, remaining=%d, prevTtl=%.2f, setting TTL=%.2f", count,
    ri_destr_collision_queue_count - count, prevTtl, get_destr_settings().destrCollisionTTL);

  if (!get_current_camera_pos || !get_occlusion_cb)
    return 0;
  const Occlusion *occl = get_occlusion_cb();
  Point3 camPos = get_current_camera_pos();

  int totalRiCount = 0;
  for (int i = 0; i < count; ++i)
  {
    float skipOccDist = get_destr_settings().destrCollisionSkipOcclusionDist;
    BBox3 &bbox = ri_destr_collision_queue[i];
    if (occl && (bbox.center() - camPos).lengthSq() > skipOccDist * skipOccDist)
    {
      bbox3f wbb = v_ldu_bbox3(bbox);
      if (!occl->isVisibleBox(wbb))
      {
        if (ri_debug_collision) [[unlikely]]
          draw_debug_box_buffered(bbox, E3DCOLOR_MAKE(255, 255, 255, 255), 3000);
        continue;
      }
    }
    int riCount = dacoll::update_ri_cache_in_volume_to_phys_world(bbox);
    totalRiCount += riCount;
    if (ri_debug_collision) [[unlikely]]
    {
      RIDESTR_DBG_LOG("ridestr:   box[%d]: loaded %d RIs", i, riCount);
      BBox3 boxToDraw = bbox;
      boxToDraw.scale(1.1f);
      draw_debug_box_buffered(boxToDraw, E3DCOLOR_MAKE(0, 0, 255, 255), 3000);
    }
  }

  int remaining = ri_destr_collision_queue_count - count;
  if (remaining > 0)
    memmove(ri_destr_collision_queue, ri_destr_collision_queue + count, remaining * sizeof(BBox3));
  ri_destr_collision_queue_count = remaining;

  RIDESTR_DBG_LOG("ridestr: processed %d queued volumes, loaded %d RI collision bodies total", count, totalRiCount);

  return totalRiCount;
}

void rendinstdestr::testObjToRestorablesIntersection(const BBox3 & /*obj_box*/, const TMatrix & /*obj_tm*/,
  rendinst::RendInstCollisionCB *coll_cb, float at_time)
{
  for (auto &restr : restorables)
  {
    if (restr.getAtTime() < at_time)
      continue;
    if (rendinst::isRIGenPosInst(restr.getRiDesc()))
      coll_cb->addTreeCheck(restr.getCollInfo());
    else
      coll_cb->addCollisionCheck(restr.getCollInfo());
  }
}

void rendinstdestr::remove_restorable_by_destructable_id(destructables::id_t id)
{
  if (id == destructables::INVALID_ID)
    return;
  auto it = eastl::find(restorables.begin(), restorables.end(), id,
    [](const RestorableRendinst &rri, destructables::id_t destr_id) { return rri.destrId == destr_id; });
  if (it != restorables.end())
  {
    call_restorable_rendinst_cb(it->getRiDesc(), rendinstdestr::RRS_DESTROYED);
    restorables.erase(it);
  }
}

void rendinstdestr::setApexEnabled(bool enabled) { rendinst::enable_apex = enabled; }
void rendinstdestr::resetApexDestructedRIList()
{
#if ENABLE_APEX
  if (apex_destructed_list.size())
    debug("clearing apex_destructed_list=%d", apex_destructed_list.size());
  apex_destructed_list.clear();
#endif
}


using DestrNeighborsSet =
  eastl::vector_map<rendinst::riex_handle_t, rendinst::CollisionInfo, eastl::less<rendinst::riex_handle_t>, framemem_allocator>;

static void findRendinstNeighbors(rendinst::RendInstDesc desc, int destroy_neighbour_recursive_depth,
  const rendinst::CollisionInfo *coll_info, DestrNeighborsSet &ri_to_destroy)
{
  if (destroy_neighbour_recursive_depth == 0 || !coll_info || !coll_info->isParent || !phys_world || !rendinst::isRiGenInWorld(desc))
    return;

  if (destroy_neighbour_recursive_depth < 0)
    destroy_neighbour_recursive_depth = coll_info->destroyNeighbourDepth;

  struct RiExtraCollisionCB final : public rendinst::RendInstCollisionCB
  {
    const rendinst::RendInstDesc &initiatorDesc;
    DestrNeighborsSet &destrNeighborsData;
    const int destroyNeighbourDepth;
    const char *tag;

    RiExtraCollisionCB(const rendinst::RendInstDesc &id, DestrNeighborsSet &dnd, int depth, const char *tg) :
      initiatorDesc(id), destrNeighborsData(dnd), destroyNeighbourDepth(depth), tag(tg)
    {}
    virtual void addCollisionCheck(const rendinst::CollisionInfo &coll_info) override
    {
      rendinst::riex_handle_t riHandle = rendinst::make_handle(coll_info.desc.pool, coll_info.desc.idx);
      if (coll_info.desc != initiatorDesc && coll_info.isDestr && !coll_info.isImmortal && coll_info.destructibleByParent &&
          strcmp(tag, coll_info.destroyedByTag) == 0)
        if (destrNeighborsData.find(riHandle) == destrNeighborsData.end())
        {
          destrNeighborsData.insert(eastl::make_pair(riHandle, coll_info));
          findRendinstNeighbors(coll_info.desc, destroyNeighbourDepth - 1, &coll_info, destrNeighborsData);
        }
    }
    virtual void addTreeCheck(const rendinst::CollisionInfo & /*coll_info*/) override {}
  } rendinstCallback(desc, ri_to_destroy, destroy_neighbour_recursive_depth, coll_info->tag.c_str());

  auto localBBox = coll_info->localBBox;
  localBBox.lim[1] += RI_DESTROY_BBOX_MARGIN_UP;
  rendinst::testObjToRendInstIntersection(localBBox, coll_info->tm, rendinstCallback, rendinst::GatherRiTypeFlag::RiExtraOnly);
}


void rendinstdestr::fill_ri_destructable_params(destructables::DestructableCreationParams &params, const rendinst::RendInstDesc &desc,
  DynamicPhysObjectData *po_data, const TMatrix &tm, rendinst::DestrOptionFlags flags)
{
  params.physObjData = po_data;
  params.tm = tm;
  if (get_current_camera_pos)
    params.camPos = get_current_camera_pos();
  const rendinst::riex_handle_t riexHandle = desc.getRiExtraHandle();
  params.resIdx = desc.pool;
  if (!params.hashVal)
    params.hashVal = riexHandle != rendinst::RIEX_HANDLE_NULL ? rendinst::get_riextra_instance_seed(riexHandle) : 0;
  params.timeToLive = riexHandle != rendinst::RIEX_HANDLE_NULL ? rendinst::get_riextra_destr_time_to_live(riexHandle) : 15.0f;
  params.timeToKinematic =
    riexHandle != rendinst::RIEX_HANDLE_NULL ? rendinst::get_riextra_destr_time_to_kinematic(riexHandle) : -1.0f;
  if (riexHandle != rendinst::RIEX_HANDLE_NULL)
  {
    params.timeToSinkUnderground = rendinst::get_riextra_destr_time_to_sink_underground(riexHandle);
    params.defaultTimeToLive = rendinst::get_riextra_destr_default_time_to_live(riexHandle);
  }
  params.isDestroyedByExplosion = bool(flags & rendinst::DestrOptionFlag::DestroyedByExplosion);
  auto disintegrationParams =
    riexHandle != rendinst::RIEX_HANDLE_NULL ? rendinst::get_riextra_destr_disintegration_params(riexHandle) : Point3(-1.0f, 0, 1);
  params.timeToStartDisintegration = disintegrationParams.x;
  params.disintegrationDuration = disintegrationParams.y;
  params.disintegrationScale = disintegrationParams.z;
}

static rendinst::RendInstDesc destroyRendinstInternal(rendinst::RendInstDesc desc, bool add_restorable, const Point3 &pos,
  const Point3 &impulse, float at_time, const rendinst::CollisionInfo *coll_info, bool create_destr_effects,
  ApexDmgInfo *apex_dmg_info, rendinstdestr::on_destr_callback on_destr_cb, rendinst::DestrOptionFlags flags)
{
  TIME_PROFILE(ridestr__destroyRendinstInternal);
  // Note: 'desc' is copied by value in order to avoid situations where desc = &coll_info->desc and
  // 'on_destr_cb' invalidates it.
  if (!rendinst::isRiGenInWorld(desc) || !phys_world)
    return desc;

  G_ASSERTF(lengthSq(impulse) < sqr(MAX_RI_DESTROY_IMPULSE), "Bad destroy rendInst impulse %@", impulse);

#if ENABLE_APEX
  if (rendinst::enable_apex && rem_physx_collision_obj_cb)
  {
    TIME_PROFILE(destroyRI_rem_physx_collision_obj_cb);
    rem_physx_collision_obj_cb(desc);
  }
#endif

  TMatrix mainTm = rendinst::getRIGenMatrix(desc);

  rendinst::RendInstBufferData riBuffer;
  rendinst::RendInstDesc offsetedDesc = rendinst::get_restorable_desc(desc); // Desc with offs set to restorable_desc.offs
  offsetedDesc.idx = desc.idx;
  const bool canAddRestorable = add_restorable && offsetedDesc.isValid() && rendinstdestr::can_sync_ri_destr(offsetedDesc) &&
                                rendinstdestr::get_destr_settings().isNetClient && coll_info &&
                                !(desc.isRiExtra() && desc.isDynamicRiExtra());

  rendinst::riex_handle_t createdDestroyedRiexHandle = rendinst::RIEX_HANDLE_NULL; // New riex replacing destroyed
  dacoll::invalidate_ri_instance(desc); // do before destring, otherwise in riextra we'll lose unique data and will not be able to
                                        // compare them

  if (on_destr_cb)
    on_destr_cb(desc);
  if (on_changed_destr_cb)                                                 // call it before data will be destroyed
    on_changed_destr_cb(desc, mainTm, pos, impulse, create_destr_effects); // TODO: always mainTm==riTm?

  int32_t userData = coll_info ? coll_info->userData : -1;
  bool outRiRemoved = false;

  uint32_t riexInstanceSeed = 0;
  if (desc.isRiExtra())
  { // get seed before node destruction (doRiGenDestr), destructible creation params need it
    rendinst::riex_handle_t riexHandle = rendinst::make_handle(desc.pool, desc.idx);
    riexInstanceSeed = riexHandle != rendinst::RIEX_HANDLE_NULL ? rendinst::get_riextra_instance_seed(riexHandle) : 0;
  }

  if (canAddRestorable)
    call_restorable_rendinst_cb(offsetedDesc, rendinstdestr::RRS_CREATED); // call restored cb BEFORE destroying, so it will be
                                                                           // called, before handle is invalidated
  const Point3 *collPoint = (pos == ZERO<Point3>()) ? nullptr : &pos;
  DynamicPhysObjectData *poData =
    rendinst::doRIGenDestr(desc, riBuffer, create_destr_effects, create_destr_effects ? ri_effect_cb : nullptr,
      createdDestroyedRiexHandle, userData, collPoint, &outRiRemoved, flags, impulse, pos);
  if (canAddRestorable && !outRiRemoved)
    call_restorable_rendinst_cb(offsetedDesc, rendinstdestr::RRS_RESTORED); // not destroyed - call restored cb to rollback previous
                                                                            // call

  BBox3 bbox = flags & rendinst::DestrOptionFlag::UseFullBbox ? rendinst::getRIGenFullBBox(desc)
               : coll_info                                    ? coll_info->localBBox
                                                              : rendinst::getRIGenBBox(desc);

  bool apex_asset_created = false;
  int apex_destructible_id = -1;
#if ENABLE_APEX
  if (
    rendinst::enable_apex && create_destr_effects && rendinstdestr::get_destr_settings().createDestr && create_apex_actors_at_point_cb)
  {
    TMatrix normalizedTm = mainTm;
    Point3 scale = Point3(mainTm.getcol(0).length(), mainTm.getcol(1).length(), mainTm.getcol(2).length());

    normalizedTm.orthonormalize();
    if (apex_destructed_list.find(desc) == apex_destructed_list.end()) // skip duplicates from duplicated server packets
    {
      TIME_PROFILE(destroyRI_create_apex_actors_at_point_cb);
      apex_destructible_id = create_apex_actors_at_point_cb(getRIGenDestrName(desc), normalizedTm, scale, pos, riexInstanceSeed,
        impulse, desc.pool, apex_dmg_info, desc.idx, bbox, bool(flags & rendinst::DestrOptionFlag::DestroyedByCollision));
      apex_destructed_list.insert(desc);
    }
    else
      logwarn("skip duplicate apex destruction of RI=%d:%d~%d:%d (apex_destructed_list=%d)", desc.cellIdx, desc.idx, desc.pool,
        desc.offs, apex_destructed_list.size());
    apex_asset_created = apex_destructible_id != -1; // NOTE: check exactly != -1 (not >=0, ..._cb may return negative values,
                                                     // depending on its logic)
  }
#else
  G_UNUSED(apex_dmg_info);
#endif

#if ENABLE_APEX
  // Only call on_rendinst_destroyed_cb if there is no apex asset since apex has its own callback
  if (!apex_asset_created)
#endif
    rendinstdestr::call_on_rendinst_destroyed_cb(desc.getRiExtraHandle(), mainTm, bbox);

  destructables::id_t destrId = destructables::INVALID_ID;
  if (!apex_asset_created && poData && create_destr_effects && rendinstdestr::get_destr_settings().createDestr) //-V560
  {
    // Queue nearby RI collision loading so debris can collide with standing rendinsts
    {
      mat44f tm44;
      v_mat44_make_from_43cu_unsafe(tm44, mainTm.array);
      if (ri_destr_collision_queue_count >= 0)
        queue_ri_collision_for_debris(tm44, v_ldu_bbox3(bbox));
    }

    destructables::DestructableCreationParams params;
    params.hashVal = riexInstanceSeed;
    rendinstdestr::fill_ri_destructable_params(params, desc, poData, mainTm, flags);
    gamephys::DestructableObject *destr = nullptr;
    mat44f tm44;
    v_mat44_make_from_43cu_unsafe(tm44, mainTm.array);
    bool isDestructableTmNaN = v_test_mat43_nan(tm44);
    G_ASSERT(!isDestructableTmNaN);
    if (!isDestructableTmNaN)
    {
      destrId = destructables::addDestructable(&destr, params, phys_world);
      if (destr) //-V1051
      {
        if (impulse.lengthSq() > 0.f)
          destr->addImpulse(*phys_world, pos, impulse);
      }
    }
  }

  if (canAddRestorable && outRiRemoved)
  {
    // At this point coll_info->desc might be invalid, since 'on_destr_cb' can
    // call 'reset' on it, but we need proper coll_info for restorable so that
    // vehicle collisions could be checked against it, so copy proper desc over.
    rendinst::CollisionInfo tmp = *coll_info;
    tmp.desc = desc;
    VERBOSE_SYNC("added restorable desc:" FMT_DESC_STR " pos:" FMT_P3_02F, FMT_DESC_V(offsetedDesc), P3D(coll_info->tm.getcol(3)));
    RestorableRendinst &restorable =
      restorables.emplace_back(offsetedDesc, riBuffer, destrId, at_time, tmp, createdDestroyedRiexHandle, apex_destructible_id);
    bool clamped = false;
    restorable.serializedImpulse.packPos(impulse, &clamped);
    if (restorable.serializedImpulse != 0)
    {
      Point3_vec4 localImpPos;
      mat44f tm, itm;
      v_mat44_make_from_43cu_unsafe(tm, mainTm.array);
      v_mat44_inverse43(itm, tm);
      v_st(&localImpPos.x, v_mat44_mul_vec3p(itm, v_ldu(&pos.x)));
      restorable.serializedLocalPos.packPos(localImpPos, &clamped);
    }
  }
  else if (desc.isRiExtra() && offsetedDesc.isValid())
    call_restorable_rendinst_cb(offsetedDesc, rendinstdestr::RRS_DESTROYED);

  return offsetedDesc;
}

void rendinstdestr::debug_draw_ri_phys()
{
  for (auto &phys : riPhys)
  {
    TMatrix tm = phys.lastValidTm;
    BBox3 bbox = rendinst::getRIGenFullBBox(phys.descForDestr);
    mat44f mat;
    v_mat44_make_from_43cu_unsafe(mat, tm.m[0]);

    bbox3f bbox_transformed;
    v_bbox3_init(bbox_transformed, mat, v_ldu_bbox3(bbox));

    BBox3 resBox;
    resBox += as_point3(&bbox_transformed.bmin);
    resBox += as_point3(&bbox_transformed.bmax);
    draw_cached_debug_box(resBox, E3DCOLOR(0, 0, 255));

    end_draw_cached_debug_lines();
    set_cached_debug_lines_wtm(TMatrix::IDENT);
  }
}

bbox3f rendinstdestr::get_ri_phys_containing_bbox(const bbox3f &intersect_bbox, const Frustum &intersect_frustum, bool only_trees)
{
  bbox3f resultBbox;
  v_bbox3_init_empty(resultBbox);
  for (auto &phys : riPhys)
  {
    if (only_trees && phys.type != RendInstPhysType::TREE)
      continue;

    TMatrix tm = phys.lastValidTm;
    BBox3 bbox = rendinst::getRIGenFullBBox(phys.descForDestr);
    mat44f mat;
    v_mat44_make_from_43cu_unsafe(mat, tm.m[0]);

    bbox3f bbox_transformed;
    v_bbox3_init(bbox_transformed, mat, v_ldu_bbox3(bbox));

    if (!v_bbox3_test_box_intersect(bbox_transformed, intersect_bbox))
      continue;
    if (!intersect_frustum.testBoxB(bbox_transformed.bmin, bbox_transformed.bmax))
      continue;

    v_bbox3_add_pt(resultBbox, bbox_transformed.bmin);
    v_bbox3_add_pt(resultBbox, bbox_transformed.bmax);
  }
  return resultBbox;
}

rendinst::RendInstDesc rendinstdestr::destroyRendinst(rendinst::RendInstDesc desc, bool add_restorable, const Point3 &pos,
  const Point3 &impulse, float at_time, const rendinst::CollisionInfo *coll_info, bool create_destr_effects,
  ApexDmgInfo *apex_dmg_info, int destroy_neighbour_recursive_depth, float impulse_mult_for_child, on_destr_callback on_destr_cb,
  rendinst::DestrOptionFlags flags)
{
  TIME_PROFILE(rendinstdestr__destroyRendinst);
  // Note: 'desc' is copied by value in order to avoid situations where desc = &coll_info->desc and
  // 'on_destr_cb' invalidates it.
  if (!rendinst::isRiGenInWorld(desc) || !phys_world)
    return desc;

  DestrNeighborsSet riToDestroy;
  findRendinstNeighbors(desc, destroy_neighbour_recursive_depth, coll_info, riToDestroy);

  // first lets try to destroy parent object first
  const auto ret = destroyRendinstInternal(desc, add_restorable, pos, impulse, at_time, coll_info, create_destr_effects, apex_dmg_info,
    on_destr_cb, flags);

  for (auto &it : riToDestroy)
    destroyRendinstInternal(it.second.desc, add_restorable, pos, impulse * impulse_mult_for_child, at_time, &it.second,
      create_destr_effects, apex_dmg_info, on_destr_cb, flags | rendinst::DestrOptionFlag::ForceDestroy);
  return ret;
}

void rendinstdestr::destroyRiExtra(rendinst::riex_handle_t riex_handle, const TMatrix &transform, bool create_destr_effects,
  const Point3 &impulse, const Point3 &impulse_pos, rendinst::DestrOptionFlags flags)
{
  mat44f tm44;
  v_mat44_make_from_43cu_unsafe(tm44, transform.array);
  bool isDestructableTmNaN = v_test_mat43_nan(tm44);
  G_ASSERT(!isDestructableTmNaN);
  if (DynamicPhysObjectData *poData = rendinst::doRIExGenDestrEx(riex_handle, create_destr_effects ? ri_effect_cb : nullptr);
      poData != nullptr && rendinstdestr::get_destr_settings().createDestr && !isDestructableTmNaN)
  {
    // Queue nearby RI collision loading so debris can collide with standing rendinsts
    if (ri_destr_collision_queue_count >= 0)
      queue_ri_collision_for_debris(tm44, rendinst::riex_get_lbb(rendinst::handle_to_ri_type(riex_handle)));

    destructables::DestructableCreationParams params;
    fill_ri_destructable_params(params, rendinst::RendInstDesc(riex_handle), poData, transform, flags);

    gamephys::DestructableObject *destr = nullptr;
    destructables::addDestructable(&destr, params, phys_world);
    if (destr && impulse.lengthSq() > 0.f)
    {
      G_ASSERTF(lengthSq(impulse) < sqr(MAX_RI_DESTROY_IMPULSE), "Bad destroy rendInst impulse %@", impulse);
      destr->addImpulse(*phys_world, impulse_pos, impulse);
    }
  }
}

static void cleanup_ri_phys(RendInstPhys &phys)
{
  if (phys.treeInstData.cutType == rendinst::TreeInstData::CutType::Top)
    currentCutTreeCount--;
  phys.cleanup();
}

void rendinstdestr::update_paused(float current_time, const Frustum *frustum) { update(0.f, current_time, frustum); }

void rendinstdestr::update(float dt, float current_time, const Frustum *frustum)
{
  TIME_PROFILE(rendinstdestr_update);

  {
    TIME_PROFILE_DEV(update_cached_collision_objects);
    for (auto it = cached_collision_objects.begin(); it != cached_collision_objects.end();)
      if ((*it)->update(dt))
        ++it;
      else
      {
        auto cco = *it;
        it = cached_collision_objects.erase_unsorted(it);
        delete cco;
      }
  }

  PhysWorld *physWorld = dacoll::get_phys_world();
  physWorld->fetchSimRes(true);

  {
    TIME_PROFILE_DEV(riPhys);

    struct RendInstUpdateTm
    {
      TMatrix tm;
      int physIdx;
      bool moved;
      bool forceUpdateDestructionData;
    };

#if TREE_DESTR_DEBUG
    const float velocityThreshold = velocityThresholdOverride < 0.0f ? 40.0f : velocityThresholdOverride;
    const float maxAngularVelocity =
      maxAngularVelocityOverride < 0.0f ? rendinstdestr::get_tree_destr().maxAngularVelocity : maxAngularVelocityOverride;
    const float groundJointBreakVelocityCap = groundJointBreakVelocityCapOverride < 0.0f ? 15.0f : groundJointBreakVelocityCapOverride;
#else
    const float velocityThreshold = 40.0f;
    const float groundJointBreakVelocityCap = 15.0f;
    const float maxAngularVelocity = rendinstdestr::get_tree_destr().maxAngularVelocity;
#endif
    const int maxDestroyedTreeCount = get_tree_destr().maxDestroyedTreeCount;

    eastl::vector<RendInstUpdateTm, framemem_allocator> updateList;
    updateList.reserve(riPhys.size());
    for (int i = 0; i < riPhys.size(); ++i)
    {
      RendInstPhys &phys = riPhys[i];

      bool isRiExtraValid = rendinst::isRiGenExtraValid(phys.ri->riHandle);

      bool physObject = phys.physModel->physType == gamephys::DynamicPhysModel::E_PHYS_OBJECT;
      const bool physObjectActive =
        physObject && (phys.physBody && phys.physBody->isActive() && phys.physBody->getInteractionLayer()) ||
        (phys.physBody && phys.additionalBody && phys.additionalBody->isActive() && phys.additionalBody->getInteractionLayer());

      bool actived = phys.ttl >= 0.f && isRiExtraValid;
      bool disableGroundJoint = false;
      bool forceUpdateDestructionData = false;

      const float currentVelocitySq = phys.physBody ? lengthSq(phys.physBody->getVelocity()) : 0.0f;
      const bool velocityThresholdExceeded = currentVelocitySq > sqr(velocityThreshold);
      const float groundJointVelocityThreshold = velocityThreshold * 0.5f;
      const bool groundJointVelocityThresholdExceeded = currentVelocitySq > sqr(groundJointVelocityThreshold);

      if (physObject)
      {
        if (physObjectActive)
        {
          if (phys.groundJointExists && groundJointVelocityThresholdExceeded)
            disableGroundJoint = true;

          if (velocityThresholdExceeded || phys.maxTtl < 0.0f)
          {
            phys.physBody->setTm(phys.lastValidTm * phys.centerOfMassTm);
            phys.physBody->setVelocity(Point3(0.f, 0.f, 0.f));
            phys.physBody->setAngularVelocity(Point3(0.f, 0.f, 0.f));
            phys.ttl = -1.f;
          }
          else
            phys.ttl = 5.f;
        }
        else
        {
          if (phys.physBody && !phys.physBody->getInteractionLayer())
            phys.physBody->activateBody(true);
          if (phys.additionalBody && !phys.additionalBody->getInteractionLayer())
            phys.additionalBody->activateBody(true);
          phys.ttl -= dt;
        }
      }
      else
      {
        if (phys.physModel->isActive())
          phys.ttl = 1.f;
        else
          phys.ttl -= dt;
      }
      if (actived && physObject && phys.maxLifeDist > 0.f && !physObjectActive)
      {
        const float distSq = lengthSq(phys.originalTm.getcol(3) - phys.lastValidTm.getcol(3));
        if (distSq > sqr(phys.maxLifeDist))
        {
          phys.maxLifeDist = -1.f;
          phys.maxTtl = -1.f;
          phys.ttl = -1.f;
        }
      }
      if (actived && physObject && phys.physBody && phys.physBody->getInteractionLayer())
      {
        Point3 angularVelocity = phys.physBody->getAngularVelocity();
        float angularSpeedSq = angularVelocity.lengthSq();
        if (angularSpeedSq > sqr(maxAngularVelocity))
        {
          angularVelocity = angularVelocity * (maxAngularVelocity / sqrt(angularSpeedSq));
          phys.physBody->setAngularVelocity(angularVelocity);
        }

        bool disableKinematicCollision = false;
        if (phys.collisionDistLimit >= 0.f)
        {
          const Point3 distanceFromOriginal = phys.originalTm.getcol(3) - phys.lastValidTm.getcol(3);
          const float distSq =
            phys.treeInstData.branchDestr.newPhysics ? lengthSq(Point2::xz(distanceFromOriginal)) : lengthSq(distanceFromOriginal);
          if (distSq > sqr(phys.collisionDistLimit))
            disableKinematicCollision = true;
        }
        if (phys.collisionAngleLimitCos >= 0.0f)
        { // originalTm contains scale, lastValidTm doesn't so only divide with it once
          const float angleCos = dot(phys.originalTm.getcol(1), phys.lastValidTm.getcol(1)) / phys.scale.y;
          if (angleCos < phys.collisionAngleLimitCos)
          {
            if (phys.groundJointExists && phys.treeInstData.cutType == rendinst::TreeInstData::CutType::None)
              disableGroundJoint = true;
            disableKinematicCollision = true;
          }
        }

        if (disableKinematicCollision)
        {
          phys.collisionDistLimit = -1.f;
          phys.collisionAngleLimitCos = -1.f;
          phys.physBody->setGroupAndLayerMask(dacoll::EPL_DEFAULT, dacoll::EPL_ALL ^ dacoll::EPL_KINEMATIC);
          if (phys.additionalBody)
            phys.additionalBody->setGroupAndLayerMask(dacoll::EPL_DEFAULT, dacoll::EPL_ALL ^ dacoll::EPL_KINEMATIC);

          if (phys.treeInstData.branchDestr.newPhysics)
          {
            phys.physBody->setDamping(0.3, 0.3);
            phys.physBody->activateBody(true);
          }
        }

        if (disableGroundJoint)
        { // Remove ground joint (it's the last joint in the list)
          // old physics don't set angle limit, so this won't happen in that case
          PhysJoint *groundJoint = phys.joints.back();
          delete groundJoint;
          phys.joints.pop_back();
          phys.groundJointExists = false;

          if (groundJointVelocityThresholdExceeded)
          { // clamp shape velocity below threshold
            const Point3 currentVelocity = phys.physBody->getVelocity();
            const Point3 clampedVelocity = currentVelocity * (groundJointBreakVelocityCap / sqrt(currentVelocitySq));
            phys.physBody->setVelocity(clampedVelocity);
            phys.physBody->setAngularVelocity(Point3(0.f, 0.f, 0.f));

            if (phys.additionalBody)
            {
              phys.additionalBody->setVelocity(Point3(0, 0, 0));
              phys.additionalBody->setAngularVelocity(Point3(0, 0, 0));
            }
          }
          phys.physBody->activateBody(true);
        }
      }
      phys.maxTtl -= dt;
      if (phys.ttl < 0.f || !isRiExtraValid)
      {
        if (!physObject)
        {
          if (rendinst::returnRIGenExternalControl(phys.desc, phys.ri))
          {
            cleanup_ri_phys(phys);
            erase_items(riPhys, i, 1);
            i--;
            continue;
          }
        }
        else if (actived && phys.physBody && phys.physBody->getInteractionLayer())
        {
          phys.physBody->setGroupAndLayerMask(0, 0);
          phys.physBody->setGravity(Point3(0.f, -0.1f, 0.f));
          phys.physBody->setDamping(0, 0);
          phys.physBody->activateBody(true);
          if (phys.treeSound.inited())
            phys.treeSound.setFalled(phys.lastValidTm);
          clear_all_ptr_items_and_shrink(phys.joints);
          del_it(phys.additionalBody);
          phys.ttl = 15.f;
          phys.maxTtl = 15.f;
          phys.treeInstData.disappearStartTimeAt = current_time;
          forceUpdateDestructionData = true;
        }
        else if (phys.physBody || !isRiExtraValid)
        {
          if (rendinst::removeRIGenExternalControl(phys.desc, phys.ri))
          {
            cleanup_ri_phys(phys);
            erase_items(riPhys, i, 1);
            i--;
          }
          continue;
        }
      }

      phys.physModel->update(dt);

      DECL_ALIGN16(TMatrix, tm) = TMatrix::IDENT;
      if (physObject)
      {
        if (phys.physBody)
        {
          phys.physBody->getTm(tm);
          if (ri_debug_collision) [[unlikely]]
            dacoll::draw_phys_body(phys.physBody, phys.physBody->getInteractionLayer() & dacoll::EPL_KINEMATIC ? 0xFFFFFF : 0xFF44FF);
        }
        TMatrix additionalTm = TMatrix::IDENT;
        if (phys.additionalBody)
        {
          phys.additionalBody->getTm(additionalTm);
          if (ri_debug_collision) [[unlikely]]
            dacoll::draw_phys_body(phys.additionalBody,
              phys.additionalBody->getInteractionLayer() & dacoll::EPL_KINEMATIC ? 0xFFFFFF : 0xFF44FF);
        }
        if (check_nan(tm.getcol(3)) || lengthSq(tm.getcol(3)) > sqr(1e6f))
        {
          logerr("Removed invalid destr phys body at pos %@", tm.getcol(3));
          if (phys.physBody) // make static analyzer happy
          {
            phys.physBody->setTm(phys.lastValidTm * phys.centerOfMassTm);
            phys.physBody->setVelocity(Point3(0.f, 0.f, 0.f));
            phys.physBody->setAngularVelocity(Point3(0.f, 0.f, 0.f));
          }
          phys.ttl = -1.f;
          continue;
        }
        tm *= inverse(phys.centerOfMassTm);
        phys.lastValidTm = tm;

        if (phys.bendHelperBody)
        {
          if (ri_debug_collision) [[unlikely]]
            dacoll::draw_phys_body(phys.bendHelperBody, 0x0000FF);
          TMatrix bendTm = TMatrix::IDENT;
          phys.bendHelperBody->getTm(bendTm);
          Point3 bendPos = bendTm.getcol(3);
          Point3 bendPosLocal = inverse(tm) * bendPos;
          phys.treeInstData.bendAnglePrev = phys.treeInstData.bendAngle;
          phys.treeInstData.bendStrengthPrev = phys.treeInstData.bendStrength;
          phys.treeInstData.bendAngle = atan2f(bendPosLocal.x, bendPosLocal.z) + PI;
          float bendStrength = phys.treeInstData.branchDestr.bendStrength;
#if TREE_DESTR_DEBUG
          if (useDebugTreeInstData)
            bendStrength = debugTreeInstData.branchDestrFromDamage.bendStrength;
#endif
          phys.treeInstData.bendStrength = sqrt(bendPosLocal.x * bendPosLocal.x + bendPosLocal.z * bendPosLocal.z) * bendStrength;
          if (phys.treeInstData.bendStrength > 0)
            forceUpdateDestructionData = true;
        }

        if (phys.additionalBody && phys.treeSound.inited() && !phys.treeSound.falled())
        {
          struct WrapperContactResultCallback
          {
            typedef ::CollisionObjectUserData obj_user_data_t;
            typedef ::gamephys::CollisionContactData contact_data_t;
            bool haveContact = false;
            void addSingleResult(const contact_data_t &, obj_user_data_t *, obj_user_data_t *) { haveContact = true; }
            static void visualDebugForSingleResult(...) {}
            bool needsCollision(obj_user_data_t *, bool b_is_static) { return b_is_static; }
          } contactCallback;

          physWorld->contactTest(phys.additionalBody, contactCallback);
          if (contactCallback.haveContact)
            phys.treeSound.setFalled(additionalTm);
          else
            phys.treeSound.updateFalling(tm);
        }

        if (
          phys.joints.size() && phys.groundJointLimitsEnabled && phys.treeInstData.cutType == rendinst::TreeInstData::CutType::Bottom)
        {
          // New physics only. Check if bottom half of a cut tree touches kinematic object
          // In this case, remove ground joint to let the tree fall. We don't support proper
          // physics simulation for the bottom half because it jerks around too much.
          struct WrapperContactResultCallback
          {
            typedef ::CollisionObjectUserData obj_user_data_t;
            typedef ::gamephys::CollisionContactData contact_data_t;
            void addSingleResult(const contact_data_t &, obj_user_data_t *, obj_user_data_t *)
            {
              Phys6DofJoint *dofJoint = Phys6DofJoint::cast(riPhys->joints[0]);
              if (dofJoint)
              {
                dofJoint->setLimit(3, Point2(-PI / 2, PI / 2));
                dofJoint->setLimit(5, Point2(-PI / 2, PI / 2));
                dofJoint->setAxisDamping(3, 0.1f);
                dofJoint->setAxisDamping(5, 0.1f);
                riPhys->physBody->activateBody(true);
              }
              riPhys->groundJointLimitsEnabled = false;
            }
            static void visualDebugForSingleResult(...) {}
            bool needsCollision(obj_user_data_t *, bool b_is_static) { return !b_is_static; }
            RendInstPhys *riPhys;
          } kinematicCallback;

          kinematicCallback.riPhys = &phys;
          physWorld->contactTest(phys.physBody, kinematicCallback, dacoll::EPL_DEFAULT, dacoll::EPL_KINEMATIC);
        }
      }
      else if (phys.riColObj.body)
      {
        phys.physModel->location.toTM(tm);
        phys.riColObj.body->setTmInstant(tm);
        if (ri_debug_collision) [[unlikely]]
          dacoll::draw_phys_body(phys.riColObj.body);
      }

      mat44f m44;
      v_mat44_make_from_43cu_unsafe(m44, tm.array);
      v_mat43_apply_scale(m44, v_ldu(&phys.scale.x));

      bbox3f bbox = rendinst::riex_get_lbb(rendinst::handle_to_ri_type(phys.ri->riHandle));
      bbox3f fullBBox;
      v_bbox3_init(fullBBox, m44, bbox);
      if (frustum && !frustum->testBoxB(fullBBox.bmin, fullBBox.bmax))
      {
        if (phys.treeInstData.originalTmDataId != rendinst::RiExtraAdditionalDataManager::INVALID_DATA_ID)
        {
          rendinst::removeRiExtraPerInstanceRenderAdditionalData(phys.treeInstData.originalTmDataId,
            rendinst::RiExtraPerInstanceDataType::INITIAL_TM_3X3);
          phys.treeInstData.originalTmDataId = rendinst::RiExtraAdditionalDataManager::INVALID_DATA_ID;
        }
        if (phys.treeInstData.destructionDataId != rendinst::RiExtraAdditionalDataManager::INVALID_DATA_ID)
        {
          rendinst::removeRiExtraPerInstanceRenderAdditionalData(phys.treeInstData.destructionDataId,
            rendinst::RiExtraPerInstanceDataType::DESTR_RENDER_DATA);
          phys.treeInstData.destructionDataId = rendinst::RiExtraAdditionalDataManager::INVALID_DATA_ID;
        }
        continue;
      }
      RendInstUpdateTm &destrPos = updateList.push_back();
      destrPos.physIdx = i;
      v_mat_43cu_from_mat44(destrPos.tm.array, m44);
      destrPos.moved = physObject;
      destrPos.forceUpdateDestructionData = forceUpdateDestructionData;
    }
    if (!updateList.empty())
    {
      TIME_PROFILE(rendinstDestrTmUpdate);

      vec3f frustumPoints[8];
      frustum->generateAllPointFrustm(frustumPoints);
      Point3 nearPlaneCenter = (as_point3(&frustumPoints[2]) + as_point3(&frustumPoints[4])) * 0.5f;

      int currentDestroyedTreeCount = 0;
      rendinst::ScopedRIExtraWriteLock wr;
      for (int i = 0; i < updateList.size(); i++)
      {
        // Iterate backwards so that if no more items fit in the render data buffer,
        // latest items take precedence.
        RendInstUpdateTm &l = updateList[updateList.size() - i - 1];
        mat44f m44;
        mat43f m43;
        v_mat44_make_from_43cu_unsafe(m44, l.tm.array);
        v_mat44_transpose_to_mat43(m43, m44);

        RendInstPhys &phys = riPhys[l.physIdx];

        const mat43f &prevTm43 = rendinst::getRIGenExtra43(phys.ri->riHandle);
        mat44f prevTm44;
        v_mat43_transpose_to_mat44(prevTm44, prevTm43);
        TMatrix prevTm;
        v_mat_43cu_from_mat44(prevTm.array, prevTm44);
        rendinst::RiExtraPerInstanceGpuItem prevTmRenderData[4];

        prevTmRenderData[0] = {bitwise_cast<uint32_t>(prevTm.m[0][0]), bitwise_cast<uint32_t>(prevTm.m[0][1]),
          bitwise_cast<uint32_t>(prevTm.m[0][2]), bitwise_cast<uint32_t>(0.0f)};
        prevTmRenderData[1] = {bitwise_cast<uint32_t>(prevTm.m[1][0]), bitwise_cast<uint32_t>(prevTm.m[1][1]),
          bitwise_cast<uint32_t>(prevTm.m[1][2]), bitwise_cast<uint32_t>(0.0f)};
        prevTmRenderData[2] = {bitwise_cast<uint32_t>(prevTm.m[2][0]), bitwise_cast<uint32_t>(prevTm.m[2][1]),
          bitwise_cast<uint32_t>(prevTm.m[2][2]), bitwise_cast<uint32_t>(0.0f)};
        prevTmRenderData[3] = {bitwise_cast<uint32_t>(prevTm.m[3][0]), bitwise_cast<uint32_t>(prevTm.m[3][1]),
          bitwise_cast<uint32_t>(prevTm.m[3][2]), bitwise_cast<uint32_t>(1.0f)};

        rendinst::setRiExtraPerInstanceRenderAdditionalData(phys.ri->riHandle, rendinst::RiExtraPerInstanceDataType::PREV_TM_DATA,
          rendinst::RiExtraPerInstanceDataPersistence::SINGLE_FRAME, prevTmRenderData, 4);

        if (phys.treeInstData.branchDestr.enableBranchDestruction &&
            phys.treeInstData.originalTmDataId == rendinst::RiExtraAdditionalDataManager::INVALID_DATA_ID)
        {
          TMatrix &tmOriginal = phys.originalTm;
          rendinst::RiExtraPerInstanceGpuItem originalTmRenderData[3];
          originalTmRenderData[0] = {bitwise_cast<uint32_t>(tmOriginal.m[0][0]), bitwise_cast<uint32_t>(tmOriginal.m[0][1]),
            bitwise_cast<uint32_t>(tmOriginal.m[0][2]), bitwise_cast<uint32_t>(0.0f)};
          originalTmRenderData[1] = {bitwise_cast<uint32_t>(tmOriginal.m[1][0]), bitwise_cast<uint32_t>(tmOriginal.m[1][1]),
            bitwise_cast<uint32_t>(tmOriginal.m[1][2]), bitwise_cast<uint32_t>(0.0f)};
          originalTmRenderData[2] = {bitwise_cast<uint32_t>(tmOriginal.m[2][0]), bitwise_cast<uint32_t>(tmOriginal.m[2][1]),
            bitwise_cast<uint32_t>(tmOriginal.m[2][2]), bitwise_cast<uint32_t>(0.0f)};
          phys.treeInstData.originalTmDataId = rendinst::setRiExtraPerInstanceRenderAdditionalData(phys.ri->riHandle,
            rendinst::RiExtraPerInstanceDataType::INITIAL_TM_3X3, rendinst::RiExtraPerInstanceDataPersistence::PERSISTENT,
            originalTmRenderData, 3);
        }

        const bool passedDistanceCheck =
          (phys.originalTm.col[3] - nearPlaneCenter).lengthSq() < sqr(phys.treeInstData.branchDestr.maxVisibleDistance);
        const bool destructionAllowed = currentDestroyedTreeCount < maxDestroyedTreeCount && passedDistanceCheck;

        uint64_t destrDataId = phys.treeInstData.destructionDataId;
        if (!destructionAllowed)
          destrDataId = rendinst::RiExtraAdditionalDataManager::INVALID_DATA_ID;
        else if (phys.treeInstData.destructionDataId == rendinst::RiExtraAdditionalDataManager::INVALID_DATA_ID ||
                 useDebugTreeInstData || l.forceUpdateDestructionData)
        {
          destrDataId = rendinst::updateTreeDestrRenderData(phys.ri->riHandle, phys.treeInstData,
            useDebugTreeInstData ? &debugTreeInstData : nullptr);
        }

        if (destrDataId == rendinst::RiExtraAdditionalDataManager::INVALID_DATA_ID &&
            phys.treeInstData.destructionDataId != rendinst::RiExtraAdditionalDataManager::INVALID_DATA_ID)
        {
          rendinst::removeRiExtraPerInstanceRenderAdditionalData(phys.treeInstData.destructionDataId,
            rendinst::RiExtraPerInstanceDataType::DESTR_RENDER_DATA);
          phys.treeInstData.destructionDataId = rendinst::RiExtraAdditionalDataManager::INVALID_DATA_ID;
        }
        else
        {
          G_ASSERT(phys.treeInstData.destructionDataId == rendinst::RiExtraAdditionalDataManager::INVALID_DATA_ID ||
                   phys.treeInstData.destructionDataId == destrDataId);
          phys.treeInstData.destructionDataId = destrDataId;
        }

        bool hideInstance = false;

        if (phys.treeInstData.destructionDataId != rendinst::RiExtraAdditionalDataManager::INVALID_DATA_ID)
          currentDestroyedTreeCount++;
        else if (phys.treeInstData.cutType == rendinst::TreeInstData::CutType::Bottom)
          hideInstance = true;

        if (hideInstance)
        {
          m43.row0 = v_perm_xyzd(v_zero(), m43.row0);
          m43.row1 = v_perm_xyzd(v_zero(), m43.row1);
          m43.row2 = v_perm_xyzd(v_zero(), m43.row2);
          rendinst::moveRIGenExtra43(phys.ri->riHandle, m43, l.moved, true);
        }
        else
        {
          rendinst::moveRIGenExtra43(phys.ri->riHandle, m43, l.moved, true);
        }
      }
    }
  }

  TIME_PROFILE_DEV(restorables_update);
  for (auto it = restorables.begin(); it != restorables.end();)
  {
    if (it->update(dt))
      ++it;
    else
    {
      bool restoreNeeded = it->isRestoreNeeded();
      rifx::composite::setEntityApproved(it->getRiDesc().getRiExtraHandle(), !restoreNeeded);
      if (restoreNeeded)
      {
        rendinst::riex_handle_t h = it->restore();
        rendinst::RendInstDesc desc = it->getRiDesc();
        if (h == rendinst::RIEX_HANDLE_NULL)
        {
          // Failed to recreate rendinst, consider it's gone for good.
          call_restorable_rendinst_cb(desc, rendinstdestr::RRS_DESTROYED);
        }
        else
        {
          // Rendinst recreated, store new handle in desc and pass to cb.
          desc.idx = rendinst::handle_to_ri_inst(h);
          G_ASSERT(rendinst::handle_to_ri_type(h) == desc.pool);
          rendinstdestr::RestorableRendinstState state = rendinstdestr::RRS_RESTORED; // Start with 'restored' state.
          for (rendinstdestr::restorable_rendinst_callback cb : restorable_rendinst_callbacks)
          {
            if (!cb(desc, state))
            {
              // Restoration canceled, report 'destroyed' to the rest.
              state = rendinstdestr::RRS_DESTROYED;
              desc.idx = 0;
            }
          }
        }
      }
      else
      {
        call_restorable_rendinst_cb(it->getRiDesc(), rendinstdestr::RRS_DESTROYED);
      }
      it = restorables.erase(it);
    }
  }
  simultaneous_destrs_count = rendinstdestr::get_destr_settings().riMaxSimultaneousDestrs;
}

static bool sort_by_offset(const rendinst::DestroyedInstanceRange &left, const rendinst::DestroyedInstanceRange &right)
{
  return left.startOffs < right.startOffs;
}

bool rendinstdestr::serialize_destr_data(danet::BitStream &bs)
{
  uint16_t cellCount = 0;
  const bool written = rendinst::getDestrCellData(0 /*primary layer*/, [&](const Tab<rendinst::DestroyedCellData> &destrCellData) {
    G_ASSERT(destrCellData.size() <= USHRT_MAX);
    cellCount = min<uint32_t>(destrCellData.size(), USHRT_MAX);

    bs.WriteCompressed(cellCount);
    for (int i = 0; i < cellCount; ++i)
    {
      const rendinst::DestroyedCellData &cell = destrCellData[i];
      G_ASSERT(cell.destroyedPoolInfo.size() <= USHRT_MAX);
      uint16_t poolCount = min<uint32_t>(cell.destroyedPoolInfo.size(), USHRT_MAX);

      bs.Write(int16_t(cell.cellId));
      bs.WriteCompressed(poolCount);
      uint8_t shift = cell.cellId < 0 ? 4 : 3; // is riExtra
      for (int j = 0; j < poolCount; ++j)
      {
        const rendinst::DestroyedPoolData &pool = cell.destroyedPoolInfo[j];
        G_ASSERT(pool.destroyedInstances.size() <= USHRT_MAX);
        G_ASSERT(eastl::is_sorted(pool.destroyedInstances.begin(), pool.destroyedInstances.end(), sort_by_offset));
        G_UNUSED(&sort_by_offset);
        uint16_t rangeCount = min<uint32_t>(pool.destroyedInstances.size(), USHRT_MAX);

        bs.WriteCompressed(uint16_t(pool.poolIdx));
        bs.WriteCompressed(rangeCount);
        uint32_t curRange = 0;
        for (int k = 0; k < rangeCount; ++k)
        {
          const rendinst::DestroyedInstanceRange &range = pool.destroyedInstances[k];
          G_ASSERTF(!(range.startOffs % (1u << shift)) && !(range.endOffs % (1u << shift)),
            "startOffs=0x%X endOffs=0x%X cellId=%d poolIdx=%d div=%d", range.startOffs, range.endOffs, cell.cellId, pool.poolIdx,
            1u << shift);
          uint32_t startOffsDiv = range.startOffs >> shift;
          uint32_t endOffsDiv = range.endOffs >> shift;
          G_ASSERT(startOffsDiv >= curRange);
          bs.WriteCompressed(startOffsDiv - curRange);
          bs.WriteCompressed(endOffsDiv - startOffsDiv);
          curRange = endOffsDiv;
        }
      }
    }

    return true;
  });

  // if nothing was written, just write 0 cell count
  if (!written)
    bs.WriteCompressed(cellCount);
  return cellCount > 0;
}

void rendinstdestr::serialize_destr_update(danet::BitStream &bs, const Point3 *camera_pos, float camera_rad_sq,
  dag::ConstSpan<DestrUpdateDesc> update_data, bool send_by_default, int max_impulses)
{
  G_ASSERT(!update_data.empty());
#ifdef _DEBUG_TAB
  G_ASSERT(eastl::is_sorted(update_data.begin(), update_data.end(), DestrUpdateDesc::lessVerify));
#endif
  struct RangeCounter
  {
    int idx;
    int count;
  };
  dag::RelocatableFixedVector<RangeCounter, 16, true /*overflow*/, framemem_allocator> cellRanges;
  dag::RelocatableFixedVector<RangeCounter, 128, true /*overflow*/, framemem_allocator> poolRanges;
  eastl::bitset<256, uint32_t> haveImpulseMask{};
  eastl::array<uint32_t, 256> impulseScore;
  int impulsesCount = 0;
  if (camera_pos || send_by_default)
  {
    for (int i = 0; i < update_data.size(); i++)
    {
      const DestrUpdateDesc &desc = update_data[i];
      if (desc.serializedImpulse == 0) // TODO: add check for ri without destrs
        continue;
      vec4f distSq = v_zero();
      uint32_t score = 0;
      if (camera_pos)
      {
        distSq = v_length3_sq_x(v_sub(v_ldu(&desc.riPos.x), v_ldu(&camera_pos->x)));
        score = v_extract_xi(v_cast_vec4i(v_rcp_unprecise_x(distSq)));
      }
      G_ASSERT(camera_rad_sq >= 0.f);
      if (v_extract_x(distSq) <= camera_rad_sq)
      {
        haveImpulseMask[i] = true;
        impulseScore[i] = (score & 0xFFFFFF00) | uint8_t(i);
        impulsesCount++;
      }
    }
  }
  if (impulsesCount > max_impulses)
  {
    if (!camera_pos)
    {
      for (int i = 0; i < update_data.size(); i++)
      {
        if (!haveImpulseMask[i])
          continue;
        uint32_t score = sqr(update_data[i].serializedImpulse.getPackedX()) + sqr(update_data[i].serializedImpulse.getPackedY()) +
                         sqr(update_data[i].serializedImpulse.getPackedZ());
        impulseScore[i] = (score << 8) | uint8_t(i);
      }
    }
    stlsort::sort_branchless(impulseScore.begin(), impulseScore.begin() + update_data.size());
    for (int i = 0; i < impulsesCount - max_impulses; i++)
    {
      int idx = uint8_t(impulseScore[i]);
      haveImpulseMask[idx] = false;
    }
    impulsesCount = max_impulses;
  }
  G_ASSERT(update_data.size() <= UCHAR_MAX);
  bs.Write(uint8_t(update_data.size()));
  bs.WriteArray((uint8_t *)haveImpulseMask.data(), (update_data.size() + 7) / CHAR_BIT);

  cellRanges.emplace_back(RangeCounter{update_data[0].cellIdx, 1});
  for (int i = 1; i < update_data.size(); i++)
  {
    if (update_data[i].cellIdx != update_data[i - 1].cellIdx)
      cellRanges.emplace_back(RangeCounter{update_data[i].cellIdx, 1});
    else
      cellRanges.back().count++;
  }
  G_ASSERT(cellRanges.size() <= USHRT_MAX);
  bs.WriteCompressed(uint16_t(cellRanges.size()));
  int cellOffset = 0;
  for (RangeCounter cellRange : cellRanges)
  {
    poolRanges.clear();
    poolRanges.emplace_back(RangeCounter{update_data[cellOffset].poolIdx, 1});
    for (int i = cellOffset + 1; i < cellOffset + cellRange.count; i++)
    {
      if (update_data[i].poolIdx != update_data[i - 1].poolIdx)
        poolRanges.emplace_back(RangeCounter{update_data[i].poolIdx, 1});
      else
        poolRanges.back().count++;
    }
    bs.Write(int16_t(cellRange.idx));
    bs.WriteCompressed(uint16_t(poolRanges.size()));
    // logerr("Write cell %i destrs %i", int16_t(cellRange.idx), uint16_t(poolRanges.size()));
    uint8_t shift = cellRange.idx < 0 ? 4 : 3; // is riExtra
    int poolOffset = cellOffset;
    uint32_t prevPoolIdx = 0;
    for (RangeCounter poolRange : poolRanges)
    {
      dag::ConstSpan<DestrUpdateDesc> destrRange(&update_data[poolOffset], poolRange.count);
      G_ASSERT(poolRange.idx > prevPoolIdx || !prevPoolIdx);
      bs.WriteCompressed(uint16_t(poolRange.idx - prevPoolIdx));
      prevPoolIdx = poolRange.idx;
      bs.WriteCompressed(uint16_t(destrRange.size()));
      // logerr("Write pool %i destrs %i", uint16_t(poolRange.idx), uint16_t(destrRange.size()));
      uint32_t prevOffsetDiv = 0;
      for (int j = 0; j < destrRange.size(); j++)
      {
        const DestrUpdateDesc &dd = destrRange[j];
        uint32_t startOffsDiv = dd.offs >> shift;
        G_ASSERT((dd.offs & (shift - 1)) == 0);
        G_ASSERT(startOffsDiv > prevOffsetDiv || !prevOffsetDiv);
        bs.WriteCompressed(startOffsDiv - prevOffsetDiv);
        prevOffsetDiv = startOffsDiv;
        bool writeImpulse = haveImpulseMask[poolOffset + j]; // index in update
        if (writeImpulse)
        {
          bs.WriteBits((uint8_t *)&dd.serializedLocalPos, rendinstdestr::QuantizedDestrImpulsePos::XYZBits);
          bs.WriteBits((uint8_t *)&dd.serializedImpulse, rendinstdestr::QuantizedDestrImpulseVec::XYZBits);
        }
        // logerr("Write destr %i %u %u imp %i", int16_t(cellRange.idx), uint16_t(poolRange.idx), dd.offs, writeImpulse);
      }
      poolOffset += poolRange.count;
    }
    cellOffset += cellRange.count;
  }
  // logerr("Serialized %i destrs, %i bytes", update_data.size(), bs.GetNumberOfBytesUsed());
}

using TempDelayedDestructionList = dag::RelocatableFixedVector<DelayedRendInstNetDestr, 32, true /*overflow*/, framemem_allocator>;

static void flush_temp_delayed_destruction_list(TempDelayedDestructionList &&list)
{
  if (list.empty())
    return;
  WinAutoLock lock(g_delayed_net_destr_list.mutex);
  auto &allDestrList = g_delayed_net_destr_list.list;
  for (const auto &desc : list)
  {
    if (eastl::find(allDestrList.begin(), allDestrList.end(), desc) == allDestrList.end())
    {
      allDestrList.push_back(desc);
      VERBOSE_SYNC("%s, delaying net destr, desc:" FMT_DESC_STR,
        desc.isPoolSyncPending ? "pool is not synced" : "could not find rendinst", FMT_DESC_V(desc.desc));
    }
  }
  list.clear();
  g_delayed_net_destr_list.isForcedUpdatePending = true;
}

template <bool IsReadLocked>
static bool destroy_rend_inst_from_net(rendinst::RendInstDesc &restorable_desc, bool has_impulse, const Point3 &local_impulse_pos,
  const Point3 &impulse_vec, bool create_destr) DAG_TS_NO_THREAD_SAFETY_ANALYSIS // too hard to analyze IsReadLocked flag
{
  for (RestorableRendinst &restr : restorables)
  {
    const rendinst::RendInstDesc &restDesc = restr.getRiDesc();
    if (restorable_desc.offs == restDesc.offs && restorable_desc.pool == restDesc.pool && restorable_desc.cellIdx == restDesc.cellIdx)
      restr.confirmDestruction();
    // TODO: don't destroy if we found restorable?
  }

  if (!isValidRILayerAndPool(restorable_desc))
  {
    logerr("deserialized invalid rendinst destruction: layer=%i, pool=%i cellIdx=%i", restorable_desc.layer, restorable_desc.pool,
      restorable_desc.cellIdx);
    return true; // invalid - count as destroyed
  }

  bool ok = true;
  G_ASSERT(restorable_desc.idx == 0);
  if (restorable_desc.isRiExtra())
  {
    if constexpr (!IsReadLocked)
      rendinst::riex_lock_read();
    ok = false;
    const int idx = rendinst::find_restorable_data_index(restorable_desc);
    if (idx >= 0)
    {
      restorable_desc.idx = idx;
      ok = true;
    }
    if constexpr (!IsReadLocked)
      rendinst::riex_unlock_read();
  }
  else
  {
    ok = rendinst::resolve_rigen_desc_subcell(restorable_desc);
  }
  if (ok)
  {
    if constexpr (IsReadLocked)
      rendinst::riex_unlock_read();
    if (rendinst::isRIGenPosInst(restorable_desc))
    {
      int poolIdxBasedSeed = restorable_desc.offs + (restorable_desc.pool) ^ (restorable_desc.cellIdx < 16);
      float angle = _srnd(poolIdxBasedSeed) * PI;
      VERBOSE_SYNC("destroying rendinst from net (tree), desc:" FMT_DESC_STR, FMT_DESC_V(restorable_desc));
      float s, c;
      sincos(angle, s, c);
      rendinstdestr::create_tree_rend_inst_destr(restorable_desc, false, local_impulse_pos, Point3(c, 0.f, s), true, true, 0.5f, 0.f,
        NULL, create_destr, false);
    }
    else
    {
      // Only call destroyRendinst if a handle (i.e. idx exists) is found, if it's not found,
      // then desc.idx will be 0 and will trigger all kinds of side effects with make_handle(pool, 0).
      // For the same reason we should NEVER call destroyRendinst without desc.idx being properly set,
      // because e.g. rendinst::doRIGenDestrEx will fail to delete RI due to this:
      //   uniqueData = riExtra[desc.pool].riUniqueData.at(desc.idx);
      //   if (riExtra[desc.pool].isDynamicRendinst || (uniqueData && uniqueData->cellId < 0))
      //     return NULL;
      Point3_vec4 impulsePos = Point3::ZERO;
      if (has_impulse)
      {
        mat44f riTm;
        rendinst::getRIGenExtra44(restorable_desc.getRiExtraHandle(), riTm);
        v_st(&impulsePos.x, v_mat44_mul_vec3p(riTm, v_ldu(&local_impulse_pos.x)));
      }
      VERBOSE_SYNC("destroying rendinst from net, desc:" FMT_DESC_STR, FMT_DESC_V(restorable_desc));
      if (g_deferred_ri_destr_list.isEnabled && false)
      {
        auto &data = g_deferred_ri_destr_list.list.push_back();
        data.riDesc = restorable_desc;
        data.pos = impulsePos;
        data.impulse = impulse_vec;
        data.atTime = 0.f;
        data.createDestr = create_destr;
        data.flags = rendinst::DestrOptionFlag::AddDestroyedRi | rendinst::DestrOptionFlag::ForceDestroy;
      }
      else
      {
        rendinstdestr::destroyRendinst(restorable_desc, /*add_restorable*/ false, impulsePos, impulse_vec, /*at_time*/ 0.f,
          /*coll_info*/ nullptr, create_destr);
      }
    }
    if constexpr (IsReadLocked)
      rendinst::riex_lock_read();
  }
  return ok;
}

template <typename F>
static void deserialize_destr_update_impl(const danet::BitStream &bs, F &&cb)
{
  uint8_t readCount = 0;
  eastl::bitset<256, uint32_t> haveImpulseMask;

  uint8_t updateSize = 0;
  bs.Read(updateSize);
  bs.ReadArray((uint8_t *)haveImpulseMask.data(), (updateSize + 7) / CHAR_BIT);
  uint16_t cellCount = 0;
  bs.ReadCompressed(cellCount);
  for (int i = 0; i < cellCount; i++)
  {
    int16_t cellIdx = 0;
    uint16_t poolCount = 0;
    bs.Read(cellIdx);
    bs.ReadCompressed(poolCount);
    // logerr("Read cell %i destrs %i", cellId, poolCount);
    uint8_t shift = cellIdx < 0 ? 4 : 3; // is riExtra
    uint32_t srvPoolIdx = 0;
    for (int j = 0; j < poolCount; j++)
    {
      uint16_t poolIdxDiff = 0;
      uint16_t destrCount = 0;
      bs.ReadCompressed(poolIdxDiff);
      srvPoolIdx += poolIdxDiff;
      bs.ReadCompressed(destrCount);

      const bool isPoolSyncPending = cellIdx < 0 && riexsync::is_server_ri_pool_sync_pending(int(srvPoolIdx));
      const uint16_t poolIdx =
        cellIdx < 0 && !isPoolSyncPending ? uint16_t(riexsync::get_client_ri_pool_id(int(srvPoolIdx))) : uint16_t(srvPoolIdx);

      // logerr("Read pool %i destrs %i", poolIdx, destrCount);
      uint32_t offs = 0;
      for (int k = 0; k < destrCount; k++)
      {
        uint32_t offsDiffDivided = 0;
        G_VERIFY(bs.ReadCompressed(offsDiffDivided));
        offs += offsDiffDivided << shift;

        rendinstdestr::DestrUpdateDesc dd;
        dd.poolIdx = poolIdx;
        dd.cellIdx = cellIdx;
        dd.offs = offs;
        bool haveImpulse = haveImpulseMask[readCount];
        if (haveImpulse)
        {
          bs.ReadBits((uint8_t *)&dd.serializedLocalPos.qpos, rendinstdestr::QuantizedDestrImpulsePos::XYZBits);
          bs.ReadBits((uint8_t *)&dd.serializedImpulse.qpos, rendinstdestr::QuantizedDestrImpulseVec::XYZBits);
        }
        // logerr("Read destr %i %u %u imp %i", int16_t(cellIdx), uint16_t(poolIdx), offs, haveImpulse);
        rendinst::RendInstDesc desc(cellIdx, 0 /*instIdx*/, poolIdx, offs, 0 /*layer*/);
        cb(desc, dd, isPoolSyncPending, haveImpulse);
        readCount++;
      }
    }
  }

  // logerr("Deserialized %i destrs, %i bytes", readCount, bs.GetNumberOfBytesUsed());
  G_ASSERT(readCount == updateSize);
}


bool rendinstdestr::deserialize_destr_update(const danet::BitStream &bs)
{
  if (!apply_reflection)
  {
    pending_destrs_updates.Write(bs);
    return false;
  }
  VERBOSE_SYNC("deserializing destruction update");

  bool shouldUpdateVb = false;
  int32_t prevCellIdx = -1;
  TempDelayedDestructionList delayedRiExtraDestruction;
  deserialize_destr_update_impl(bs,
    [&](rendinst::RendInstDesc desc, const rendinstdestr::DestrUpdateDesc &dd, bool isPoolSyncPending, bool haveImpulse) {
      if (prevCellIdx != desc.cellIdx)
      {
        if (shouldUpdateVb && prevCellIdx >= 0)
          rendinst::updateRiGenVbCell(0, prevCellIdx);
        shouldUpdateVb = false;
        prevCellIdx = desc.cellIdx;
      }

      const uint32_t maxValue = (1 << rendinstdestr::QuantizedDestrImpulseVec::XYZBits) - 1;
      bool createDestr = true;
      Point3 localImpulsePos = Point3::ZERO, impulseVec = Point3::ZERO;
      if (haveImpulse)
      {
        if ((dd.serializedLocalPos.qpos & maxValue) == maxValue && (dd.serializedImpulse.qpos & maxValue) == maxValue)
          createDestr = false;
        else
        {
          localImpulsePos = dd.serializedLocalPos.unpackPos();
          impulseVec = dd.serializedImpulse.unpackPos();
        }
      }
      if ((isPoolSyncPending || !destroy_rend_inst_from_net<false>(desc, haveImpulse, localImpulsePos, impulseVec,
                                  createDestr && --simultaneous_destrs_count >= 0)) &&
          g_delayed_net_destr_list.isEnabled)
        delayedRiExtraDestruction.emplace_back(desc, isPoolSyncPending);
      shouldUpdateVb = true;
    });
  if (shouldUpdateVb && prevCellIdx >= 0)
    rendinst::updateRiGenVbCell(0, prevCellIdx);
  flush_temp_delayed_destruction_list(eastl::move(delayedRiExtraDestruction));
  return true;
}


bool rendinstdestr::deserialize_destr_data(const danet::BitStream &bs, int apply_flags, int max_simultaneous_destrs)
{
  // Temporary structure to keep it
  Tab<rendinst::DestroyedCellData> cellsNewDestrInfo(framemem_ptr());
  int updateSize = 0;

  // always read cell count
  uint16_t cellCount = 0;
  if (!apply_reflection)
  {
    pending_initial_destrs = bs;
    bs.ReadCompressed(cellCount);
    for (int i = 0; i < cellCount; ++i)
    {
      uint16_t poolCount = 0;

      bs.IgnoreBytes(sizeof(int16_t));
      bs.ReadCompressed(poolCount);
      for (int j = 0; j < poolCount; ++j)
      {
        uint16_t rangeCount = 0;
        uint16_t poolIdx = 0;

        bs.ReadCompressed(poolIdx);
        bs.ReadCompressed(rangeCount);

        for (int k = 0; k < rangeCount; k++)
        {
          uint32_t startOffs, endOffs;
          bs.ReadCompressed(startOffs);
          bs.ReadCompressed(endOffs);
        }
      }
    }
    return false;
  }
  bs.ReadCompressed(cellCount);

  VERBOSE_SYNC("deserializing destruction initial snapshot");
  constexpr uint16_t POOL_SYNC_PENDING_BIT = 0x8000u;
  rendinst::getDestrCellData(0 /*primary layer*/, [&](const Tab<rendinst::DestroyedCellData> &destrCellData) {
    cellsNewDestrInfo.resize(cellCount);

    for (int i = 0; i < cellCount; ++i)
    {
      int16_t cellId = 0;
      uint16_t poolCount = 0;

      bs.Read(cellId);
      bs.ReadCompressed(poolCount);

      // Search for cell
      const rendinst::DestroyedCellData *cell = NULL;
      for (int si = 0; si < destrCellData.size(); ++si)
      {
        if (destrCellData[si].cellId != cellId)
          continue;
        // Found this cell in our local list
        cell = &destrCellData[si];
        break;
      }
      cellsNewDestrInfo[i].cellId = cellId;
      dag::set_allocator(cellsNewDestrInfo[i].destroyedPoolInfo, framemem_ptr());
      cellsNewDestrInfo[i].destroyedPoolInfo.resize(poolCount);
      uint8_t shift = cellId < 0 ? 4 : 3; // is riExtra
      for (int j = 0; j < poolCount; ++j)
      {
        uint16_t srvPoolIdx = 0;
        uint16_t rangeCount = 0;

        bs.ReadCompressed(srvPoolIdx);
        bs.ReadCompressed(rangeCount);

        const bool isPoolSyncPending = cellId < 0 && riexsync::is_server_ri_pool_sync_pending(int(srvPoolIdx));
        uint16_t poolIdx =
          cellId < 0 && !isPoolSyncPending ? uint16_t(riexsync::get_client_ri_pool_id(int(srvPoolIdx))) : uint16_t(srvPoolIdx);
        G_ASSERT(!bool(poolIdx & POOL_SYNC_PENDING_BIT));
        if (isPoolSyncPending)
          poolIdx |= POOL_SYNC_PENDING_BIT;
        cellsNewDestrInfo[i].destroyedPoolInfo[j].poolIdx = poolIdx;
        dag::set_allocator(cellsNewDestrInfo[i].destroyedPoolInfo[j].destroyedInstances, framemem_ptr());

        const rendinst::DestroyedPoolData *pool = NULL;
        if (cell && !isPoolSyncPending)
        {
          // If we have this cell - search for pool in it
          for (int sj = 0; sj < cell->destroyedPoolInfo.size(); ++sj)
          {
            if (cell->destroyedPoolInfo[sj].poolIdx != poolIdx)
              continue;
            // Ok, pool found. Now we need to match ranges.
            pool = &cell->destroyedPoolInfo[sj];
            break;
          }
        }
        int lastIdx = 0;
        int stride = rendinst::getRIGenStride(0, cellId, poolIdx);
        unsigned curRange = 0;
        for (int k = 0; k < rangeCount; ++k)
        {
          unsigned start = 0;
          unsigned end = 0;

          bs.ReadCompressed(start);
          bs.ReadCompressed(end);
          start += curRange;
          end += start;
          curRange = end;

          int startOffs = start << shift;
          int endOffs = end << shift;

          if (pool)
          {
            rendinst::DestroyedInstanceRange *addedRange = NULL;
            bool finished = false;
            for (; lastIdx < pool->destroyedInstances.size();)
            {
              const rendinst::DestroyedInstanceRange &range = pool->destroyedInstances[lastIdx];
              if (addedRange)
              {
                if (endOffs > range.endOffs)
                {
                  // Finish prev one.
                  addedRange->endOffs = range.startOffs;
                  updateSize += (addedRange->endOffs - addedRange->startOffs) / stride;

                  // Start new one
                  addedRange = &cellsNewDestrInfo[i].destroyedPoolInfo[j].destroyedInstances.push_back();
                  addedRange->startOffs = range.endOffs;
                  addedRange->endOffs = endOffs;
                  updateSize += (addedRange->endOffs - addedRange->startOffs) / stride;
                  lastIdx++; // continue to next one
                }
                else if (endOffs >= range.startOffs)
                {
                  // Just finish prev one and break;
                  addedRange->endOffs = range.startOffs;
                  updateSize += (addedRange->endOffs - addedRange->startOffs) / stride;
                  addedRange = NULL;
                  finished = true;
                  break;
                }
                else
                {
                  // It's before start, so just finish it and break;
                  addedRange->endOffs = endOffs;
                  updateSize += (addedRange->endOffs - addedRange->startOffs) / stride;
                  addedRange = NULL;
                  finished = true;
                  break;
                }
              }
              else
              {
                if (startOffs < range.endOffs && startOffs >= range.startOffs)
                {
                  // It's starts inside range, so we need to place start marker at end of this range.
                  if (endOffs > range.endOffs)
                  {
                    addedRange = &cellsNewDestrInfo[i].destroyedPoolInfo[j].destroyedInstances.push_back();
                    addedRange->startOffs = range.endOffs;
                    addedRange->endOffs = endOffs;
                    updateSize += (addedRange->endOffs - addedRange->startOffs) / stride;
                    lastIdx++;
                  }
                  else
                  {
                    finished = true;
                    break; // it's consumed by it
                  }
                }
                else if (startOffs < range.startOffs)
                {
                  // It starts before range (as it's sorted, so it should be good)
                  addedRange = &cellsNewDestrInfo[i].destroyedPoolInfo[j].destroyedInstances.push_back();
                  addedRange->startOffs = startOffs;
                  addedRange->endOffs = startOffs;
                  // Do not skip to next one, this one should be finished first.
                }
                else
                  lastIdx++;
              }
            }
            if (!finished && !addedRange)
            {
              rendinst::DestroyedInstanceRange &range = cellsNewDestrInfo[i].destroyedPoolInfo[j].destroyedInstances.push_back();
              range.startOffs = startOffs;
              range.endOffs = endOffs;
              updateSize += (endOffs - startOffs) / stride;
            }
            // Great that also means that there's point to search for restorables right here, to confirm it and delete it from tab
            for (auto &restr : restorables)
            {
              const rendinst::RendInstDesc &desc = restr.getRiDesc();
              if (desc.cellIdx != cellId || desc.pool != poolIdx || desc.offs < startOffs || desc.offs >= endOffs)
                continue;
              restr.confirmDestruction();
            }
          }
          else
          {
            // if no pool found previously - just simply add everything.
            rendinst::DestroyedInstanceRange &range = cellsNewDestrInfo[i].destroyedPoolInfo[j].destroyedInstances.push_back();
            range.startOffs = startOffs;
            range.endOffs = endOffs;
            updateSize += (endOffs - startOffs) / stride;
          }
        }
      }
    }
    return true;
  });

  int newDestrs = ((apply_flags & INITIAL_REPLICATION) != 0) ? 0 : min(updateSize, max_simultaneous_destrs);
  TempDelayedDestructionList delayedRiExtraDestruction;

  for (int i = 0; i < cellsNewDestrInfo.size(); ++i)
  {
    const rendinst::DestroyedCellData &cell = cellsNewDestrInfo[i];
    bool shouldUpdateVb = false;
    for (int j = 0; j < cell.destroyedPoolInfo.size(); ++j)
    {
      const rendinst::DestroyedPoolData &pool = cell.destroyedPoolInfo[j];
      const uint16_t poolIdx = pool.poolIdx & ~POOL_SYNC_PENDING_BIT;
      const bool isPoolSyncPending = bool(pool.poolIdx & POOL_SYNC_PENDING_BIT);
      int stride = rendinst::getRIGenStride(0, cell.cellId, poolIdx);
      for (int k = 0; k < pool.destroyedInstances.size(); ++k)
      {
        const rendinst::DestroyedInstanceRange &range = pool.destroyedInstances[k];
        for (int offs = range.startOffs; offs < range.endOffs; offs += stride)
        {
          rendinst::RendInstDesc desc(cell.cellId, 0, poolIdx, offs, 0);
          if ((isPoolSyncPending || !destroy_rend_inst_from_net<false>(desc, false, Point3::ZERO, Point3::ZERO,
                                      /* create_destr */ --newDestrs >= 0)) &&
              g_delayed_net_destr_list.isEnabled)
            delayedRiExtraDestruction.emplace_back(desc, isPoolSyncPending);
          shouldUpdateVb = true;
        }
      }
    }
    if (shouldUpdateVb && cell.cellId >= 0)
      rendinst::updateRiGenVbCell(0, cell.cellId);
  }

  debug("Finished ridestr snapshot deserialize. updateSize=%i delayedRiExtraDestruction=%i", updateSize,
    delayedRiExtraDestruction.size());
  flush_temp_delayed_destruction_list(eastl::move(delayedRiExtraDestruction));
  return true;
}


bool rendinstdestr::serialize_client_destr_requests(danet::BitStream &bs, float ttl_min, float ttl_max)
{
  FRAMEMEM_REGION;
  FRAMEMEM_VALIDATE;
  bool anyAtMinTtl = false;
  for (RestorableRendinst &restorable : restorables)
  {
    if (restorable.isRequestSentToServer || !restorable.isRestoreNeeded() || restorable.getTtl() > ttl_min)
      continue;
    anyAtMinTtl = true;
    break;
  }
  if (!anyAtMinTtl)
    return false;
  dag::RelocatableFixedVector<DestrUpdateDesc, 64, true, framemem_allocator> riDestrData;
  for (RestorableRendinst &restorable : restorables)
  {
    if (restorable.isRequestSentToServer || !restorable.isRestoreNeeded() || restorable.getTtl() > ttl_max)
      continue;
    const rendinst::RendInstDesc &desc = restorable.getRiDesc();
    if (!rendinstdestr::can_sync_ri_destr(desc))
      continue;
    const int serverPoolId = riexsync::get_server_ri_pool_id(desc.pool, -1);
    if (serverPoolId == -1)
      continue;
    DestrUpdateDesc &rec = riDestrData.push_back();
    rec.cellIdx = int16_t(desc.cellIdx);
    rec.poolIdx = uint16_t(serverPoolId);
    rec.offs = desc.offs;
    rec.serializedLocalPos = restorable.serializedLocalPos;
    rec.serializedImpulse = restorable.serializedImpulse;
    restorable.isRequestSentToServer = true;
    G_ASSERT(rec.cellIdx == desc.cellIdx && rec.poolIdx == serverPoolId);
    VERBOSE_SYNC("requesting confirmation from server [pool:%i(%i) cell:%i offs:%i]", desc.pool, serverPoolId, rec.cellIdx, rec.offs);
    // deduplicate
    if (eastl::find(riDestrData.begin(), riDestrData.end() - 1, riDestrData.back()) != riDestrData.end() - 1)
      riDestrData.pop_back();
    if (riDestrData.size() == 255)
      break;
  }
  if (riDestrData.empty())
    return false;
  stlsort::sort_branchless(riDestrData.begin(), riDestrData.end());
  // we don't care about camera pos, as these are client updates
  serialize_destr_update(bs, nullptr, FLT_MAX, make_span_const(riDestrData), true, 255);
  return true;
}


void rendinstdestr::deserialize_client_destr_requests(const danet::BitStream &bs,
  dag::FixedMoveOnlyFunction<32, void(const rendinst::RendInstDesc &, bool, const Point3 &, const Point3 &)> cb)
{
  deserialize_destr_update_impl(bs,
    [&](rendinst::RendInstDesc desc, const rendinstdestr::DestrUpdateDesc &dd, bool isPoolSyncPending, bool has_impulse) {
      G_VERIFY(!isPoolSyncPending); // it is only called on server
      G_UNUSED(desc);

      bool exists = true;
      if (!rendinst::isValidRILayerAndPool(desc))
        exists = false;
      else if (desc.isRiExtra())
      {
        desc.idx = rendinst::find_restorable_data_index(desc);
        exists = desc.idx >= 0;
      }
      else
      {
        exists = rendinst::resolve_rigen_desc_subcell(desc);
      }
      exists = exists && rendinst::isRiGenInWorld(desc);
      VERBOSE_SYNC("deserializing client destr request desc:" FMT_DESC_STR " exists:%i", FMT_DESC_V(desc), exists);
      constexpr uint32_t maxValue = (1 << rendinstdestr::QuantizedDestrImpulseVec::XYZBits) - 1;
      if ((dd.serializedLocalPos.qpos & maxValue) == maxValue && (dd.serializedImpulse.qpos & maxValue) == maxValue)
        has_impulse = false;
      Point3 localPos = Point3::ZERO, impulse = Point3::ZERO;
      if (has_impulse)
      {
        localPos = dd.serializedLocalPos.unpackPos();
        impulse = dd.serializedImpulse.unpackPos();
      }
      cb(desc, exists, localPos, impulse);
    });
}


// NOTE: Required RI extra to be locked for read!
static void do_delayed_ri_extra_destruction_impl() DAG_TS_REQUIRES_SHARED(rendinst::ccExtra)
{
  if (!g_delayed_net_destr_list.isEnabled)
    return;

  WinAutoLock lock(g_delayed_net_destr_list.mutex);
  if (g_delayed_net_destr_list.list.empty())
    return;
  TIME_PROFILE(do_delayed_ri_extra_destruction)

  int destrCnt = 0;
  for (auto it = g_delayed_net_destr_list.list.begin(); it != g_delayed_net_destr_list.list.end();)
  {
    DelayedRendInstNetDestr &destr = *it;

    // try sync pool, skip, if failed
    if (destr.isPoolSyncPending)
    {
      if (riexsync::is_server_ri_pool_sync_pending(destr.desc.pool))
      {
        ++it;
        continue;
      }
      const int prevPoolIdx = destr.desc.pool;
      G_UNUSED(prevPoolIdx);
      destr.desc.pool = riexsync::get_client_ri_pool_id(destr.desc.pool);
      VERBOSE_SYNC("synced pool for delayed net destr (%i->%i), desc:" FMT_DESC_STR, prevPoolIdx, int(destr.desc.pool),
        FMT_DESC_V(destr.desc));
      destr.isPoolSyncPending = false;
    }

    // try destroy, skip, if failed, otherwise erase
    if (destroy_rend_inst_from_net<true>(destr.desc, false, Point3::ZERO, Point3::ZERO, --simultaneous_destrs_count >= 0))
    {
      it = g_delayed_net_destr_list.list.erase_unsorted(it);
      destrCnt++;
    }
    else
      ++it;
  }
  const uint64_t version = rendinst::getRIExtraGlobalWorldVersion(true, false);
  if (destrCnt > 0)
    debug("delayed rend inst net destr: version:%llu->%llu destroyed:%i pending:%i", g_delayed_net_destr_list.lastUpdatedWorldVersion,
      version, destrCnt, int(g_delayed_net_destr_list.list.size()));

  {
    // NOTE: ccExtra locked for read
    g_delayed_net_destr_list.lastUpdatedWorldVersion = version;
    g_delayed_net_destr_list.isForcedUpdatePending = false;
  }
}


bool rendinstdestr::apply_damage_to_riextra(rendinst::riex_handle_t handle, float dmg, const Point3 &pos, const Point3 &impulse,
  float at_time)
{
  if (!rendinst::isRiGenExtraValid(handle))
    return false;
  bool isDestr = false;
  apply_damage_to_ri(rendinst::RendInstDesc(handle), dmg, 0.f, pos, impulse, at_time, true /*createDestr*/, 1.f, &isDestr);
  return isDestr;
}

void rendinstdestr::apply_damage_to_ri(const rendinst::RendInstDesc &desc, float dmg, float impulse_to_hp, const Point3 &pos,
  const Point3 &impulse, float at_time, bool create_destr_effects, float impulse_mult_for_child, bool *isDestroyed)
{
  rendinst::CollisionInfo collInfo(desc);
  collInfo = rendinst::getRiGenDestrInfo(desc);
  float hp = collInfo.hp > 0.f ? collInfo.hp : collInfo.destrImpulse * impulse_to_hp;
  if (desc.isRiExtra() && collInfo.hp > 0.f)
  {
    if (rendinst::applyDamageRIGenExtra(desc, dmg))
      dmg = hp + 1.f;
    else
    {
      rendinst::play_riextra_dmg_fx(desc.getRiExtraHandle(), pos, create_destr_effects ? ri_effect_cb : nullptr);
      dmg = hp - 1.f;
    }
  }

  if (isDestroyed)
    *isDestroyed = collInfo.initialHp > 0.f && dmg >= hp;
  if (collInfo.isDestr && dmg >= hp)
    rendinstdestr::destroyRendinst(desc, true, pos, impulse, at_time, &collInfo, create_destr_effects, NULL, -1,
      impulse_mult_for_child);
}

void rendinstdestr::damage_ri_in_sphere(const Point3 &pos, float rad, const Point2 &dmg_near_far, float impulse_to_hp, float at_time,
  bool create_destr_effects, on_riextra_destroyed_callback &&riex_destr_cb, riextra_should_damage &&should_damage)
{
  rendinst::GatherRiTypeFlags flags =
    impulse_to_hp > 0.f ? rendinst::GatherRiTypeFlag::RiGenAndExtra : rendinst::GatherRiTypeFlag::RiExtraOnly;
  rendinst::rigen_collidable_data_t collidableData;
  gatherRIGenCollidableInRadius(collidableData, pos, rad, flags, true /*ignore_immortal*/);
  stlsort::sort_branchless(collidableData.begin(), collidableData.end(),
    [](const rendinst::RiGenCollidableData &a, const rendinst::RiGenCollidableData &b) { return a.dist < b.dist; });

  for (rendinst::RiGenCollidableData &targetData : collidableData)
  {
    if (!targetData.desc.isValid()) // ignore already destroyed rendinsts (by parent destruction)
      continue;
    if (targetData.immortal)
      continue;
    if (targetData.desc.isRiExtra() && should_damage && !should_damage(targetData.desc.getRiExtraHandle()))
    {
      targetData.immortal = true;
      continue;
    }

    mat44f tm;
    v_mat44_make_from_43cu_unsafe(tm, targetData.tm.array);
    vec3f centerPos = v_mat44_mul_vec3p(tm, v_bbox3_center(v_ldu_bbox3(targetData.collres->boundingBox)));
    vec3f dir = v_sub(centerPos, v_ldu(&pos.x));
    vec3f dist = v_length3(dir);
    float t = v_extract_x(dist);
    dir = v_safediv(dir, dist);
    Point3_vec4 normDir;
    v_st(&normDir.x, dir);

    targetData.collres->traceRay(targetData.tm, pos, normDir, t);
    if (dacoll::rayhit_normalized(pos, normDir, t, dacoll::ETF_DEFAULT & ~(dacoll::ETF_RI | dacoll::ETF_RI_PHYS)))
      continue;

    auto found = eastl::find_if(collidableData.begin(), collidableData.end(), [&](rendinst::RiGenCollidableData &obstacleData) {
      return &obstacleData != &targetData && obstacleData.collres->rayHit(obstacleData.tm, nullptr, pos, normDir, t);
    });
    if (found != collidableData.end())
      continue;

    float dmg = cvt(t, 0.f, rad, dmg_near_far.x, dmg_near_far.y); // scale damage by distance
    if (targetData.desc.isRiExtra() && !rendinst::applyDamageRIGenExtra(targetData.desc, dmg))
      continue;
    rendinst::CollisionInfo collInfo = getRiGenDestrInfo(targetData.desc);
    if (!targetData.desc.isRiExtra() && dmg < collInfo.destrImpulse * impulse_to_hp)
      continue;

    if (!targetData.desc.isRiExtra() || collInfo.isDestr)
    {
      auto onDestroyCb = [&collidableData, riex_destr_cb](const rendinst::RendInstDesc &desc) {
        if (desc.isRiExtra())
        {
          rendinst::riex_handle_t id = rendinst::make_handle(desc.pool, desc.idx);
          riex_destr_cb(id);
        }
        for (rendinst::RiGenCollidableData &cdata : collidableData)
        {
          if (cdata.desc == desc)
          {
            cdata.desc.invalidate();
            break;
          }
        }
      };
      Point3 hitPos = pos + normDir * t;
      Point3 impulse = normDir * dmg;
      rendinstdestr::destroyRendinst(targetData.desc, true, hitPos, impulse, at_time, &collInfo, create_destr_effects, nullptr, -1,
        1.f, eastl::move(onDestroyCb));
    }
    else
      riex_destr_cb(targetData.desc.getRiExtraHandle());
  }
}

void rendinstdestr::set_on_rendinst_destroyed_cb(on_rendinst_destroyed_callback cb)
{
  auto &cblist = on_rendinst_destroyed_callbacks;
  if (cb && eastl::find(cblist.begin(), cblist.end(), cb) == cblist.end())
    cblist.push_back(cb);
}

void rendinstdestr::call_on_rendinst_destroyed_cb(rendinst::riex_handle_t handle, const TMatrix &tm, const BBox3 &box)
{
  for (on_rendinst_destroyed_callback cb : on_rendinst_destroyed_callbacks)
    cb(handle, tm, box);
}

CollisionObject &rendinstdestr::get_tree_collision() { return tree_collision; }

void rendinstdestr::clear_tree_collision()
{
  if (tree_collision)
    dacoll::destroy_dynamic_collision(tree_collision);
  tree_collision.clear_ptrs();
}

void rendinstdestr::set_ri_damage_effect_cb(rendinst::ri_damage_effect_cb cb) { ri_effect_cb = cb; }

void rendinstdestr::set_on_tree_destr_created_cb(on_tree_destr_created_callback cb) { on_tree_destr_created_cb = cb; }

void rendinstdestr::register_restorable_rendinst_cb(restorable_rendinst_callback cb)
{
  if (find_value_idx(restorable_rendinst_callbacks, cb) == -1)
    restorable_rendinst_callbacks.push_back(cb);
}

bool rendinstdestr::unregister_restorable_rendinst_cb(restorable_rendinst_callback cb)
{
  return erase_item_by_value(restorable_rendinst_callbacks, cb);
}

rendinst::ri_damage_effect_cb rendinstdestr::get_ri_damage_effect_cb() { return ri_effect_cb; }

CachedCollisionObjectInfo *rendinstdestr::get_or_add_cached_collision_object(const rendinst::RendInstDesc &ri_desc, float at_time)
{
  return get_or_add_cached_collision_object(ri_desc, at_time, rendinst::getRiGenDestrInfo(ri_desc));
}

void CachedCollisionObjectInfo::operator delete(void *p) { cco_alloc.freeOneBlock(p); }

CachedCollisionObjectInfo *rendinstdestr::get_or_add_cached_collision_object(const rendinst::RendInstDesc &ri_desc, float at_time,
  const rendinst::CollisionInfo &coll_info)
{
  CachedCollisionObjectInfo *objInfo = rendinstdestr::get_cached_collision_object(ri_desc);
  if (!objInfo)
  {
    if (!coll_info.isDestr)
      return nullptr;
    void *p = cco_alloc.allocateOneBlock();
    const float hitPointsToDestrImpulseMult = rendinstdestr::get_destr_settings().hitPointsToDestrImpulseMult;
    float destrImpulse = coll_info.destrImpulse > 0.f ? coll_info.destrImpulse : coll_info.hp * hitPointsToDestrImpulseMult;
    objInfo = new (p, _NEW_INPLACE) RendinstImpulseThresholdData(destrImpulse, coll_info.desc, at_time, coll_info);
    cached_collision_objects.push_back(objInfo);
  }
  objInfo->refreshTimeToLive();
  return objInfo;
}

CachedCollisionObjectInfo *rendinstdestr::get_or_add_cached_tree_collision_object(const rendinst::RendInstDesc &ri_desc, float impulse,
  float at_time, const rendinst::CollisionInfo &coll_info)
{
  CachedCollisionObjectInfo *objInfo = rendinstdestr::get_cached_collision_object(ri_desc);
  if (!objInfo)
  {
    void *p = cco_alloc.allocateOneBlock();
    objInfo = new (p, _NEW_INPLACE) TreeRendinstImpulseThresholdData(impulse, coll_info.desc, at_time, coll_info);
    cached_collision_objects.push_back(objInfo);
  }
  objInfo->refreshTimeToLive();
  objInfo->atTime = at_time;
  return objInfo;
}

CachedCollisionObjectInfo *rendinstdestr::get_cached_collision_object(const rendinst::RendInstDesc &ri_desc)
{
  for (int i = 0; i < cached_collision_objects.size(); ++i)
    if (cached_collision_objects[i]->riDesc == ri_desc)
      return cached_collision_objects[i];
  return nullptr;
}

void rendinstdestr::remove_tree_rendinst_destr(const rendinst::RendInstDesc &desc)
{
  rendinst::delRIGenExtraFromCell(desc);

  for (int i = 0; i < riPhys.size(); ++i)
  {
    if (riPhys[i].descForDestr != desc)
      continue;

    rendinst::removeRIGenExternalControl(riPhys[i].desc, riPhys[i].ri);
    cleanup_ri_phys(riPhys[i]);
    erase_items(riPhys, i, 1);
    return;
  }
}

void rendinstdestr::clear_phys_objs()
{
  const bool clearExternal = rendinst::should_clear_external_controls();
  for (int i = 0; i < riPhys.size(); ++i)
  {
    if (clearExternal && riPhys[i].physBody)
      rendinst::removeRIGenExternalControl(riPhys[i].desc, riPhys[i].ri);
    cleanup_ri_phys(riPhys[i]);
  }
  clear_and_shrink(riPhys);
}

int rendinstdestr::test_dynobj_to_ri_phys_collision(const CollisionObject &coA, const TMatrix &tmA, float max_rad)
{
  PhysWorld *physWorld = dacoll::get_phys_world();
  int countCollision = 0;
  coA.body->setTmInstant(tmA);
  physWorld->fetchSimRes(true);
  for (int i = 0; i < riPhys.size(); ++i)
  {
    RendInstPhys &phys = riPhys[i];

    bool physObject = phys.physModel->physType == gamephys::DynamicPhysModel::E_PHYS_OBJECT;
    if (physObject)
      continue;

    const float maxRiPhysRad = 5.f;
    TMatrix physTm = TMatrix::IDENT;
    dacoll::get_coll_obj_tm(phys.riColObj, physTm);
    if (lengthSq(physTm.getcol(3) - tmA.getcol(3)) > sqr(maxRiPhysRad + max_rad))
      continue;

    int prevCountContacts = phys.physModel->contacts.size();
    WrapperContactResultCB physCollCb(phys.physModel->contacts);
    physWorld->contactTestPair(phys.riColObj.body, coA.body, physCollCb);
    countCollision += phys.physModel->contacts.size() - prevCountContacts;
  }
  return countCollision;
}

int rendinstdestr::test_dynobj_to_ri_phys_collision(const CollisionObject &coA, float max_rad)
{
  TMatrix tmA = TMatrix::IDENT;

  PhysWorld *physWorld = dacoll::get_phys_world();
  physWorld->fetchSimRes(true);
  coA.body->getTmInstant(tmA);

  return test_dynobj_to_ri_phys_collision(coA, tmA, max_rad);
}

static rendinst::RendInstDesc create_tree_rend_inst_destr_old(const rendinst::RendInstDesc &desc, bool add_restorable,
  const Point3 &impactPos, const Point3 &_impulse, bool create_phys, bool constrained_phys, float wanted_omega, float at_time,
  const rendinst::CollisionInfo *coll_info, bool create_destr, bool from_damage)
{
  G_UNUSED(impactPos);
  Point3 impulse = _impulse;
  if (check_nan(_impulse) || lengthSq(_impulse) > sqr(MAX_RI_DESTROY_IMPULSE))
  {
    logerr("Bad impulse %@ in create_tree_rend_inst_destr", _impulse);
    impulse = Point3();
  }

  // Rendinst system can invalidate static shadows for very large cells in `rendinst::doRIGenExternalControl` and
  // `rendinstdestr::destroyRendinst`. But it is not needed if we have special callback with invalidation for only
  // instance bbox
  if (destrSettings.hasStaticShadowsInvalidationCallback)
    rendinst::render::avoidStaticShadowRecalc = true;

  const rendinst::RiDestrData riDestrData = rendinst::gather_ri_destr_data(desc, from_damage);
  if (!riDestrData.branchDestr)
    return desc;

  rendinst::DestroyedRi *ri = create_destr ? rendinst::doRIGenExternalControl(desc, !create_phys) : NULL;
  if (ri && on_tree_destr_created_cb)
    on_tree_destr_created_cb(desc, ri->riHandle);
  Point2 impulseDir = normalizeDef(Point2::xz(impulse), {1.f, 0.f});
  rendinst::RendInstDesc offsetedDesc = desc;
  rendinst::DestrOptionFlags flags =
    rendinst::DestrOptionFlag::AddDestroyedRi | rendinst::DestrOptionFlag::ForceDestroy | rendinst::DestrOptionFlag::UseFullBbox;
  if (create_phys)
    offsetedDesc = rendinstdestr::destroyRendinst(desc, add_restorable, ZERO<Point3>(), ZERO<Point3>(), at_time, coll_info,
      create_destr, nullptr, -1, 1.0f, nullptr, flags);

  if (destrSettings.hasStaticShadowsInvalidationCallback)
    rendinst::render::avoidStaticShadowRecalc = false;

  if (!ri)
    return offsetedDesc;

  const float treeHeightSpringThreshold = 3.f;
  const rendinstdestr::TreeDestr &treeDestr = rendinstdestr::get_tree_destr();

  const RendInstGenData *rgl = RendInstGenData::getGenDataByLayer(desc);
  const RendInstGenData::RendinstProperties *riProp = nullptr;
  if (!desc.isRiExtra() && (uint32_t)desc.pool < rgl->rtData->riProperties.size())
    riProp = &rgl->rtData->riProperties[desc.pool];

  mat43f m43 = rendinst::getRIGenExtra43(ri->riHandle);
  mat44f m44;
  v_mat43_transpose_to_mat44(m44, m43);
  vec3f scaleSq = v_mat44_scale43_sq(m44);
  if (v_test_xyz_nan(scaleSq) || v_test_xyz_nan(m44.col3) || !v_test_xyz_abs_lt(m44.col3, v_splats(1e5f)) ||
      v_check_xyz_any_true(v_is_unsafe_divisor(scaleSq)))
  {
    logerr("rendinstdestr: failed to create %s[%d] phys body with bad tm " FMT_TM, desc.isRiExtra() ? "riExtra" : "riGen", desc.pool,
      VTMD(m44));
    return offsetedDesc;
  }
  TMatrix tmOriginal;
  v_mat_43cu_from_mat44(tmOriginal.array, m44);

  v_mat44_orthonormalize33(m44, m44);
  TMatrix tm;
  v_mat_43cu_from_mat44(tm.array, m44);
  BBox3 bbox = rendinst::getRIGenBBox(desc);
  Point3_vec4 scale;
  v_st(&scale.x, v_sqrt(scaleSq));

  BBox3 fullBb = rendinst::getRIGenFullBBox(desc);
  BBox3 canopyBox = bbox;
  if (riProp)
  {
    const bool hasCanopy = rendinst::getRIGenCanopyBBox(*riProp, fullBb, canopyBox);
    const float canopyCutHt = (canopyBox.width().y * 0.5f) * 0.95f; // take a height of 10% of its middle
    if (hasCanopy && canopyBox.lim[1].y > canopyCutHt)
    {
      canopyBox.lim[1].y -= canopyCutHt;
      canopyBox.lim[0].y += canopyCutHt;
    }
    else
    {
      canopyBox.lim[1].y = bbox.lim[1].y;
      canopyBox.lim[0].y = bbox.lim[1].y - bbox.width().y * 0.1f;
    }
  }
  canopyBox.lim[0] *= scale.y;
  canopyBox.lim[1] *= scale.y;
  bbox.lim[0] *= scale.y;
  bbox.lim[1] *= scale.y;

  const float fullTreeHt = canopyBox.lim[1].y;
  const float height = fullTreeHt * riDestrData.collisionHeightScale;
  if (height <= 0)
  {
    logerr("%s[%d] gives bbox=%@ and invalid height=%.1f (scale=%@ collisionHeightScale=%g)", desc.isRiExtra() ? "riExtra" : "riGen",
      desc.pool, bbox, height, scale, riDestrData.collisionHeightScale);
    return offsetedDesc;
  }
  const float bushRadiusMult = (riDestrData.bushBehaviour ? treeDestr.bushDestrRadiusMult : 1.0f);
  const Point3 boxSize = bbox.width();
  float radius = clamp_min(min(boxSize.x, boxSize.z) * 0.5f * bushRadiusMult, 0.1f);
  if (riProp && riProp->trunkRadius > 0.f)
    radius = riProp->trunkRadius;

  const float density = riDestrData.bushBehaviour ? treeDestr.bushDensity : treeDestr.treeDensity;
  const float mass = PI * sqr(radius) * height * density;
  const Point3 momentOfInertia = Point3(mass * (3.f * sqr(radius) + sqr(height)) / 12.f, mass * sqr(radius) * 0.5f,
    mass * (3.f * sqr(radius) + sqr(height)) / 12.f);
  const bool allowSpring = height < treeHeightSpringThreshold && desc.layer == 1;
  gamephys::DynamicPhysModel::PhysType physModelType =
    create_phys ? gamephys::DynamicPhysModel::E_PHYS_OBJECT
                : (allowSpring ? gamephys::DynamicPhysModel::E_ANGULAR_SPRING : gamephys::DynamicPhysModel::E_FORCED_POS_BY_COLLISION);

  RendInstPhys &phys =
    riPhys.emplace_back(RendInstPhysType::TREE, desc, tm, mass, momentOfInertia, scale, Point3(0.f, height * 0.5f, 0.f), physModelType,
      tmOriginal, rendinstdestr::get_destr_settings().rendInstMaxLifeTime, treeDestr.maxLifeDistance, ri);

  {
    rendinst::RiExtraPerInstanceGpuItem originalPosRenderData = {bitwise_cast<uint32_t>(tmOriginal.m[3][0]),
      bitwise_cast<uint32_t>(tmOriginal.m[3][1]), bitwise_cast<uint32_t>(tmOriginal.m[3][2]), bitwise_cast<uint32_t>(1.0f)};
    rendinst::setRiExtraPerInstanceRenderAdditionalDataDeferred(ri->riHandle, rendinst::RiExtraPerInstanceDataType::INITIAL_TM_POS,
      rendinst::RiExtraPerInstanceDataPersistence::PERSISTENT, &originalPosRenderData, 1);
  }

  if (!allowSpring && rendinstdestr::get_destr_settings().hasSound)
    phys.treeSound.init(g_tree_sound_cb, tm, bbox.lim[1].y * scale.y, riDestrData.bushBehaviour);

  phys.descForDestr = offsetedDesc;

  if (phys.physModel->physType == gamephys::DynamicPhysModel::E_PHYS_OBJECT)
  {
    float canopyStartHt = canopyBox.lim[0].y;
    float trunkStartHt = (constrained_phys ? height * 0.035f : 0.f) + radius;
    float trunkHt = max(canopyStartHt * 0.9f - trunkStartHt, 2.f * radius);
    float trunkHalfHt = trunkHt * 0.5f;

    TMatrix fallDirTm;
    fallDirTm.setcol(0, Point3::x0y(impulseDir));
    fallDirTm.setcol(2, fallDirTm.getcol(0) % tm.getcol(1));
    fallDirTm.setcol(1, fallDirTm.getcol(2) % fallDirTm.getcol(0));
    fallDirTm.setcol(3, Point3(0, 0, 0));

    TMatrix invObjDir = inverse(tm);
    invObjDir.col[3].zero();
    TMatrix centerOfMassTm = fallDirTm * invObjDir;
    centerOfMassTm.setcol(3, Point3(0.f, trunkHalfHt + trunkStartHt, 0.f));
    phys.centerOfMassTm = centerOfMassTm;
    PhysWorld *physWorld = dacoll::get_phys_world();

    // trunk
    {
      // shape
      PhysBoxCollision trunkShape(radius * sqrt(2.f), trunkHt, radius * sqrt(2.f), false);

      PhysBodyCreationData pbcd;
      pbcd.momentOfInertia = momentOfInertia;
      if (constrained_phys)
      {
        phys.collisionDistLimit = 1.0f;
        pbcd.autoMask = false, pbcd.group = dacoll::EPL_DEFAULT, pbcd.mask = dacoll::EPL_ALL ^ dacoll::EPL_KINEMATIC; //-V1048
      }

      // body
      phys.physBody = new PhysBody(physWorld, mass, &trunkShape, tm * centerOfMassTm, pbcd);
    }

    // canopy
    float sphRad = (height - canopyStartHt) * 0.5f;
    float impulseDirMult = constrained_phys ? 0.1f * sphRad : 0.f;
    TMatrix sphTm = phys.centerOfMassTm;
    sphTm.setcol(3, Point3(impulseDir.x * impulseDirMult, canopyBox.center().y, impulseDir.y * impulseDirMult));
    const float canopyMass = canopyBox.volume() * treeDestr.canopyDensity;
    Point3 canopyInertia = treeDestr.canopyInertia * 0.4f * canopyMass;
    {
      // shape
      Point3 canopyBSize = canopyBox.width();
      PhysBoxCollision boxCanopyCollision(canopyBSize.x, canopyBSize.y, canopyBSize.z);

      PhysBodyCreationData pbcd;
      pbcd.momentOfInertia = canopyInertia;
      pbcd.rollingFriction = treeDestr.canopyRollingFriction;
      pbcd.restitution = treeDestr.canopyRestitution;
      pbcd.linearDamping = treeDestr.canopyLinearDamping;
      pbcd.angularDamping = treeDestr.canopyAngularDamping;
      if (constrained_phys)
        pbcd.autoMask = false, pbcd.group = dacoll::EPL_DEFAULT, pbcd.mask = dacoll::EPL_ALL ^ dacoll::EPL_KINEMATIC; //-V1048

      // canopy body
      phys.additionalBody = new PhysBody(physWorld, canopyMass, &boxCanopyCollision, tm * sphTm, pbcd);
    }

    // joint
    {
      TMatrix halfSphTm(1);
      halfSphTm.setcol(3, sphTm.getcol(3) * 0.5f);
      TMatrix canopyCoMTm = sphTm;
      canopyCoMTm.setcol(3, sphTm.getcol(3) * 0.5f);
      PhysJoint *joint = physWorld->create6DofSpringJoint(phys.physBody, phys.additionalBody, halfSphTm * inverse(phys.centerOfMassTm),
        inverse(canopyCoMTm));
      phys.joints.push_back() = joint;
      Phys6DofSpringJoint *dofSpring = Phys6DofSpringJoint::cast(joint);
      if (dofSpring)
      {
        const float cmLen = length(sphTm.getcol(3));
        const float stiffness = (mass + canopyMass * cmLen * 3.f) * treeDestr.canopyStiffnessMult;
        const float angularLimit = treeDestr.canopyAngularLimit;
        dofSpring->setLimit(0, Point2(0.f, 0.f));
        dofSpring->setLimit(1, Point2(0.f, 0.f));
        dofSpring->setLimit(2, Point2(0.f, 0.f));
        dofSpring->setSpring(3, true, stiffness, 0.005f, Point2(-angularLimit, angularLimit));
        dofSpring->setLimit(4, Point2(0.f, 0.f));
        dofSpring->setSpring(5, true, stiffness, 0.005f, Point2(-angularLimit, angularLimit));
        for (int i = 0; i < 6; ++i)
        {
#if defined(USE_BULLET_PHYSICS)
          dofSpring->setParam(BT_CONSTRAINT_STOP_ERP, 0.2, i);
#endif
          dofSpring->setAxisDamping(i, 0.5f);
        }
        dofSpring->setEquilibriumPoint();
      }
      phys.lastValidTm = tm;
    }

    // Impulse is calculated in such way it doesn't violate constraint
    // spdbot = omega * arm + vel
    // vel = omega * arm
    // omega = vel / arm
    // omega = imp * arm2 / inertia
    // vel = imp / mass
    // arm2 = omega * inertia / imp
    // arm2 = (vel / arm) * inertia / imp
    // arm2 = ((imp / mass) / arm) * inertia / imp
    // arm2 = (imp * inertia) / (mass * arm * imp)
    // arm2 = inertia / (mass * arm)

    // imp = omega * inertia / arm2
    float omega = clamp(wanted_omega, 0.3f, 1.f);
    float arm = safediv(momentOfInertia.x, mass * (trunkHalfHt + trunkStartHt));
    float vel = omega * arm;
    G_UNUSED(vel);
    float impulseVal = safediv(omega * momentOfInertia.x, arm);
    G_ASSERTF(!check_nan(impulseVal) && impulseVal < MAX_RI_DESTROY_IMPULSE, "impulseVal %f", impulseVal);

    if (constrained_phys)
    {
      PhysJoint *joint = physWorld->create6DofJoint(phys.physBody, NULL, inverse(phys.centerOfMassTm), TMatrix::IDENT);
      phys.joints.push_back() = joint;
      Phys6DofJoint *dofJoint = Phys6DofJoint::cast(joint);
      if (dofJoint)
      {
        dofJoint->setLimit(0, Point2(0.f, 0.f));
        dofJoint->setLimit(1, treeDestr.constraintLimitY);
        dofJoint->setLimit(2, Point2(0.f, 0.f));
#if defined(USE_BULLET_PHYSICS)
        for (int i = 0; i < 6; ++i)
          dofJoint->setParam(BT_CONSTRAINT_STOP_ERP, 0.2, i);
#endif
      }

      phys.physBody->addImpulse(tm * (phys.centerOfMassTm.getcol(3) + Point3(0.f, arm, 0.f)), Point3::x0y(impulseDir) * impulseVal);

      // omega = vel / arm
      // vel = omega * arm
      // imp = vel * mass
      // imp = omega * arm * mass
      float additionalImpulseVal = omega * (height - sphRad) * canopyMass;
      G_ASSERTF(!check_nan(additionalImpulseVal) && additionalImpulseVal < MAX_RI_DESTROY_IMPULSE, "additionalImpulseVal %f",
        additionalImpulseVal);
      phys.additionalBody->addImpulse((tm * sphTm).getcol(3), additionalImpulseVal * Point3::x0y(impulseDir));
    }
    phys.lastValidTm = tm;
    phys.treeInstData.creationTimeAt = at_time;
    if (!constrained_phys && (coll_info->destrFxId >= 0 || !coll_info->destrFxTemplate.empty()) && ri_effect_cb)
      ri_effect_cb(coll_info->destrFxId, TMatrix::IDENT, tm, coll_info->desc.pool, false, nullptr, coll_info->destrFxTemplate.c_str());

    const bool enableBranchDestr =
      is_branch_destruction_supported && riDestrData.branchDestr->enableBranchDestruction && at_time > 0.0f;
    if (enableBranchDestr)
    {
      phys.treeInstData.branchDestr = *riDestrData.branchDestr;
      phys.treeInstData.impactXZ = impulseDir * vel;
      if (!rendinst::fillTreeInstData(desc, from_damage, phys.treeInstData))
        phys.treeInstData.branchDestr.enableBranchDestruction = false;
      else
        debugTreeInstData.last_object_was_from_damage = from_damage;
    }
  }
  else
  {
    float hh = height * 0.5f;
    TMatrix cotm = TMatrix::IDENT;
    cotm.m[3][1] = hh;
    phys.riColObj = dacoll::add_dynamic_cylinder_collision(cotm, radius, height, /*uptr*/ nullptr, /*add_to_world*/ false);
    dacoll::obj_apply_impulse(phys.riColObj, impulse * mass, Point3(0.f, hh, 0.f));
  }

  if (create_destr && rendinstdestr::get_destr_settings().createDestr && !create_phys && ri_effect_cb)
  {
    if (riDestrData.fxType != -1 || *riDestrData.fxTemplate != 0)
    {
      TMatrix fxTm = tm;
      fxTm *= riDestrData.fxScale;
      fxTm.setcol(3, tm.getcol(3));
      ri_effect_cb(riDestrData.fxType, TMatrix::IDENT, fxTm, desc.pool, false, nullptr, riDestrData.fxTemplate);
    }
  }

  return offsetedDesc;
}

static rendinst::RendInstDesc create_tree_rend_inst_destr_new(const rendinst::RendInstDesc &desc, bool add_restorable,
  const Point3 &impactPos, const Point3 &_impulse, bool create_phys, bool constrained_phys, float wanted_omega, float at_time,
  const rendinst::CollisionInfo *coll_info, bool create_destr, bool from_damage)
{
  debugTreeInstData.lastHitPos = impactPos;
  debugTreeInstData.impulsePositions.clear();

  Point3 impulse = _impulse;
  float impulseStrength = length(_impulse);
  if (check_nan(_impulse) || impulseStrength > MAX_RI_DESTROY_IMPULSE)
  {
    logerr("Bad impulse %@ in create_tree_rend_inst_destr", _impulse);
    impulseStrength = 0.0f;
    impulse = Point3();
  }

  const rendinstdestr::TreeDestr &treeDestr = rendinstdestr::get_tree_destr();
  const rendinst::RiDestrData riDestrData = rendinst::gather_ri_destr_data(desc, from_damage);
  if (!riDestrData.branchDestr)
    return desc;

  static const float min_cut_height = 0.5f;

  const bool enableBranchDestr = is_branch_destruction_supported && riDestrData.branchDestr->enableBranchDestruction && at_time > 0.0f;
  bool cutTreeInHalf = enableBranchDestr && create_destr && from_damage && !riDestrData.bushBehaviour;
  if (from_damage)
  {
    // A big tank cannon does about 1000 damage, we want this to have a noticeable increase in rotation speed
    float damageEquivalent = impulseStrength / rendinstdestr::get_destr_settings().hitPointsToDestrImpulseMult;
    wanted_omega *= 1 + (damageEquivalent * 0.001f);

    float cutInHalfThreshold = riDestrData.branchDestr->cutInHalfDamageThreshold;
    if (useDebugTreeInstData)
      cutInHalfThreshold = debugTreeInstData.branchDestrFromDamage.cutInHalfDamageThreshold;
    if (cutInHalfThreshold < 0 || cutInHalfThreshold > damageEquivalent || currentCutTreeCount >= treeDestr.maxCutTreeCount)
      cutTreeInHalf = false;

    float cutHeight = impactPos.y - rendinst::getRIGenMatrix(desc).col[3].y;
    if (cutHeight < min_cut_height)
    {
      // this could happen if the collision geometry of the tree reaches below the model origin
      cutTreeInHalf = false;
    }
  }

  if (useDebugTreeInstData && debugTreeInstData.disableCut)
    cutTreeInHalf = false;


  // Rendinst system can invalidate static shadows for very large cells in `rendinst::doRIGenExternalControl` and
  // `rendinstdestr::destroyRendinst`. But it is not needed if we have special callback with invalidation for only
  // instance bbox
  if (destrSettings.hasStaticShadowsInvalidationCallback)
    rendinst::render::avoidStaticShadowRecalc = true;

  int sliceCount = cutTreeInHalf ? 2 : 1;

  Tab<rendinst::DestroyedRi *> ris =
    create_destr ? rendinst::doRIGenExternalControlMultiple(desc, sliceCount, !create_phys) : Tab<rendinst::DestroyedRi *>();
  if (on_tree_destr_created_cb)
    for (const rendinst::DestroyedRi *ri : ris)
      on_tree_destr_created_cb(desc, ri->riHandle);
  Point2 impulseDir = normalizeDef(Point2::xz(impulse), {1.f, 0.f});
  rendinst::RendInstDesc offsetedDesc = desc;
  rendinst::DestrOptionFlags flags =
    rendinst::DestrOptionFlag::AddDestroyedRi | rendinst::DestrOptionFlag::ForceDestroy | rendinst::DestrOptionFlag::UseFullBbox;
  if (create_phys)
    offsetedDesc = rendinstdestr::destroyRendinst(desc, add_restorable, ZERO<Point3>(), ZERO<Point3>(), at_time, coll_info,
      create_destr, nullptr, 1, 1.0f, nullptr, flags);

  if (destrSettings.hasStaticShadowsInvalidationCallback)
    rendinst::render::avoidStaticShadowRecalc = false;

  if (ris.size() == 0)
    return offsetedDesc;

  const float treeHeightSpringThreshold = 3.f;

  mat43f m43 = rendinst::getRIGenExtra43(ris[0]->riHandle);
  mat44f m44;
  v_mat43_transpose_to_mat44(m44, m43);
  vec3f scaleSq = v_mat44_scale43_sq(m44);
  if (v_test_xyz_nan(scaleSq) || v_test_xyz_nan(m44.col3) || !v_test_xyz_abs_lt(m44.col3, v_splats(1e5f)) ||
      v_check_xyz_any_true(v_is_unsafe_divisor(scaleSq)))
  {
    logerr("rendinstdestr: failed to create %s[%d] phys body with bad tm " FMT_TM, desc.isRiExtra() ? "riExtra" : "riGen", desc.pool,
      VTMD(m44));
    return offsetedDesc;
  }

  if (cutTreeInHalf)
    currentCutTreeCount++;

  TMatrix tmOriginal;
  v_mat_43cu_from_mat44(tmOriginal.array, m44);

  v_mat44_orthonormalize33(m44, m44);
  TMatrix tmNoScale;
  v_mat_43cu_from_mat44(tmNoScale.array, m44);
  BBox3 bbox = rendinst::getRIGenBBox(desc);
  Point3_vec4 scale;
  v_st(&scale.x, v_sqrt(scaleSq));

  BBox3 canopyBboxLocal;
  rendinst::getRIGenCanopyBBox(desc, TMatrix::IDENT, canopyBboxLocal);

  const RendInstGenData *rgl = RendInstGenData::getGenDataByLayer(desc);
  const RendInstGenData::RendinstProperties *riProp = nullptr;
  if (!desc.isRiExtra() && (uint32_t)desc.pool < rgl->rtData->riProperties.size())
    riProp = &rgl->rtData->riProperties[desc.pool];

  Point3 bboxSize = bbox.width();
  float totalTreeHeight = bboxSize.y * scale.y * riDestrData.collisionHeightScale;
  const float bushRadiusMult = (riDestrData.bushBehaviour ? treeDestr.bushDestrRadiusMult : 1.0f);

  const Point3 boxSize = bbox.width();
  const float maxTrunkRadius = 0.1f;
  float radius = clamp_min(min(boxSize.x, boxSize.z) * 0.5f * bushRadiusMult, maxTrunkRadius);
  if (riProp && riProp->trunkRadius > 0.f)
    radius = riProp->trunkRadius;

  const float density = riDestrData.bushBehaviour ? treeDestr.bushDensity : treeDestr.treeDensity;

#if TREE_DESTR_DEBUG
  const float groundOffsetMul = groundOffsetMulOverride < 0.0f ? treeDestr.groundOffsetMul : groundOffsetMulOverride;
  const float groundDamping = groundDampingOverride < 0.0f ? treeDestr.groundDamping : groundDampingOverride;
  const float groundJointDamping = groundJointDampingOverride < 0.0f ? treeDestr.groundJointDamping : groundJointDampingOverride;
  const float groundJointMoveDistance =
    groundJointMoveDistanceOverride < 0.0f ? treeDestr.constraintLimitY.y : groundJointMoveDistanceOverride;
  const float groundJointAngleLimitWhole =
    groundJointAngleLimitWholeOverride < 0.0f ? treeDestr.groundJointAngleLimitWhole : groundJointAngleLimitWholeOverride;
  const float groundJointAngleLimitBottom =
    groundJointAngleLimitBottomOverride < 0.0f ? treeDestr.groundJointAngleLimitBottom : groundJointAngleLimitBottomOverride;
  const float treeTopTrunkDampingLin =
    treeTopTrunkDampingLinOverride < 0.0f ? treeDestr.treeDampingLinear : treeTopTrunkDampingLinOverride;
  const float treeTopTrunkDampingAng =
    treeTopTrunkDampingAngOverride < 0.0f ? treeDestr.treeDampingAngular : treeTopTrunkDampingAngOverride;
  const float canopyJointAngLimit = canopyJointAngLimitOverride < 0.0f ? treeDestr.canopyAngularLimit : canopyJointAngLimitOverride;
  const float canopyDampingLin = canopyDampingLinOverride < 0.0f ? treeDestr.canopyLinearDamping : canopyDampingLinOverride;
  const float canopyDampingAng = canopyDampingAngOverride < 0.0f ? treeDestr.canopyAngularDamping : canopyDampingAngOverride;
  const float canopyStiffnessMult = canopyStiffnessMultOverride < 0.0f ? treeDestr.canopyStiffnessMult : canopyStiffnessMultOverride;
  const float canopySpringDampingAng = canopySpringDampingAngOverride < 0.0f ? 0.005f : canopySpringDampingAngOverride;
  const float canopyJointDamping = canopyJointDampingOverride < 0.0f ? treeDestr.canopyJointDamping : canopyJointDampingOverride;
  const float canopyRadiusMul = canopyRadiusMulOverride >= 0.0f ? canopyRadiusMulOverride : 0.7f;
  const float canopyUnderhangMul = canopyUnderhangMulOverride >= 0.0f ? canopyUnderhangMulOverride : 0.8f;
  const float canopyImpulseMul = canopyImpulseMulOverride >= 0.0f ? canopyImpulseMulOverride : 1.0f;
  const float collisionDistLimit = linearLimitOverride >= 0.0f ? linearLimitOverride : treeDestr.collisionDistLimit;
  const float collisionAngleLimitCos =
    angleLimitOverride >= 0.0f ? cosf(angleLimitOverride * DEG_TO_RAD) : cosf(treeDestr.collisionAngleLimit * DEG_TO_RAD);
#else
  const float &groundOffsetMul = treeDestr.groundOffsetMul;
  const float &groundDamping = treeDestr.groundDamping;
  const float &groundJointDamping = treeDestr.groundJointDamping;
  const float &groundJointAngleLimitWhole = treeDestr.groundJointAngleLimitWhole;
  const float &groundJointAngleLimitBottom = treeDestr.groundJointAngleLimitBottom;
  const float &groundJointMoveDistance = treeDestr.constraintLimitY.y;
  const float &treeTopTrunkDampingLin = treeDestr.treeDampingLinear;
  const float &treeTopTrunkDampingAng = treeDestr.treeDampingAngular;
  const float &canopyJointAngLimit = treeDestr.canopyAngularLimit;
  const float &canopyDampingLin = treeDestr.canopyLinearDamping;
  const float &canopyDampingAng = treeDestr.canopyAngularDamping;
  const float &canopyStiffnessMult = treeDestr.canopyStiffnessMult;
  const float canopySpringDampingAng = 0.005f;
  const float &canopyJointDamping = treeDestr.canopyJointDamping;
  const float canopyRadiusMul = 0.7f;
  const float canopyUnderhangMul = 0.8f;
  const float canopyImpulseMul = 1.0f;
  const float &collisionDistLimit = treeDestr.collisionDistLimit;
  const float collisionAngleLimitCos = cosf(treeDestr.collisionAngleLimit * DEG_TO_RAD);
#endif

  const bool allowSpring = totalTreeHeight < treeHeightSpringThreshold && desc.layer == 1;
  gamephys::DynamicPhysModel::PhysType physModelType =
    create_phys ? gamephys::DynamicPhysModel::E_PHYS_OBJECT
                : (allowSpring ? gamephys::DynamicPhysModel::E_ANGULAR_SPRING : gamephys::DynamicPhysModel::E_FORCED_POS_BY_COLLISION);

  PhysWorld *physWorld = dacoll::get_phys_world();

  for (int iTreeSlice = 0; iTreeSlice < ris.size(); iTreeSlice++)
  {
    rendinst::DestroyedRi *ri = ris[iTreeSlice];

    Point3 localImpactPos = inverse(tmOriginal) * impactPos;
    float cutHeight = 0.0f;

    rendinst::TreeInstData::CutType cutType = rendinst::TreeInstData::CutType::None;
    if (cutTreeInHalf)
    {
      cutHeight = impactPos.y - tmOriginal.col[3].y;
      cutType = iTreeSlice == 0 ? rendinst::TreeInstData::CutType::Top : rendinst::TreeInstData::CutType::Bottom;
      cutHeight = clamp(cutHeight, 0.0f, totalTreeHeight);
    }

    float height = totalTreeHeight;
    if (cutType == rendinst::TreeInstData::CutType::Top)
      height = totalTreeHeight - cutHeight;
    else if (cutType == rendinst::TreeInstData::CutType::Bottom)
      height = cutHeight;

    if (cutType == rendinst::TreeInstData::CutType::Top)
      rendinst::setRiExtraPerInstanceOptionalFlag(ri->riHandle, rendinst::RiExtraOptionalFlag::HIDE_MARKED_MATERIAL);

    {
      rendinst::RiExtraPerInstanceGpuItem originalPosRenderData = {bitwise_cast<uint32_t>(tmOriginal.m[3][0]),
        bitwise_cast<uint32_t>(tmOriginal.m[3][1]), bitwise_cast<uint32_t>(tmOriginal.m[3][2]), bitwise_cast<uint32_t>(1.0f)};
      rendinst::setRiExtraPerInstanceRenderAdditionalData(ri->riHandle, rendinst::RiExtraPerInstanceDataType::INITIAL_TM_POS,
        rendinst::RiExtraPerInstanceDataPersistence::PERSISTENT, &originalPosRenderData, 1);
    }

    if (totalTreeHeight <= 0 || height <= 0)
    {
      logerr("%s[%d] gives bbox=%@ and invalid height=%.1f (scale=%@ collisionHeightScale=%g totalTreeHeight=%0.1f)",
        desc.isRiExtra() ? "riExtra" : "riGen", desc.pool, bbox, height, scale, riDestrData.collisionHeightScale, totalTreeHeight);
      return offsetedDesc;
    }

    const float mass = PI * sqr(radius) * height * density;
    const Point3 momentOfInertia = Point3(mass * (3.f * sqr(radius) + sqr(height)) / 12.f, mass * sqr(radius) * 0.5f,
      mass * (3.f * sqr(radius) + sqr(height)) / 12.f);

    // Impulse is calculated in such way it doesn't violate constraint
    // spdbot = omega * arm + vel
    // vel = omega * arm
    // omega = vel / arm
    // omega = imp * arm2 / inertia
    // vel = imp / mass
    // arm2 = omega * inertia / imp
    // arm2 = (vel / arm) * inertia / imp
    // arm2 = ((imp / mass) / arm) * inertia / imp
    // arm2 = (imp * inertia) / (mass * arm * imp)
    // arm2 = inertia / (mass * arm)

    float omega = clamp(wanted_omega, 0.3f, 1.f);
    float arm = safediv(momentOfInertia.x, mass * height * 0.5f);
    float vel = omega * arm;
    float impulseVal = safediv(omega * momentOfInertia.x, arm);
    G_ASSERTF(!check_nan(impulseVal) && impulseVal < MAX_RI_DESTROY_IMPULSE, "impulseVal %f", impulseVal);

    RendInstPhys &phys =
      riPhys.emplace_back(RendInstPhysType::TREE, desc, tmNoScale, mass, momentOfInertia, scale, Point3(0.f, height * 0.5f, 0.f),
        physModelType, tmOriginal, rendinstdestr::get_destr_settings().rendInstMaxLifeTime, treeDestr.maxLifeDistance, ri);

    phys.treeInstData.creationTimeAt = at_time;
    phys.treeInstData.localImpactPos = localImpactPos;
    phys.treeInstData.cutType = cutType;

    phys.descForDestr = offsetedDesc;

    if (cutType != rendinst::TreeInstData::CutType::Bottom)
    {
      if (!allowSpring && rendinstdestr::get_destr_settings().hasSound)
        phys.treeSound.init(g_tree_sound_cb, tmNoScale, bbox.lim[1].y * scale.y, riDestrData.bushBehaviour);
    }

    if (phys.physModel->physType == gamephys::DynamicPhysModel::E_PHYS_OBJECT)
    {
      Point3 canopySizeScaled = canopyBboxLocal.width() * scale.y;
      canopySizeScaled.x *= canopyRadiusMul;
      canopySizeScaled.z *= canopyRadiusMul;
      float canopyCenterHt = canopyBboxLocal.center().y * scale.y;

      TMatrix fallDirTm;
      fallDirTm.setcol(0, Point3::x0y(impulseDir));
      fallDirTm.setcol(2, fallDirTm.getcol(0) % tmNoScale.getcol(1));
      fallDirTm.setcol(1, fallDirTm.getcol(2) % fallDirTm.getcol(0));
      fallDirTm.setcol(3, tmNoScale.getcol(3));
      TMatrix canopyImpulseDirTm = inverse(tmNoScale) % fallDirTm;

      PhysBodyCreationData pbcd;
      pbcd.friction = treeDestr.treeFriction;
      pbcd.rollingFriction = 0.95f;
      pbcd.autoInertia = true;
      if (constrained_phys && cutType == rendinst::TreeInstData::CutType::Bottom)
      {
        pbcd.linearDamping = groundDamping;
        pbcd.angularDamping = groundDamping;
      }
      else
      {
        pbcd.linearDamping = treeTopTrunkDampingLin;
        pbcd.angularDamping = treeTopTrunkDampingAng;
      }

      if (constrained_phys && cutType == rendinst::TreeInstData::CutType::Bottom)
      { // Bottom half won't collide with tanks. The ground joint will simply lose angle limits at tank collision.
        pbcd.autoMask = false;
        pbcd.mask = dacoll::EPL_ALL ^ dacoll::EPL_KINEMATIC;
      }
      else
      { // Free moving or whole parts collide with tanks until distance or angle limits are reached.
        phys.collisionDistLimit = collisionDistLimit;
        phys.collisionAngleLimitCos = collisionAngleLimitCos;
      }

      float trunkShapeHeight;
      if (riDestrData.bushBehaviour)
      {
        canopySizeScaled *= 0.6f; // bush canopy is smaller
        PhysBoxCollision boxShape(canopySizeScaled.x, canopySizeScaled.y, canopySizeScaled.z);
        trunkShapeHeight = canopySizeScaled.y;

        TMatrix centerOfMassTm = TMatrix::IDENT;
        centerOfMassTm.setcol(3, Point3(0.f, canopyCenterHt, 0.f));
        phys.centerOfMassTm = centerOfMassTm;

        float canopyMass = canopySizeScaled.x * canopySizeScaled.y * canopySizeScaled.z * treeDestr.canopyDensity;
        phys.physBody = new PhysBody(physWorld, canopyMass, &boxShape, tmNoScale * centerOfMassTm, pbcd);
      }
      else
      {
        float trunkStartHt = totalTreeHeight * groundOffsetMul + radius;
        float trunkEndHt = totalTreeHeight;
        if (cutType == rendinst::TreeInstData::CutType::Top)
          trunkStartHt = cutHeight + radius * 0.5f;
        else if (cutType == rendinst::TreeInstData::CutType::Bottom)
          trunkEndHt = cutHeight;

        float canopyBottom = canopyCenterHt - canopySizeScaled.y * 0.5f;
        if (canopyBottom < trunkStartHt)
        { // make sure the canopy doesn't start below the trunk
          float canopyUnderhang = (trunkStartHt - canopyBottom);
          float newCanopyHeight = (canopySizeScaled.y - canopyUnderhang) * canopyUnderhangMul;
          float offset = (canopySizeScaled.y - newCanopyHeight) * 0.5f;
          canopySizeScaled.y = newCanopyHeight;
          canopyCenterHt += offset;
        }

        trunkShapeHeight = max(trunkEndHt - trunkStartHt, 2.f * radius);

        TMatrix centerOfMassTm = TMatrix::IDENT;
        centerOfMassTm.setcol(3, Point3(0.f, trunkShapeHeight * 0.5f + trunkStartHt, 0.f));
        phys.centerOfMassTm = centerOfMassTm;

        // trunk
        {
          // shape
          PhysCapsuleCollision capsuleShape(radius, trunkShapeHeight, 1 /*Y*/);
          phys.physBody = new PhysBody(physWorld, mass, &capsuleShape, tmNoScale * centerOfMassTm, pbcd);
        }
      }

      TMatrix sphTm = TMatrix::IDENT;
      float canopyMass = canopySizeScaled.x * canopySizeScaled.y * canopySizeScaled.z * treeDestr.canopyDensity;
      if (!riDestrData.bushBehaviour &&
          (cutType == rendinst::TreeInstData::CutType::Top || cutType == rendinst::TreeInstData::CutType::None) && canopyMass > 0.0f)
      {
        // canopy
        float impulseDirMult = constrained_phys ? 0.1f * canopySizeScaled.y : 0.f;
        sphTm.setcol(3, Point3(impulseDir.x * impulseDirMult, canopyCenterHt, impulseDir.y * impulseDirMult));
        {
          // shape
          float canopyRadius = max(canopySizeScaled.x, canopySizeScaled.z) * 0.5f;
          float colSize = clamp(canopyRadius, 0.05f, treeDestr.canopyMaxRaduis);
          PhysSphereCollision sphCanopyCollision(colSize);

          float canopyMaxHorizontalSize = min(treeDestr.canopyMaxRaduis * 2.0f, canopySizeScaled.y * 0.8f); // make sure box is
                                                                                                            // vertical
          PhysBoxCollision boxCanopyCollision(clamp(canopySizeScaled.x, 0.05f, canopyMaxHorizontalSize), canopySizeScaled.y,
            clamp(canopySizeScaled.z, 0.05f, canopyMaxHorizontalSize));

          Tab<Point3> pyramidPoints(tmpmem);
          float canopyWidthX = canopyBboxLocal.width().x * 0.5f * scale.y * canopyRadiusMul;
          float canopyWidthZ = canopyBboxLocal.width().z * 0.5f * scale.y * canopyRadiusMul;
          float canopyBottom = canopyCenterHt - canopySizeScaled.y * 0.5f;
          float canopyBottomElevated = canopyBottom + canopySizeScaled.y * 0.2f;
          Point3 bottomCenter = {canopyBboxLocal.center().x, canopyBottomElevated, canopyBboxLocal.center().z};
          pyramidPoints.push_back(bottomCenter + Point3(-canopyWidthX, 0.f, -canopyWidthZ));
          pyramidPoints.push_back(bottomCenter + Point3(-canopyWidthX, 0.f, canopyWidthZ));
          pyramidPoints.push_back(bottomCenter + Point3(canopyWidthX, 0.f, canopyWidthZ));
          pyramidPoints.push_back(bottomCenter + Point3(canopyWidthX, 0.f, -canopyWidthZ));
          pyramidPoints.push_back(bottomCenter + Point3(0.f, canopySizeScaled.y, 0.f));
          bottomCenter.y = canopyBottom;
          pyramidPoints.push_back(bottomCenter);
          PhysConvexHullCollision pyramidCollision(pyramidPoints, false);

          const PhysCollision *selCanopyCollision = &sphCanopyCollision;
          if (treeDestr.useBoxAsCanopyCollision)
            selCanopyCollision = &boxCanopyCollision;

          bool usePyramidCanopy = riDestrData.canopyTriangle || riDestrData.branchDestr->forcePyramidCanopy;
          if (useDebugTreeInstData)
          {
            const rendinstdestr::BranchDestr &debugBranchDestr =
              from_damage ? debugTreeInstData.branchDestrFromDamage : debugTreeInstData.branchDestrOther;
            if (debugBranchDestr.forcePyramidCanopy)
              usePyramidCanopy = true;
          }
          if (usePyramidCanopy)
          {
            sphTm.setcol(3, Point3::ZERO);
            selCanopyCollision = &pyramidCollision;
            canopyMass *= 0.33f; // pyramid volume is 1/3 of the box
          }
          PhysBodyCreationData canopyPbcd;
          canopyPbcd.friction = treeDestr.treeFriction;
          canopyPbcd.rollingFriction = treeDestr.canopyRollingFriction;
          canopyPbcd.restitution = treeDestr.canopyRestitution;
          canopyPbcd.linearDamping = canopyDampingLin;
          canopyPbcd.angularDamping = canopyDampingAng;
          canopyPbcd.autoInertia = true;
          // canopy should never collide with tank or other trees
          canopyPbcd.autoMask = false;
          canopyPbcd.mask = dacoll::EPL_ALL ^ dacoll::EPL_KINEMATIC ^ dacoll::EPL_DEFAULT;

          // body
          phys.additionalBody = new PhysBody(physWorld, canopyMass, selCanopyCollision, fallDirTm * sphTm, canopyPbcd);
        }

        // joint
        {
          TMatrix physBodyTm, canopyBodyTm;
          phys.physBody->getTmInstant(physBodyTm);
          phys.additionalBody->getTmInstant(canopyBodyTm);
          physBodyTm.setcol(3, phys.physBody->getCenterOfMassPosition());
          canopyBodyTm.setcol(3, phys.additionalBody->getCenterOfMassPosition());

          TMatrix jointTm = canopyImpulseDirTm;
          jointTm.setcol(3, tmNoScale.getcol(3));
          jointTm.col[3].y += (canopyCenterHt - canopySizeScaled.y * 0.5f);

          TMatrix body0Frame = inverse(physBodyTm) * jointTm;
          TMatrix body1Frame = inverse(canopyBodyTm) * jointTm;

          PhysJoint *joint = physWorld->create6DofSpringJoint(phys.physBody, phys.additionalBody, body0Frame, body1Frame);
          phys.joints.push_back() = joint;
          Phys6DofSpringJoint *dofSpring = Phys6DofSpringJoint::cast(joint);
          if (dofSpring)
          {
            const float cmLen = length(sphTm.getcol(3));
            const float stiffness = (mass + canopyMass * cmLen * 3.f) * canopyStiffnessMult;
            const float angularLimit = canopyJointAngLimit;
            dofSpring->setLimit(0, Point2(0.f, 0.f));
            dofSpring->setLimit(1, Point2(0.f, 0.f));
            dofSpring->setLimit(2, Point2(0.f, 0.f));
            dofSpring->setSpring(3, true, stiffness, canopySpringDampingAng, Point2(-angularLimit, angularLimit));
            dofSpring->setLimit(4, Point2(0.f, 0.f));
            dofSpring->setSpring(5, true, stiffness, canopySpringDampingAng, Point2(-angularLimit, angularLimit));
            for (int i = 0; i < 6; ++i)
            {
#if defined(USE_BULLET_PHYSICS)
              dofSpring->setParam(BT_CONSTRAINT_STOP_ERP, 0.2, i);
#endif
              dofSpring->setAxisDamping(i, canopyJointDamping);
            }
            dofSpring->setEquilibriumPoint();
          }

          if (riDestrData.branchDestr->bendStrength > 0.0f && cutType == rendinst::TreeInstData::CutType::None)
          {
            // Bend helper body
            {
              PhysSphereCollision sphBendHelperShape(0.5f);
              TMatrix bendHelperTm = TMatrix::IDENT;
              bendHelperTm.setcol(3, Point3(0, totalTreeHeight, 0));

              PhysBodyCreationData bendHelperPbcd;
              bendHelperPbcd.linearDamping = treeDestr.bendLinDamping;
              bendHelperPbcd.angularDamping = treeDestr.bendAngDamping;
              bendHelperPbcd.autoInertia = true;
              // disable all collisions (TODO: see if ground collision is needed)
              bendHelperPbcd.autoMask = false;
              bendHelperPbcd.mask = 0;

              // body
              phys.bendHelperBody =
                new PhysBody(physWorld, treeDestr.bendMass, &sphBendHelperShape, fallDirTm * bendHelperTm, bendHelperPbcd);
            }

            // bend helper joint
            {
              TMatrix physBodyTm, bendHelperBodyTm;
              phys.physBody->getTmInstant(physBodyTm);
              phys.bendHelperBody->getTmInstant(bendHelperBodyTm);
              physBodyTm.setcol(3, phys.physBody->getCenterOfMassPosition());
              bendHelperBodyTm.setcol(3, phys.bendHelperBody->getCenterOfMassPosition());

              TMatrix jointTm = canopyImpulseDirTm;
              jointTm.setcol(3, tmNoScale.getcol(3));
              jointTm.col[3].y += totalTreeHeight;

              TMatrix body0Frame = inverse(physBodyTm) * jointTm;
              TMatrix body1Frame = inverse(bendHelperBodyTm) * jointTm;

              PhysJoint *joint = physWorld->create6DofSpringJoint(phys.physBody, phys.bendHelperBody, body0Frame, body1Frame);
              phys.joints.push_back() = joint;
              Phys6DofSpringJoint *dofSpring = Phys6DofSpringJoint::cast(joint);
              if (dofSpring)
              {
                const float stiffness = treeDestr.bendSpringStiffness;
                dofSpring->setSpring(0, true, stiffness, 0.0f, Point2::ZERO);
                dofSpring->setLimit(1, Point2::ZERO);
                dofSpring->setSpring(2, true, stiffness, 0.0f, Point2::ZERO);

                dofSpring->setSpring(3, true, stiffness, 0.0f, Point2::ZERO);
                dofSpring->setLimit(4, Point2::ZERO);
                dofSpring->setSpring(5, true, stiffness, 0.0f, Point2::ZERO);
                dofSpring->setEquilibriumPoint();
              }
            }
          }
        }
        phys.lastValidTm = tmNoScale;
      }

      if (constrained_phys && (cutType == rendinst::TreeInstData::CutType::Bottom || cutType == rendinst::TreeInstData::CutType::None))
      {
        PhysJoint *joint = physWorld->create6DofJoint(phys.physBody, NULL, inverse(phys.centerOfMassTm), TMatrix::IDENT);
        phys.joints.push_back() = joint;
        Phys6DofJoint *dofJoint = Phys6DofJoint::cast(joint);
        if (dofJoint)
        {
          for (int i = 0; i < 6; ++i)
          {
            dofJoint->setLimit(i, Point2(0.0f, 0.0f));
            dofJoint->setAxisDamping(i, groundJointDamping);
          }
          dofJoint->setLimit(1, Point2(0.0f, groundJointMoveDistance));

          if (cutType == rendinst::TreeInstData::CutType::Bottom)
          {
            dofJoint->setLimit(3, Point2(-groundJointAngleLimitBottom * DEG_TO_RAD, groundJointAngleLimitBottom * DEG_TO_RAD));
            dofJoint->setLimit(5, Point2(-groundJointAngleLimitBottom * DEG_TO_RAD, groundJointAngleLimitBottom * DEG_TO_RAD));
          }
          else
          {
            dofJoint->setLimit(3, Point2(-groundJointAngleLimitWhole * DEG_TO_RAD, groundJointAngleLimitWhole * DEG_TO_RAD));
            dofJoint->setLimit(4, Point2(-FLT_MAX, FLT_MAX));
            dofJoint->setLimit(5, Point2(-groundJointAngleLimitWhole * DEG_TO_RAD, groundJointAngleLimitWhole * DEG_TO_RAD));
          }

#if defined(USE_BULLET_PHYSICS)
          for (int i = 0; i < 6; ++i)
            dofJoint->setParam(BT_CONSTRAINT_STOP_ERP, 0.2, i);
#endif

          phys.groundJointLimitsEnabled = true;
          phys.groundJointExists = true;
        }
      }

      float impulseOffsetY = localImpactPos.y;
      if (phys.treeInstData.cutType == rendinst::TreeInstData::CutType::Top)
        impulseOffsetY = -trunkShapeHeight * 0.5f;
      else if (phys.treeInstData.cutType == rendinst::TreeInstData::CutType::Bottom)
        impulseOffsetY = trunkShapeHeight * 0.5f;

      const Point3 impulsePos = tmNoScale * (phys.centerOfMassTm.getcol(3) + Point3(0.0f, impulseOffsetY, 0.0f));
      debugTreeInstData.impulsePositions.push_back(impulsePos);
      phys.physBody->addImpulse(impulsePos, Point3::x0y(impulseDir) * impulseVal);

      if (phys.additionalBody && (cutType == rendinst::TreeInstData::CutType::Top || cutType == rendinst::TreeInstData::CutType::None))
      {
        // canopy impulse
        // omega = vel / arm
        // vel = omega * arm
        // imp = vel * mass
        // imp = omega * arm * mass
        // Try to just not add impulse to the canopy separately
        float additionalImpulseVal = omega * canopySizeScaled.y * 0.5f * canopyMass * canopyImpulseMul;
        G_ASSERTF(!check_nan(additionalImpulseVal) && additionalImpulseVal < MAX_RI_DESTROY_IMPULSE, "additionalImpulseVal %f",
          additionalImpulseVal);
        phys.additionalBody->addImpulse((tmNoScale * sphTm).getcol(3), additionalImpulseVal * Point3::x0y(impulseDir));
      }

      phys.treeInstData.impactXZ = impulseDir * vel;

      if (enableBranchDestr)
      {
        phys.treeInstData.branchDestr = *riDestrData.branchDestr;
        if (!rendinst::fillTreeInstData(desc, from_damage, phys.treeInstData))
          phys.treeInstData.branchDestr.enableBranchDestruction = false;
        else
          debugTreeInstData.last_object_was_from_damage = from_damage;
      }
      phys.lastValidTm = tmNoScale;

      if (!constrained_phys && coll_info && (coll_info->destrFxId >= 0 || !coll_info->destrFxTemplate.empty()) && ri_effect_cb)
        ri_effect_cb(coll_info->destrFxId, TMatrix::IDENT, tmNoScale, coll_info->desc.pool, false, nullptr,
          coll_info->destrFxTemplate.c_str());
    }
    else
    {
      float hh = height * 0.5f;
      TMatrix cotm = TMatrix::IDENT;
      cotm.m[3][1] = hh;
      phys.riColObj = dacoll::add_dynamic_cylinder_collision(cotm, radius, height, /*uptr*/ nullptr, /*add_to_world*/ false);
      dacoll::obj_apply_impulse(phys.riColObj, impulse * mass, Point3(0.f, hh, 0.f));
    }

    if (create_destr && !create_phys && ri_effect_cb)
    {
      if (riDestrData.fxType != -1 || *riDestrData.fxTemplate != 0)
      {
        TMatrix fxTm = tmNoScale;
        fxTm *= riDestrData.fxScale;
        fxTm.setcol(3, tmNoScale.getcol(3));
        ri_effect_cb(riDestrData.fxType, TMatrix::IDENT, fxTm, desc.pool, false, nullptr, riDestrData.fxTemplate);
      }
    }
  }
  return offsetedDesc;
}

rendinst::RendInstDesc rendinstdestr::create_tree_rend_inst_destr(const rendinst::RendInstDesc &desc, bool add_restorable,
  const Point3 &impactPos, const Point3 &_impulse, bool create_phys, bool constrained_phys, float wanted_omega, float at_time,
  const rendinst::CollisionInfo *coll_info, bool create_destr, bool from_damage)
{
  rendinst::RiDestrData riDestrData = rendinst::gather_ri_destr_data(desc, from_damage);
  if (!riDestrData.branchDestr)
    return desc;
  if (!get_destr_settings().createTreeDestr)
    create_destr = false;

  bool newPhysics = riDestrData.branchDestr->newPhysics;
#if TREE_DESTR_DEBUG
  if (useDebugTreeInstData)
    newPhysics = from_damage ? debugTreeInstData.branchDestrFromDamage.newPhysics : debugTreeInstData.branchDestrOther.newPhysics;
#endif

  // at_time is not reliable, in some cases it's set to zero or very high values
  at_time = g_get_current_time_cb ? g_get_current_time_cb() : 0.0f;

  if (newPhysics)
    return create_tree_rend_inst_destr_new(desc, add_restorable, impactPos, _impulse, create_phys, constrained_phys, wanted_omega,
      at_time, coll_info, create_destr, from_damage);
  else
    return create_tree_rend_inst_destr_old(desc, add_restorable, impactPos, _impulse, create_phys, constrained_phys, wanted_omega,
      at_time, coll_info, create_destr, from_damage);
}

void rendinstdestr::perform_delayed_destruction(int quota_usec)
{
  G_ASSERT(is_main_thread());
  TIME_PROFILE(doDeferredRiDestruction);
  const auto start_time = ref_time_ticks();

  if (!g_delayed_net_destr_list.list.empty())
  {
    rendinst::ScopedRIExtraReadLock riVersionLock;
    uint64_t riExtraGlobalVersion = rendinst::getRIExtraGlobalWorldVersion(true, false);
    if (g_delayed_net_destr_list.isForcedUpdatePending || g_delayed_net_destr_list.lastUpdatedWorldVersion != riExtraGlobalVersion)
      do_delayed_ri_extra_destruction_impl(); // will set lastUpdatedWorldVersion and isForcedUpdatePending
  }

  while (!g_deferred_ri_destr_list.list.empty())
  {
    if (quota_usec > 0 && get_time_usec(start_time) > quota_usec)
      return;

    const auto data = g_deferred_ri_destr_list.list.back();
    g_deferred_ri_destr_list.list.pop_back();
    rendinst::CollisionInfo collInfo(data.riDesc);
    collInfo = rendinst::getRiGenDestrInfo(data.riDesc);
    rendinstdestr::destroyRendinst(data.riDesc, true, data.pos, data.impulse, data.atTime, &collInfo, data.createDestr, nullptr, -1,
      1.f, nullptr, data.flags);
  }
}

void rendinstdestr::doRIExtraDamageInBox(const BBox3 &box, float at_time, bool create_destr, const Point3 &view_pos,
  calc_expl_damage_cb calc_expl_dmg_cb, const BSphere3 *check_sphere, const TMatrix *check_itm, rendinst::DestrOptionFlags flags)
{
  G_ASSERT(is_main_thread());
  TIME_PROFILE(rendinstdestr__doRIExtraDamageInBox);

  rendinst::riex_collidable_t ri_h;
  rendinst::gatherRIGenExtraCollidable(ri_h, box, true /*read_lock*/);
  bool tooManyDestructables = ri_h.size() >= destructables::maxNumberOfDestructableBodies;

  if (create_destr && tooManyDestructables)
  {
    float riDamageRadiusSq = (box.center() - box.lim[0]).lengthSq();
    if (riDamageRadiusSq >= destructables::minDestrRadiusSq)
    {
      Point3 dirToExplosion = box.center() - view_pos;
      if (dirToExplosion.lengthSq() > riDamageRadiusSq + destructables::minDestrRadiusSq)
        create_destr = false;
    }
  }

  const bool isForced = bool(flags & rendinst::DestrOptionFlag::ForceDestroy);
  for (int j = 0; j < ri_h.size(); ++j)
  {
    rendinst::RendInstDesc riDesc(-1, rendinst::handle_to_ri_inst(ri_h[j]), rendinst::handle_to_ri_type(ri_h[j]), 0, 0);
    if (riDesc.pool >= 0 && rendinst::get_riextra_immortality(ri_h[j]) && !isForced)
      continue;
    rendinst::CollisionInfo collInfo(riDesc);
    collInfo = rendinst::getRiGenDestrInfo(riDesc);

    if (!(collInfo.isDestr || isForced) || collInfo.riPoolRef < 0 || collInfo.tm == TMatrix::ZERO)
      continue;

    Point3 riPos = collInfo.tm.getcol(3);
    if (check_sphere)
    {
      const BBox3 riBBox = collInfo.tm * collInfo.localBBox;
      if (!(*check_sphere & riBBox))
        continue;
    }
    if (check_itm)
    {
      BBox3 checkBox(Point3(-0.5f, 0.f, -0.5f), Point3(0.5f, 1.f, 0.5f));
      Point3 p = *check_itm * riPos;
      p.y = 0.5f;
      if (!(checkBox & p))
        continue;
    }

    if (calc_expl_dmg_cb)
    {
      TIME_PROFILE(rendinstdestr__doRIExtraDamageInBox_calc_expl_dmg);

      float distance = sqrt(point_to_box_distance_sq(inverse(collInfo.tm) * box.center(), collInfo.localBBox));
      float damage = calc_expl_dmg_cb(collInfo, distance);
      if (damage == 0.f)
        continue;
      if (collInfo.hp > 0.f && !rendinst::applyDamageRIGenExtra(collInfo.desc, damage))
        continue;
    }
    bool local_create_destr = false;
    if (create_destr && (!tooManyDestructables || ((view_pos - riPos).lengthSq() < destructables::minDestrRadiusSq)))
      local_create_destr = true;

    rendinst::DestrOptionFlags curFlags = flags;
    if (collInfo.isDestr)
      curFlags |= rendinst::DestrOptionFlag::ForceDestroy;

    if (g_deferred_ri_destr_list.isEnabled)
    {
      auto &data = g_deferred_ri_destr_list.list.push_back();
      data.riDesc = riDesc;
      data.pos = box.center();
      data.impulse = ZERO<Point3>();
      data.atTime = at_time;
      data.createDestr = local_create_destr;
      data.flags = curFlags;
    }
    else
    {
      rendinstdestr::destroyRendinst(riDesc, true, box.center(), ZERO<Point3>(), at_time, &collInfo, local_create_destr, NULL, -1, 1.f,
        nullptr, curFlags);
    }
  }
}

void rendinstdestr::remove_ri_without_collision_in_radius(const Point3 &pos, float radius)
{
  BBox3 box(pos, radius * 2);
  rendinst::hideRIGenExtraNotCollidable(box, false);

  struct ForeachCB final : public rendinst::ForeachCB
  {
    eastl::fixed_vector<rendinst::RendInstDesc, 32, true /* bEnableOverflow */> riDescToRemove;

    void executeForTm(RendInstGenData *, const rendinst::RendInstDesc &ri_desc, const TMatrix &) override
    {
      if (!rendinst::ri_gen_has_collision(ri_desc))
        riDescToRemove.push_back(ri_desc);
    }

    void executeForPos(RendInstGenData *, const rendinst::RendInstDesc &ri_desc, const TMatrix &) override
    {
      if (!rendinst::ri_gen_has_collision(ri_desc))
        riDescToRemove.push_back(ri_desc);
    }
  } cb;

  rendinst::foreachRIGenInSphere(BSphere3(pos, radius), rendinst::GatherRiTypeFlag::RiGenOnly, cb);

  for (const rendinst::RendInstDesc &desc : cb.riDescToRemove)
  {
    rendinst::DestroyedRi *destroyedRi = rendinst::doRIGenExternalControl(desc, true);
    rendinst::removeRIGenExternalControl(desc, destroyedRi);
  }
}

#if TREE_DESTR_DEBUG
namespace rendinstdestr
{
static void imguiWindow()
{
  static bool isPlaying = false;

  rendinstdestr::BranchDestr &branchDestr =
    debugTreeInstData.last_object_was_from_damage ? debugTreeInstData.branchDestrFromDamage : debugTreeInstData.branchDestrOther;

  if (ImGui::Checkbox("Enable Branch Destruction", &get_tree_destr_mutable().branchDestrFromDamage.enableBranchDestruction))
    get_tree_destr_mutable().branchDestrOther.enableBranchDestruction =
      get_tree_destr_mutable().branchDestrFromDamage.enableBranchDestruction;

  ImGui::Checkbox("Use Debug Values", &useDebugTreeInstData);
  ImGui::Checkbox("Use New Physics", &branchDestr.newPhysics);
  if (ImGui::Button("Reset Debug Values"))
  {
    debugTreeInstData.timer_offset = 0.0f;
    branchDestr = get_tree_destr_mutable().branchDestrFromDamage;
  }

  static float maxTime = 3.0f;
  if (ImGui::Button(isPlaying ? "Stop" : "Play"))
    isPlaying = !isPlaying;

  if (isPlaying)
  {
    debugTreeInstData.timer_offset += ImGui::GetIO().DeltaTime;
    if (debugTreeInstData.timer_offset > maxTime)
      debugTreeInstData.timer_offset = 0.0f;
  }

  ImGui::SliderFloat("Timer Offset", &debugTreeInstData.timer_offset, -2.0f, maxTime, "%0.2f");
  ImGui::SliderFloat("Timer Max (For Play)", &maxTime, 0.0, 15.0f, "%0.2f");
  ImGui::Spacing();
  ImGui::SliderFloat("Impulse Mul", &branchDestr.impulseMul, 0, 15.0f, "%0.2f");

  ImGui::SliderFloat("Impulse Min", &branchDestr.impulseMin, 0, 100.0f, "%0.1f", ImGuiSliderFlags_Logarithmic);
  ImGui::SliderFloat("Impulse Max", &branchDestr.impulseMax, 0, 100.0f, "%0.1f", ImGuiSliderFlags_Logarithmic);
  ImGui::SliderFloat("Start keeping bigger branches than", &branchDestr.branchSizeMin, 0, 100.0f, "%0.2f",
    ImGuiSliderFlags_Logarithmic);
  ImGui::SliderFloat("Keep all branches bigger than", &branchDestr.branchSizeMax, 0, 100.0f, "%0.2f", ImGuiSliderFlags_Logarithmic);
  ImGui::SliderFloat("Rotate Speed X", &branchDestr.rotateRandomSpeedMulX, 0, 10.0f, "%0.2f", ImGuiSliderFlags_Logarithmic);
  ImGui::SliderFloat("Rotate Speed Y", &branchDestr.rotateRandomSpeedMulY, 0, 10.0f, "%0.2f", ImGuiSliderFlags_Logarithmic);
  ImGui::SliderFloat("Rotate Speed Z", &branchDestr.rotateRandomSpeedMulZ, 0, 10.0f, "%0.2f", ImGuiSliderFlags_Logarithmic);
  ImGui::SliderFloat("Angle Spread", &branchDestr.rotateRandomAngleSpread, 0, 10.0f, "%0.2f", ImGuiSliderFlags_Logarithmic);
  ImGui::SliderFloat("Branch Size Slow Down", &branchDestr.branchSizeSlowDown, 0, 1.0f, "%0.3f", ImGuiSliderFlags_Logarithmic);
  ImGui::SliderFloat("Falling Speed", &branchDestr.fallingSpeedMul, 0, 2.0f, "%0.3f", ImGuiSliderFlags_Logarithmic);
  ImGui::SliderFloat("Falling Speed Rnd", &branchDestr.fallingSpeedRnd, 0, 1.0f, "%0.3f", ImGuiSliderFlags_Logarithmic);
  ImGui::SliderFloat("Horizontal Speed", &branchDestr.horizontalSpeedMul, 0, 20.0f, "%0.3f", ImGuiSliderFlags_Logarithmic);
  ImGui::SliderFloat("Fall Through Ground If Bigger", &branchDestr.fallThroughGroundIfBigger, 0, 20.0f, "%0.3f",
    ImGuiSliderFlags_Logarithmic);
  ImGui::SliderFloat("Bend Strength", &branchDestr.bendStrength, 0, 0.2f, "%0.3f", ImGuiSliderFlags_Logarithmic);
  ImGui::Checkbox("Force Pyramid Canopy", &branchDestr.forcePyramidCanopy);
  ImGui::Checkbox("Disable cutting", &debugTreeInstData.disableCut);
  ImGui::Checkbox("Use Cut Height Override", &debugTreeInstData.useCutHeightOverride);
  ImGui::SliderFloat("Cut Height", &debugTreeInstData.cutHeightOverride, 0, 20.0f, "%0.3f", ImGuiSliderFlags_Logarithmic);
  ImGui::SliderFloat("Cut Damage Threshold", &branchDestr.cutInHalfDamageThreshold, -1, 2000.0f, "%0.1f",
    ImGuiSliderFlags_Logarithmic);

  float lastImpulse = -1.0f;
  float chance = 0.0f;
  if (riPhys.size())
  {
    lastImpulse = riPhys[riPhys.size() - 1].treeInstData.impactXZ.length();
    chance =
      clamp((lastImpulse - branchDestr.impulseMin) / max(branchDestr.impulseMax - branchDestr.impulseMin, 0.01f), 0.0f, 1.0f) * 100.0f;
  }

  ImGui::Text("Last impulse: %.2f. Chance: %.2f%% (%s)", lastImpulse, chance,
    debugTreeInstData.last_object_was_from_damage ? "From damage" : "Non-damage");
  ImGui::Text("Cut tree count: %d / %d", currentCutTreeCount, get_tree_destr().maxCutTreeCount);

  auto fnActivateTreeDestroyTool = [](bool activate, float damage, float radius, bool weaponDamage) {
    char cmd[100]; // Create a buffer to store the formatted string
    snprintf(cmd, sizeof(cmd), "dm.ri_damage %.2f %.2f %s", activate ? damage : 0.0f, radius, weaponDamage ? "true" : "false");
    console::command(cmd);
  };

  static bool showPhysicsShapes = false;
  if (ImGui::Checkbox("Show Physics Shapes", &showPhysicsShapes))
  {
    char cmd[100]; // Create a buffer to store the formatted string
    snprintf(cmd, sizeof(cmd), "physWorld.debug_draw %s", showPhysicsShapes ? "true" : "false");
    console::command(cmd);
  }

  static bool treeDestroyToolActive = false;
  static bool treeDestroyToolWeaponDamage = true;
  static float treeDestroyDamage = 500.0f;
  static float treeDestroyRadius = 0.0f;
  ImGui::SliderFloat("Destroy Damage", &treeDestroyDamage, 0, 5000, "%0.0f", ImGuiSliderFlags_Logarithmic);
  if (ImGui::Checkbox("Tree destroy with weapon damage", &treeDestroyToolWeaponDamage))
  {
    if (treeDestroyToolActive)
      fnActivateTreeDestroyTool(true, treeDestroyDamage, treeDestroyRadius, treeDestroyToolWeaponDamage);
  }

  static bool treeDestroyInArea = false;
  if (ImGui::Checkbox("Tree destroy in area", &treeDestroyInArea))
  {
    treeDestroyRadius = treeDestroyInArea ? 50.0f : 0.0f;
    fnActivateTreeDestroyTool(true, treeDestroyDamage, treeDestroyRadius, treeDestroyToolWeaponDamage);
  }

  if (ImGui::Checkbox("Tree destroy tool", &treeDestroyToolActive))
  {
    fnActivateTreeDestroyTool(treeDestroyToolActive, treeDestroyDamage, treeDestroyRadius, treeDestroyToolWeaponDamage);
  }

  if (ImGui::Button("Restore trees"))
  {
    for (int i = 0; i < riPhys.size(); ++i)
    {
      RendInstPhys &phys = riPhys[i];
      if (rendinst::returnRIGenExternalControl(phys.desc, phys.ri))
      {
        cleanup_ri_phys(phys);
        erase_items(riPhys, i, 1);
        i--;
        continue;
      }
    }
  }
}

REGISTER_IMGUI_WINDOW("Render", "Tree Destruction", imguiWindow);
} // namespace rendinstdestr

static bool ridestr_console_handler(const char *argv[], int argc)
{
  int found = 0;
  CONSOLE_CHECK_NAME("ridestr", "mass_destroy", 1, 3)
  {
    // Usage: ridestr.mass_destroy [radius=50] [probability=0.1]
    // Destroys destructible riExtra instances around the camera with the given probability.
    if (!get_current_camera_pos)
    {
      console::print_d("ridestr.mass_destroy: no camera pos callback");
      return true;
    }
    float radius = argc >= 2 ? console::to_real(argv[1]) : 50.f;
    float probability = argc >= 3 ? clamp(console::to_real(argv[2]), 0.f, 1.f) : 0.1f;

    if (phys_world)
      phys_world->fetchSimRes(true);

    Point3 camPos = get_current_camera_pos();
    BSphere3 sphere(camPos, radius);

    rendinst::riex_collidable_t handles;
    rendinst::gatherRIGenExtraCollidable(handles, sphere, true);

    int destroyed = 0;
    int total = 0;
    for (int i = 0; i < handles.size(); ++i)
    {
      rendinst::RendInstDesc desc(handles[i]);
      rendinst::CollisionInfo collInfo = rendinst::getRiGenDestrInfo(desc);
      if (!collInfo.isDestr)
        continue;
      total++;
      if (rnd_float(0.f, 1.f) > probability)
        continue;
      rendinstdestr::destroyRendinst(desc, true, collInfo.tm.getcol(3), ZERO<Point3>(), 0.f, &collInfo, true, nullptr, -1, 1.f,
        nullptr, rendinst::DestrOptionFlag::AddDestroyedRi | rendinst::DestrOptionFlag::ForceDestroy);
      destroyed++;
    }
    console::print_d("ridestr.mass_destroy: destroyed %d / %d RIs with debris in radius %.0f (p=%.0f%%)", destroyed, total, radius,
      probability * 100.f);
    return true;
  }
  return found;
}
REGISTER_CONSOLE_HANDLER(ridestr_console_handler);

#endif // TREE_DESTR_DEBUG
