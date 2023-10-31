#include <gameRes/dag_gameResSystem.h>
#include <gameRes/dag_stdGameRes.h>
#include <gameRes/dag_dumpResRefCountImpl.h>
#include <shaders/dag_dynSceneRes.h>
#include <shaders/dag_shaderMeshTexLoadCtrl.h>
#include <shaders/dag_shaderResUnitedData.h>
#include <3d/fileTexFactory.h>
#include <3d/dag_texMgrTags.h>
#include <3d/dag_drv3dReset.h>
#include <ioSys/dag_dataBlock.h>
#include <util/dag_delayedAction.h>
#include <debug/dag_log.h>
#include <generic/dag_initOnDemand.h>
#include <osApiWrappers/dag_critSec.h>
#include <util/dag_console.h>
#include <perfMon/dag_perfTimer.h>


ShaderResUnitedVdata<DynamicRenderableSceneLodsResource> unitedvdata::dmUnitedVdata("DynModelReloadVdata");
int unitedvdata::dmUnitedVdataUsed = 0;

using unitedvdata::dmUnitedVdata;
using unitedvdata::dmUnitedVdataUsed;

class DynModelGameResFactory : public GameResourceFactory
{
public:
  struct GameRes
  {
    int resId = NULL_GAMERES_ID;
    Ptr<DynamicRenderableSceneLodsResource> sceneRes;

    GameRes() = default;
    GameRes(int rid, DynamicRenderableSceneLodsResource *res) : resId(rid), sceneRes(res) {}
  };

  Tab<GameRes> gameRes;
  PtrTab<DynamicRenderableSceneLodsResource> obsoleteRes;


  int findGameRes(int res_id);
  int findGameRes(DynamicRenderableSceneLodsResource *res);


  unsigned getResClassId() override { return DynModelGameResClassId; }

  const char *getResClassName() override { return "DynModel"; }

  bool isResLoaded(int res_id) override { return findGameRes(res_id) >= 0; }
  bool checkResPtr(GameResource *res) override
  {
    auto r = (DynamicRenderableSceneLodsResource *)res;
    return find_value_idx(obsoleteRes, r) >= 0 || findGameRes(r) >= 0;
  }

  GameResource *getGameResource(int res_id) override;


  bool addRefGameResource(GameResource *resource) override;

  void releaseGameResource(int res_id) override;

  bool releaseGameResource(GameResource *resource) override;

  bool freeUnusedResources(bool forced_free_unref_packs, bool once = false) override;

  GameResource *discardOlderResAfterUpdate(int res_id);

  void loadGameResourceData(int res_id, IGenLoad &cb) override;
  void createGameResource(int /*res_id*/, const int * /*ref_ids*/, int /*num_refs*/) override {}

  void reset() override;

  IMPLEMENT_DUMP_RESOURCES_REF_COUNT(gameRes, resId, sceneRes->getRefCount())
};

int DynModelGameResFactory::findGameRes(int res_id)
{
  for (int i = 0; i < gameRes.size(); ++i)
    if (gameRes[i].resId == res_id)
      return i;

  return -1;
}


int DynModelGameResFactory::findGameRes(DynamicRenderableSceneLodsResource *res)
{
  for (int i = 0; i < gameRes.size(); ++i)
    if (gameRes[i].sceneRes == res)
      return i;

  return -1;
}


GameResource *DynModelGameResFactory::getGameResource(int res_id)
{
  int id = findGameRes(res_id);

  if (id < 0)
    ::load_game_resource_pack(res_id);

  id = findGameRes(res_id);

  if (id < 0)
    return NULL;

  gameRes[id].sceneRes.addRef();
  return (GameResource *)(DynamicRenderableSceneLodsResource *)gameRes[id].sceneRes;
}


bool DynModelGameResFactory::addRefGameResource(GameResource *resource)
{
  int id = findGameRes((DynamicRenderableSceneLodsResource *)resource);
  if (id < 0)
    return false;

  gameRes[id].sceneRes.addRef();
  return true;
}


void DynModelGameResFactory::releaseGameResource(int res_id)
{
  int id = findGameRes(res_id);
  if (id < 0)
    return;

  gameRes[id].sceneRes.delRef();
}


