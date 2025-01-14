// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <gameRes/dag_gameResSystem.h>
#include <gameRes/dag_stdGameRes.h>
#include <anim/dag_animBlend.h>
#include <ioSys/dag_genIo.h>
#include <ioSys/dag_dataBlock.h>
#include <ioSys/dag_roDataBlock.h>
#include <generic/dag_tab.h>
#include <generic/dag_initOnDemand.h>
#include <EASTL/vector.h>
#include <EASTL/unique_ptr.h>
#include <debug/dag_log.h>
#include <debug/dag_debug.h>
#include <animChar/dag_animCharacter2.h>
#include <gameRes/dag_dumpResRefCountImpl.h>
#include <osApiWrappers/dag_critSec.h>

class AnimBnlGameResFactory final : public GameResourceFactory
{
public:
  struct GameData
  {
    int resId;
    int refCount;

    PtrTab<AnimV20::AnimData> a2d_list;

    GameData() : resId(-1), refCount(0), a2d_list(tmpmem) {}
  };

  Tab<GameData> gameData;

  unsigned getResClassId() override { return AnimBnlGameResClassId; }
  const char *getResClassName() override { return "AnimBnl"; }

  bool isResLoaded(int res_id) override { return findRes(res_id) >= 0; }
  bool checkResPtr(GameResource *res) override { return findGameData(res) >= 0; }

  int findRes(int res_id) const
  {
    for (int i = 0; i < gameData.size(); ++i)
      if (gameData[i].resId == res_id)
        return i;

    return -1;
  }


  int findGameData(GameResource *res) const
  {
    for (int i = 0; i < gameData.size(); ++i)
      if ((void *)&gameData[i].a2d_list == (void *)res)
        return i;

    return -1;
  }

  GameResource *getGameResource(int res_id) override
  {
    int id = findRes(res_id);

    if (id < 0)
      ::load_game_resource_pack(res_id);

    id = findRes(res_id);

    if (id < 0)
      return NULL;

    gameData[id].refCount++;
    return (GameResource *)(void *)(&gameData[id].a2d_list);
  }


  bool addRefGameResource(GameResource *resource) override
  {
    int id = findGameData(resource);
    if (id < 0)
      return false;

    gameData[id].refCount++;
    return true;
  }


  void delRef(int id)
  {
    if (id < 0)
      return;

    if (gameData[id].refCount == 0)
    {
      String name;
      get_game_resource_name(gameData[id].resId, name);
      G_ASSERT_LOG(0, "Attempt to release %s resource '%s' with 0 refcount", "AnimBnl", name.str());
      return;
    }

    gameData[id].refCount--;
  }


  void releaseGameResource(int res_id) override
  {
    int id = findRes(res_id);
    delRef(id);
  }


  bool releaseGameResource(GameResource *resource) override
  {
    int id = findGameData(resource);
    delRef(id);
    return id >= 0;
  }


  bool freeUnusedResources(bool forced_free_unref_packs, bool once /*= false*/) override
  {
    bool ret = false;
    for (int i = gameData.size() - 1; i >= 0; --i)
    {
      if (get_refcount_game_resource_pack_by_resid(gameData[i].resId) > 0)
        continue;
      if (gameData[i].refCount != 0)
      {
        if (!forced_free_unref_packs)
          continue;
        else
          FATAL_ON_UNLOADING_USED_RES(gameData[i].resId, gameData[i].refCount);
      }

      erase_items(gameData, i, 1);
      ret = true;
      if (once)
        break;
    }
    return ret;
  }


  void loadGameResourceData(int /*res_id*/, IGenLoad & /*cb*/) override {}


  void createGameResource(int res_id, const int *ref_ids, int num_refs) override
  {
    WinCritSec &cs = get_gameres_main_cs();
    {
      WinAutoLock lock(cs);
      if (findRes(res_id) >= 0)
        return;

      if (!num_refs)
      {
        gameData.push_back().resId = res_id;
        return;
      }
    }

    PtrTab<AnimV20::AnimData> a2d_list;

    for (int i = 0; i < num_refs; ++i)
      if (ref_ids[i] == NULL_GAMERES_ID)
        a2d_list.push_back(NULL);
      else
      {
        a2d_list.push_back((AnimV20::AnimData *)::get_game_resource(ref_ids[i]));
        ::release_game_resource(ref_ids[i]);
      }

    WinAutoLock lock2(cs);
    GameData &gd = gameData.push_back();
    gd.a2d_list.swap(a2d_list);
    gd.resId = res_id;
  }

