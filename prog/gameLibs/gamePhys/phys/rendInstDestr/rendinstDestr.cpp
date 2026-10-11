// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <gamePhys/phys/rendinstDestr.h>
#include <rendInst/rendInstExtra.h>
#include <rendInst/rendInstAccess.h>
#include <rendInst/rendInstExtraAccess.h>
#include <rendInst/rendInstGenDamageInfo.h>
#include <rendInst/rendInstGenRender.h>
#include <rendInst/rendInstFx.h>
#include <rendInst/rendInstCollision.h>
#include "../../../rendInst/riGen/riGenData.h"
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
#include <math/dag_plane3.h>
#include <osApiWrappers/dag_critSec.h>
#include <osApiWrappers/dag_miscApi.h>
#include <ioSys/dag_dataBlock.h>
#include <memory/dag_framemem.h>
#include <math/dag_math3d.h>
#include <util/dag_convar.h>
#include <util/dag_stlqsort.h>
#include <util/dag_threadPool.h>
#include <generic/dag_sort.h>
#include <perfMon/dag_statDrv.h>
#include <perfMon/dag_cpuFreq.h>
#include <debug/dag_debug3d.h>
#include <gui/dag_visualLog.h>
#include <util/dag_string.h>
#include <util/dag_finally.h>
#include <scene/dag_occlusion.h>
#include <util/dag_console.h>

#include "frxDestr.h"
#include "netDestrSync.h"
#include "riDestrInternal.h"
#include "riDestrProps.h"
#include "treeDestr.h"


#if DAGOR_DBGLEVEL > 0
namespace rendinstdestr
{
CONSOLE_BOOL_VAL("ridestr", ri_debug_collision, false);
}
#endif

