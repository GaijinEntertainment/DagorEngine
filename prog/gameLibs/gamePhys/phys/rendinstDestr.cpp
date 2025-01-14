// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <gamePhys/phys/rendinstDestr.h>
#include <rendInst/rendInstExtra.h>
#include <rendInst/rendInstAccess.h>
#include <rendInst/rendInstExtraAccess.h>
#include <rendInst/rendInstGenDamageInfo.h>
#include <rendInst/rendInstGenRender.h>
#include <rendInst/rendInstFx.h>
#include <rendInst/rendInstCollision.h>
#include <rendInst/rendInstAccess.h>
#include <EASTL/hash_set.h>
#include <EASTL/bitset.h>
#include <EASTL/array.h>
#include <EASTL/vector_set.h>

#include <phys/dag_physDecl.h>
#include <phys/dag_physics.h>

#include <gamePhys/phys/destructableObject.h>
#include <gamePhys/phys/dynamicPhysModel.h>
#include <gamePhys/phys/rendinstPhys.h>
#include <gamePhys/collision/collisionLib.h>
#include <gamePhys/collision/rendinstCollision.h>
#include <gamePhys/collision/rendinstCollisionUserInfo.h>
#include <gamePhys/collision/contactResultWrapper.h>
#include <gamePhys/collision/cachedCollisionObject.h>
#include <math/dag_vecMathCompatibility.h>
#include <math/random/dag_random.h>
#include <math/dag_TMatrix4.h>
#include <math/dag_mathUtils.h>
#include <osApiWrappers/dag_critSec.h>
#include <osApiWrappers/dag_miscApi.h>
#include <ioSys/dag_dataBlock.h>
#include <memory/dag_framemem.h>
#include <math/dag_math3d.h>
#include <math/dag_vecMathCompatibility.h>
#include <util/dag_convar.h>
#include <util/dag_stlqsort.h>
#include <generic/dag_sort.h>
#include <perfMon/dag_statDrv.h>
#include <perfMon/dag_cpuFreq.h>

#if DAGOR_DBGLEVEL > 0
#include <imgui/imgui.h>
#include <gui/dag_imgui.h>
#endif

CONSOLE_BOOL_VAL("ridestr", ri_debug_collision, false);

static constexpr int INITIAL_REPLICATION = 1;
static const Point3 RI_DESTROY_BBOX_MARGIN_UP = Point3(0, 0.05, 0);
static const float MAX_RI_DESTROY_IMPULSE = 1e8f;

static bool apply_reflection = false;
static bool apply_pending_destrs = false;
static rendinstdestr::create_tree_rend_inst_destr_cb create_tree_cb = NULL;
static rendinstdestr::remove_tree_rendinst_destr_cb rem_tree_cb = NULL;
static rendinstdestr::remove_physx_collision_object_callback rem_physx_collision_obj_cb = NULL;
static rendinstdestr::create_apex_actors_callback create_apex_actors_at_point_cb = NULL;
static rendinstdestr::apex_force_remove_actor_callback apex_force_remove_actor_cb = NULL;
static rendinstdestr::on_destr_changed_callback on_changed_destr_cb = NULL;
static rendinstdestr::get_camera_pos get_current_camera_pos = NULL;
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

// deferred destr to remove lag spikes
struct DeferredRiDestrData
{
  rendinst::RendInstDesc riDesc;
  Point3 pos;
  float at_time;
  rendinst::DestrOptionFlags flags;
  bool local_create_destr;
};
struct DeferredRendInstDestrList
{
  bool isEnabled;
  dag::Vector<DeferredRiDestrData> list;
  void clear() { clear_and_shrink(list); }
};
static DeferredRendInstDestrList g_deferred_ri_destr_list;

static Tab<RendInstPhys> riPhys(midmem);
static Tab<CachedCollisionObjectInfo *> cached_collision_objects(midmem);
static CollisionObject tree_collision;
static void do_delayed_ri_extra_destruction_impl();

static bool useDebugTreeInstData;
static rendinst::TreeInstDebugData debugTreeInstData;

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

struct NetSyncData
{
  dag::Vector<int> serverToClientPoolRemap;
  dag::Vector<int> clientToServerPoolRemap;
  eastl::vector_map<uint32_t, int> hashToClientPoolId;
  eastl::vector_map<uint32_t, int> pendingServerPoolIds;
  eastl::bitvector<> pendingServerPoolsBitVec;
  eastl::vector_set<int> allPoolsToSync;
  eastl::vector_set<int> newPoolsToSync;

  void clear()
  {
    serverToClientPoolRemap.clear();
    clientToServerPoolRemap.clear();
    allPoolsToSync.clear();
    newPoolsToSync.clear();
    hashToClientPoolId.clear();
    pendingServerPoolIds.clear();
    pendingServerPoolsBitVec.clear();
    debug("[ri destr sync] all pool sync data cleared");
  }
};
static NetSyncData g_pool_sync;

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
    rendinst::riex_handle_t h = rendinst::restoreRiGenDestr(riDesc, riBuffer);
    VERBOSE_SYNC("restored ri (likely no sync received) desc:" FMT_DESC_STR " restored:%i", FMT_DESC_V(riDesc),
      h != rendinst::RIEX_HANDLE_NULL);
    rendinst::delRIGenExtra(generatedHandle);
    destructables::removeDestructableById(destrId);
    if (rem_tree_cb)
      rem_tree_cb(riDesc);
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

void rendinstdestr::init_ex(rendinstdestr::on_destr_changed_callback on_destr_cb, create_tree_rend_inst_destr_cb create_tree_destr_cb,
  remove_tree_rendinst_destr_cb rem_tree_destr_cb, ri_tree_sound_cb tree_sound_cb, get_camera_pos get_current_camera_pos_,
  remove_physx_collision_object_callback remove_physx_obj_cb, create_apex_actors_callback create_apex_actors_cb,
  apex_force_remove_actor_callback apex_remove_actor_cb)
{
  on_changed_destr_cb = on_destr_cb;
  apply_pending_destrs = true;
  g_delayed_net_destr_list.isEnabled = true;
  g_deferred_ri_destr_list.isEnabled = true;
  rendinst::enable_apex = false;
  create_tree_cb = create_tree_destr_cb;
  rem_tree_cb = rem_tree_destr_cb;
  g_tree_sound_cb = tree_sound_cb;
  rem_physx_collision_obj_cb = remove_physx_obj_cb;
  create_apex_actors_at_point_cb = create_apex_actors_cb;
  apex_force_remove_actor_cb = apex_remove_actor_cb;
  get_current_camera_pos = get_current_camera_pos_;
  rendinst::registerRIGenExtraInvalidateHandleCb(invalidate_handle_cb);
  rendinst::do_delayed_ri_extra_destruction = do_delayed_ri_extra_destruction_impl;
  debugTreeInstData.branchDestrFromDamage = get_tree_destr().branchDestrFromDamage;
  debugTreeInstData.branchDestrOther = get_tree_destr().branchDestrOther;
}