  void reset() override { gameData.clear(); }

  IMPLEMENT_DUMP_RESOURCES_REF_COUNT(gameData, resId, refCount)
};

/****************************************************************************/

class AnimCharGameResFactory final : public GameResourceFactory
{
public:
  struct ResData
  {
    int resId;
    int ver;

    eastl::unique_ptr<DataBlock> blk;
    SimpleString agResName;

    ResData() = default;
    ResData(int rid) : resId(rid) {}
  };

  struct GameRes
  {
    int resId = NULL_GAMERES_ID;
    int refCount = 0;
    AnimCharData charData;
  };

  eastl::vector<ResData> resData;
  Tab<GameRes> gameRes;
  bool warnAboutMissingAnimRes = true;


  int findResData(int res_id) const
  {
    for (int i = 0; i < resData.size(); ++i)
      if (resData[i].resId == res_id)
        return i;

    return -1;
  }


  int findGameRes(int res_id) const
  {
    for (int i = 0; i < gameRes.size(); ++i)
      if (gameRes[i].resId == res_id)
        return i;

    return -1;
  }

  int findGameRes(AnimCharData *res) const
  {
    for (int i = 0; i < gameRes.size(); ++i)
      if (&(gameRes[i].charData) == res)
        return i;

    return -1;
  }

  unsigned getResClassId() override { return AnimCharGameResClassId; }

  const char *getResClassName() override { return "AnimChar"; }

  bool isResLoaded(int res_id) override { return findGameRes(res_id) >= 0; }
  bool checkResPtr(GameResource *res) override { return findGameRes((AnimCharData *)res) >= 0; }

  GameResource *getGameResource(int res_id) override
  {
    int id = findGameRes(res_id);
    if (id < 0)
      ::load_game_resource_pack(res_id);

    id = findGameRes(res_id);
    if (id < 0)
      return NULL;

    gameRes[id].refCount++;
    return (GameResource *)(void *)(&gameRes[id].charData);
  }


  bool addRefGameResource(GameResource *resource) override
  {
    int id = findGameRes((AnimCharData *)resource);
    if (id < 0)
      return false;

    gameRes[id].refCount++;
    return true;
  }


  void releaseGameResource(int res_id) override
  {
    int id = findGameRes(res_id);
    if (id < 0)
      return;
    gameRes[id].refCount--;
  }


  bool releaseGameResource(GameResource *resource) override
  {
    int id = findGameRes((AnimCharData *)resource);
    if (id < 0)
      return false;

    gameRes[id].refCount--;
    return true;
  }


  bool freeUnusedResources(bool forced_free_unref_packs, bool once /*= false*/) override
  {
    bool result = false;
    for (int i = gameRes.size() - 1; i >= 0; --i)
    {
      if (get_refcount_game_resource_pack_by_resid(gameRes[i].resId) > 0)
        continue;
      if (gameRes[i].refCount > 1)
      {
        if (!forced_free_unref_packs)
          continue;
        else
          FATAL_ON_UNLOADING_USED_RES(gameRes[i].resId, gameRes[i].refCount);
      }

      erase_items(gameRes, i, 1);
      result = true;
      if (once)
        break;
    }

    bool result2 = false;
    for (int i = resData.size() - 1; i >= 0; i--)
    {
      bool used = false;
      int rid = resData[i].resId;
      for (int j = 0; j < gameRes.size(); j++)
        if (gameRes[j].resId == rid)
        {
          used = true;
          break;
        }
      if (used || get_refcount_game_resource_pack_by_resid(rid) > 0)
        continue;

      resData.erase(resData.begin() + i);
      result2 = true;
      if (once)
        break;
    }

    return result || result2;
  }


  void loadGameResourceData(int res_id, IGenLoad &cb) override
  {
    WinCritSec &cs = get_gameres_main_cs();
    {
      WinAutoLock lock(cs);
      if (findResData(res_id) >= 0)
        return;
    }

    ResData rr(res_id);

    int ver = cb.readInt();
    if (ver != _MAKE4C('ac2') && ver != _MAKE4C('ac1') && ver != _MAKE4C('ac1+'))
    {
      String name;
      get_game_resource_name(res_id, name);
      rr.resId = res_id;
      DAG_FATAL("Obsolete format %c%c%c%c for AnimChar resource %s\nReexport resources, please", _DUMP4C(ver), name.str());
      return;
    }
    rr.ver = ver;

    RoDataBlock *blk = RoDataBlock::load(cb);
    rr.blk.reset(new DataBlock());
    *rr.blk = *blk;
    delete blk;

    if (ver == _MAKE4C('ac2'))
    {
      cb.readString(rr.agResName);
    }
    else if (ver == _MAKE4C('ac1'))
    {
      //== obsolete
    }
    else if (ver == _MAKE4C('ac1+'))
      cb.readString(rr.agResName);

    if (!*rr.agResName.c_str())
      rr.agResName.clear();

    {
      WinAutoLock lock2(cs);
      resData.push_back(eastl::move(rr));
    }
  }