#define RIDESTR_DBG_LOG(fmt, ...)                          \
  do                                                       \
  {                                                        \
    if (rendinstdestr::ri_debug_collision) [[unlikely]]    \
    {                                                      \
      String msg(256, framemem_ptr(), fmt, ##__VA_ARGS__); \
      debug("%s", msg.c_str());                            \
      visuallog::logmsg(msg.c_str());                      \
    }                                                      \
  } while (0)

static const Point3 RI_DESTROY_BBOX_MARGIN_UP = Point3(0, 0.05, 0);

static rendinstdestr::remove_physx_collision_object_callback rem_physx_collision_obj_cb = NULL;
static rendinstdestr::create_apex_actors_callback create_apex_actors_at_point_cb = NULL;
static rendinstdestr::apex_force_remove_actor_callback apex_force_remove_actor_cb = NULL;
static rendinstdestr::on_destr_changed_callback on_changed_destr_cb = NULL;
static rendinstdestr::get_camera_pos get_current_camera_pos = NULL;
static rendinstdestr::get_current_time_callback g_get_current_time_cb = NULL;
static rendinstdestr::get_occlusion_callback get_occlusion_cb = NULL;
static dag::RelocatableFixedVector<rendinstdestr::on_rendinst_destroyed_callback, 3> on_rendinst_destroyed_callbacks;
namespace rendinstdestr
{
on_ri_restored_callback on_ri_restored_cb = NULL;
DestrSettings destrSettings;
rendinst::ri_damage_effect_cb ri_effect_cb = nullptr;
} // namespace rendinstdestr
static PhysWorld *phys_world = NULL;

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
static FixedBlockAllocator cco_alloc(eastl::max(sizeof(RendinstImpulseThresholdData), sizeof(TreeRendinstImpulseThresholdData)), 32);
static Tab<CachedCollisionObjectInfo *> cached_collision_objects(midmem);
// Kept separate from cached_collision_objects: that list is looked up by desc alone from
// several unrelated systems (projectile impact, mech walker vision) that assume any entry found
// for a desc represents a genuinely destructible RI. Mixing this immortal-bush bookkeeping into
// it would make those readers treat a decorative bush as destructible once a vehicle had touched
// it.
static Tab<NonDestrBushImpulseThresholdData *> cached_nondestr_bush_objects(midmem);

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

// One material of the node, preferring a non-default one, or PHYSMAT_INVALID when it has none.
// A fused node's set is in coverage order, so what a destructable adopts follows the material covering most faces AMONG the
// non-default ones, not the exporter's node split.
static PhysMat::MatID get_preferred_node_physmat(const CollisionResource &coll_res, const CollisionNode &node)
{
  PhysMat::MatID matId = PHYSMAT_INVALID;
  for (int i = 0, e = coll_res.getNodePhysMatCount(node.nodeIndex); i < e; ++i)
  {
    const PhysMat::MatID nodeMat = coll_res.getNodePhysMatId(node.nodeIndex, i);
    if (nodeMat < 0)
      continue;
    matId = nodeMat;
    if (matId != PHYSMAT_DEFAULT)
      break;
  }
  return matId;
}

PhysMat::MatID rendinstdestr::get_destructable_physmat_for_ri(const rendinst::RendInstDesc &desc)
{
  PhysMat::MatID matId = PHYSMAT_INVALID;
  if (const CollisionResource *collRes = rendinst::getRiGenCollisionResource(desc))
    for (const CollisionNode &node : collRes->getAllNodes())
    {
      if (!collRes->checkNodeBehaviorFlags(node.nodeIndex, CollisionNode::PHYS_COLLIDABLE))
        continue;
      const PhysMat::MatID nodeMat = get_preferred_node_physmat(*collRes, node);
      if (nodeMat < 0)
        continue;
      matId = nodeMat;
      if (matId != PHYSMAT_DEFAULT)
        break;
    }
  return matId;
}

void rendinstdestr::fill_destr_obj_params(destructables::DestructableCreationParams &params, const DestrObjProps *obj)
{
  static const DestrObjProps default_obj;
  if (!obj)
    obj = &default_obj;
  params.timeToLive = obj->timeToLive;
  params.inactiveTimeBeforeSink = obj->inactiveTimeBeforeSink;
  params.timeToKinematic = obj->timeToKinematic;
  params.timeToSinkUnderground = obj->timeToSinkUnderground;
  params.timeToStartDisintegration = obj->timeToStartDisintegration;
  params.disintegrationDuration = obj->disintegrationDuration;
  params.disintegrationScale = obj->disintegrationScale;
  params.keepAlive = obj->keepAlive;
  params.interactiveDebris = obj->interactiveDebris;
}


rendinstdestr::DestrSettings &rendinstdestr::get_mutable_destr_settings() { return destrSettings; }

const rendinstdestr::DestrSettings &rendinstdestr::get_destr_settings() { return destrSettings; }

void rendinstdestr::init_ex(rendinstdestr::on_destr_changed_callback on_destr_cb, ri_tree_sound_cb tree_sound_cb,
  get_camera_pos get_current_camera_pos_, get_current_time_callback get_current_time_cb, bool enable_branch_destruction,
  remove_physx_collision_object_callback remove_physx_obj_cb, create_apex_actors_callback create_apex_actors_cb,
  apex_force_remove_actor_callback apex_remove_actor_cb)
{
  on_changed_destr_cb = on_destr_cb;
  netsync::init_sync(true, true);
  g_deferred_ri_destr_list.isEnabled = true;
  rendinst::enable_apex = false;
  rem_physx_collision_obj_cb = remove_physx_obj_cb;
  create_apex_actors_at_point_cb = create_apex_actors_cb;
  apex_force_remove_actor_cb = apex_remove_actor_cb;
  get_current_camera_pos = get_current_camera_pos_;
  g_get_current_time_cb = get_current_time_cb;
  init_tree_destr(tree_sound_cb, enable_branch_destruction);

  init_ri_destr_props();
}

void rendinstdestr::init(rendinstdestr::on_destr_changed_callback on_destr_cb, bool apply_pending, bool enable_branch_destruction,
  ri_tree_sound_cb tree_sound_cb, get_camera_pos get_camera_pos_cb, get_current_time_callback get_current_time_cb)
{
  on_changed_destr_cb = on_destr_cb;
  netsync::init_sync(apply_pending, false);
  g_deferred_ri_destr_list.isEnabled = true;
  rendinst::enable_apex = false;

  rem_physx_collision_obj_cb = nullptr;
  create_apex_actors_at_point_cb = nullptr;
  apex_force_remove_actor_cb = nullptr;
  get_current_camera_pos = get_camera_pos_cb;
  g_get_current_time_cb = get_current_time_cb;

  init_ri_destr_props();

  init_tree_destr(tree_sound_cb, enable_branch_destruction);
}


void rendinstdestr::set_occlusion_callback(get_occlusion_callback cb) { get_occlusion_cb = cb; }

void rendinstdestr::clear()
{
  term_fracture_jobs();
  netsync::clear_sync();
  clear_tree_collision();
  clear_nondestr_bush_collision();
  clear_all_ptr_items_and_shrink(cached_collision_objects);
  clear_all_ptr_items_and_shrink(cached_nondestr_bush_objects);
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

  shutdown_ri_destr_props();
}

void rendinstdestr::startSession(void *phys_wld)
{
  phys_world = static_cast<PhysWorld *>(phys_wld);
  if (ri_destr_collision_queue_count != RI_DESTR_COLLISION_DISABLED)
    ri_destr_collision_queue_count = 0;

  netsync::start_session();
}

void rendinstdestr::endSession()
{
  clear();
  netsync::end_session();
  g_deferred_ri_destr_list.clear();
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
    if (rendinstdestr::ri_debug_collision) [[unlikely]]
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
      if (rendinstdestr::ri_debug_collision) [[unlikely]]
      {
        RIDESTR_DBG_LOG("ridestr: queue full (%d), new box not closer than furthest", queueCapacity);
        draw_debug_box_buffered(worldBox, E3DCOLOR_MAKE(255, 0, 0, 255), 3000);
      }
    }
    else
    {
      if (rendinstdestr::ri_debug_collision) [[unlikely]]
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
    if (rendinstdestr::ri_debug_collision) [[unlikely]]
    {
      RIDESTR_DBG_LOG("ridestr: queued box " FMT_P3_02F "-" FMT_P3_02F ", queue size=%d", P3D(worldBox.lim[0]), P3D(worldBox.lim[1]),
        ri_destr_collision_queue_count);
      draw_debug_box_buffered(worldBox, E3DCOLOR_MAKE(0, 255, 0, 255), 3000);
    }
  }
}

