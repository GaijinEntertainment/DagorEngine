#include <gameRes/dag_collisionResource.h>
#include <gameRes/dag_gameResSystem.h>
#include <generic/dag_initOnDemand.h>
#include <gameRes/dag_dumpResRefCountImpl.h>
#include <osApiWrappers/dag_critSec.h>

class CollisionGameResFactory final : public GameResourceFactory
{
public:
  struct ResData
  {
    int resId = -1;
    Ptr<CollisionResource> resource;
  };

  Tab<ResData> resData;

  CollisionGameResFactory() { CollisionResource::check_avx_mesh_api_support(); }

  int findResData(int res_id) const
  {
    for (int i = 0; i < resData.size(); ++i)
    {
      if (resData[i].resId == res_id)
        return i;
    }

    return -1;
  }


  int findRes(CollisionResource *res) const
  {
    if (!res)
      return -1;

    for (int i = 0; i < resData.size(); ++i)
    {
      if (resData[i].resource == res)
        return i;
    }

    return -1;
  }


  unsigned getResClassId() override { return CollisionGameResClassId; }

  const char *getResClassName() override { return "CollisionResource"; }

  bool isResLoaded(int res_id) override { return findResData(::validate_game_res_id(res_id)) >= 0; }
  bool checkResPtr(GameResource *res) override { return findRes((CollisionResource *)res) >= 0; }

  GameResource *getGameResource(int res_id) override
  {
    // get real-res id
    if (::validate_game_res_id(res_id) == NULL_GAMERES_ID)
      return NULL;

    int id = findResData(res_id);

    // no resource - load pack
    if (id < 0)
      ::load_game_resource_pack(res_id);

    id = findResData(res_id);
    if (id < 0)
      return NULL;

    resData[id].resource.addRef();
    return (GameResource *)(CollisionResource *)resData[id].resource;
  }


  bool addRefGameResource(GameResource *resource) override
  {
    int id = findRes((CollisionResource *)resource);
    if (id < 0)
      return false;

    resData[id].resource.addRef();
    return true;
  }


  void delRef(int id)
  {
    if (id < 0)
      return;
    if (!resData[id].resource)
      return;

    resData[id].resource.delRef();
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
    int id = findRes((CollisionResource *)resource);
    delRef(id);
    return id >= 0;
  }


  bool freeUnusedResources(bool force, bool once /*= false*/) override
  {
    bool result = false;
    for (int i = resData.size() - 1; i >= 0; --i)
    {
      if (get_refcount_game_resource_pack_by_resid(resData[i].resId) > 0)
        continue;
      if (!resData[i].resource || resData[i].resource->getRefCount() > 1)
      {
        if (!force)
          continue;
        else
          FATAL_ON_UNLOADING_USED_RES(resData[i].resId, resData[i].resource->getRefCount());
      }
      erase_items(resData, i, 1);
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

    CollisionResource *resource = CollisionResource::loadResource(cb, res_id);
    if (!resource)
    {
      String name;
      get_game_resource_name(res_id, name);
      logerr("ERROR: loading collision resource %s", name);
    }

    WinAutoLock lock2(cs);
    ResData &rd = resData.push_back();
    rd.resource = resource;
    rd.resId = res_id;
  }
  void createGameResource(int /*res_id*/, const int * /*reference_ids*/, int /*num_refs*/) override {}

  void reset() override
  {
    freeUnusedResources(false, false);
    if (!resData.empty())
    {
      logerr("%d leaked Collision", resData.size());
      for (unsigned int resNo = 0; resNo < resData.size(); resNo++)
      {
        String name;
        get_game_resource_name(resData[resNo].resId, name);
        logerr("    '%s', refCount=%d", name.str(), resData[resNo].resource->getRefCount());
      }
    }

    resData.clear();
  }

  IMPLEMENT_DUMP_RESOURCES_REF_COUNT(resData, resId, resource->getRefCount())
};


static InitOnDemand<CollisionGameResFactory> collision_gameres_factory;

void CollisionResource::registerFactory() { add_factory(collision_gameres_factory.demandInit()); }