bool DynModelGameResFactory::releaseGameResource(GameResource *resource)
{
  int obsolete_idx = find_value_idx(obsoleteRes, (DynamicRenderableSceneLodsResource *)resource);
  if (obsolete_idx >= 0)
  {
    obsoleteRes[obsolete_idx].delRef();
    if (obsoleteRes[obsolete_idx]->getRefCount() == 1 + dmUnitedVdataUsed)
    {
      if (dmUnitedVdataUsed)
        dmUnitedVdata.delRes(obsoleteRes[obsolete_idx]);
      erase_items(obsoleteRes, obsolete_idx, 1);
    }
    return true;
  }

  int id = findGameRes((DynamicRenderableSceneLodsResource *)resource);
  if (id < 0)
    return false;

  gameRes[id].sceneRes.delRef();
  return true;
}


bool DynModelGameResFactory::freeUnusedResources(bool forced_free_unref_packs, bool once /*= false*/)
{
  bool result = false;
  for (int i = gameRes.size() - 1; i >= 0; --i)
  {
    if (get_refcount_game_resource_pack_by_resid(gameRes[i].resId) > 0)
      continue;
    // 1 ref for dmUnitedVdata, 1 ref for res, others are for created instances
    if (gameRes[i].sceneRes && gameRes[i].sceneRes->getRefCount() > 1 + dmUnitedVdataUsed)
    {
      if (!forced_free_unref_packs)
        continue;
      else
        FATAL_ON_UNLOADING_USED_RES(gameRes[i].resId, gameRes[i].sceneRes->getRefCount());
    }
    if (dmUnitedVdataUsed)
      dmUnitedVdata.delRes(gameRes[i].sceneRes);
    erase_items(gameRes, i, 1);
    result = true;
    if (once)
      break;
  }

  for (int i = obsoleteRes.size() - 1; i >= 0; --i)
    if (obsoleteRes[i]->getRefCount() == 1 + dmUnitedVdataUsed)
    {
      if (dmUnitedVdataUsed)
        dmUnitedVdata.delRes(obsoleteRes[i]);
      erase_items(obsoleteRes, i, 1);
      result = true;
    }

  if (dmUnitedVdataUsed && dmUnitedVdata.getResCount() == 0)
    dmUnitedVdata.clear();
  else if (dmUnitedVdataUsed && result)
    dmUnitedVdata.releaseUnusedBuffers();
  return result;
}

GameResource *DynModelGameResFactory::discardOlderResAfterUpdate(int res_id)
{
  GameResource *res = nullptr;
  int id = findGameRes(res_id);
  if (id >= 0)
  {
    obsoleteRes.push_back(gameRes[id].sceneRes);
    res = (GameResource *)gameRes[id].sceneRes.get();
    erase_items(gameRes, id, 1);
  }
  return res;
}

void DynModelGameResFactory::reset()
{
  if (dmUnitedVdataUsed)
  {
    dmUnitedVdata.clear();
    freeUnusedResources(false, false);
  }

  if (!gameRes.empty())
  {
    logerr("%d leaked DynModels", gameRes.size());
    for (unsigned int resNo = 0; resNo < gameRes.size(); resNo++)
    {
      String name;
      get_game_resource_name(gameRes[resNo].resId, name);
      logerr("    '%s', refCount=%d", name.str(), gameRes[resNo].sceneRes->getRefCount());
    }
  }

  gameRes.clear();
}

void DynModelGameResFactory::loadGameResourceData(int res_id, IGenLoad &cb)
{
  WinCritSec &cs = get_gameres_main_cs();
  {
    WinAutoLock lock(cs);
    if (findGameRes(res_id) >= 0)
      return;
  }

#ifndef NO_3D_GFX
  String name;
  get_game_resource_name(res_id, name);
  FATAL_CONTEXT_AUTO_SCOPE(name);
  dagor_set_sm_tex_load_ctx_type(DynModelGameResClassId);
  if (dagor_sm_tex_load_controller)
    dagor_set_sm_tex_load_ctx_name(name);
  textag_mark_begin(TEXTAG_DYNMODEL);
  int flags = (dmUnitedVdataUsed ? SRLOAD_SRC_ONLY : 0) | SRLOAD_NO_TEX_REF;
  Ptr<DynamicRenderableSceneLodsResource> srcRes =
    DynamicRenderableSceneLodsResource::loadResource(cb, flags, -1, gameres_dynmodel_desc.getBlockByName(name));
  if (srcRes && dmUnitedVdataUsed)
    dmUnitedVdata.addRes(srcRes);
  textag_mark_end();
  dagor_reset_sm_tex_load_ctx();
  if (!srcRes)
    logerr("Error loading DynModel resource %s", name);
#else
  DynamicRenderableSceneLodsResource *srcRes = NULL;
#endif
  WinAutoLock lock2(cs);
  GameRes &gr = gameRes.push_back();
  gr.resId = res_id;
  gr.sceneRes = srcRes;
}