void rendinstdestr::queue_ri_collision_for_debris_if_enabled(const mat44f &inst_tm, const bbox3f &local_bbox)
{
  if (ri_destr_collision_queue_count >= 0)
    queue_ri_collision_for_debris(inst_tm, local_bbox);
}

float rendinstdestr::get_sync_time() { return g_get_current_time_cb ? g_get_current_time_cb() : 0.0f; }

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

void rendinstdestr::setApexEnabled(bool enabled) { rendinst::enable_apex = enabled; }
void rendinstdestr::resetApexDestructedRIList()
{
#if ENABLE_APEX
  if (apex_destructed_list.size())
    debug("clearing apex_destructed_list=%d", apex_destructed_list.size());
  apex_destructed_list.clear();
#endif
}

#if ENABLE_APEX
bool rendinstdestr::remove_apex_destructed_ri(const rendinst::RendInstDesc &desc, int apex_destr_id)
{
  if (rendinst::enable_apex && apex_force_remove_actor_cb && apex_destr_id >= 0)
    apex_force_remove_actor_cb(apex_destr_id);
  return apex_destructed_list.erase(desc) > 0;
}
#endif

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

    RiExtraCollisionCB(const rendinst::RendInstDesc &id, DestrNeighborsSet &dnd, int depth) :
      initiatorDesc(id), destrNeighborsData(dnd), destroyNeighbourDepth(depth)
    {}
    virtual void addCollisionCheck(const rendinst::CollisionInfo &coll_info) override
    {
      rendinst::riex_handle_t riHandle = rendinst::make_handle(coll_info.desc.pool, coll_info.desc.idx);
      if (coll_info.desc != initiatorDesc && coll_info.isDestr && !coll_info.isImmortal && coll_info.destructibleByParent &&
          strcmp(rendinst::getRIGenDestrPropsTag(initiatorDesc), rendinst::getRIGenDestrPropsDestroyedByTag(coll_info.desc)) == 0)
        if (destrNeighborsData.find(riHandle) == destrNeighborsData.end())
        {
          destrNeighborsData.insert(eastl::make_pair(riHandle, coll_info));
          findRendinstNeighbors(coll_info.desc, destroyNeighbourDepth - 1, &coll_info, destrNeighborsData);
        }
    }
    virtual void addTreeCheck(const rendinst::CollisionInfo & /*coll_info*/) override {}
  } rendinstCallback(desc, ri_to_destroy, destroy_neighbour_recursive_depth);

  auto localBBox = coll_info->localBBox;
  localBBox.lim[1] += RI_DESTROY_BBOX_MARGIN_UP;
  rendinst::testObjToRendInstIntersection(localBBox, coll_info->tm, rendinstCallback, rendinst::GatherRiTypeFlag::RiExtraOnly);
}


