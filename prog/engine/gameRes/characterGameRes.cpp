#include <gameRes/dag_gameResSystem.h>
#include <gameRes/dag_stdGameRes.h>
#include <animChar/dag_animCharacter2.h>
#include <ioSys/dag_genIo.h>
#include <ioSys/dag_roDataBlock.h>
#include <EASTL/vector.h>
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
  createVisInst = true;

  useCharDep = false;
  pyOfs = 0;
  pxScale = pyScale = pzScale = 1;
  sScale = 1;
}

void AnimCharCreationProps::release()
{
  centerNode.setStrRaw(nullptr);
  props.release();
  rootNode.setStrRaw(nullptr);
}

} // namespace AnimV20

class CharacterGameResFactory final : public GameResourceFactory
{
public:
  struct CharData : public AnimV20::AnimCharCreationProps
  {
    int resId;
    CharData(int rid) : resId(rid) {}
  };

  struct AnimChar
  {
    int resId;
    int refCount;
    eastl::unique_ptr<AnimV20::IAnimCharacter2> animChar;
  };

  eastl::vector<CharData> charData;
  eastl::vector<AnimChar> animChars;

  int findResData(int res_id)
  {
    for (int i = 0; i < charData.size(); ++i)
      if (charData[i].resId == res_id)
        return i;

    return -1;
  }


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

  bool isResLoaded(int res_id) override { return findRes(res_id) >= 0; }
  bool checkResPtr(GameResource *res) override { return findAnimChar(res) >= 0; }

  GameResource *getGameResource(int res_id) override
  {
    int id = findRes(res_id);

    if (id < 0)
      ::load_game_resource_pack(res_id);

    id = findRes(res_id);

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

    if (animChars[id].refCount == 0)
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


  bool freeUnusedResources(bool forced_free_unref_packs, bool once /*= false*/) override
  {
    bool result = false;
    for (int i = animChars.size() - 1; i >= 0; --i)
    {
      if (get_refcount_game_resource_pack_by_resid(animChars[i].resId) > 0)
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
      if (findResData(res_id) >= 0)
        return;
    }

    CharData chd(res_id);
    chd.createVisInst = false;

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
      case _MAKE4C('chr1'): cb.readString(chd.centerNode); [[fallthrough]];
      case _MAKE4C('chr0'):
        chd.noAnimDist = cb.readReal();
        chd.noRenderDist = cb.readReal();
        chd.bsphRad = cb.readReal();
        break;
      default: break;
    };

    WinAutoLock lock2(cs);
    charData.emplace_back(eastl::move(chd));
  }


  void createGameResource(int res_id, const int *ref_ids, int num_refs) override
  {
    WinCritSec &cs = get_gameres_main_cs();
    AnimV20::AnimCharCreationProps animPropsView(AnimV20::AnimCharCreationProps::CtorNoInit{});
    String name;

    {
      WinAutoLock lock(cs);

      int id = findRes(res_id);
      if (id >= 0)
        return;

      id = findResData(res_id);
      get_game_resource_name(res_id, name);
      if (id < 0)
      {
        logerr("BUG: no Character resource %s", name.str());
        return;
      }

      if (num_refs < 2)
      {
        animChars.emplace_back().resId = res_id;
        return;
      }

      memcpy(&animPropsView, &charData[id], sizeof(animPropsView)); // -V780
    }

    FATAL_CONTEXT_AUTO_SCOPE(name);

    AnimChar ac{res_id, 0, eastl::make_unique<AnimCharV20::IAnimCharacter2>()};
    ac.animChar->baseComp().creationInfo.resName = name;

    int modelId = ref_ids[0];
    int skeletonId = ref_ids[1];
    //    int agId = num_refs >= 3 ? ref_ids[2] : -1;
    //    int pdlId = num_refs >= 4 ? ref_ids[3] : -1;
    int physId = num_refs >= 4 ? ref_ids[3] : -1;
    int acId = num_refs >= 3 ? ref_ids[2] : -1;

    ac.animChar->load(animPropsView, modelId, skeletonId, acId, physId);

    {
      WinAutoLock lock2(cs);
      animChars.emplace_back(eastl::move(ac));
    }

    animPropsView.release();
  }

  void reset() override
  {
    charData.clear();
    animChars.clear();
  }

  IMPLEMENT_DUMP_RESOURCES_REF_COUNT(animChars, resId, refCount)
};


static InitOnDemand<CharacterGameResFactory> animchar_factory;

void register_character_gameres_factory()
{
  animchar_factory.demandInit();
  ::add_factory(animchar_factory);
}
