#include <gameRes/dag_gameResSystem.h>
#include <gameRes/dag_stdGameRes.h>
#include <shaders/dag_dynSceneRes.h>
#include <phys/dag_physResource.h>
#include <ioSys/dag_genIo.h>
#include <generic/dag_tab.h>
#include <generic/dag_initOnDemand.h>
#include <EASTL/utility.h>
#include <debug/dag_log.h>
#include <gameRes/dag_dumpResRefCountImpl.h>
#include <osApiWrappers/dag_critSec.h>


class PhysObjGameResFactory final : public GameResourceFactory
{
public:
  struct ResData
  {
    int resId;
    int modelCount = 0;
    Ptr<PhysicsResource> resource;
    ResData(int rid = -1) : resId(rid) {}
  };

  struct GameRes //-V730
  {
    int resId;
    int refCount = 0;
    eastl::unique_ptr<DynamicPhysObjectData> data;

    GameRes() = default;
    GameRes(GameRes &&) = default;
    GameRes &operator=(GameRes &&) = default;
    ~GameRes()
    {
      if (data)
        for (int i = 0; i < data->models.size(); i++)
          if (data->models[i])
            data->models[i]->delInstanceRef();
    }
  };

  Tab<ResData> resData;
  Tab<GameRes> gameRes;


  int findResData(int res_id) const
  {
    for (int i = 0; i < resData.size(); ++i)
      if (resData[i].resId == res_id)
        return i;

    return -1;
  }


  int findRes(int res_id) const
  {
    for (int i = 0; i < gameRes.size(); ++i)
      if (gameRes[i].resId == res_id)
        return i;

    return -1;
  }


  int findRes(GameResource *res) const
  {
    for (int i = 0; i < gameRes.size(); ++i)
      if (gameRes[i].data.get() == (DynamicPhysObjectData *)res)
        return i;

    return -1;
  }


  unsigned getResClassId() override { return PhysObjGameResClassId; }

  const char *getResClassName() override { return "PhysObj"; }

  bool isResLoaded(int res_id) override { return findRes(res_id) >= 0; }
  bool checkResPtr(GameResource *res) override { return findRes(res) >= 0; }

  GameResource *getGameResource(int res_id) override
  {
    int id = findRes(res_id);

    if (id < 0)
      ::load_game_resource_pack(res_id);

    id = findRes(res_id);

    if (id < 0)
      return NULL;

    gameRes[id].refCount++;
    return (GameResource *)gameRes[id].data.get();
  }


  bool addRefGameResource(GameResource *resource) override
  {
    int id = findRes(resource);
    if (id < 0)
      return false;

    gameRes[id].refCount++;
    return true;
  }


  void delRef(int id)
  {
    if (id < 0)
      return;

    if (gameRes[id].refCount == 0)
    {
      String name;
      get_game_resource_name(gameRes[id].resId, name);
      G_ASSERT_LOG(0, "Attempt to release %s resource '%s' with 0 refcount", "PhysObj", name.str());
      return;
    }

    gameRes[id].refCount--;
  }


  void releaseGameResource(int res_id) override
  {
    int id = findRes(res_id);
    delRef(id);
  }


  bool releaseGameResource(GameResource *resource) override
  {
    int id = findRes(resource);
    delRef(id);
    return id >= 0;
  }


  bool freeUnusedResources(bool forced_free_unref_packs, bool once /*= false*/) override
  {
    bool result = false;
    for (int i = gameRes.size() - 1; i >= 0; --i)
    {
      if (get_refcount_game_resource_pack_by_resid(gameRes[i].resId) > 0)
        continue;
      if (gameRes[i].refCount != 0)
      {
        if (!forced_free_unref_packs)
          continue;
        else
          FATAL_ON_UNLOADING_USED_RES(gameRes[i].resId, gameRes[i].refCount);
      }

      if (gameRes[i].data->nodeTree)
        ::release_game_resource((GameResource *)gameRes[i].data->nodeTree);

      erase_items(gameRes, i, 1);
      result = true;
      if (once)
        break;
    }

    if (result)
      for (int j = resData.size() - 1; j >= 0; j--)
        if (resData[j].resource->getRefCount() == 1)
          erase_items(resData, j, 1);

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

    ResData rrd(res_id);

    int label = cb.readInt();
    if (label == _MAKE4C('pho0'))
    {
      const int version = 1;
      if (cb.readInt() == version)
        rrd.modelCount = cb.readInt();
      else
        logerr("Invalid version PhysObj. Need rebuild resources");
    }
    else if (label == _MAKE4C('po1s'))
    {
      rrd.modelCount = 1;
      rrd.resource = PhysicsResource::loadResource(cb, 0);
    }

    WinAutoLock lock(cs);
    resData.push_back() = eastl::move(rrd);
  }


  void createGameResource(int res_id, const int *ref_ids, int num_refs) override
  {
    WinCritSec &cs = get_gameres_main_cs();
    PhysicsResource *resource;
    int modelCount;
    {
      WinAutoLock lock(cs);
      if (findRes(res_id) >= 0)
        return;
      int rid = findResData(res_id);
      if (rid < 0)
      {
        String name;
        get_game_resource_name(res_id, name);
        logerr("BUG: no PhysObj resource %s", name.str());
        return;
      }

      resource = resData[rid].resource.get();
      if ((num_refs < 2 && !resource) || (resource && num_refs < 1))
      {
        gameRes.push_back().resId = res_id;
        return;
      }
      modelCount = resData[rid].modelCount;
    }

    DynamicPhysObjectData *data = new DynamicPhysObjectData;

    for (int index = 0; index < modelCount; index++)
    {
      auto m = (DynamicRenderableSceneLodsResource *)::get_game_resource(ref_ids[index]);
      data->models.push_back(m);
      if (m)
        m->addInstanceRef();
      ::release_game_resource((GameResource *)m);
    }

    if (resource)
    {
      data->physRes = resource;
      data->nodeTree = (num_refs < modelCount + 1) ? NULL : (GeomNodeTree *)::get_game_resource(ref_ids[modelCount]);
    }
    else
    {
      data->physRes = (PhysicsResource *)::get_game_resource(ref_ids[modelCount]);
      ::release_game_resource((GameResource *)data->physRes.get());
      data->nodeTree = (GeomNodeTree *)::get_game_resource(ref_ids[modelCount + 1]);
    }

    WinAutoLock lock2(cs);
    GameRes &rd = gameRes.push_back();
    rd.resId = res_id;
    rd.data.reset(data);
  }

  void reset() override
  {
    if (!gameRes.empty())
    {
      logerr("%d leaked PhysObjs", gameRes.size());
      for (unsigned int resNo = 0; resNo < gameRes.size(); resNo++)
      {
        String name;
        get_game_resource_name(gameRes[resNo].resId, name);
        logerr("    '%s', refCount=%d", name.str(), gameRes[resNo].refCount);
      }
    }

    resData.clear();
    gameRes.clear();
  }

  IMPLEMENT_DUMP_RESOURCES_REF_COUNT(gameRes, resId, refCount)
};

static InitOnDemand<PhysObjGameResFactory> phys_obj_factory;

void register_phys_obj_gameres_factory()
{
  phys_obj_factory.demandInit();
  ::add_factory(phys_obj_factory);
}