void rendinstdestr::fill_ri_destructable_params(destructables::DestructableCreationParams &params, const rendinst::RendInstDesc &desc,
  const DynamicPhysObjectData *po_data, const TMatrix &tm, rendinst::DestrOptionFlags flags)
{
  params.physObjData = po_data;
  params.tm = tm;
  if (get_current_camera_pos)
    params.camPos = get_current_camera_pos();
  const rendinst::riex_handle_t riexHandle = desc.getRiExtraHandle();
  params.resIdx = desc.pool;
  if (!params.hashVal)
    params.hashVal = riexHandle != rendinst::RIEX_HANDLE_NULL ? rendinst::get_riextra_instance_seed(riexHandle) : 0;
  // for compatibility reasons riGen keeps the built-in lifetime, presets apply to riExtra only
  const rendinstdestr::RiDestrProps *props = desc.isRiExtra() ? rendinstdestr::get_ri_destr_props(desc) : nullptr;
  fill_destr_obj_params(params, props ? props->destrObj : nullptr);
  params.isDestroyedByExplosion = bool(flags & rendinst::DestrOptionFlag::DestroyedByExplosion);
  if (const auto physMatId = get_destructable_physmat_for_ri(desc); physMatId != PHYSMAT_INVALID)
    params.riPhysMatId = physMatId;
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

  G_ASSERT(desc.layer >= 0);
  G_ASSERTF(lengthSq(impulse) < sqr(MAX_RI_DESTROY_IMPULSE), "Bad destroy rendInst impulse %@", impulse);

#if ENABLE_APEX
  if (rendinst::enable_apex && rem_physx_collision_obj_cb)
  {
    TIME_PROFILE(destroyRI_rem_physx_collision_obj_cb);
    rem_physx_collision_obj_cb(desc);
  }
#endif

  rendinst::RendInstBufferData riBuffer;
  G_VERIFY(rendinst::fillRendInstBufferData(desc, riBuffer)); // should never fail, as RI is checked to exist
  TMatrix mainTm;
  v_mat_43cu_from_mat44(mainTm.array, riBuffer.tm);
  const DynamicPhysObjectData *poData = rendinst::getRIGenDestroyedPhysRes(desc);
  const uint32_t riexInstanceSeed = desc.isRiExtra() ? riBuffer.riExUserData[0] : 0;

  dacoll::invalidate_ri_instance(desc); // do before destring, otherwise in riextra we'll lose unique data and will not be able to
                                        // compare them

  if (on_destr_cb)
    on_destr_cb(desc);
  if (on_changed_destr_cb)                                                 // call it before data will be destroyed
    on_changed_destr_cb(desc, mainTm, pos, impulse, create_destr_effects); // TODO: always mainTm==riTm?


  rendinst::RendInstDesc offsetedDesc = rendinst::get_restorable_desc(desc); // Desc with offs set to restorable_desc.offs
  offsetedDesc.idx = desc.idx;
  const bool canAddRestorable = add_restorable && offsetedDesc.isValid() && rendinstdestr::can_sync_ri_destr(offsetedDesc) &&
                                rendinstdestr::get_destr_settings().isNetClient && coll_info;

  const BBox3 bbox = flags & rendinst::DestrOptionFlag::UseFullBbox ? rendinst::getRIGenFullBBox(desc)
                     : coll_info                                    ? coll_info->localBBox
                                                                    : rendinst::getRIGenBBox(desc);
  // visual consumers span the drawn crown whatever box the destroying path asked for
  const BBox3 fullBBox = flags & rendinst::DestrOptionFlag::UseFullBbox ? bbox : rendinst::getRIGenFullBBox(desc);

  // dynamic RI destruction is handled fully inside callbacks, so exit for dynamic RIs
  {
    const int32_t userData = coll_info ? coll_info->userData : -1;
    if (rendinst::doDynamicRIExtraDestr(desc, create_destr_effects, userData, impulse, pos))
    {
      rendinstdestr::call_on_rendinst_destroyed_cb(desc.getRiExtraHandle(), mainTm, bbox, fullBBox);
      if (desc.isRiExtra() && offsetedDesc.isValid())
        rendinstdestr::netsync::call_restorable_rendinst_cb(offsetedDesc, rendinstdestr::RRS_DESTROYED);
      return offsetedDesc;
    }
  }

  // call restored cb BEFORE destroying, so it will be called, before handle is invalidated
  if (canAddRestorable)
    rendinstdestr::netsync::call_restorable_rendinst_cb(offsetedDesc, rendinstdestr::RRS_CREATED);
  rendinst::riex_handle_t createdDestroyedRiexHandle = rendinst::RIEX_HANDLE_NULL; // New riex replacing destroyed

  // Rendinst system can invalidate static shadows for very large cells in `rendinst::doRIGenDestr`. But it is not
  // needed if we have special callback with invalidation for only instance bbox (called below).
  // Save/restore instead of set/reset because we can be called inside an already guarded scope (tree destr).
  const bool prevAvoidStaticShadowRecalc = rendinst::render::avoidStaticShadowRecalc;
  if (rendinstdestr::get_destr_settings().hasStaticShadowsInvalidationCallback)
    rendinst::render::avoidStaticShadowRecalc = true;
  const bool outRiRemoved = rendinst::doRIGenDestr(desc, createdDestroyedRiexHandle, flags);

  rendinst::render::avoidStaticShadowRecalc = prevAvoidStaticShadowRecalc;
  // if not destroyed - call restored cb to rollback previous call
  if (!outRiRemoved)
    rendinstdestr::netsync::call_restorable_rendinst_cb(offsetedDesc, rendinstdestr::RRS_RESTORED);

  if (outRiRemoved && create_destr_effects)
    rendinst::playRIGenDestrEffect(riBuffer, rendinstdestr::ri_effect_cb, pos != Point3::ZERO ? &pos : nullptr);

  bool isDestructableCreated = false;

  if (outRiRemoved && create_destr_effects && rendinstdestr::get_destr_settings().createDestr)
    isDestructableCreated = rendinstdestr::frx_preview_intercept_destruction(riBuffer, createdDestroyedRiexHandle, pos) ||
                            rendinstdestr::perform_procedural_destruction(riBuffer, pos, impulse);

  bool apex_asset_created = false;
  int apex_destructible_id = -1;
#if ENABLE_APEX
  if (!isDestructableCreated && rendinst::enable_apex && create_destr_effects && rendinstdestr::get_destr_settings().createDestr &&
      create_apex_actors_at_point_cb)
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
    isDestructableCreated = apex_asset_created;
  }
