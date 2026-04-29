// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <gameRes/dag_gameResSystem.h>
#include <gameRes/dag_stdGameRes.h>
#include <animChar/dag_animCharacter2.h>
#include <ioSys/dag_genIo.h>
#include <ioSys/dag_roDataBlock.h>
#include <generic/dag_initOnDemand.h>
#include <debug/dag_log.h>
#include <debug/dag_debug.h>
#include <gameRes/dag_dumpResRefCountImpl.h>
#include <osApiWrappers/dag_critSec.h>

namespace AnimV20
{

AnimCharCreationProps::AnimCharCreationProps(AnimCharCreationProps::CtorNoInit) {} //-V730

AnimCharCreationProps::AnimCharCreationProps()
{
  noAnimDist = noRenderDist = 1000;
  bsphRad = 1000;
  createVisInst = false;

  useCharDep = false;
  pyOfs = 0;
  pxScale = pyScale = pzScale = 1;
  sScale = 1;
}

void AnimCharCreationProps::release()
{
  if (centerNode.c_str() != (char *)&centerNode) // ...not SSOed
    centerNode.detach();
  rootNode.setStrRaw(nullptr);
  props.release();
}

} // namespace AnimV20

class CharacterGameResFactory final : public GameResourceFactory
{
public:
  struct AnimChar : public AnimV20::AnimCharCreationProps
  {
    int resId;
    int refCount = 0;
    eastl::unique_ptr<AnimV20::IAnimCharacter2> animChar;
    AnimChar(int rid) : resId(rid) {}
  };
  dag::Vector<AnimChar> animChars;

  int findRes(int res_id)
  {
    for (int i = 0; i < animChars.size(); ++i)
      if (animChars[i].resId == res_id)
        return i;

    return -1;
  }


  int findAnimChar(GameResource *res)
  {
    for (int i = 0; i < animChars.size(); ++i)
      if (animChars[i].animChar.get() == (AnimV20::IAnimCharacter2 *)res)
        return i;

    return -1;
  }


  unsigned getResClassId() override { return CharacterGameResClassId; }
  const char *getResClassName() override { return "Character"; }

  bool isResLoaded(int res_id) override
  {
    int id = findRes(res_id);
    return id >= 0 && animChars[id].animChar;
  }
  bool checkResPtr(GameResource *res) override { return findAnimChar(res) >= 0; }

  GameResource *getGameResource(RRL rrl, int res_id) override
  {
    WinAutoLock lock(get_gameres_main_cs());
    int id = findRes(res_id);
    if (id < 0)
    {
      load_game_resource_pack_gameres_main_cs_locked(res_id, rrl);
      id = findRes(res_id);
    }
    if (id < 0)
      return NULL;
    animChars[id].refCount++;
    return (GameResource *)animChars[id].animChar.get();
  }


  bool addRefGameResource(GameResource *resource) override
  {
    int id = findAnimChar(resource);
    if (id < 0)
      return false;

    animChars[id].refCount++;
    return true;
  }


  void delRef(int id)
  {
    if (id < 0)
      return;

    if (animChars[id].refCount == 0) [[unlikely]]
    {
      String name;
      get_game_resource_name(animChars[id].resId, name);
      G_ASSERT_LOG(0, "Attempt to release %s resource '%s' with 0 refcount", "Character", name.str());
      return;
    }

    animChars[id].refCount--;
  }


  void releaseGameResource(int res_id) override
  {
    int id = findRes(res_id);
    delRef(id);
  }


  bool releaseGameResource(GameResource *resource) override
  {
    int id = findAnimChar(resource);
    delRef(id);
    return id >= 0;
  }