void rendinstdestr::init(rendinstdestr::on_destr_changed_callback on_destr_cb, bool apply_pending,
  create_tree_rend_inst_destr_cb create_tree_destr_cb, remove_tree_rendinst_destr_cb rem_tree_destr_cb, ri_tree_sound_cb tree_sound_cb,
  get_camera_pos get_camera_pos_cb)
{
  on_changed_destr_cb = on_destr_cb;
  rendinst::enable_apex = false;

  apply_pending_destrs = apply_pending;

  create_tree_cb = create_tree_destr_cb;
  rem_tree_cb = rem_tree_destr_cb;
  g_tree_sound_cb = tree_sound_cb;
  rem_physx_collision_obj_cb = nullptr;
  create_apex_actors_at_point_cb = nullptr;
  apex_force_remove_actor_cb = nullptr;
  get_current_camera_pos = get_camera_pos_cb;

  debugTreeInstData.branchDestrFromDamage = get_tree_destr().branchDestrFromDamage;
  debugTreeInstData.branchDestrOther = get_tree_destr().branchDestrOther;
}


void rendinstdestr::clear()
{
  restorables.clear();
  restorables.shrink_to_fit();
  clear_synced_ri_extra_pools();
  clear_tree_collision();
  clear_all_ptr_items(cached_collision_objects);
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
  if (destroy_neighbour_recursive_depth <= 0 || !coll_info || !coll_info->isParent || !phys_world || !rendinst::isRiGenDescValid(desc))
    return;

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
      if (coll_info.desc != initiatorDesc && coll_info.isDestr && !coll_info.isImmortal && coll_info.destructibleByParent)
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
  rendinst::testObjToRIGenIntersection(localBBox, coll_info->tm, rendinstCallback, rendinst::GatherRiTypeFlag::RiExtraOnly);
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
  params.isDestroyedByExplosion = bool(flags & rendinst::DestrOptionFlag::DestroyedByExplosion);
}

