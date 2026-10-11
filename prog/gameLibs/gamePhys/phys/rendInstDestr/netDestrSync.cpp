// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "netDestrSync.h"

#include <EASTL/array.h>
#include <EASTL/bitset.h>
#include <EASTL/vector.h>
#include <generic/dag_relocatableFixedVector.h>
#include <math/random/dag_random.h>
#include <vecmath/dag_vecMath.h>
#include <memory/dag_framemem.h>
#include <osApiWrappers/dag_critSec.h>
#include <perfMon/dag_statDrv.h>
#include <util/dag_stlqsort.h>

#include <rendInst/rendInstDebris.h>
#include <rendInst/rendInstExtra.h>
#include <rendInst/rendInstExtraAccess.h>
#include <rendInst/rendInstFx.h>
#include <rendInst/rendInstGen.h>
#include <rendInst/rendInstGenRender.h>
#include <rendInst/riexSync.h>

#include "riDestrInternal.h"

#define VERBOSE_SYNC(...) // debug("[ri destr sync] " __VA_ARGS__)


static constexpr int INITIAL_REPLICATION = 1;

static bool apply_reflection = false;
static bool apply_pending_destrs = false;
static dag::RelocatableFixedVector<rendinstdestr::restorable_rendinst_callback, 2> restorable_rendinst_callbacks;
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

static void do_delayed_ri_extra_destruction_impl();


namespace rendinstdestr::netsync
{

class RestorableRendinst
{
  rendinst::RendInstDesc riDesc;
  // To restore
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
    float at_time, const rendinst::CollisionInfo &coll_info, rendinst::riex_handle_t generated_handle, int apex_destr_id);

  bool isRestoreNeeded() const { return !confirmedDestruction; }

  rendinst::riex_handle_t restore();
  bool update(float dt); // return false if need to be destroyed
  void confirmDestruction();
  void invalidateHandle(rendinst::riex_handle_t invalidate_handle);

  const rendinst::RendInstDesc &getRiDesc() const { return riDesc; }
  float getAtTime() const { return atTime; }
  float getTtl() const { return ttl; }
  const rendinst::CollisionInfo &getCollInfo() const { return collInfo; }
};

static eastl::vector<RestorableRendinst> restorables;

void call_restorable_rendinst_cb(const rendinst::RendInstDesc &desc, rendinstdestr::RestorableRendinstState state)
{
  constexpr const char *STATE_NAMES[3] = {"CREATED", "RESTORED", "DESTROYED"};
  G_UNUSED(STATE_NAMES);
  VERBOSE_SYNC("  restorable callback called desc:" FMT_DESC_STR " state:%s", FMT_DESC_V(desc), STATE_NAMES[int(state)]);
  for (rendinstdestr::restorable_rendinst_callback cb : restorable_rendinst_callbacks)
    cb(desc, state);
}

RestorableRendinst::RestorableRendinst(const rendinst::RendInstDesc &desc, const rendinst::RendInstBufferData &buffer,
  destructables::id_t destr_id, float at_time, const rendinst::CollisionInfo &coll_info, rendinst::riex_handle_t generated_handle,
  int apex_destr_id) :
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

rendinst::riex_handle_t RestorableRendinst::restore()
{
  rendinstdestr::remove_tree_rendinst_destr(riDesc);
  rendinst::delRIGenExtra(generatedHandle);
  destructables::removeDestructableById(destrId);
  rendinst::riex_handle_t h = rendinst::restoreRendInst(riBuffer, true);
  VERBOSE_SYNC("restored ri (likely no sync received) desc:" FMT_DESC_STR " restored:%i", FMT_DESC_V(riDesc),
    h != rendinst::RIEX_HANDLE_NULL);
#if ENABLE_APEX
  remove_apex_destructed_ri(riDesc, apexDestrId);
#endif
  return h;
}

bool RestorableRendinst::update(float dt)
{
  ttl -= dt;
  return !confirmedDestruction && ttl > 0.f;
}

void RestorableRendinst::confirmDestruction()
{
  VERBOSE_SYNC("confirmed ri destruction desc:" FMT_DESC_STR, FMT_DESC_V(riDesc));
  confirmedDestruction = true;
}