  void createGameResource(int res_id, const int *ref_ids, int num_refs) override
  {
    WinCritSec &cs = get_gameres_main_cs();
    String name;
    int ver;
    const char *agResName;
    DataBlock *blk;
    {
      WinAutoLock lock(cs);
      if (findGameRes(res_id) >= 0)
        return;
      int id = findResData(res_id);
      get_game_resource_name(res_id, name);
      if (id < 0)
      {
        logerr("BUG: no animChar resource %s", name.str());
        return;
      }
      ResData &rr = resData[id];
      ver = rr.ver;
      agResName = rr.agResName.c_str();
      blk = rr.blk.get();
    }

    FATAL_CONTEXT_AUTO_SCOPE(name);

    GameRes gr;
    gr.resId = res_id;
    gr.charData.agResName = agResName;

    Tab<int> nodesWithIgnoredAnimation(tmpmem);

    if (ver == _MAKE4C('ac2'))
    {
      SmallTab<AnimV20::AnimData *, TmpmemAlloc> a2d_list;
      clear_and_resize(a2d_list, num_refs);
      for (int i = 0; i < num_refs; i++)
      {
        if (ref_ids[i] != NULL_GAMERES_ID)
        {
          int res_id_ = ref_ids[i];
          a2d_list[i] = (AnimV20::AnimData *)::get_game_resource(res_id_);
          G_ASSERTF(!(i == 0 && !a2d_list[i] && is_ignoring_unavailable_resources()) || !warnAboutMissingAnimRes,
            "%s can't load animation #0 to ignore unavailable resources (see log)", __FUNCTION__);
          if (!a2d_list[i] && is_ignoring_unavailable_resources() && i)
          {
            res_id_ = ref_ids[0];
            a2d_list[i] = (AnimV20::AnimData *)::get_game_resource(res_id_);
            nodesWithIgnoredAnimation.push_back(i);
          }
#if DAGOR_DBGLEVEL > 0
          if (!a2d_list[i] && warnAboutMissingAnimRes)
          {
            String res_name;
            get_game_resource_name(res_id_, res_name);
            G_ASSERTF(0, "%s can't load resource '%s'", __FUNCTION__, res_name.str());
          }
#endif
        }
        else
          a2d_list[i] = NULL;
      }

      gr.charData.graph = AnimResManagerV20::loadUniqueAnimGraph(*blk, a2d_list, nodesWithIgnoredAnimation);

      for (int i = 0; i < a2d_list.size(); i++)
        if (a2d_list[i])
          ::release_game_resource((GameResource *)a2d_list[i]);
    }
    else
    {
      Tab<AnimV20::AnimData *> *a2d_list = (Tab<AnimV20::AnimData *> *)::get_game_resource(ref_ids[0]);

      gr.charData.graph = AnimResManagerV20::loadUniqueAnimGraph(*blk, *a2d_list, nodesWithIgnoredAnimation);
      ::release_game_resource(ref_ids[0]);
    }
    if (gr.charData.graph)
    {
      gr.charData.graph->resId = res_id;
      gr.charData.graph->addRef();
    }

    {
      WinAutoLock lock2(cs);
      memcpy(&gameRes.push_back(), &gr, sizeof(gr)); //-V780
    }

    gr.charData.graph = nullptr;
  }

  void reset() override
  {
    resData.clear();
    gameRes.clear();
    AnimV20::AnimBlender::releaseContextMemory();
  }

  IMPLEMENT_DUMP_RESOURCES_REF_COUNT(gameRes, resId, refCount)
};

static InitOnDemand<AnimCharGameResFactory> char_factory;
static InitOnDemand<AnimBnlGameResFactory> bnl_factory;

void register_animchar_gameres_factory(bool warn_about_missing_anim)
{
  bnl_factory.demandInit();
  ::add_factory(bnl_factory);

  char_factory.demandInit();
  char_factory->warnAboutMissingAnimRes = warn_about_missing_anim;
  ::add_factory(char_factory);
}
