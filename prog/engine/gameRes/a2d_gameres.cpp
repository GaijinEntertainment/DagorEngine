#include <gameRes/dag_gameResSystem.h>
#include <gameRes/dag_stdGameRes.h>
#include <anim/dag_animChannels.h>
#include <ioSys/dag_genIo.h>
#include <generic/dag_tab.h>
#include <generic/dag_initOnDemand.h>
#include <debug/dag_log.h>
#include <stdio.h>
#include <gameRes/dag_dumpResRefCountImpl.h>
#include <osApiWrappers/dag_critSec.h>
#include <ioSys/dag_oodleIo.h>
#include <ioSys/dag_zstdIo.h>
#include <ioSys/dag_btagCompr.h>


class Anim2DataGameResFactory final : public GameResourceFactory
{
public:
  struct A2dData
  {
    int resId = -1;
    int refCount = 0;
    Ptr<AnimV20::AnimData> anim;

    A2dData() = default;
    A2dData(int rid, AnimV20::AnimData *_anim) : resId(rid), anim(_anim) {}
  };

  Tab<A2dData> a2dData;


  int findRes(int res_id) const
  {
    for (int i = 0; i < a2dData.size(); ++i)
      if (a2dData[i].resId == res_id)
        return i;

    return -1;
  }


  int findAnim2Data(AnimV20::AnimData *_anim) const
  {
    for (int i = 0; i < a2dData.size(); ++i)
      if (a2dData[i].anim == _anim)
        return i;

    return -1;
  }


  unsigned getResClassId() override { return Anim2DataGameResClassId; }

  const char *getResClassName() override { return "Anim2Data"; }

  bool isResLoaded(int res_id) override { return findRes(::validate_game_res_id(res_id)) >= 0; }
  bool checkResPtr(GameResource *res) override { return findAnim2Data((AnimV20::AnimData *)res) >= 0; }

  GameResource *getGameResource(int res_id) override
  {
    // get real-res id
    if (::validate_game_res_id(res_id) == NULL_GAMERES_ID)
      return (GameResource *)NULL;

    int id = findRes(res_id);

    // no data - load pack
    if (id < 0)
    {
      ::load_game_resource_pack(res_id);
      id = findRes(res_id);
      if (id < 0)
        return (GameResource *)NULL;
    }

    a2dData[id].refCount++;
    return (GameResource *)a2dData[id].anim.get();
  }


  bool addRefGameResource(GameResource *resource) override
  {
    int id = findAnim2Data((AnimV20::AnimData *)resource);
    if (id >= 0)
      a2dData[id].refCount++;
    return id >= 0;
  }

  void delRef(int id)
  {
    if (id < 0)
      return;

    if (a2dData[id].refCount == 0)
    {
      String name;
      get_game_resource_name(a2dData[id].resId, name);
      G_ASSERT_LOG(0, "Attempt to release %s resource '%s' with 0 refcount", "a2d", name.str());
      return;
    }

    a2dData[id].refCount--;
  }


  void releaseGameResource(int res_id) override { delRef(findRes(::validate_game_res_id(res_id))); }


  bool releaseGameResource(GameResource *resource) override
  {
    int id = findAnim2Data((AnimV20::AnimData *)resource);
    delRef(id);
    return id >= 0;
  }


  bool freeUnusedResources(bool forced_free_unref_packs, bool once /*= false*/) override
  {
    bool ret = false;
    for (int i = a2dData.size() - 1; i >= 0; --i)
    {
      if (!a2dData[i].anim || get_refcount_game_resource_pack_by_resid(a2dData[i].resId) > 0)
        continue;
      if (a2dData[i].refCount)
      {
        if (!forced_free_unref_packs)
          continue;
        else
          FATAL_ON_UNLOADING_USED_RES(a2dData[i].resId, a2dData[i].refCount);
      }

      erase_items(a2dData, i, 1);
      ret = true;
      if (once)
        break;
    }
    return ret;
  }


  void loadGameResourceData(int res_id, IGenLoad &cb) override
  {
    WinCritSec &cs = get_gameres_main_cs();
    {
      WinAutoLock lock(cs);
      if (findRes(res_id) >= 0)
        return;
    }

    unsigned int blockFlags = 0;
    const int compr_data_sz = cb.beginBlock(&blockFlags);

    uint8_t zcrdStorage[max(sizeof(ZstdLoadCB), sizeof(OodleLoadCB))];
    IGenLoad *zcrd = nullptr;
    if (blockFlags == btag_compr::ZSTD)
      zcrd = new (zcrdStorage, _NEW_INPLACE) ZstdLoadCB(cb, compr_data_sz);
    else if (blockFlags == btag_compr::OODLE)
      zcrd = new (zcrdStorage, _NEW_INPLACE) OodleLoadCB(cb, compr_data_sz - 4, cb.readInt());

    AnimV20::AnimData *anim = new AnimV20::AnimData(res_id);
    if (!anim->load(zcrd ? *zcrd : cb))
    {
      anim->addRef();
      anim->delRef();
      anim = NULL; //-V773
    }
    if (zcrd)
      zcrd->~IGenLoad();
    cb.endBlock();

    {
      WinAutoLock lock2(cs);
      a2dData.push_back(A2dData(res_id, anim));
    }

    if (!anim)
    {
      String name;
      get_game_resource_name(res_id, name);
      logerr("Error loading Anim2Data resource %s", name);
    }
  }
  void createGameResource(int /*res_id*/, const int * /*reference_ids*/, int /*num_refs*/) override {}

  void reset() override { a2dData.clear(); }

  IMPLEMENT_DUMP_RESOURCES_REF_COUNT(a2dData, resId, refCount)
};

static InitOnDemand<Anim2DataGameResFactory> a2d_factory;

void register_a2d_gameres_factory()
{
  a2d_factory.demandInit();
  ::add_factory(a2d_factory);
}