void RestorableRendinst::invalidateHandle(rendinst::riex_handle_t invalidate_handle)
{
  if (generatedHandle == invalidate_handle)
    generatedHandle = rendinst::RIEX_HANDLE_NULL;
}

static void invalidate_handle_cb(rendinst::riex_handle_t invalidate_handle)
{
  for (RestorableRendinst &restr : restorables)
    restr.invalidateHandle(invalidate_handle);
}

} // namespace rendinstdestr::netsync

void rendinstdestr::netsync::update_restorables(float dt)
{
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

void rendinstdestr::netsync::add_restorable(const rendinst::RendInstDesc &desc, const rendinst::RendInstDesc &offsetedDesc,
  const rendinst::RendInstBufferData &riBuffer, destructables::id_t destrId, float at_time, const rendinst::CollisionInfo *coll_info,
  rendinst::riex_handle_t createdDestroyedRiexHandle, int apex_destructible_id, const TMatrix &mainTm, const Point3 &pos,
  const Point3 &impulse)
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

void rendinstdestr::testObjToRestorablesIntersection(const BBox3 & /*obj_box*/, const TMatrix & /*obj_tm*/,
  rendinst::RendInstCollisionCB *coll_cb, float at_time)
{
  for (auto &restr : netsync::restorables)
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
  auto it = eastl::find(netsync::restorables.begin(), netsync::restorables.end(), id,
    [](const netsync::RestorableRendinst &rri, destructables::id_t destr_id) { return rri.destrId == destr_id; });
  if (it != netsync::restorables.end())
  {
    netsync::call_restorable_rendinst_cb(it->getRiDesc(), rendinstdestr::RRS_DESTROYED);
    netsync::restorables.erase(it);
  }
}

void rendinstdestr::register_restorable_rendinst_cb(restorable_rendinst_callback cb)
{
  if (find_value_idx(restorable_rendinst_callbacks, cb) == -1)
    restorable_rendinst_callbacks.push_back(cb);
}

bool rendinstdestr::unregister_restorable_rendinst_cb(restorable_rendinst_callback cb)
{
  return erase_item_by_value(restorable_rendinst_callbacks, cb);
}


void rendinstdestr::netsync::init_sync(bool apply_pending, bool register_ri_callbacks)
{
  apply_pending_destrs = apply_pending;
  g_delayed_net_destr_list.isEnabled = true;
  riexsync::pool_mapping_change_callback = on_riex_pool_mapping_change;
  riexsync::sync_active = true;
  if (register_ri_callbacks)
  {
    rendinst::registerRIGenExtraInvalidateHandleCb(invalidate_handle_cb);
    rendinst::do_delayed_ri_extra_destruction = +[] {
      rendinst::ScopedRIExtraReadLock rl;
      do_delayed_ri_extra_destruction_impl();
    };
  }
}

void rendinstdestr::netsync::start_session()
{
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

void rendinstdestr::netsync::end_session()
{
  apply_reflection = false;
  pending_initial_destrs.Clear();
  pending_destrs_updates.Clear();

  g_delayed_net_destr_list.clear();
  simultaneous_destrs_count = 0;
}

void rendinstdestr::netsync::clear_sync()
{
  restorables.clear();
  restorables.shrink_to_fit();
  riexsync::clear_synced_ri_extra_pools();
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
  for (rendinstdestr::netsync::RestorableRendinst &restr : rendinstdestr::netsync::restorables)
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
      rendinstdestr::destroyRendinst(restorable_desc, /*add_restorable*/ false, impulsePos, impulse_vec, /*at_time*/ 0.f,
        /*coll_info*/ nullptr, create_destr);
    }
    if constexpr (IsReadLocked)
      rendinst::riex_lock_read();
  }
  return ok;
}

// Batched cell VB update after applying net destruction: precise per-instance static shadow invalidation
// already happened in on_rendinst_destroyed callbacks for every instance of the batch, no need to
// invalidate static shadows for the whole cell
static void update_ri_gen_vb_cell_after_destr_batch(int layer_idx, int cell_idx)
{
  const bool prevAvoidStaticShadowRecalc = rendinst::render::avoidStaticShadowRecalc;
  if (rendinstdestr::get_destr_settings().hasStaticShadowsInvalidationCallback)
    rendinst::render::avoidStaticShadowRecalc = true;
  rendinst::updateRiGenVbCell(layer_idx, cell_idx);
  rendinst::render::avoidStaticShadowRecalc = prevAvoidStaticShadowRecalc;
}


