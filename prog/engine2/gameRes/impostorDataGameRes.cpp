#include <debug/dag_debug.h>
#include <gameRes/dag_dumpResRefCountImpl.h>
#include <gameRes/dag_gameResSystem.h>
#include <gameRes/dag_stdGameRes.h>
#include <generic/dag_initOnDemand.h>
#include <ioSys/dag_dataBlock.h>
#include <ioSys/dag_genIo.h>
#include <ioSys/dag_roDataBlock.h>
#include <osApiWrappers/dag_critSec.h>
#include <EASTL/unique_ptr.h>
#include <EASTL/vector.h>

class ImpostorDataGameResFactory final : public GameResourceFactory
{
public:
  struct Data
  {
    int resId;
    int refCount;
    eastl::unique_ptr<DataBlock> data;
  };

  eastl::vector<Data> resList;

  int findRes(int res_id) const
  {
    for (int i = 0; i < resList.size(); ++i)
      if (resList[i].resId == res_id)
        return i;

    return -1;
  }

  int findData(GameResource *res) const
  {
    for (int i = 0; i < resList.size(); ++i)
      if (resList[i].data.get() == reinterpret_cast<const DataBlock *>(res))
        return i;

    return -1;
  }

  unsigned getResClassId() override { return ImpostorDataGameResClassId; }
  const char *getResClassName() override { return "impostorData"; }

  bool isResLoaded(int res_id) override { return findRes(res_id) >= 0; }
  bool checkResPtr(GameResource *res) override { return findData(res) >= 0; }

  GameResource *getGameResource(int res_id) override
  {
    int id = findRes(res_id);
    if (id < 0)
      ::load_game_resource_pack(res_id);

    id = findRes(res_id);
    if (id < 0)
      return nullptr;

    resList[id].refCount++;

    return reinterpret_cast<GameResource *>(resList[id].data.get());
  }

  bool addRefGameResource(GameResource *resource) override
  {
    int id = findData(resource);
    if (id >= 0)
      resList[id].refCount++;

    return id >= 0;
  }
  void releaseGameResource(int res_id) override
  {
    int id = findRes(res_id);
    delRef(id);
  }
  bool releaseGameResource(GameResource *resource) override
  {
    int id = findData(resource);
    delRef(id);
    return id >= 0;
  }

  void delRef(int id)
  {
    if (id < 0)
      return;

    G_ASSERT(resList[id].data);
    G_ASSERT(resList[id].refCount >= 1);

    resList[id].refCount--;
  }

  bool freeUnusedResources(bool forced_free_unref_packs, bool once /*= false*/) override
  {
    bool result = false;

    for (int i = (int)resList.size() - 1; i >= 0; --i)
    {
      if (!resList[i].data || get_refcount_game_resource_pack_by_resid(resList[i].resId) > 0)
        continue;

      if (resList[i].refCount > 0)
      {
        if (!forced_free_unref_packs)
          continue;
        else
          FATAL_ON_UNLOADING_USED_RES(resList[i].resId, resList[i].refCount);
      }

      resList.erase(resList.begin() + i);
      result = true;
      if (once)
        break;
    }

    return result;
  }

  void loadGameResourceData(int res_id, IGenLoad &crd) override
  {
    WinCritSec &cs = get_gameres_main_cs();
    {
      WinAutoLock lock(cs);
      if (findRes(res_id) >= 0)
        return;
    }
    Data data;
    data.resId = res_id;
    data.refCount = 0;
    crd.beginBlock();
    RoDataBlock *blk = RoDataBlock::load(crd);
    data.data.reset(new DataBlock());
    *data.data = *blk;
    delete blk;
    crd.endBlock();

    WinAutoLock lock2(cs);
    resList.push_back(eastl::move(data));
  }

  void createGameResource(int, const int *, int) override {}

  void reset() override { resList.clear(); }

  IMPLEMENT_DUMP_RESOURCES_REF_COUNT(resList, resId, refCount)
};

static InitOnDemand<ImpostorDataGameResFactory> impostor_data_factory;

void register_impostor_data_gameres_factory()
{
  impostor_data_factory.demandInit();
  ::add_factory(impostor_data_factory);
}