  bool freeUnusedResources(RRL rrl, bool forced_free_unref_packs, bool once /*= false*/) override
  {
    bool result = false;
    for (int i = animChars.size() - 1; i >= 0; --i)
    {
      if (rrl && is_res_required(rrl, animChars[i].resId))
        continue;
      if (animChars[i].refCount > 0)
      {
        if (!forced_free_unref_packs)
          continue;
        else
          FATAL_ON_UNLOADING_USED_RES(animChars[i].resId, animChars[i].refCount);
      }
      animChars.erase(animChars.begin() + i);
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
      if (findRes(res_id) >= 0)
        return;
    }

    AnimChar chd(res_id);

    unsigned ver = cb.readInt();
    if (ver == _MAKE4C('CHR2') || ver == _MAKE4C('CHR1'))
      chd.props.reset(RoDataBlock::load(cb));

    switch (ver)
    {
      case _MAKE4C('CHR2'):
      case _MAKE4C('chr2'):
        chd.useCharDep = true;
        chd.pyOfs = cb.readReal();
        chd.pxScale = cb.readReal();
        chd.pyScale = cb.readReal();
        chd.pzScale = cb.readReal();
        chd.sScale = cb.readReal();
        cb.readString(chd.rootNode);
        [[fallthrough]];
      case _MAKE4C('CHR1'):
      case _MAKE4C('chr1'):
        cb.readString(chd.centerNode);
        if (chd.centerNode == "@root")
          chd.centerNode.clear();
        [[fallthrough]];
      case _MAKE4C('chr0'):
        chd.noAnimDist = cb.readReal();
        chd.noRenderDist = cb.readReal();
        chd.bsphRad = cb.readReal();
        break;
      default: break;
    };

    WinAutoLock lock2(cs);
    animChars.emplace_back(eastl::move(chd));
  }


  void createGameResource(RRL rrl, int res_id, const int *ref_ids, int num_refs) override
  {
    WinCritSec &cs = get_gameres_main_cs();
    AnimV20::AnimCharCreationProps animPropsView(AnimV20::AnimCharCreationProps::CtorNoInit{});
    String name;

    int id;
    {
      WinAutoLock lock(cs);

      id = findRes(res_id);
      if (id >= 0)
      {
        if (animChars[id].animChar)
          return;
      }
      else
        id = findRes(res_id);

      get_game_resource_name(res_id, name);
      if (id < 0)
      {
        logerr("BUG: no Character resource %s", name.str());
        return;
      }

      if (num_refs < 2)
        return;

      memcpy(&animPropsView, &animChars[id], sizeof(animPropsView)); // -V780 To consider: use ref counted ptr instead?
    }

    FATAL_CONTEXT_AUTO_SCOPE(name);

    auto animChar = eastl::make_unique<AnimV20::IAnimCharacter2>();
    animChar->baseComp().creationInfo.resName = name;

    auto *modelRes = (DynamicRenderableSceneLodsResource *)get_game_resource(ref_ids[0], rrl);
    auto *skeletonRes = (GeomNodeTree *)get_game_resource(ref_ids[1], rrl);
    bool ag_set = (num_refs >= 3 && ref_ids[2] >= 0);
    auto *agRes = ag_set ? (AnimGraphResData *)get_game_resource(ref_ids[2], rrl) : nullptr;
    bool phys_set = (num_refs >= 4 && ref_ids[3] >= 0);
    auto *physRes = phys_set ? (DynamicPhysObjectData *)get_game_resource(ref_ids[3], rrl) : nullptr;
    bool res_load_failed = !skeletonRes || (ag_set && !agRes);

    if (!res_load_failed)
      animChar->load(animPropsView, modelRes, skeletonRes, agRes, physRes);

    if (modelRes)
      ::release_game_resource_ex(modelRes, DynModelGameResClassId);
    if (skeletonRes)
      ::release_game_resource_ex(skeletonRes, GeomNodeTreeGameResClassId);
    if (agRes)
      ::release_game_resource_ex(agRes, AnimGraphGameResClassId);
    if (physRes)
      ::release_game_resource_ex(physRes, PhysObjGameResClassId);

    {
      WinAutoLock lock2(cs);
      animChars[id].animChar = eastl::move(animChar);
    }

    animPropsView.release();
  }

  void iterateGameResources(const eastl::function<void(GameResource *)> &cb)
  {
    for (AnimChar &ac : animChars)
      cb((GameResource *)ac.animChar.get());
  }

  void reset() override { animChars.clear(); }

  IMPLEMENT_DUMP_RESOURCES_REF_COUNT(animChars, resId, refCount)
};


static InitOnDemand<CharacterGameResFactory> animchar_factory;

void register_character_gameres_factory()
{
  animchar_factory.demandInit();
  ::add_factory(animchar_factory);
}
