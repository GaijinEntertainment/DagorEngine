#include <gameRes/dag_gameResSystem.h>
#include <gameRes/dag_stdGameRes.h>
#include <phys/dag_physResource.h>
#include <generic/dag_tab.h>
#include <generic/dag_initOnDemand.h>
#include <util/dag_globDef.h>
#include <util/dag_string.h>
#include <debug/dag_log.h>
#include <gameRes/dag_dumpResRefCountImpl.h>
#include <osApiWrappers/dag_critSec.h>


class PhysSysGameResFactory : public GameResourceFactory
{
public:
  struct ResData
  {
    int resId = -1;
    Ptr<PhysicsResource> resource = nullptr;
  };

  Tab<ResData> resData;


  PhysSysGameResFactory() : resData(midmem_ptr()) {}


  int findRes(int res_id)
  {
    for (int i = 0; i < resData.size(); ++i)
      if (resData[i].resId == res_id)
        return i;

    return -1;
  }


  int findRes(PhysicsResource *res)
  {
    if (!res)
      return -1;

    for (int i = 0; i < resData.size(); ++i)
      if (resData[i].resource == res)
        return i;

    return -1;
  }


  virtual unsigned getResClassId() { return PhysSysGameResClassId; }


  virtual const char *getResClassName() { return "PhysSys"; }

  virtual bool isResLoaded(int res_id) { return findRes(::validate_game_res_id(res_id)) >= 0; }
  virtual bool checkResPtr(GameResource *res) { return findRes((PhysicsResource *)res) >= 0; }

  virtual GameResource *getGameResource(int res_id)
  {
    // get real-res id
    if (::validate_game_res_id(res_id) == NULL_GAMERES_ID)
      return NULL;

    int id = findRes(res_id);

    // no resource - load pack
    if (id < 0)
      ::load_game_resource_pack(res_id);

    id = findRes(res_id);
    if (id < 0)
      return NULL;

    resData[id].resource.addRef();
    return (GameResource *)(PhysicsResource *)resData[id].resource;
  }


  virtual bool addRefGameResource(GameResource *resource)
  {
    int id = findRes((PhysicsResource *)resource);
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


  virtual void releaseGameResource(int res_id)
  {
    if (::validate_game_res_id(res_id) == NULL_GAMERES_ID)
      return;

    int id = findRes(res_id);
    delRef(id);
  }


  virtual bool releaseGameResource(GameResource *resource)
  {
    int id = findRes((PhysicsResource *)resource);
    delRef(id);
    return id >= 0;
  }


  virtual bool freeUnusedResources(bool forced_free_unref_packs, bool once /*= false*/)
  {
    bool result = false;
    for (int i = resData.size() - 1; i >= 0; --i)
    {
      if (!resData[i].resource || get_refcount_game_resource_pack_by_resid(resData[i].resId) > 0)
        continue;
      if (resData[i].resource->getRefCount() > 1)
      {
        if (!forced_free_unref_packs)
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
    WinAutoLock lock(get_gameres_main_cs());
    int id = findRes(res_id);
    if (id >= 0)
      return;

    ResData &rd = resData.push_back();
    lock.unlockFinal();

    rd.resource = PhysicsResource::loadResource(cb, 0);
    rd.resId = res_id;

    if (!rd.resource)
    {
      String name;
      get_game_resource_name(res_id, name);
      logerr("Error loading PhysSys resource %s", name);
    }
  }
  virtual void createGameResource(int /*res_id*/, const int * /*reference_ids*/, int /*num_refs*/) {}

  virtual void reset() { resData.clear(); }

  IMPLEMENT_DUMP_RESOURCES_REF_COUNT(resData, resId, resource->getRefCount())
};

class RagdollGameResFactory : public PhysSysGameResFactory
{
public:
  virtual unsigned getResClassId() { return RagdollGameResClassId; }


  virtual const char *getResClassName() { return "Ragdoll"; }
};

static InitOnDemand<PhysSysGameResFactory> phys_sys_factory;
static InitOnDemand<RagdollGameResFactory> ragdoll_factory;

void register_phys_sys_gameres_factory()
{
  phys_sys_factory.demandInit();
  ::add_factory(phys_sys_factory);
}

void register_ragdoll_gameres_factory()
{
  ragdoll_factory.demandInit();
  ::add_factory(ragdoll_factory);
}