#else
  G_UNUSED(apex_dmg_info);
  G_UNUSED(apex_asset_created);
#endif

#if ENABLE_APEX
  // Only call on_rendinst_destroyed_cb if there is no apex asset since apex has its own callback
  if (!apex_asset_created)
#endif
    rendinstdestr::call_on_rendinst_destroyed_cb(desc.getRiExtraHandle(), mainTm, bbox, fullBBox);

  destructables::id_t destrId = destructables::INVALID_ID;
  if (!isDestructableCreated && poData && create_destr_effects && rendinstdestr::get_destr_settings().createDestr) //-V560
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
      destrId = destructables::addDestructable(&destr, eastl::move(params), phys_world);
      if (destr) //-V1051
      {
        if (impulse.lengthSq() > 0.f)
          destr->applyInitialImpulse(*phys_world, mainTm, bbox, pos, impulse);
      }
    }
  }

  if (canAddRestorable && outRiRemoved)
    rendinstdestr::netsync::add_restorable(desc, offsetedDesc, riBuffer, destrId, at_time, coll_info, createdDestroyedRiexHandle,
      apex_destructible_id, mainTm, pos, impulse);
  else if (desc.isRiExtra() && offsetedDesc.isValid())
    rendinstdestr::netsync::call_restorable_rendinst_cb(offsetedDesc, rendinstdestr::RRS_DESTROYED);

  return offsetedDesc;
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
  rendinst::RendInstDesc desc(riex_handle);
  const DynamicPhysObjectData *poData = rendinst::getRIGenDestroyedPhysRes(desc);

  if (ri_effect_cb && create_destr_effects)
    if (rendinst::RendInstBufferData buffer; rendinst::fillRendInstBufferData(desc, buffer))
      rendinst::playRIGenDestrEffect(buffer, ri_effect_cb, /* coll point */ nullptr);

  if (poData != nullptr && rendinstdestr::get_destr_settings().createDestr && !isDestructableTmNaN)
  {
    // Queue nearby RI collision loading so debris can collide with standing rendinsts
    if (ri_destr_collision_queue_count >= 0)
      queue_ri_collision_for_debris(tm44, rendinst::riex_get_lbb(rendinst::handle_to_ri_type(riex_handle)));

    destructables::DestructableCreationParams params;
    fill_ri_destructable_params(params, rendinst::RendInstDesc(riex_handle), poData, transform, flags);

    gamephys::DestructableObject *destr = nullptr;
    destructables::addDestructable(&destr, eastl::move(params), phys_world);
    if (destr && impulse.lengthSq() > 0.f)
    {
      G_ASSERTF(lengthSq(impulse) < sqr(MAX_RI_DESTROY_IMPULSE), "Bad destroy rendInst impulse %@", impulse);
      destr->applyInitialImpulse(*phys_world, transform, rendinst::getRIGenBBox(desc), impulse_pos, impulse);
    }
  }
}

