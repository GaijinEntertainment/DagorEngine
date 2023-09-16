#include <shaders/dag_rendInstRes.h>
#include <shaders/dag_shaderResUnitedData.h>
#include <gameRes/dag_gameResSystem.h>
#include <gameRes/dag_stdGameRes.h>
#include <shaders/dag_shaderMeshTexLoadCtrl.h>
#include <ioSys/dag_dataBlock.h>
#include <EASTL/vector.h>
#include <generic/dag_initOnDemand.h>
#include <3d/fileTexFactory.h>
#include <3d/dag_drv3d.h>
#include <3d/dag_texMgrTags.h>
#include <3d/dag_drv3dReset.h>
#include <util/dag_delayedAction.h>
#include <debug/dag_log.h>
#include <gameRes/dag_dumpResRefCountImpl.h>
#include <osApiWrappers/dag_critSec.h>
#include <util/dag_console.h>
#include <perfMon/dag_perfTimer.h>

ShaderResUnitedVdata<RenderableInstanceLodsResource> unitedvdata::riUnitedVdata("RIReloadVdata");

using unitedvdata::riUnitedVdata;

extern template MulticastEvent<void(const RenderableInstanceLodsResource *)>
  ShaderResUnitedVdata<RenderableInstanceLodsResource>::on_mesh_relems_updated;

class RendInstGameResFactory : public GameResourceFactory
{
public:
  using RPtr = Ptr<RenderableInstanceLodsResource>;

  struct GameRes
  {
    int resId;
    RPtr sceneRes;
  };

  eastl::vector<GameRes> gameRes;


  int findGameRes(int res_id) const
  {
    for (int i = 0; i < gameRes.size(); ++i)
      if (gameRes[i].resId == res_id)
        return i;

    return -1;
  }


  int findGameRes(RenderableInstanceLodsResource *res)
  {
    for (int i = 0; i < gameRes.size(); ++i)
      if (gameRes[i].sceneRes == res)
        return i;

    return -1;
  }


  unsigned getResClassId() override { return RendInstGameResClassId; }

  const char *getResClassName() override { return "RendInst"; }

  bool isResLoaded(int res_id) override { return findGameRes(res_id) >= 0; }
  bool checkResPtr(GameResource *res) override { return findGameRes((RenderableInstanceLodsResource *)res) >= 0; }

  GameResource *getGameResource(int res_id) override
  {
    int id = findGameRes(res_id);

    if (id < 0)
      ::load_game_resource_pack(res_id);

    id = findGameRes(res_id);

    if (id < 0)
      return NULL;

    gameRes[id].sceneRes.addRef();
    return (GameResource *)(RenderableInstanceLodsResource *)gameRes[id].sceneRes;
  }


  bool addRefGameResource(GameResource *resource) override
  {
    int id = findGameRes((RenderableInstanceLodsResource *)resource);
    if (id < 0)
      return false;

    gameRes[id].sceneRes.addRef();
    return true;
  }

  void releaseGameResource(int res_id) override
  {
    int id = findGameRes(res_id);
    if (id < 0)
      return;

    gameRes[id].sceneRes.delRef();
  }

  bool releaseGameResource(GameResource *resource) override
  {
    int id = findGameRes((RenderableInstanceLodsResource *)resource);
    if (id < 0)
      return false;

    gameRes[id].sceneRes.delRef();
    return true;
  }

  bool freeUnusedResources(bool forced_free_unref_packs, bool once /*= false*/) override
  {
    bool result = false;
    bool ri_factory = (getResClassId() == RendInstGameResClassId);
    for (int i = gameRes.size() - 1; i >= 0; --i)
    {
      if (get_refcount_game_resource_pack_by_resid(gameRes[i].resId) > 0)
        continue;
      // 1 ref for riUnitedVdata, 1 ref for res
      if (gameRes[i].sceneRes && gameRes[i].sceneRes->getRefCount() > 1 + (ri_factory ? 1 : 0))
      {
        if (!forced_free_unref_packs)
          continue;
        else
          FATAL_ON_UNLOADING_USED_RES(gameRes[i].resId, gameRes[i].sceneRes->getRefCount());
      }
      if (ri_factory && RenderableInstanceLodsResource::on_higher_lod_required)
        riUnitedVdata.delRes(gameRes[i].sceneRes);
      gameRes.erase(gameRes.begin() + i);
      result = true;
      if (once)
        break;
    }

    if (ri_factory && riUnitedVdata.getResCount() == 0)
      riUnitedVdata.clear();
    return result;
  }