// NOTE: Required RI extra to be locked for read!
static void do_delayed_ri_extra_destruction_impl() DAG_TS_REQUIRES_SHARED(rendinst::ccExtra)
{
  if (!g_delayed_net_destr_list.isEnabled)
    return;

  g_delayed_net_destr_list.mutex.lock();
  const uint64_t version = rendinst::getRIExtraGlobalWorldVersion(true, false);
  auto list = eastl::move(g_delayed_net_destr_list.list);
  g_delayed_net_destr_list.mutex.unlock();
  if (list.empty())
    return;
  TIME_PROFILE(do_delayed_ri_extra_destruction)

  int destrCnt = 0;
  for (auto it = list.begin(); it != list.end();)
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
      it = list.erase_unsorted(it);
      destrCnt++;
    }
    else
      ++it;
  }

  WinAutoLock lock(g_delayed_net_destr_list.mutex);
  if (destrCnt > 0)
    debug("delayed rend inst net destr: version:%llu->%llu destroyed:%i pending:%i", g_delayed_net_destr_list.lastUpdatedWorldVersion,
      version, destrCnt, int(list.size()));
  auto &allDestrList = g_delayed_net_destr_list.list;
  g_delayed_net_destr_list.lastUpdatedWorldVersion = version;
  g_delayed_net_destr_list.isForcedUpdatePending = !allDestrList.empty(); // if there are new entries, we need to re-run it
  if (allDestrList.empty())
    allDestrList.swap(list);
  else
    for (const auto &d : list)
      if (eastl::find(allDestrList.begin(), allDestrList.end(), d) == allDestrList.end())
        allDestrList.push_back(d);
}

void rendinstdestr::netsync::perform_delayed_destruction()
{
  if (!g_delayed_net_destr_list.list.empty())
  {
    rendinst::ScopedRIExtraReadLock riVersionLock;
    uint64_t riExtraGlobalVersion = rendinst::getRIExtraGlobalWorldVersion(true, false);
    if (g_delayed_net_destr_list.isForcedUpdatePending || g_delayed_net_destr_list.lastUpdatedWorldVersion != riExtraGlobalVersion)
      do_delayed_ri_extra_destruction_impl(); // will set lastUpdatedWorldVersion and isForcedUpdatePending
  }
}


bool rendinstdestr::serialize_destr_data(danet::BitStream &bs)
{
  [[maybe_unused]] auto sortByOffset = [](const auto &left, const auto &right) { return left.startOffs < right.startOffs; };
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
        G_ASSERT(eastl::is_sorted(pool.destroyedInstances.begin(), pool.destroyedInstances.end(), sortByOffset));
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
            for (auto &restr : netsync::restorables)
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
      update_ri_gen_vb_cell_after_destr_batch(0, cell.cellId);
  }

  debug("Finished ridestr snapshot deserialize. updateSize=%i delayedRiExtraDestruction=%i", updateSize,
    delayedRiExtraDestruction.size());
  flush_temp_delayed_destruction_list(eastl::move(delayedRiExtraDestruction));
  return true;
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
          update_ri_gen_vb_cell_after_destr_batch(0, prevCellIdx);
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
    update_ri_gen_vb_cell_after_destr_batch(0, prevCellIdx);
  flush_temp_delayed_destruction_list(eastl::move(delayedRiExtraDestruction));
  return true;
}


bool rendinstdestr::serialize_client_destr_requests(danet::BitStream &bs, float ttl_min, float ttl_max)
{
  FRAMEMEM_REGION;
  FRAMEMEM_VALIDATE;
  bool anyAtMinTtl = false;
  for (netsync::RestorableRendinst &restorable : netsync::restorables)
  {
    if (restorable.isRequestSentToServer || !restorable.isRestoreNeeded() || restorable.getTtl() > ttl_min)
      continue;
    anyAtMinTtl = true;
    break;
  }
  if (!anyAtMinTtl)
    return false;
  dag::RelocatableFixedVector<DestrUpdateDesc, 64, true, framemem_allocator> riDestrData;
  for (netsync::RestorableRendinst &restorable : netsync::restorables)
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