static InitOnDemand<DynModelGameResFactory> dynmodel_factory;
static PtrTab<DynamicRenderableSceneLodsResource> pendingReloadResList;
static std::mutex pendingReloadResListMutex;

namespace
{
struct MeshStreamingStat
{
  struct Stat
  {
    unsigned total = 0, max_at_frame = 0;
    void reset()
    {
      interlocked_exchange(total, 0);
      interlocked_exchange(max_at_frame, 0);
    }
    void add(int val)
    {
      interlocked_add(total, val);
      if (interlocked_acquire_load(max_at_frame) < val)
        interlocked_exchange(max_at_frame, val);
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
  float get_avg_per_frame(const Stat &s) const
  {
    return float(interlocked_acquire_load(s.total)) / (end_frame_no - start_frame_no + 1);
  }
};
static MeshStreamingStat mss;
} // namespace

static void batch_reload_res(void *)
{
  int rcnt = 0;
#if DAGOR_DBGLEVEL > 0
  int64_t reft = profile_ref_ticks();
#endif

  {
    std::lock_guard<std::mutex> scopedLock(pendingReloadResListMutex);
    rcnt = pendingReloadResList.size();
    dmUnitedVdata.discardUnusedResToFreeReqMem();
    for (DynamicRenderableSceneLodsResource *res : pendingReloadResList)
      dmUnitedVdata.reloadRes(res);
    pendingReloadResList.clear();
  }

#if DAGOR_DBGLEVEL > 0
  if (dagor_frame_no() > mss.end_frame_no + 16 || mss.end_frame_no > mss.start_frame_no + 600)
    mss.reset();
  mss.end_frame_no = dagor_frame_no();
  mss.brr_usec.add(profile_time_usec(reft));
  mss.brr_rcnt.add(rcnt);
  mss.pendint_cnt.add(dmUnitedVdata.getPendingReloadResCount());
  mss.failed_cnt.add(dmUnitedVdata.getFailedReloadResCount());
#endif
}
static void on_higher_lod_required(DynamicRenderableSceneLodsResource *res, unsigned req_lod, unsigned /*cur_lod*/)
{
  interlocked_exchange(mss.end_frame_no, dagor_frame_no());
  mss.ohlr_call.add(1);
  int64_t reft = profile_ref_ticks();
  if (req_lod >= res->getQlBestLod())
    return;
  DynamicRenderableSceneLodsResource *res0 = res->getFirstOriginal();
  if (res0 != res)
    return res0->updateReqLod(req_lod);

  std::lock_guard<std::mutex> scopedLock(pendingReloadResListMutex);
  if (res->getResLoadingFlag())
    return;
  mss.ohlr_rcnt.add(1);
  if (find_value_idx(pendingReloadResList, res) >= 0)
    return;
  pendingReloadResList.push_back(res);
  if (pendingReloadResList.size() == 1)
    add_delayed_callback(&batch_reload_res, nullptr);
  mss.ohlr_usec.add(profile_time_usec(reft));
}

void register_dynmodel_gameres_factory(bool use_united_vdata, bool allow_united_vdata_rebuild)
{
  if (use_united_vdata)
  {
    dmUnitedVdataUsed = 1;
    dmUnitedVdata.setDelResAllowed(true);
    dmUnitedVdata.setSepTightVdataAllowed(true);
    dmUnitedVdata.setRebuildAllowed(allow_united_vdata_rebuild);
    if (auto hints_blk = ::dgs_get_game_params()->getBlockByName("unitedVdata.dynModel.def"))
      dmUnitedVdata.setHints(*hints_blk);
    DynamicRenderableSceneLodsResource::on_higher_lod_required = &on_higher_lod_required;
  }

  dynmodel_factory.demandInit();
  ::add_factory(dynmodel_factory);
}

static void before_reset_dynmodel_buffers(bool full_reset)
{
  if (dmUnitedVdataUsed && full_reset)
    dmUnitedVdata.onBeforeD3dReset();
}
static void reset_dynmodel_buffers(bool full_reset)
{
  if (dmUnitedVdataUsed && full_reset)
    dmUnitedVdata.onAfterD3dReset();
}

REGISTER_D3D_BEFORE_RESET_FUNC(before_reset_dynmodel_buffers);
REGISTER_D3D_AFTER_RESET_FUNC(reset_dynmodel_buffers);

using namespace console;
static bool resolve_res_name(String &nm, DynamicRenderableSceneLodsResource *r)
{
  if (!dynmodel_factory.get())
    return false;
  for (auto &res : dynmodel_factory->gameRes)
    if (res.sceneRes == r)
    {
      get_game_resource_name(res.resId, nm);
      return true;
    }
  return false;
}
static bool dmUnitedVdata_console_handler(const char *argv[], int argc)
{
  if (argc < 1)
    return false;
  int found = 0;
  CONSOLE_CHECK_NAME("dmUnitedVdata", "list", 1, 1)
  {
    if (!dmUnitedVdataUsed)
      console::print_d("dmUnitedVdata is not inited");
    else
    {
      String list;
      dmUnitedVdata.buildStatusStr(list, true, &resolve_res_name);
      console::print_d("dmUnitedVdata:\n%s", list);
    }
  }
  CONSOLE_CHECK_NAME("dmUnitedVdata", "status", 1, 1)
  {
    if (!dmUnitedVdataUsed)
      console::print_d("dmUnitedVdata is not inited");
    else
    {
      String list;
      dmUnitedVdata.buildStatusStr(list, false);
      console::print_d("dmUnitedVdata:\n%s", list);
    }
  }
  CONSOLE_CHECK_NAME("dmUnitedVdata", "dumpMem", 1, 1)
  {
    if (!dmUnitedVdataUsed)
      console::print_d("dmUnitedVdata is not inited");
    else
    {
      String summary;
      dmUnitedVdata.dumpMemBlocks(&summary);
      console::print_d("dmUnitedVdata: %s (detailed memory map listed to debug)", summary);
    }
  }
  CONSOLE_CHECK_NAME("dmUnitedVdata", "perfStat", 1, 1)
  {
    if (!dmUnitedVdataUsed)
      console::print_d("dmUnitedVdata is not inited");
    else
    {
      console::print_d(
        "dmUnitedVdata: for %d frames (%d..%d)\n"
        " batch_reload_res(): avg %.0fus/frame (%dus max), avg %.1fres/frame (%d max)\n"
        " on_higher_lod_required(): avg %.0fus/frame (%dus max), avg %.1fcalls/frame (%d max), avg %.1fres/frame (%d max)\n"
        " pending load: avg %.1fres/frame (%d max),  failed load: avg %.1fres/frame (%d max)",
        mss.end_frame_no - mss.start_frame_no + 1, mss.start_frame_no - dagor_frame_no(), mss.end_frame_no - dagor_frame_no(),
        mss.get_avg_per_frame(mss.brr_usec), interlocked_acquire_load(mss.brr_usec.max_at_frame), mss.get_avg_per_frame(mss.brr_rcnt),
        interlocked_acquire_load(mss.brr_rcnt.max_at_frame), mss.get_avg_per_frame(mss.ohlr_usec),
        interlocked_acquire_load(mss.ohlr_usec.max_at_frame), mss.get_avg_per_frame(mss.ohlr_call),
        interlocked_acquire_load(mss.ohlr_call.max_at_frame), mss.get_avg_per_frame(mss.ohlr_rcnt),
        interlocked_acquire_load(mss.ohlr_rcnt.max_at_frame), mss.get_avg_per_frame(mss.pendint_cnt),
        interlocked_acquire_load(mss.pendint_cnt.max_at_frame), mss.get_avg_per_frame(mss.failed_cnt),
        interlocked_acquire_load(mss.failed_cnt.max_at_frame));
    }
  }
  return found;
}

REGISTER_CONSOLE_HANDLER(dmUnitedVdata_console_handler);
