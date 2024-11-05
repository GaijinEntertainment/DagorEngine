// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <gameRes/dag_gameResSystem.h>
#include <gameRes/dag_stdGameRes.h>
#include <math/dag_geomTree.h>
#include <generic/dag_initOnDemand.h>
#include <util/dag_globDef.h>
#include <util/dag_string.h>
#include <EASTL/vector.h>
#include <EASTL/unique_ptr.h>
#include <debug/dag_log.h>
#include <gameRes/dag_dumpResRefCountImpl.h>
#include <osApiWrappers/dag_critSec.h>


class GeomNodeTreeGameResFactory final : public GameResourceFactory
{
public:
  struct TreeData
  {
    int resId;
    int refCount;
    eastl::unique_ptr<GeomNodeTree> nodeTree;
  };

  eastl::vector<TreeData> treeData;


  int findResData(int res_id) const
  {
    for (int i = 0; i < treeData.size(); ++i)
      if (treeData[i].resId == res_id)
        return i;

    return -1;
  }


  int findTree(GeomNodeTree *tree) const
  {
    if (!tree)
      return -1;

    for (int i = 0; i < treeData.size(); ++i)
      if (treeData[i].nodeTree.get() == tree)
        return i;

    return -1;
  }


  unsigned getResClassId() override { return GeomNodeTreeGameResClassId; }


  const char *getResClassName() override { return "GeomNodeTree"; }

  bool isResLoaded(int res_id) override { return findResData(::validate_game_res_id(res_id)) >= 0; }
  bool checkResPtr(GameResource *res) override { return findTree((GeomNodeTree *)res) >= 0; }

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

    treeData[id].refCount++;

    return (GameResource *)treeData[id].nodeTree.get();
  }


  bool addRefGameResource(GameResource *resource) override
  {
    int id = findTree((GeomNodeTree *)resource);
    if (id < 0)
      return false;

    treeData[id].refCount++;
    return true;
  }


  void delRef(int id)
  {
    if (id < 0)
      return;

    if (treeData[id].refCount == 0)
    {
      String name;
      get_game_resource_name(treeData[id].resId, name);
      G_ASSERT_LOG(0, "Attempt to release %s resource '%s' with 0 refcount", "GeomNodeTree", name.str());
      return;
    }

    treeData[id].refCount--;
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
    int id = findTree((GeomNodeTree *)resource);
    delRef(id);
    return id >= 0;
  }


  bool freeUnusedResources(bool forced_free_unref_packs, bool once /*= false*/) override
  {
    bool result = false;
    for (int i = treeData.size() - 1; i >= 0; --i)
    {
      if (get_refcount_game_resource_pack_by_resid(treeData[i].resId) > 0)
        continue;
      if (treeData[i].refCount != 0)
      {
        if (!forced_free_unref_packs)
          continue;
        else
          FATAL_ON_UNLOADING_USED_RES(treeData[i].resId, treeData[i].refCount);
      }
      treeData.erase(treeData.begin() + i);
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

    TreeData td{res_id, 0, eastl::make_unique<GeomNodeTree>()};
    td.nodeTree->load(cb);
    td.nodeTree->partialCalcWtm(dag::Index16(td.nodeTree->nodeCount()));

    WinAutoLock lock2(cs);
    treeData.push_back(eastl::move(td));
  }


  void createGameResource(int /*res_id*/, const int * /*reference_ids*/, int /*num_refs*/) override {}

  void reset() override { treeData.clear(); }

  IMPLEMENT_DUMP_RESOURCES_REF_COUNT(treeData, resId, refCount)
};

static InitOnDemand<GeomNodeTreeGameResFactory> geom_node_tree_factory;

void register_geom_node_tree_gameres_factory()
{
  geom_node_tree_factory.demandInit();
  ::add_factory(geom_node_tree_factory);
}