  void loadGameResourceData(int res_id, IGenLoad &cb) override
  {
    WinCritSec &cs = get_gameres_main_cs();
    {
      WinAutoLock lock(cs);
      if (findGameRes(res_id) >= 0)
        return;
    }

    String name;
    get_game_resource_name(res_id, name);
    FATAL_CONTEXT_AUTO_SCOPE(name);
    dagor_set_sm_tex_load_ctx_type(RendInstGameResClassId);
    if (dagor_sm_tex_load_controller)
      dagor_set_sm_tex_load_ctx_name(name);
    textag_mark_begin(TEXTAG_RENDINST);
    int flags = SRLOAD_SRC_ONLY;
    RPtr srcRes = RenderableInstanceLodsResource::loadResource(cb, flags, name, gameres_rendinst_desc.getBlockByName(name));
    if (srcRes)
      riUnitedVdata.addRes(srcRes);
    textag_mark_end();
    dagor_reset_sm_tex_load_ctx();

    if (!srcRes)
      logerr("Error loading %s resource %s", getResClassName(), name);

    WinAutoLock lock2(cs);
    gameRes.emplace_back(GameRes{res_id, srcRes});
  }

  void createGameResource(int /*res_id*/, const int * /*ref_ids*/, int /*num_refs*/) override {}

  void reset() override
  {
    if (getResClassId() == RendInstGameResClassId)
    {
      riUnitedVdata.clear();
      freeUnusedResources(false, false);
    }

    gameRes.clear();
  }

  IMPLEMENT_DUMP_RESOURCES_REF_COUNT(gameRes, resId, sceneRes->getRefCount())
};

class RendInstGameResFactoryFinal final : public RendInstGameResFactory
{};

static InitOnDemand<RendInstGameResFactoryFinal> rendinst_factory;

static PtrTab<RenderableInstanceLodsResource> pendingReloadResList;
static std::mutex pendingReloadResListMutex;

#if DAGOR_DBGLEVEL > 0 && !defined(__SANITIZE_THREAD__)
struct MeshStreamingStat
{
  struct Stat
  {
    unsigned total = 0, max_at_frame = 0;
    void reset() { total = max_at_frame = 0; }
    void add(int val)
    {
      total += val;
      if (max_at_frame < val)
        max_at_frame = val;
    }
  };

  unsigned start_frame_no = 0, end_frame_no = 0;
  Stat brr_usec, brr_rcnt;
  Stat ohlr_usec, ohlr_call, ohlr_rcnt;
  Stat pendint_cnt, failed_cnt;

  void reset()
  {
    start_frame_no = end_frame_no = dagor_frame_no();
    brr_usec.reset();
    brr_rcnt.reset();
    ohlr_usec.reset();
    ohlr_call.reset();
    ohlr_rcnt.reset();
    pendint_cnt.reset();
    failed_cnt.reset();
  }
  float get_avg_per_frame(const Stat &s) const { return float(s.total) / (end_frame_no - start_frame_no + 1); }
};
static MeshStreamingStat mss;
#endif

