// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <gameRes/dag_gameResSystem.h>
#include <gameRes/dag_stdGameRes.h>
#include <phys/dag_fastPhys.h>
#include <ioSys/dag_genIo.h>
#include <ioSys/dag_memIo.h>
#include <util/dag_globDef.h>
#include <util/dag_string.h>
#include <EASTL/vector.h>
#include <generic/dag_initOnDemand.h>
#include <debug/dag_log.h>
#include <gameRes/dag_dumpResRefCountImpl.h>
#include <osApiWrappers/dag_critSec.h>


class FastPhysGameResFactory final : public GameResourceFactory
{
public:
  struct FastPhysData
  {
    int resId;
    int refCount;
    SmallTab<unsigned, MidmemAlloc> fastPhys;
  };

  eastl::vector<FastPhysData> ffData;


  int findResData(int res_id) const
  {
    for (int i = 0; i < ffData.size(); ++i)
      if (ffData[i].resId == res_id)
        return i;

    return -1;
  }


  int findPhys(void *phys) const
  {
    if (!phys)
      return -1;

    for (int i = 0; i < ffData.size(); ++i)
      if (ffData[i].fastPhys.data() == phys)
        return i;

    return -1;
  }


  unsigned getResClassId() override { return FastPhysDataGameResClassId; }


  const char *getResClassName() override { return "FastPhys"; }

  bool isResLoaded(int res_id) override { return findResData(::validate_game_res_id(res_id)) >= 0; }
  bool checkResPtr(GameResource *res) override { return findPhys((FastPhysSystem *)res) >= 0; }

  GameResource *getGameResource(int res_id) override
  {
    // get real-res id
    if (::validate_game_res_id(res_id) == NULL_GAMERES_ID)
      return NULL;

    int id = findResData(res_id);

    // no resource - load pack
    if (id < 0)
    {
      ::load_game_resource_pack(res_id);
      id = findResData(res_id);
      if (id < 0)
        return NULL;
    }

    ffData[id].refCount++;
    return (GameResource *)ffData[id].fastPhys.data();
  }


  bool addRefGameResource(GameResource *resource) override
  {
    int id = findPhys((FastPhysSystem *)resource);
    if (id < 0)
      return false;

    ffData[id].refCount++;
    return true;
  }


  void delRef(int id)
  {
    if (id < 0)
      return;

    if (ffData[id].refCount == 0)
    {
      String name;
      get_game_resource_name(ffData[id].resId, name);
      G_ASSERT_LOG(0, "Attempt to release %s resource '%s' with 0 refcount", "FastPhys", name.str());
      return;
    }

    ffData[id].refCount--;
  }


  void releaseGameResource(int res_id) override
  {
    if (::validate_game_res_id(res_id) == NULL_GAMERES_ID)
      return;

    int id = findResData(res_id);
    delRef(id);
  }


  bool releaseGameResource(GameResource *resource) override
  {
    int id = findPhys((FastPhysSystem *)resource);
    delRef(id);
    return id >= 0;
  }


  bool freeUnusedResources(bool forced_free_unref_packs, bool once /*= false*/) override
  {
    bool result = false;
    for (int i = ffData.size() - 1; i >= 0; --i)
    {
      if (get_refcount_game_resource_pack_by_resid(ffData[i].resId) > 0)
        continue;
      if (ffData[i].refCount != 0)
      {
        if (!forced_free_unref_packs)
          continue;
        else
          FATAL_ON_UNLOADING_USED_RES(ffData[i].resId, ffData[i].refCount);
      }
      ffData.erase(ffData.begin() + i);
      result = true;
      if (once)
        break;
    }
    return result;
  }


  void loadGameResourceData(int res_id, IGenLoad &cb) override
  {
    WinCritSec &cs = get_gameres_main_cs();
    {
      WinAutoLock lock(cs);
      if (findResData(res_id) >= 0)
        return;
    }

    decltype(FastPhysData::fastPhys) fastPhys;

    cb.beginBlock();
    clear_and_resize(fastPhys, (cb.getBlockRest() + 3) / 4 + 1);
    fastPhys[0] = cb.getBlockRest();
    cb.read(fastPhys.data() + 1, cb.getBlockRest());
    cb.endBlock();

    WinAutoLock lock(cs);
    ffData.push_back(FastPhysData{res_id, 0, eastl::move(fastPhys)});
  }
  void createGameResource(int /*res_id*/, const int * /*reference_ids*/, int /*num_refs*/) override {}

  void reset() override { ffData.clear(); }

  IMPLEMENT_DUMP_RESOURCES_REF_COUNT(ffData, resId, refCount)
};

static InitOnDemand<FastPhysGameResFactory> fast_phys_factory;

void register_fast_phys_gameres_factory()
{
  fast_phys_factory.demandInit();
  ::add_factory(fast_phys_factory);
}

FastPhysSystem *create_fast_phys_from_gameres(const char *res_name)
{
  GameResHandle handle = GAMERES_HANDLE_FROM_STRING(res_name);
  const unsigned *data = (const unsigned *)get_game_resource_ex(handle, FastPhysDataGameResClassId);
  if (!data)
    return NULL;
  FastPhysSystem *fp = new FastPhysSystem;
  InPlaceMemLoadCB crd(data + 1, data[0]);
  fp->load(crd);
  release_game_resource((GameResource *)data);
  return fp;
}