static rendinst::RendInstDesc destroyRendinstInternal(rendinst::RendInstDesc desc, bool add_restorable, const Point3 &pos,
  const Point3 &impulse, float at_time, const rendinst::CollisionInfo *coll_info, bool create_destr_effects,
  ApexDmgInfo *apex_dmg_info, rendinstdestr::on_destr_callback on_destr_cb, rendinst::DestrOptionFlags flags)
{
  TIME_PROFILE(ridestr__destroyRendinstInternal);
  // Note: 'desc' is copied by value in order to avoid situations where desc = &coll_info->desc and
  // 'on_destr_cb' invalidates it.
  const bool isGenDescValid = rendinst::isRiGenDescValid(desc);
  if ((add_restorable && !isGenDescValid) || !phys_world) // desc is valid only when we provide it in non-offseted
                                                          // state. otherwise we process tons of destroyRendinsts
    return desc;

  G_ASSERTF(lengthSq(impulse) < sqr(MAX_RI_DESTROY_IMPULSE), "Bad destroy rendInst impulse %@", impulse);

#if ENABLE_APEX
  if (rendinst::enable_apex && rem_physx_collision_obj_cb)
  {
    TIME_PROFILE(destroyRI_rem_physx_collision_obj_cb);
    rem_physx_collision_obj_cb(desc);
  }
#endif

  TMatrix mainTm = TMatrix::IDENT;
  if (add_restorable)
    mainTm = rendinst::getRIGenMatrix(desc);
  else
    mainTm = rendinst::getRIGenMatrixDestr(desc);

  rendinst::RendInstBufferData riBuffer;
  DynamicPhysObjectData *poData = NULL;
  rendinst::RendInstDesc offsetedDesc;
  if (add_restorable || isGenDescValid)
  {
    offsetedDesc = rendinst::get_restorable_desc(desc); // Desc with offs set to restorable_desc.offs
    offsetedDesc.idx = desc.idx;
  }
  const bool canAddRestorable = add_restorable && offsetedDesc.isValid() && (offsetedDesc.isRiExtra() || offsetedDesc.layer == 0) &&
                                rendinstdestr::get_destr_settings().isNetClient && coll_info;

  rendinst::riex_handle_t createdDestroyedRiexHandle = rendinst::RIEX_HANDLE_NULL; // New riex replacing destroyed
  dacoll::invalidate_ri_instance(desc); // do before destring, otherwise in riextra we'll lose unique data and will not be able to
                                        // compare them

  if (on_destr_cb)
    on_destr_cb(desc);
  if (on_changed_destr_cb)                           // call it before data will be destroyed
    on_changed_destr_cb(desc, mainTm, pos, impulse); // TODO: always mainTm==riTm?

  int32_t userData = coll_info ? coll_info->userData : -1;
  bool outRiRemoved = false;

  uint32_t riexInstanceSeed = 0;
  if (desc.isRiExtra())
  { // get seed before node destruction (doRiGenDestr), destructible creation params need it
    rendinst::riex_handle_t riexHandle = rendinst::make_handle(desc.pool, desc.idx);
    riexInstanceSeed = riexHandle != rendinst::RIEX_HANDLE_NULL ? rendinst::get_riextra_instance_seed(riexHandle) : 0;
  }

  if (desc.isRiExtra() && desc.isDynamicRiExtra() && coll_info)
    rendinst::onRiExtraDestruction(rendinst::make_handle(desc.pool, desc.idx), true, create_destr_effects, userData, impulse, pos);
  else if (add_restorable)
  {
    if (canAddRestorable)
      call_restorable_rendinst_cb(offsetedDesc, rendinstdestr::RRS_CREATED); // call restored cb BEFORE destroying, so it will be
                                                                             // called, before handle is invalidated
    const Point3 *collPoint = (pos == ZERO<Point3>()) ? nullptr : &pos;
    poData = rendinst::doRIGenDestr(desc, riBuffer, create_destr_effects, create_destr_effects ? ri_effect_cb : nullptr,
      createdDestroyedRiexHandle, userData, collPoint, &outRiRemoved, flags, impulse, pos);
    if (canAddRestorable && !outRiRemoved)
      call_restorable_rendinst_cb(offsetedDesc, rendinstdestr::RRS_RESTORED); // not destroyed - call restored cb to rollback previous
                                                                              // call
  }
  else
    poData = rendinst::doRIGenDestrEx(desc, create_destr_effects, create_destr_effects ? ri_effect_cb : nullptr, userData);

  BBox3 bbox = flags & rendinst::DestrOptionFlag::UseFullBbox ? rendinst::getRIGenFullBBox(desc)
               : coll_info                                    ? coll_info->localBBox
                                                              : rendinst::getRIGenBBox(desc);

  bool apex_asset_created = false;
  int apex_destructible_id = -1;
#if ENABLE_APEX
  if (rendinst::enable_apex && create_destr_effects && create_apex_actors_at_point_cb)
  {
    TMatrix normalizedTm = mainTm;
    Point3 scale = Point3(mainTm.getcol(0).length(), mainTm.getcol(1).length(), mainTm.getcol(2).length());

    normalizedTm.orthonormalize();
    if (apex_destructed_list.find(desc) == apex_destructed_list.end()) // skip duplicates from duplicated server packets
    {
      TIME_PROFILE(destroyRI_create_apex_actors_at_point_cb);
      apex_destructible_id = create_apex_actors_at_point_cb(getRIGenDestrName(desc), normalizedTm, scale, pos, impulse, desc.pool,
        apex_dmg_info, desc.idx, bbox);
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
    destructables::DestructableCreationParams params;
    params.hashVal = riexInstanceSeed;
    rendinstdestr::fill_ri_destructable_params(params, desc, poData, mainTm, flags);
    gamephys::DestructableObject *destr = nullptr;
    destrId = destructables::addDestructable(&destr, params, phys_world);
    if (destr) //-V1051
    {
      if (impulse.lengthSq() > 0.f)
        destr->addImpulse(*phys_world, pos, impulse);
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
    restorables.emplace_back(offsetedDesc, riBuffer, destrId, at_time, tmp, createdDestroyedRiexHandle, apex_destructible_id);
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
  if ((add_restorable && !rendinst::isRiGenDescValid(desc)) || !phys_world) // desc is valid only when we provide it in non-offseted
                                                                            // state. otherwise we process tons of destroyRendinsts
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
  if (DynamicPhysObjectData *poData = rendinst::doRIExGenDestrEx(riex_handle, create_destr_effects ? ri_effect_cb : nullptr);
      poData != nullptr && rendinstdestr::get_destr_settings().createDestr)
  {
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

void rendinstdestr::update_paused(const Frustum *frustum)
{
  if (useDebugTreeInstData)
    update(0.f, frustum);
}

void rendinstdestr::update(float dt, const Frustum *frustum)
{
  TIME_PROFILE(rendinstdestr_update);

  {
    TIME_PROFILE_DEV(update_cached_collision_objects);
    for (int i = cached_collision_objects.size() - 1; i >= 0; --i)
      if (!cached_collision_objects[i]->update(dt))
        erase_ptr_items(cached_collision_objects, i, 1);
  }

  PhysWorld *physWorld = dacoll::get_phys_world();
  physWorld->fetchSimRes(true);

  {
    TIME_PROFILE_DEV(riPhys);

    struct RendInstUpdateTm
    {
      bool moved;
      rendinst::riex_handle_t id;
      TMatrix tm;
      TMatrix originalTm;
      rendinst::TreeInstData treeInstData;
    };

    eastl::vector<RendInstUpdateTm, framemem_allocator> updateList;
    updateList.reserve(riPhys.size());
    for (int i = 0; i < riPhys.size(); ++i)
    {
      RendInstPhys &phys = riPhys[i];
      phys.treeInstData.timer += dt;

      bool physObject = phys.physModel->physType == gamephys::DynamicPhysModel::E_PHYS_OBJECT;
      const bool physObjectActive =
        physObject && (phys.physBody && phys.physBody->isActive() && phys.physBody->getInteractionLayer()) ||
        (phys.physBody && phys.additionalBody && phys.additionalBody->isActive() && phys.additionalBody->getInteractionLayer());

      bool actived = phys.ttl >= 0.f;
      if (physObject)
      {
        if (physObjectActive)
        {
          if (lengthSq(phys.physBody->getVelocity()) > sqr(20.f) || phys.maxTtl < 0.f)
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
      if (actived && physObject && phys.distConstrainedPhys > 0.f && phys.physBody && phys.physBody->getInteractionLayer())
      {
        const float distSq = lengthSq(phys.originalTm.getcol(3) - phys.lastValidTm.getcol(3));
        if (distSq > sqr(phys.distConstrainedPhys))
        {
          phys.distConstrainedPhys = -1.f;
          phys.physBody->setGroupAndLayerMask(dacoll::EPL_DEFAULT, dacoll::EPL_ALL ^ dacoll::EPL_KINEMATIC);
          if (phys.additionalBody)
            phys.additionalBody->setGroupAndLayerMask(dacoll::EPL_DEFAULT, dacoll::EPL_ALL ^ dacoll::EPL_KINEMATIC);
        }
      }
      phys.maxTtl -= dt;
      if (phys.ttl < 0.f)
      {
        if (!physObject)
        {
          if (rendinst::returnRIGenExternalControl(phys.desc, phys.ri))
          {
            phys.cleanup();
            erase_items(riPhys, i, 1);
            i--;
            continue;
          }
        }
        else if (actived && phys.physBody && phys.physBody->getInteractionLayer())
        {
          phys.physBody->setGroupAndLayerMask(0, 0);
          phys.physBody->setGravity(Point3(0.f, -0.1f, 0.f));
          phys.physBody->activateBody(true);
          if (phys.treeSound.inited())
            phys.treeSound.setFalled(phys.lastValidTm);
          clear_all_ptr_items_and_shrink(phys.joints);
          del_it(phys.additionalBody);
          phys.ttl = 15.f;
          phys.maxTtl = 15.f;
          phys.treeInstData.disappearStartTime = phys.treeInstData.timer;
        }
        else if (phys.physBody)
        {
          if (rendinst::removeRIGenExternalControl(phys.desc, phys.ri))
          {
            phys.cleanup();
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
          if (ri_debug_collision)
            dacoll::draw_phys_body(phys.physBody);
        }
        TMatrix additionalTm = TMatrix::IDENT;
        if (phys.additionalBody)
        {
          phys.additionalBody->getTm(additionalTm);
          if (ri_debug_collision)
            dacoll::draw_phys_body(phys.additionalBody);
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
      }
      else
      {
        phys.physModel->location.toTM(tm);
        phys.riColObj.body->setTmInstant(tm);
      }

      mat44f m44;
      v_mat44_make_from_43cu_unsafe(m44, tm.array);
      v_mat43_apply_scale(m44, v_ldu(&phys.scale.x));

      BBox3 bbox = rendinst::getRIGenBBox(phys.desc);
      bbox3f fullBBox;
      v_bbox3_init(fullBBox, m44, v_ldu_bbox3(bbox));
      if (frustum && !frustum->testBoxB(fullBBox.bmin, fullBBox.bmax))
        continue;
      RendInstUpdateTm &destrPos = updateList.push_back();
      destrPos.treeInstData = phys.treeInstData;
      destrPos.originalTm = phys.originalTm;
      destrPos.id = phys.ri->riHandle;
      v_mat_43cu_from_mat44(destrPos.tm.array, m44);
      destrPos.moved = physObject;
    }
    if (!updateList.empty())
    {
      TIME_PROFILE(rendinstDestrTmUpdate);

      vec3f frustumPoints[8];
      frustum->generateAllPointFrustm(frustumPoints);
      Point3 nearPlaneCenter = (as_point3(&frustumPoints[2]) + as_point3(&frustumPoints[4])) * 0.5f;

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
        rendinst::moveRIGenExtra43(l.id, m43, l.moved, true);

        if ((l.originalTm.col[3] - nearPlaneCenter).lengthSq() < sqr(l.treeInstData.branchDestr.maxVisibleDistance))
          rendinst::updateTreeDestrRenderData(l.originalTm, l.id, l.treeInstData, useDebugTreeInstData ? &debugTreeInstData : nullptr);
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

static bool destroy_rend_inst_from_net(rendinst::RendInstDesc &restorable_desc, bool has_impulse, const Point3 &local_impulse_pos,
  const Point3 &impulse_vec, bool create_destr)
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
    ok = false;
    const int idx = rendinst::find_restorable_data_index(restorable_desc);
    if (idx >= 0)
    {
      restorable_desc.idx = idx;
      ok = true;
    }
  }
  if (ok)
  {
    if (rendinst::isRIGenPosInst(restorable_desc))
    {
      int poolIdxBasedSeed = restorable_desc.offs + (restorable_desc.pool) ^ (restorable_desc.cellIdx < 16);
      float angle = _srnd(poolIdxBasedSeed) * PI;
      if (create_tree_cb)
      {
        VERBOSE_SYNC("destroying rendinst from net (tree), desc:" FMT_DESC_STR, FMT_DESC_V(restorable_desc));
        float s, c;
        sincos(angle, s, c);
        create_tree_cb(restorable_desc, false, local_impulse_pos, Point3(c, 0.f, s), true, true, 0.5f, 0.f, NULL, create_destr, false);
      }
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
      rendinstdestr::destroyRendinst(restorable_desc, /*add_restorable*/ false, impulsePos, impulse_vec, /*at_time*/ 0.f,
        /*coll_info*/ nullptr, rendinstdestr::get_destr_settings().createDestr && create_destr);
    }
  }
  return ok;
}

bool rendinstdestr::deserialize_destr_update(const danet::BitStream &bs)
{
  if (!apply_reflection)
  {
    pending_destrs_updates.Write(bs);
    return false;
  }
  VERBOSE_SYNC("deserializing destruction update");

  uint8_t readCount = 0;
  eastl::bitset<256, uint32_t> haveImpulseMask;
  TempDelayedDestructionList delayedRiExtraDestruction;

  uint8_t updateSize = 0;
  bs.Read(updateSize);
  bs.ReadArray((uint8_t *)haveImpulseMask.data(), (updateSize + 7) / CHAR_BIT);
  uint16_t cellCount = 0;
  bs.ReadCompressed(cellCount);
  for (int i = 0; i < cellCount; i++)
  {
    bool shouldUpdateVb = false;

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

      const bool isPoolSyncPending = cellIdx < 0 && is_server_ri_pool_sync_pending(int(srvPoolIdx));
      const uint16_t poolIdx =
        cellIdx < 0 && !isPoolSyncPending ? uint16_t(get_client_ri_pool_id(int(srvPoolIdx))) : uint16_t(srvPoolIdx);

      // logerr("Read pool %i destrs %i", poolIdx, destrCount);
      uint32_t offs = 0;
      for (int k = 0; k < destrCount; k++)
      {
        uint32_t offsDiffDivided = 0;
        G_VERIFY(bs.ReadCompressed(offsDiffDivided));
        offs += offsDiffDivided << shift;

        rendinstdestr::QuantizedDestrImpulsePos qpos;
        rendinstdestr::QuantizedDestrImpulseVec qimp;
        Point3 localImpulsePos = Point3::ZERO, impulseVec = Point3::ZERO;
        bool haveImpulse = haveImpulseMask[readCount];
        if (haveImpulse)
        {
          bs.ReadBits((uint8_t *)&qpos.qpos, rendinstdestr::QuantizedDestrImpulsePos::XYZBits);
          bs.ReadBits((uint8_t *)&qimp.qpos, rendinstdestr::QuantizedDestrImpulseVec::XYZBits);
          localImpulsePos = qpos.unpackPos();
          impulseVec = qimp.unpackPos();
        }
        // logerr("Read destr %i %u %u imp %i", int16_t(cellIdx), uint16_t(poolIdx), offs, haveImpulse);

        readCount++;
        rendinst::RendInstDesc desc(cellIdx, 0 /*instIdx*/, poolIdx, offs, 0 /*layer*/);
        if ((isPoolSyncPending || !destroy_rend_inst_from_net(desc, haveImpulse, localImpulsePos, impulseVec,
                                    /* create_destr */ --simultaneous_destrs_count >= 0)) &&
            g_delayed_net_destr_list.isEnabled)
          delayedRiExtraDestruction.emplace_back(desc, isPoolSyncPending);
        shouldUpdateVb = true;
      }
    }

    if (shouldUpdateVb && cellIdx >= 0)
      rendinst::updateRiGenVbCell(0, cellIdx);
  }

  flush_temp_delayed_destruction_list(eastl::move(delayedRiExtraDestruction));

  // logerr("Deserialized %i destrs, %i bytes", readCount, bs.GetNumberOfBytesUsed());
  G_ASSERT(readCount == updateSize);
  return true;
}


bool rendinstdestr::deserialize_destr_data(const danet::BitStream &bs, int apply_flags, int max_simultaneous_destrs)
{
  // Temporary structure to keep it
  Tab<rendinst::DestroyedCellData> cellsNewDestrInfo(framemem_ptr());
  int newDestrs = 0;

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

        const bool isPoolSyncPending = cellId < 0 && is_server_ri_pool_sync_pending(int(srvPoolIdx));
        uint16_t poolIdx = cellId < 0 && !isPoolSyncPending ? uint16_t(get_client_ri_pool_id(int(srvPoolIdx))) : uint16_t(srvPoolIdx);
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
                  newDestrs += (addedRange->endOffs - addedRange->startOffs) / stride;

                  // Start new one
                  addedRange = &cellsNewDestrInfo[i].destroyedPoolInfo[j].destroyedInstances.push_back();
                  addedRange->startOffs = range.endOffs;
                  addedRange->endOffs = endOffs;
                  newDestrs += (addedRange->endOffs - addedRange->startOffs) / stride;
                  lastIdx++; // continue to next one
                }
                else if (endOffs >= range.startOffs)
                {
                  // Just finish prev one and break;
                  addedRange->endOffs = range.startOffs;
                  newDestrs += (addedRange->endOffs - addedRange->startOffs) / stride;
                  addedRange = NULL;
                  finished = true;
                  break;
                }
                else
                {
                  // It's before start, so just finish it and break;
                  addedRange->endOffs = endOffs;
                  newDestrs += (addedRange->endOffs - addedRange->startOffs) / stride;
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
                    newDestrs += (addedRange->endOffs - addedRange->startOffs) / stride;
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
              newDestrs += (endOffs - startOffs) / stride;
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
            newDestrs += (endOffs - startOffs) / stride;
          }
        }
      }
    }
    return true;
  });

  newDestrs = ((apply_flags & INITIAL_REPLICATION) != 0) ? 0 : min(newDestrs, max_simultaneous_destrs);
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
          if ((isPoolSyncPending || !destroy_rend_inst_from_net(desc, false, Point3::ZERO, Point3::ZERO,
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

  flush_temp_delayed_destruction_list(eastl::move(delayedRiExtraDestruction));
  return true;
}

static void do_delayed_ri_extra_destruction_impl()
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
      if (rendinstdestr::is_server_ri_pool_sync_pending(destr.desc.pool))
      {
        ++it;
        continue;
      }
      const int prevPoolIdx = destr.desc.pool;
      G_UNUSED(prevPoolIdx);
      destr.desc.pool = rendinstdestr::get_client_ri_pool_id(destr.desc.pool);
      VERBOSE_SYNC("synced pool for delayed net destr (%i->%i), desc:" FMT_DESC_STR, prevPoolIdx, int(destr.desc.pool),
        FMT_DESC_V(destr.desc));
      destr.isPoolSyncPending = false;
    }

    // try destroy, skip, if failed, otherwise erase
    if (destroy_rend_inst_from_net(destr.desc, false, Point3::ZERO, Point3::ZERO, --simultaneous_destrs_count >= 0))
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
    rendinst::ScopedRIExtraReadLock riVersionLock;
    g_delayed_net_destr_list.lastUpdatedWorldVersion = version;
    g_delayed_net_destr_list.isForcedUpdatePending = false;
  }
}


static void add_synced_ri_extra_pool_remap(int server_idx, int client_idx)
{
  if (g_pool_sync.serverToClientPoolRemap.size() <= server_idx)
    g_pool_sync.serverToClientPoolRemap.resize(server_idx + 8, -1);
  g_pool_sync.serverToClientPoolRemap[server_idx] = client_idx;
  if (g_pool_sync.clientToServerPoolRemap.size() <= client_idx)
    g_pool_sync.clientToServerPoolRemap.resize(client_idx + 8, -1);
  g_pool_sync.clientToServerPoolRemap[client_idx] = server_idx;
  g_delayed_net_destr_list.isForcedUpdatePending = true;
  VERBOSE_SYNC("    synced pool %i to server pool %i", client_idx, server_idx);
}

static void add_synced_ri_extra_pool_impl(int pool_idx, const char *name)
{
  G_ASSERT_RETURN(name != nullptr, );
  const uint32_t hash = str_hash_fnv1<32>(name);
  VERBOSE_SYNC("add client pool %i %s %08x", pool_idx, name, hash);
  // check if already registered
  if (const auto it = g_pool_sync.hashToClientPoolId.find(hash); it != g_pool_sync.hashToClientPoolId.end())
  {
    G_ASSERTF(it->second == pool_idx, "synced ri extra pool hash collision %i %i %s %u", pool_idx, it->second, name, hash);
    return;
  }
  // check if was synced from server
  if (const auto it = g_pool_sync.pendingServerPoolIds.find(hash); it != g_pool_sync.pendingServerPoolIds.end())
  {
    add_synced_ri_extra_pool_remap(it->second, pool_idx);
    VERBOSE_SYNC("    remove pending server pool %i", it->second);
    g_pool_sync.pendingServerPoolsBitVec.set(it->second, false);
    g_pool_sync.pendingServerPoolIds.erase(it);
  }
  // add
  g_pool_sync.hashToClientPoolId.emplace(hash, pool_idx);
  G_VERIFY(g_pool_sync.allPoolsToSync.insert(pool_idx).second);
  g_pool_sync.newPoolsToSync.insert(pool_idx);
}

void rendinstdestr::clear_synced_ri_extra_pools() { g_pool_sync.clear(); }

void rendinstdestr::sync_all_ri_extra_pools()
{
  rendinst::iterateRIExtraMap([](int id, const char *name) { add_synced_ri_extra_pool_impl(id, name); });
}

void rendinstdestr::sync_ri_extra_pool(int pool_id) { add_synced_ri_extra_pool_impl(pool_id, rendinst::getRIGenExtraName(pool_id)); }

int get_ri_pool_id_remap_impl(const dag::Vector<int> &remap, int pool_id)
{
  if (unsigned(pool_id) < remap.size())
    if (const int id = remap[pool_id]; id != -1)
      return id;
  return pool_id;
}

int rendinstdestr::get_client_ri_pool_id(int server_pool_id)
{
  return get_ri_pool_id_remap_impl(g_pool_sync.serverToClientPoolRemap, server_pool_id);
}

int rendinstdestr::get_server_ri_pool_id(int client_pool_id)
{
  return get_ri_pool_id_remap_impl(g_pool_sync.clientToServerPoolRemap, client_pool_id);
}

bool rendinstdestr::is_server_ri_pool_sync_pending(int server_pool_id)
{
  return g_pool_sync.pendingServerPoolsBitVec.test(server_pool_id, false);
}

bool rendinstdestr::serialize_synced_ri_extra_pools(danet::BitStream &bs, bool full, bool skip_if_no_data)
{
  dag::ConstSpan<int> pools = full ? make_span_const(g_pool_sync.allPoolsToSync) : make_span_const(g_pool_sync.newPoolsToSync);
  if (skip_if_no_data && pools.empty())
    return false;
  bs.Write(full);
  bool writeIndices = false;
  for (int i = 0; i < pools.size(); i++)
    writeIndices |= i != pools[i];
  bs.Write(writeIndices);
  bs.AlignWriteToByteBoundary();
  bs.WriteCompressed(int(pools.size()));
  int lastPoolIdx = -1;
  for (const int pool : pools)
  {
    const char *name = rendinst::getRIGenExtraName(pool);
    const uint32_t hash = str_hash_fnv1<32>(name);
    bs.Write(hash);
    G_ASSERT(lastPoolIdx < pool);
    if (writeIndices)
      bs.WriteCompressed(int(pool - (lastPoolIdx + 1)));
    lastPoolIdx = pool;
  }
  if (!full)
    g_pool_sync.newPoolsToSync.clear();
  return true;
}

bool rendinstdestr::deserialize_synced_ri_extra_pools(const danet::BitStream &bs)
{
  bool ok = true;
  bool isFullData = false;
  bool hasIndices = false;
  int count = 0;
  ok &= bs.Read(isFullData);
  ok &= bs.Read(hasIndices);
  bs.AlignReadToByteBoundary();
  ok &= bs.ReadCompressed(count);
  if (ok && isFullData)
  {
    g_pool_sync.serverToClientPoolRemap.clear();
    g_pool_sync.clientToServerPoolRemap.clear();
    g_pool_sync.pendingServerPoolIds.clear();
    g_pool_sync.pendingServerPoolsBitVec.clear();
  }
  VERBOSE_SYNC("received %s ri pool sync count=%i", isFullData ? "full" : "partial", count);
  int lastPoolIdx = -1;
  for (int i = 0; i < count; i++)
  {
    uint32_t hash = 0;
    ok &= bs.Read(hash);
    int delta = 0;
    if (hasIndices)
      ok &= bs.ReadCompressed(delta);
    if (!ok)
      break;
    const int pool = lastPoolIdx + 1 + delta;
    lastPoolIdx = pool;
    VERBOSE_SYNC("  server pool %i %08x", pool, hash);
    if (const auto it = g_pool_sync.hashToClientPoolId.find(hash); it != g_pool_sync.hashToClientPoolId.end())
      add_synced_ri_extra_pool_remap(pool, it->second);
    else
    {
      g_pool_sync.pendingServerPoolIds.emplace(hash, pool);
      g_pool_sync.pendingServerPoolsBitVec.set(pool, true);
      VERBOSE_SYNC("    add pending server pool %i", pool);
    }
  }
  return ok;
}


bool rendinstdestr::apply_damage_to_riextra(rendinst::riex_handle_t handle, float dmg, const Point3 &pos, const Point3 &impulse,
  float at_time)
{
  if (!rendinst::isRiGenExtraValid(handle))
    return false;
  bool isDestr = false;
  apply_damage_to_ri(rendinst::RendInstDesc(handle), dmg, 0.f, pos, impulse, at_time, destrSettings.createDestr && ri_effect_cb, 1.f,
    &isDestr);
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
    rendinstdestr::destroyRendinst(desc, true, pos, impulse, at_time, &collInfo, create_destr_effects, NULL,
      collInfo.destroyNeighbourDepth, impulse_mult_for_child);
}

void rendinstdestr::damage_ri_in_sphere(const Point3 &pos, float rad, const Point2 &dmg_near_far, float impulse_to_hp, float at_time,
  bool create_destr_effects, on_riextra_destroyed_callback &&riex_destr_cb, riextra_should_damage &&should_damage)
{
  rendinst::GatherRiTypeFlags flags =
    impulse_to_hp > 0.f ? rendinst::GatherRiTypeFlag::RiGenAndExtra : rendinst::GatherRiTypeFlag::RiExtraOnly;
  rendinst::rigen_collidable_data_t collidableData;
  gatherRIGenCollidableInRadius(collidableData, pos, rad, flags);
  stlsort::sort_branchless(collidableData.begin(), collidableData.end(),
    [](const rendinst::RiGenCollidableData &a, const rendinst::RiGenCollidableData &b) {
      if (a.immortal && b.immortal)
        return false;
      return a.dist < b.dist;
    });

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
      rendinstdestr::destroyRendinst(targetData.desc, true, hitPos, impulse, at_time, &collInfo, create_destr_effects, nullptr,
        collInfo.destroyNeighbourDepth, 1.f, eastl::move(onDestroyCb));
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

CachedCollisionObjectInfo *rendinstdestr::get_or_add_cached_collision_object(const rendinst::RendInstDesc &ri_desc, float at_time,
  const rendinst::CollisionInfo &coll_info)
{
  CachedCollisionObjectInfo *objInfo = rendinstdestr::get_cached_collision_object(ri_desc);
  if (!objInfo)
  {
    if (!coll_info.isDestr)
      return nullptr;
    const float hitPointsToDestrImpulseMult = rendinstdestr::get_destr_settings().hitPointsToDestrImpulseMult;
    float destrImpulse = coll_info.destrImpulse > 0.f ? coll_info.destrImpulse : coll_info.hp * hitPointsToDestrImpulseMult;
    objInfo = new RendinstCollisionUserInfo::RendinstImpulseThresholdData(destrImpulse, coll_info.desc, at_time, coll_info);
    rendinstdestr::add_cached_collision_object(objInfo);
  }
  return objInfo;
}

CachedCollisionObjectInfo *rendinstdestr::get_cached_collision_object(const rendinst::RendInstDesc &ri_desc)
{
  for (int i = 0; i < cached_collision_objects.size(); ++i)
    if (cached_collision_objects[i]->riDesc == ri_desc)
      return cached_collision_objects[i];
  return nullptr;
}

void rendinstdestr::add_cached_collision_object(CachedCollisionObjectInfo *object) { cached_collision_objects.push_back(object); }

void rendinstdestr::remove_tree_rendinst_destr(const rendinst::RendInstDesc &desc)
{
  rendinst::delRIGenExtraFromCell(desc);

  for (int i = 0; i < riPhys.size(); ++i)
  {
    if (riPhys[i].descForDestr != desc)
      continue;

    rendinst::removeRIGenExternalControl(riPhys[i].desc, riPhys[i].ri);
    riPhys[i].cleanup();
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
    riPhys[i].cleanup();
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

rendinst::RendInstDesc rendinstdestr::create_tree_rend_inst_destr(const rendinst::RendInstDesc &desc, bool add_restorable,
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

  rendinst::DestroyedRi *ri = create_destr ? rendinst::doRIGenExternalControl(desc, !create_phys) : NULL;
  Point2 impulseDir = normalizeDef(Point2::xz(impulse), {1.f, 0.f});
  rendinst::RendInstDesc offsetedDesc = desc;
  rendinst::DestrOptionFlags flags =
    rendinst::DestrOptionFlag::AddDestroyedRi | rendinst::DestrOptionFlag::ForceDestroy | rendinst::DestrOptionFlag::UseFullBbox;
  if (create_phys)
    offsetedDesc = rendinstdestr::destroyRendinst(desc, add_restorable, ZERO<Point3>(), ZERO<Point3>(), at_time, coll_info,
      create_destr && get_destr_settings().createDestr, nullptr, 1, 1.0f, nullptr, flags);

  if (destrSettings.hasStaticShadowsInvalidationCallback)
    rendinst::render::avoidStaticShadowRecalc = false;

  if (!ri)
    return offsetedDesc;

  const float treeHeightSpringThreshold = 3.f;
  const rendinstdestr::TreeDestr &treeDestr = rendinstdestr::get_tree_destr();

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

  const rendinst::RiDestrData &riDestrData = rendinst::gather_ri_destr_data(desc);
  const float height = bbox.lim[1].y * scale.y * riDestrData.collisionHeightScale;
  if (height <= 0)
  {
    logerr("%s[%d] gives bbox=%@ and invalid height=%.1f (scale=%@ collisionHeightScale=%g)", desc.isRiExtra() ? "riExtra" : "riGen",
      desc.pool, bbox, height, scale, riDestrData.collisionHeightScale);
    return offsetedDesc;
  }
  const float bushRadiusMult = (riDestrData.bushBehaviour ? treeDestr.bushDestrRadiusMult : 1.0f);
  const float radius = bbox.width().x * 0.5f * bushRadiusMult;
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

  if (!allowSpring && rendinstdestr::get_destr_settings().hasSound)
    phys.treeSound.init(g_tree_sound_cb, tm, bbox.lim[1].y * scale.y, riDestrData.bushBehaviour);

  phys.descForDestr = offsetedDesc;

  if (phys.physModel->physType == gamephys::DynamicPhysModel::E_PHYS_OBJECT)
  {
    float canopyStartHt = height * 0.6f;
    float trunkStartHt = (constrained_phys ? height * 0.035f : 0.f) + radius;
    float trunkHt = max(canopyStartHt * 0.9f - trunkStartHt, 2.f * radius);
    float trunkHalfHt = trunkHt * 0.5f;
    TMatrix centerOfMassTm = TMatrix::IDENT;
    centerOfMassTm.setcol(3, Point3(0.f, trunkHalfHt + trunkStartHt, 0.f));
    phys.centerOfMassTm = centerOfMassTm;
    PhysWorld *physWorld = dacoll::get_phys_world();

    TMatrix fallDirTm;
    fallDirTm.setcol(0, Point3::x0y(impulseDir));
    fallDirTm.setcol(2, fallDirTm.getcol(0) % tm.getcol(1));
    fallDirTm.setcol(1, fallDirTm.getcol(2) % fallDirTm.getcol(0));
    fallDirTm.setcol(3, tm.getcol(3));
    TMatrix canopyImpulseDirTm = inverse(tm) % fallDirTm;

    // trunk
    {
      // shape
      PhysCapsuleCollision capsuleShape(radius, trunkHt, 1 /*Y*/);

      PhysBodyCreationData pbcd;
      pbcd.momentOfInertia = momentOfInertia;
      if (constrained_phys)
      {
        phys.distConstrainedPhys = 1.0f;
        pbcd.autoMask = false, pbcd.group = dacoll::EPL_DEFAULT, pbcd.mask = dacoll::EPL_ALL ^ dacoll::EPL_KINEMATIC; //-V1048
      }

      // body
      phys.physBody = new PhysBody(physWorld, mass, &capsuleShape, tm * centerOfMassTm, pbcd);
    }

    // canopy
    float sphRad = (height - canopyStartHt) * 0.5f;
    TMatrix sphTm = TMatrix::IDENT;
    float impulseDirMult = constrained_phys ? 0.1f * sphRad : 0.f;
    sphTm.setcol(3, Point3(impulseDir.x * impulseDirMult, height - sphRad, impulseDir.y * impulseDirMult));
    float canopyMass = sqr(sphRad) * sphRad * PI * 4.f / 3.f * treeDestr.canopyDensity;
    Point3 canopyInertia = treeDestr.canopyInertia * 0.4f * canopyMass * sqr(sphRad);
    {
      // shape
      const float canopyMaxRadius = treeDestr.canopyMaxRaduis;
      float colSize = clamp(sphRad, 0.05f, canopyMaxRadius);
      PhysSphereCollision sphCanopyCollision(colSize);
      PhysBoxCollision boxCanopyCollision(colSize, colSize, colSize);
      const PhysCollision *selCanopyCollision = &sphCanopyCollision;
      if (treeDestr.useBoxAsCanopyCollision)
        selCanopyCollision = &boxCanopyCollision;

      PhysBodyCreationData pbcd;
      pbcd.momentOfInertia = canopyInertia;
      pbcd.rollingFriction = treeDestr.canopyRollingFriction;
      pbcd.restitution = treeDestr.canopyRestitution;
      pbcd.linearDamping = treeDestr.canopyLinearDamping;
      pbcd.angularDamping = treeDestr.canopyAngularDamping;
      if (constrained_phys)
        pbcd.autoMask = false, pbcd.group = dacoll::EPL_DEFAULT, pbcd.mask = dacoll::EPL_ALL ^ dacoll::EPL_KINEMATIC; //-V1048

      // body
      phys.additionalBody = new PhysBody(physWorld, canopyMass, selCanopyCollision, fallDirTm * sphTm, pbcd);
    }

    // joint
    {
      TMatrix halfSphTm = sphTm;
      halfSphTm.setcol(3, sphTm.getcol(3) * 0.5f);
      PhysJoint *joint = physWorld->create6DofSpringJoint(phys.physBody, phys.additionalBody, halfSphTm * inverse(phys.centerOfMassTm),
        inverse(halfSphTm * canopyImpulseDirTm));
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

    if (!constrained_phys && (coll_info->destrFxId >= 0 || !coll_info->destrFxTemplate.empty()) && ri_effect_cb)
      ri_effect_cb(coll_info->destrFxId, TMatrix::IDENT, tm, coll_info->desc.pool, false, nullptr, coll_info->destrFxTemplate.c_str());

    const rendinstdestr::TreeDestr::BranchDestr &branchDestr =
      from_damage ? treeDestr.branchDestrFromDamage : treeDestr.branchDestrOther;
    if (branchDestr.enableBranchDestruction)
    {
      phys.treeInstData.branchDestr = branchDestr;
      if (!rendinst::fillTreeInstData(desc, impulseDir * vel, from_damage, phys.treeInstData))
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

  if (create_destr && !create_phys && ri_effect_cb)
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

void rendinstdestr::perform_delayed_destruction(int quota_usec)
{
  G_ASSERT(is_main_thread());
  TIME_PROFILE(doDeferredRiDestruction);
  const auto start_time = ref_time_ticks();

  if (!g_delayed_net_destr_list.list.empty())
  {
    uint64_t riExtraGlobalVersion;
    {
      rendinst::ScopedRIExtraReadLock riVersionLock;
      riExtraGlobalVersion = rendinst::getRIExtraGlobalWorldVersion(true, false);
    }
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
    rendinstdestr::destroyRendinst(data.riDesc, true, data.pos, ZERO<Point3>(), data.at_time, &collInfo, data.local_create_destr,
      nullptr, collInfo.destroyNeighbourDepth, 1.f, nullptr, data.flags);
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

  for (int j = 0; j < ri_h.size(); ++j)
  {
    rendinst::RendInstDesc riDesc(-1, rendinst::handle_to_ri_inst(ri_h[j]), rendinst::handle_to_ri_type(ri_h[j]), 0, 0);
    if (riDesc.pool >= 0 && rendinst::get_riextra_immortality(ri_h[j]) && !(flags & rendinst::DestrOptionFlag::ForceDestroy))
      continue;
    rendinst::CollisionInfo collInfo(riDesc);
    collInfo = rendinst::getRiGenDestrInfo(riDesc);

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

      if (!collInfo.isDestr || collInfo.riPoolRef < 0 || collInfo.tm == TMatrix::ZERO)
        continue;

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
      data.at_time = at_time;
      data.local_create_destr = local_create_destr;
      data.flags = curFlags;
    }
    else
    {
      rendinstdestr::destroyRendinst(riDesc, true, box.center(), ZERO<Point3>(), at_time, &collInfo, local_create_destr, NULL,
        collInfo.destroyNeighbourDepth, 1.f, nullptr, curFlags);
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

  rendinst::foreachRIGenInBox(box, rendinst::GatherRiTypeFlag::RiGenOnly, cb);

  for (const rendinst::RendInstDesc &desc : cb.riDescToRemove)
  {
    rendinst::DestroyedRi *destroyedRi = rendinst::doRIGenExternalControl(desc, true);
    rendinst::removeRIGenExternalControl(desc, destroyedRi);
  }
}

#if DAGOR_DBGLEVEL > 0
namespace rendinstdestr
{
static void imguiWindow()
{
  static bool isPlaying = false;

  auto &globalBranchDestr = debugTreeInstData.last_object_was_from_damage ? get_tree_destr_mutable().branchDestrFromDamage
                                                                          : get_tree_destr_mutable().branchDestrOther;

  rendinstdestr::TreeDestr::BranchDestr &branchDestr =
    debugTreeInstData.last_object_was_from_damage ? debugTreeInstData.branchDestrFromDamage : debugTreeInstData.branchDestrOther;

  ImGui::Checkbox("Enable Branch Destruction", &globalBranchDestr.enableBranchDestruction);
  ImGui::Checkbox("Use Debug Values", &useDebugTreeInstData);
  if (ImGui::Button("Reset Debug Values"))
  {
    debugTreeInstData.timer_offset = 0.0f;
    branchDestr = globalBranchDestr;
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
}

REGISTER_IMGUI_WINDOW("Render", "Tree Destruction", imguiWindow);
} // namespace rendinstdestr
#endif // DAGOR_DBGLEVEL > 0