static void batch_reload_res(void *)
{
#if DAGOR_DBGLEVEL > 0 && !defined(__SANITIZE_THREAD__)
  int64_t reft = profile_ref_ticks();
#endif
  PtrTab<RenderableInstanceLodsResource> pendingReloadResListCopy;
  {
    // Move buffer away, so that we can release the lock before calling functions on riUnitedVdata
    std::lock_guard<std::mutex> scopedLock(pendingReloadResListMutex);
    pendingReloadResListCopy = eastl::move(pendingReloadResList);
  }
  riUnitedVdata.discardUnusedResToFreeReqMem();
  for (RenderableInstanceLodsResource *res : pendingReloadResListCopy)
    riUnitedVdata.reloadRes(res);

#if DAGOR_DBGLEVEL > 0 && !defined(__SANITIZE_THREAD__)
  if (dagor_frame_no() > mss.end_frame_no + 16 || mss.end_frame_no > mss.start_frame_no + 600)
    mss.reset();
  mss.end_frame_no = dagor_frame_no();
  mss.brr_usec.add(profile_time_usec(reft));
  mss.brr_rcnt.add(pendingReloadResListCopy.size());
  mss.pendint_cnt.add(riUnitedVdata.getPendingReloadResCount());
  mss.failed_cnt.add(riUnitedVdata.getFailedReloadResCount());
#endif
}
static void on_higher_lod_required(RenderableInstanceLodsResource *res, unsigned req_lod, unsigned /*cur_lod*/)
{
#if DAGOR_DBGLEVEL > 0 && !defined(__SANITIZE_THREAD__)
  mss.end_frame_no = dagor_frame_no();
  mss.ohlr_call.add(1);
  int64_t reft = profile_ref_ticks();
#endif
  if (req_lod >= res->getQlBestLod())
    return;

  std::lock_guard<std::mutex> scopedLock(pendingReloadResListMutex);
  if (res->getResLoadingFlag())
    return;
#if DAGOR_DBGLEVEL > 0 && !defined(__SANITIZE_THREAD__)
  mss.ohlr_rcnt.add(1);
#endif
  if (find_value_idx(pendingReloadResList, res) >= 0)
    return;
  pendingReloadResList.push_back(res);
  if (pendingReloadResList.size() == 1)
    add_delayed_callback(&batch_reload_res, nullptr);
#if DAGOR_DBGLEVEL > 0 && !defined(__SANITIZE_THREAD__)
  mss.ohlr_usec.add(profile_time_usec(reft));
#endif
}

void register_rendinst_gameres_factory(bool use_united_vdata_streaming)
{
  riUnitedVdata.setDelResAllowed(use_united_vdata_streaming);
  riUnitedVdata.setSepTightVdataAllowed(false);
  riUnitedVdata.setRebuildAllowed(!use_united_vdata_streaming);
  riUnitedVdata.setMaxVbSize(::dgs_get_game_params()->getInt("rendinstMaxVbSize", 64 << 20));
  if (auto hints_blk = ::dgs_get_game_params()->getBlockByName("unitedVdata.rendInst.def"))
    riUnitedVdata.setHints(*hints_blk);
  RenderableInstanceLodsResource::on_higher_lod_required = !use_united_vdata_streaming ? nullptr : &on_higher_lod_required;

  rendinst_factory.demandInit();
  ::add_factory(rendinst_factory);
  register_rndgrass_gameres_factory();
  register_impostor_data_gameres_factory();
}

static void before_reset_rendinst_buffers(bool full_reset)
{
  if (full_reset)
    riUnitedVdata.onBeforeD3dReset();
}
REGISTER_D3D_BEFORE_RESET_FUNC(before_reset_rendinst_buffers);

static void reset_rendinst_buffers(bool full_reset)
{
  if (full_reset)
    riUnitedVdata.onAfterD3dReset();
}
REGISTER_D3D_AFTER_RESET_FUNC(reset_rendinst_buffers);