void rendinstdestr::update_paused(float current_time, const Frustum *frustum) { update(0.f, current_time, frustum); }

void rendinstdestr::update(float dt, float current_time, const Frustum *frustum)
{
  TIME_PROFILE(rendinstdestr_update);

  update_fracture_jobs();

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
    for (auto it = cached_nondestr_bush_objects.begin(); it != cached_nondestr_bush_objects.end();)
      if ((*it)->update(dt))
        ++it;
      else
      {
        auto cco = *it;
        it = cached_nondestr_bush_objects.erase_unsorted(it);
        delete cco;
      }
  }

  update_ri_phys(dt, current_time, frustum);

  netsync::update_restorables(dt);
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
  bool create_destr_effects, on_riextra_destroyed_callback riex_destr_cb, riextra_should_damage should_damage)
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
        1.f, onDestroyCb);
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

void rendinstdestr::set_on_ri_restored_cb(on_ri_restored_callback cb) { on_ri_restored_cb = cb; }

void rendinstdestr::call_on_rendinst_destroyed_cb(rendinst::riex_handle_t handle, const TMatrix &tm, const BBox3 &box,
  const BBox3 &full_bbox)
{
  for (on_rendinst_destroyed_callback cb : on_rendinst_destroyed_callbacks)
    cb(handle, tm, box, full_bbox);
}

void rendinstdestr::set_ri_damage_effect_cb(rendinst::ri_damage_effect_cb cb) { ri_effect_cb = cb; }

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

static NonDestrBushImpulseThresholdData *get_cached_nondestr_bush_collision_object(const rendinst::RendInstDesc &ri_desc)
{
  for (int i = 0; i < cached_nondestr_bush_objects.size(); ++i)
    if (cached_nondestr_bush_objects[i]->riDesc == ri_desc)
      return cached_nondestr_bush_objects[i];
  return nullptr;
}

CachedCollisionObjectInfo *rendinstdestr::get_or_add_cached_nondestr_bush_collision_object(const rendinst::RendInstDesc &ri_desc,
  float impulse, float at_time)
{
  NonDestrBushImpulseThresholdData *objInfo = get_cached_nondestr_bush_collision_object(ri_desc);
  if (!objInfo)
  {
    void *p = cco_alloc.allocateOneBlock();
    objInfo = new (p, _NEW_INPLACE) NonDestrBushImpulseThresholdData(impulse, ri_desc, at_time);
    cached_nondestr_bush_objects.push_back(objInfo);
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

void rendinstdestr::perform_delayed_destruction(int quota_usec)
{
  G_ASSERT(is_main_thread());
  TIME_PROFILE(doDeferredRiDestruction);
  const auto start_time = ref_time_ticks();

  netsync::perform_delayed_destruction();

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
  calc_expl_damage_cb_t calc_expl_dmg_cb, const BSphere3 *check_sphere, const TMatrix *check_itm, rendinst::DestrOptionFlags flags)
{
  G_ASSERT(is_main_thread());
  TIME_PROFILE(rendinstdestr__doRIExtraDamageInBox);

  rendinst::riex_collidable_t ri_h;
  rendinst::gatherRIGenExtraCollidable(ri_h, box, true /*read_lock*/);
  bool tooManyDestructables = ri_h.size() >= destructables::get_config().maxNumberOfDestructableBodies;

  if (create_destr && tooManyDestructables)
  {
    float riDamageRadiusSq = (box.center() - box.lim[0]).lengthSq();
    if (riDamageRadiusSq >= destructables::get_config().minDestrRadiusSq)
    {
      Point3 dirToExplosion = box.center() - view_pos;
      if (dirToExplosion.lengthSq() > riDamageRadiusSq + destructables::get_config().minDestrRadiusSq)
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
    if (create_destr && (!tooManyDestructables || ((view_pos - riPos).lengthSq() < destructables::get_config().minDestrRadiusSq)))
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
    rendinst::delRIGen(desc, false);
}

#if DAGOR_DBGLEVEL > 0
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

#endif
