// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <gameRes/dag_gameResSystem.h>
#include <gameRes/dag_stdGameRes.h>
#include <fx/dag_baseFxClasses.h>
#include <shaders/dag_shaderMeshTexLoadCtrl.h>
#include <util/dag_string.h>
#include <memory/dag_framemem.h>
#include <ioSys/dag_genIo.h>
#include <generic/dag_smallTab.h>
#include <EASTL/vector.h>
#include <EASTL/unique_ptr.h>
#include <generic/dag_initOnDemand.h>
#include <debug/dag_log.h>
#include <3d/dag_texMgr.h>
#include <3d/dag_texMgrTags.h>
#include <fx/dag_paramScriptsPool.h>
#include <drv/3d/dag_tex3d.h>
#include <gameRes/dag_dumpResRefCountImpl.h>
#include <osApiWrappers/dag_critSec.h>


static String tmpStr;

class EffectGameResFactory final : public GameResourceFactory
{
public:
  struct ResData
  {
    int resId;
    BaseEffectFactory *factory;
    SmallTab<char, MidmemAlloc> data;
  };

  struct GameRes
  {
    int resId;
    int refCount;
    eastl::unique_ptr<BaseEffectObject> object;
  };

  eastl::vector<ResData> resData;
  eastl::vector<GameRes> gameRes;

  ParamScriptsPool fxPool;


  EffectGameResFactory() : fxPool(inimem) {}


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


  int findGameRes(GameResource *res) const
  {
    for (int i = 0; i < gameRes.size(); ++i)
      if (gameRes[i].object.get() == (BaseEffectObject *)res)
        return i;

    return -1;
  }


  unsigned getResClassId() override { return EffectGameResClassId; }

  const char *getResClassName() override { return "Effect"; }

  bool isResLoaded(int res_id) override { return findRes(res_id) >= 0; }
  bool checkResPtr(GameResource *res) override { return findGameRes(res) >= 0; }

  GameResource *getGameResource(int res_id) override
  {
    int id = findRes(res_id);

    if (id < 0)
      ::load_game_resource_pack(res_id);

    id = findRes(res_id);

    if (id < 0)
      return NULL;

    auto res = (GameResource *)gameRes[id].object.get();
    gameRes[id].refCount += res ? 1 : 0;
    return res;
  }


  bool addRefGameResource(GameResource *resource) override
  {
    int id = findGameRes(resource);
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
      G_ASSERT_LOG(0, "Attempt to release %s resource '%s' with 0 refcount", "Effect", name.str());
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
    int id = findGameRes(resource);
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

      gameRes.erase(gameRes.begin() + i);
      result = true;
      if (once)
        break;
    }

    if (gameRes.empty())
      decltype(gameRes)().swap(gameRes);
    decltype(resData)().swap(resData);

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

    String className;
    cb.readShortString(className);

    ParamScriptFactory *factory = fxPool.getFactoryByName(className);
    if (!factory || !factory->isSubOf(HUID_BaseEffectObject))
    {
      String resName(framemem_ptr());
      get_game_resource_name(res_id, resName);
      logwarn("no factory for fx class '%s' while attempting to create '%s'", className.str(), resName);
      factory = nullptr;
    }

    decltype(ResData::data) data;
    if (factory)
    {
      int len = cb.beginBlock();
      clear_and_resize(data, len);
      cb.read(&data[0], len);
      cb.endBlock();
    }

    WinAutoLock lock2(cs);
    resData.push_back(ResData{res_id, (BaseEffectFactory *)factory, eastl::move(data)});
  }


  void createGameResource(int res_id, const int *ref_ids, int num_refs) override
  {
    WinCritSec &cs = get_gameres_main_cs();
    BaseEffectFactory *factory;
    const char *dataPtr;
    int dataLen;
    {
      WinAutoLock lock(cs);
      if (findRes(res_id) >= 0)
        return;
      int id = findResData(res_id);
      if (id < 0)
      {
        String name;
        get_game_resource_name(res_id, name);
        logerr("BUG: no Effect resource %s", name.str());
        return;
      }
      auto &rrd = resData[id];
      factory = rrd.factory;
      dataPtr = rrd.data.data();
      dataLen = data_size(rrd.data);
    }

    class LoadCB final : public BaseParamScriptLoadCB
    {
    public:
      const int *refIds;
      int numRefs;
      int selfId;

      LoadCB(const int *ref_ids, int num_refs, int self_id) : refIds(ref_ids), numRefs(num_refs), selfId(self_id) {}

      int getSelfGameResId() override { return selfId; }
      void *getReference(int id) override
      {
        if (id < 0 || id >= numRefs)
          return NULL;

        if (get_gameres_sys_ver() == 2)
        {
          textag_mark_begin(TEXTAG_FX);
          ::get_game_resource_name(refIds[id], tmpStr);
          tmpStr += "*";
          TEXTUREID texid = ::get_managed_texture_id(tmpStr);

          if (texid != BAD_TEXTUREID)
          {
            String tmp_storage;
            dagor_set_sm_tex_load_ctx_type(EffectGameResClassId);
            dagor_set_sm_tex_load_ctx_name(NULL);
            if (const char *name = IShaderMatVdataTexLoadCtrl::preprocess_tex_name(tmpStr, tmp_storage))
              texid = ::get_managed_texture_id(name);
            else
              texid = BAD_TEXTUREID;
            dagor_reset_sm_tex_load_ctx();

            BaseTexture *tex = acquire_managed_tex(texid);
            if (tex)
            {
              tex->setAnisotropy(1);
              add_anisotropy_exception(texid);
            }
            textag_mark_end();
            return (void *)(uintptr_t) unsigned(texid);
          }
          textag_mark_end();
          int cls = get_game_res_class_id(refIds[id]);
          if (cls && cls != EffectGameResClassId)
          {
            ::get_game_resource_name(refIds[id], tmpStr);
            logerr("bad FX ref[%d]: id=%d name=%s cls=%08X", id, refIds[id], tmpStr, cls);
            return NULL;
          }
          return ::get_game_resource(refIds[id]);
        }

        return ::get_game_resource(refIds[id]);
      }
    } loadCb(ref_ids, num_refs, res_id);

    auto object = factory ? (BaseEffectObject *)factory->createObject() : nullptr;
    if (object)
    {
      object->res_id = res_id;
      object->loadParamsData(dataPtr, dataLen, &loadCb);
    }

    WinAutoLock lock2(cs);
    gameRes.push_back(GameRes{res_id, 0, eastl::unique_ptr<BaseEffectObject>(object)});
  }

  void reset() override
  {
    if (!gameRes.empty())
      logerr("%d leaked effects", gameRes.size());

    resData.clear();
    gameRes.clear();
    fxPool.unloadAll();
  }

  IMPLEMENT_DUMP_RESOURCES_REF_COUNT(gameRes, resId, refCount)
};

static InitOnDemand<EffectGameResFactory> fx_factory;

void register_effect_factory(BaseEffectFactory *f)
{
  fx_factory.demandInit();
  fx_factory->fxPool.registerFactory(f);
}

ParamScriptsPool *get_effect_scripts_pool()
{
  if (!fx_factory)
    return NULL;
  return &fx_factory->fxPool;
}

void register_effect_gameres_factory()
{
  fx_factory.demandInit();
  ::add_factory(fx_factory);
}