using namespace console;
static bool resolve_res_name(String &nm, RenderableInstanceLodsResource *r)
{
  if (!rendinst_factory.get())
    return false;
  for (auto &res : rendinst_factory->gameRes)
    if (res.sceneRes == r)
    {
      get_game_resource_name(res.resId, nm);
      return true;
    }
  return false;
}
static bool riUnitedVdata_console_handler(const char *argv[], int argc)
{
  if (argc < 1)
    return false;
  int found = 0;
  CONSOLE_CHECK_NAME("riUnitedVdata", "list", 1, 1)
  {
    String list;
    riUnitedVdata.buildStatusStr(list, true, &resolve_res_name);
    console::print_d("riUnitedVdata:\n%s", list);
  }
  CONSOLE_CHECK_NAME("riUnitedVdata", "status", 1, 1)
  {
    String list;
    riUnitedVdata.buildStatusStr(list, false);
    console::print_d("riUnitedVdata:\n%s", list);
  }
  CONSOLE_CHECK_NAME("riUnitedVdata", "dumpMem", 1, 1)
  {
    String summary;
    riUnitedVdata.dumpMemBlocks(&summary);
    console::print_d("riUnitedVdata: %s (detailed memory map listed to debug)", summary);
  }
#if DAGOR_DBGLEVEL > 0 && !defined(__SANITIZE_THREAD__)
  CONSOLE_CHECK_NAME("riUnitedVdata", "perfStat", 1, 1)
  {
    console::print_d(
      "riUnitedVdata: for %d frames (%d..%d)\n"
      " batch_reload_res(): avg %.0fus/frame (%dus max), avg %.1fres/frame (%d max)\n"
      " on_higher_lod_required(): avg %.0fus/frame (%dus max), avg %.1fcalls/frame (%d max), avg %.1fres/frame (%d max)\n"
      " pending load: avg %.1fres/frame (%d max),  failed load: avg %.1fres/frame (%d max)",
      mss.end_frame_no - mss.start_frame_no + 1, mss.start_frame_no - dagor_frame_no(), mss.end_frame_no - dagor_frame_no(),
      mss.get_avg_per_frame(mss.brr_usec), mss.brr_usec.max_at_frame, mss.get_avg_per_frame(mss.brr_rcnt), mss.brr_rcnt.max_at_frame,
      mss.get_avg_per_frame(mss.ohlr_usec), mss.ohlr_usec.max_at_frame, mss.get_avg_per_frame(mss.ohlr_call),
      mss.ohlr_call.max_at_frame, mss.get_avg_per_frame(mss.ohlr_rcnt), mss.ohlr_rcnt.max_at_frame,
      mss.get_avg_per_frame(mss.pendint_cnt), mss.pendint_cnt.max_at_frame, mss.get_avg_per_frame(mss.failed_cnt),
      mss.failed_cnt.max_at_frame);
  }
#endif
  return found;
}

REGISTER_CONSOLE_HANDLER(riUnitedVdata_console_handler);


class RndGrassGameResFactory final : public RendInstGameResFactory
{
public:
  unsigned getResClassId() override { return RndGrassGameResClassId; }
  const char *getResClassName() override { return "RndGrass"; }

  void loadGameResourceData(int res_id, IGenLoad &cb) override
  {
    WinCritSec &cs = get_gameres_main_cs();
    {
      WinAutoLock lock(cs);
      if (findGameRes(res_id) >= 0)
        return;
    }
    String name;
    get_game_resource_name(res_id, name);
    FATAL_CONTEXT_AUTO_SCOPE(name);
    auto srcRes = RenderableInstanceLodsResource::loadResource(cb, SRLOAD_TO_SYSMEM, name);
    dagor_reset_sm_tex_load_ctx();

    if (!srcRes)
      logerr("Error loading %s resource %s", getResClassName(), name);

    WinAutoLock lock2(cs);
    gameRes.emplace_back(GameRes{res_id, RPtr(srcRes)});
  }

  void createGameResource(int /*res_id*/, const int * /*ref_ids*/, int /*num_refs*/) override {}
};
static InitOnDemand<RndGrassGameResFactory> rndgrass_factory;

void register_rndgrass_gameres_factory()
{
  rndgrass_factory.demandInit();
  ::add_factory(rndgrass_factory);
}
