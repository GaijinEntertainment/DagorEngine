// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <de3_interface.h>
#include <de3_objEntity.h>
#include <de3_entityPool.h>
#include <de3_entityFilter.h>
#include <de3_lightService.h>
#include <de3_baseInterfaces.h>
#include <de3_animCtrl.h>
#include <de3_lodController.h>
#include <oldEditor/de_common_interface.h>
#include <assets/assetChangeNotify.h>
#include <assets/assetMgr.h>
#include <assets/assetRefs.h>
#include <EditorCore/ec_interface.h>
#include <animChar/dag_animCharacter2.h>
#include <anim/dag_animBlend.h>
#include <anim/dag_animBlendCtrl.h>
#include <anim/dag_animPostBlendCtrl.h>
#include <gameRes/dag_gameResSystem.h>
#include <gameRes/dag_stdGameRes.h>
#include <phys/dag_fastPhys.h>
#include <shaders/dag_dynSceneRes.h>
#include <startup/dag_globalSettings.h>
#include <drv/3d/dag_matricesAndPerspective.h>
#include <drv/3d/dag_driver.h>
#include <3d/dag_render.h>
#include <render/dag_cur_view.h>
#include <math/dag_geomTree.h>
#include <math/dag_TMatrix.h>
#include <debug/dag_debug.h>
#include <de3_writeObjsToPlaceDump.h>
#include <oldEditor/de_interface.h>
#include <oldEditor/de_workspace.h>
#include <winGuiWrapper/wgw_busy.h>
#include <osApiWrappers/dag_direct.h>
#include <render/dynmodelRenderer.h>

static int animCharEntityClassId = -1;
static ISceneLightService *ltService;

static bool registeredNotifier = false;
static FastNameMapEx usedAssetNames;

static float simDtScale = 1.0f;
static bool simPaused = false;
static DagorAssetMgr *aMgr = NULL;

bool validateName = false;
Tab<String> gameDataPath;

class AnimCharEntity;

typedef SingleEntityPool<AnimCharEntity> AnimCharEntityPool;

class VirtualAnimCharEntity : public IObjEntity
{
public:
  struct DynamicAttachement
  {
    int slotId = -1;
    int animVarId = -1;
    const DataBlock *bEnum = NULL;
    String attSuffix, configDir, varBlockNm;
    bool varTypeInt = false;
    float curValue = -1;
    int setAssetNid = -1;
    Tab<int> syncVarsF, syncVarsI;
  };
  struct AttachmentStor
  {
    int slotId;
    AnimCharV20::IAnimCharacter2 *ac;
  };

public:
  VirtualAnimCharEntity(int cls) : IObjEntity(cls), pool(NULL), ac(NULL)
  {
    bbox = BBox3(Point3(-0.5, -0.5, -0.5), Point3(0.5, 0.5, 0.5));
    bsph = BSphere3(Point3(0, 0, 0), 0.5);
    assetNameId = -1;
    usedAssetNid = -1;
  }

  virtual ~VirtualAnimCharEntity()
  {
    clear();
    assetNameId = -1;
  }

  void clear()
  {
    termDebugFifo();
    destroy_it(ac);
    dynAtt.clear();
    for (auto &att : attStor)
      destroy_it(att.ac);
  }

  virtual void setTm(const TMatrix &_tm) {}
  virtual void getTm(TMatrix &_tm) const { _tm = TMatrix::IDENT; }
  virtual void destroy() { delete this; }
  virtual void *queryInterfacePtr(unsigned huid) { return NULL; }

  virtual BSphere3 getBsph() const { return bsph; }
  virtual BBox3 getBbox() const { return bbox; }

  virtual const char *getObjAssetName() const { return usedAssetNames.getName(usedAssetNid); }

  void setup(const DagorAsset &asset)
  {
    const char *name = asset.getName();
    assetNameId = asset.getNameId();
    usedAssetNid = usedAssetNames.addNameId(asset.getNameTypified());
    usedRefNids.clear();

    DAEDITOR3.setFatalHandler(false);
    try
    {
      FastNameMap resNameMap;
      resNameMap.addNameId(name);
      ::set_required_res_list_restriction(resNameMap);
      GameResHandle h = GAMERES_HANDLE_FROM_STRING(name);
      GameResource *res = ::get_game_resource_ex(h, CharacterGameResClassId);
      ::reset_required_res_list_restriction();
      if (res && !((AnimCharV20::IAnimCharacter2 *)res)->getVisualResource())
      {
        ::release_game_resource(res);
        res = nullptr;
      }

      if (res)
      {
        ac = ((AnimCharV20::IAnimCharacter2 *)res)->clone();
        ::release_game_resource(res);

        const char *fp_name = asset.props.getStr("ref_fastPhys", NULL);
        if (fp_name)
        {
          resNameMap.reset();
          resNameMap.addNameId(fp_name);
          ::set_required_res_list_restriction(resNameMap);
          FastPhysSystem *fpRes = create_fast_phys_from_gameres(fp_name);
          ::reset_required_res_list_restriction();

          if (!fpRes)
            DAEDITOR3.conError("cannot find fastphys <%s> for animChar <%s>", fp_name, name);
          else
            ac->setFastPhysSystem(fpRes);
        }

        // setup attachments
        for (int i = 0, nid = asset.props.getNameId("ref_attachment"); i < asset.props.blockCount(); i++)
          if (asset.props.getBlock(i)->getBlockNameId() == nid && ac)
          {
            const DataBlock &b = *asset.props.getBlock(i);
            const char *slot_name = b.getStr("slot", NULL);
            int slot_id = AnimCharV20::getSlotId(slot_name);
            if (slot_id < 0)
            {
              DAEDITOR3.conError("attachment bad slot <%s> in animChar <%s>", slot_name, name);
              continue;
            }

            resNameMap.reset();
            if (const char *attAcNm = b.getStr("animChar", NULL))
            {
              AnimV20::attachment_uid_t uid = AnimV20::INVALID_ATTACHMENT_UID;
              DagorAsset *a = asset.getMgr().findAsset(attAcNm, animCharEntityClassId);
              if (a)
              {
                usedRefNids.push_back(usedAssetNames.addNameId(a->getNameTypified()));
                uid = a->getNameId() + 1;
              }
              resNameMap.addNameId(attAcNm);
              ::set_required_res_list_restriction(resNameMap);
              GameResource *attAcRes = ::get_game_resource_ex(GAMERES_HANDLE_FROM_STRING(attAcNm), CharacterGameResClassId);
              AnimCharV20::IAnimCharacter2 *attAc = attAcRes ? ((AnimCharV20::IAnimCharacter2 *)attAcRes)->clone() : NULL;
              ::release_game_resource(attAcRes);
              ::reset_required_res_list_restriction();
              if (attAc && a)
                init_ref_default_state(attAc, a->props);

              if (attAc && ac->setAttachedChar(slot_id, uid, &attAc->baseComp()) >= 0)
              {
                DAEDITOR3.conNote("attached char <%s> to slot <%s> of animChar <%s>", attAcNm, slot_name, name);
                replaceAtt(slot_id, attAc);
              }
              else
              {
                DAEDITOR3.conError("cannot set attachment char <%s>=%p to slot <%s> of animChar <%s>", attAcNm, attAc, slot_name,
                  name);
                destroy_it(attAc);
              }
            }
            else if (const char *attDmNm = b.getStr("dynModel", NULL))
            {
              DAEDITOR3.conError("%s: ref_attachment with dynModel/skeleton is not supported (dropped)", name);
            }
            else
              DAEDITOR3.conError("cannot set attachment to slot <%s> of animChar <%s>", slot_name, name);
          }

        // setup dynamic attachments
        for (int i = 0, nid = asset.props.getNameId("ref_attachment_dynamic"); i < asset.props.blockCount(); i++)
          if (asset.props.getBlock(i)->getBlockNameId() == nid && ac && ac->getAnimGraph())
          {
            const DataBlock &b = *asset.props.getBlock(i);
            const char *slot_name = b.getStr("slot", NULL);
            const char *var_name = b.getStr("var", NULL);
            const char *enum_cls = b.getStr("enum", NULL);
            AnimV20::AnimationGraph *anim = ac->getAnimGraph();

            DynamicAttachement &att = dynAtt.push_back();
            att.slotId = AnimCharV20::getSlotId(slot_name);
            att.animVarId = anim->getParamId(var_name, AnimV20::IAnimStateHolder::PT_ScalarParam);
            if (att.animVarId < 0)
            {
              att.animVarId = anim->getParamId(var_name, AnimV20::IAnimStateHolder::PT_ScalarParamInt);
              att.varTypeInt = true;
            }
            att.bEnum = &anim->getEnumList(enum_cls);

            if (att.slotId < 0)
            {
              DAEDITOR3.conError("%s: attachment bad slot <%s>", asset.getName(), slot_name);
              dynAtt.pop_back();
              continue;
            }
            if (att.animVarId < 0)
            {
              DAEDITOR3.conError("%s: missing anim var <%s> for slot <%s>", asset.getName(), var_name, slot_name);
              dynAtt.pop_back();
              continue;
            }
            if (!att.bEnum->blockCount())
            {
              DAEDITOR3.conError("%s: empty enum <%s> for slot <%s>", asset.getName(), enum_cls, slot_name);
              dynAtt.pop_back();
              continue;
            }

            att.attSuffix = b.getStr("animCharSuffix", "_char");
            att.configDir = b.getStr("configDir", "");
            if (!att.configDir.empty() && att.configDir[0] == '#')
              att.configDir = String(0, "%s%s", EDITORCORE->getBaseWorkspace().getAppDir(), &att.configDir[1]);

            att.varBlockNm = b.getStr("varsForHolderBlockName", "");

            int nid = b.getNameId("syncVar");
            for (int j = 0; j < b.paramCount(); j++)
              if (b.getParamNameId(j) == nid && b.getParamType(j) == b.TYPE_STRING)
              {
                int varId = anim->getParamId(b.getStr(j), AnimV20::IAnimStateHolder::PT_ScalarParam);
                if (varId >= 0)
                  att.syncVarsF.push_back(varId);
                else
                {
                  varId = anim->getParamId(b.getStr(j), AnimV20::IAnimStateHolder::PT_ScalarParamInt);
                  if (varId >= 0)
                    att.syncVarsI.push_back(varId);
                  else
                    DAEDITOR3.conError("missing syncVar <%s> in animchar %s (for %s)", b.getStr(j), asset.getName(),
                      "ref_attachment_dynamic");
                }
              }
          }
      }
    }
    catch (...)
    {
      logerr("exception while building asset: %s", name);
      ac = NULL;
    }
    DAEDITOR3.popFatalHandler();

    if (!ac)
    {
      DAEDITOR3.conError("cannot load animChar: %s", name);
      debug_flush(false);
      return;
    }

    // calculating bbox and bsphere
    setTm(TMatrix::IDENT);
    bbox3f wbb;
    ac->rendComp().calcWorldBox(wbb, ac->getFinalWtm(), false);
    bbox[0] = as_point3(&wbb.bmin);
    bbox[1] = as_point3(&wbb.bmax);
    bsph = bbox;

    const char *ref_anim = asset.props.getStr("ref_anim", NULL);
    if (ref_anim && !ac->getAnimStateDirector())
      setAnim(ref_anim);


    if (IAnimCharController *animCtrl = queryInterface<IAnimCharController>())
      init_ref_default_state(animCtrl->getAnimChar(), asset.props);
  }

  static int getIntParam(const DataBlock &b, const char *nm, int def)
  {
    int i = b.findParam(nm);
    if (i < 0)
      return def;
    if (b.getParamType(i) == b.TYPE_STRING)
      return AnimV20::getEnumValueByName(b.getStr(i));
    if (b.getParamType(i) == b.TYPE_INT)
      return b.getInt(i);
    return def;
  }
  static float getFloatParam(const DataBlock &b, const char *nm, float def)
  {
    int i = b.findParam(nm);
    if (i < 0)
      return def;
    if (b.getParamType(i) == b.TYPE_STRING)
      return (float)AnimV20::getEnumValueByName(b.getStr(i));
    if (b.getParamType(i) == b.TYPE_REAL)
      return b.getReal(i);
    return def;
  }

  void init_ref_default_state(AnimV20::IAnimCharacter2 *ac, const DataBlock &props)
  {
    AnimV20::AnimationGraph *ag = ac->getAnimGraph();
    if (!ag)
      return;

    AnimV20::IAnimStateHolder &st = *ac->getAnimState();
    if (const DataBlock *b = props.getBlockByName("ref_animvars"))
      iterate_names(ag->getParamNames(), [&](int id, const char *name) {
        switch (ag->getParamType(id))
        {
          case AnimV20::IPureAnimStateHolder::PT_ScalarParam:
          case AnimV20::IPureAnimStateHolder::PT_TimeParam: st.setParam(id, getFloatParam(*b, name, st.getParam(id))); break;
          case AnimV20::IPureAnimStateHolder::PT_ScalarParamInt: st.setParamInt(id, getIntParam(*b, name, st.getParamInt(id))); break;
        }
      });

    if (const DataBlock *b = props.getBlockByName("ref_states"))
      for (int i = 0, nid = b->getNameId("state"); i < b->paramCount(); i++)
        if (b->getParamNameId(i) == nid && b->getParamType(i) == b->TYPE_STRING)
        {
          int s_idx = ag->getStateIdx(b->getStr(i));
          if (s_idx >= 0)
            ag->enqueueState(st, ag->getState(s_idx), -1);
        }
  }

  bool isNonVirtual() const { return pool; }
  int getAssetNameId() const { return assetNameId; }
  inline const char *assetName() const { return aMgr ? aMgr->getAssetName(assetNameId) : NULL; }

  AnimV20::AnimBlendCtrl_Fifo3 *initDebugFifo(bool set_anim)
  {
    AnimV20::AnimationGraph *ag = ac ? ac->getAnimGraph() : NULL;
    if (!ag)
      return NULL;
    AnimV20::IAnimStateHolder *as = ac->getAnimState();
    if (dbgHub.get())
    {
      dbgHub->setBlendNodeWt(*as, dbgOldRoot, set_anim ? 0.0f : 1.0f);
      dbgHub->setBlendNodeWt(*as, dbgFifo, set_anim ? 1.0f : 0.0f);
      return dbgFifo;
    }
    if (!set_anim)
      return NULL;

    dbgHub = new AnimV20::AnimBlendCtrl_Hub;
    dbgFifo = new AnimV20::AnimBlendCtrl_Fifo3(*ag, "debug.fifo::var");
    dbgOldRoot = ag->getRoot();

    ag->registerBlendNode(dbgHub, "~debug.hub");
    ag->registerBlendNode(dbgFifo, "debug.fifo");

    dbgHub->addBlendNode(dbgFifo, false, 1.0);
    if (dbgOldRoot.get())
      dbgHub->addBlendNode(dbgOldRoot, true, 1.0);
    dbgHub->finalizeInit(*ag, "debug.hub::var");

    ag->replaceRoot(dbgHub);
    ac->reset();
    dbgHub->setBlendNodeWt(*as, dbgOldRoot, set_anim ? 0.0f : 1.0f);
    dbgHub->setBlendNodeWt(*as, dbgFifo, set_anim ? 1.0f : 0.0f);
    return dbgFifo;
  }
  void termDebugFifo()
  {
    AnimV20::AnimationGraph *ag = ac ? ac->getAnimGraph() : NULL;
    if (!ag)
      return;

    if (dbgHub.get())
    {
      ag->replaceRoot(dbgOldRoot);
      ag->unregisterBlendNode(dbgHub);
      ag->unregisterBlendNode(dbgFifo);
      dbgHub = NULL;
      dbgFifo = NULL;
      dbgOldRoot = NULL;
      ac->reset();
    }
  }

  bool enqueueAnim(const char *anim)
  {
    AnimV20::AnimationGraph *ag = ac ? ac->getAnimGraph() : NULL;
    if (!ag)
      return false;
    if (anim && !*anim)
      anim = NULL;

    AnimV20::IAnimStateHolder *as = ac->getAnimState();
    AnimV20::AnimBlendCtrl_Fifo3 *fifo = NULL;
    if (ag->getRoot() && ag->getRoot()->isSubOf(AnimV20::AnimBlendCtrl_Fifo3CID))
      fifo = reinterpret_cast<AnimV20::AnimBlendCtrl_Fifo3 *>(ag->getRoot());
    else
      fifo = initDebugFifo(anim != NULL);

    if (!fifo && anim)
    {
      DAEDITOR3.conWarning("cant set anim <%s> due to AG:root is missing or not fifo3", anim);
      return false;
    }
    if (anim)
    {
      AnimV20::IAnimBlendNode *n = ag->getBlendNodePtr(anim);
      if (!n)
      {
        DAEDITOR3.conWarning("cant set missing anim <%s>", anim);
        return false;
      }
      if (!fifo->isEnqueued(*as, n))
        n->resume(*as, true);
      fifo->enqueueState(*as, n, 0.15, 0.0);
    }
    return true;
  }

  bool setAnim(const char *anim)
  {
    if (!ac || !ac->getAnimGraph())
    {
      DAEDITOR3.conWarning("cant set ref anim <%s> due to AG is missing in animChar", anim);
      return false;
    }
    if (!enqueueAnim(anim))
    {
      enqueueAnim(NULL);
      return false;
    }

    // recalc bounds
    ac->act(0.001, ::grs_cur_view.pos, true);
    bbox3f wbb;
    ac->rendComp().calcWorldBox(wbb, ac->getFinalWtm(), false);
    bbox[0] = as_point3(&wbb.bmin);
    bbox[1] = as_point3(&wbb.bmax);
    bsph = bbox;
    if (anim && *anim)
      DAEDITOR3.conNote("set anim <%s>", anim);
    else
      DAEDITOR3.conNote("reset anim");
    return true;
  }

  void update(float dt)
  {
    if (!ac)
      return;
    ac->act(dt, ::grs_cur_view.pos);
    for (int i = 0; i < dynAtt.size(); i++)
    {
      DynamicAttachement &att = dynAtt[i];
      float val = att.varTypeInt ? ac->getAnimState()->getParamInt(att.animVarId) : ac->getAnimState()->getParam(att.animVarId);
      if (fabsf(att.curValue - val) < 1e-3)
        continue;

      const char *enum_val = NULL;
      for (int pi = 0; pi < att.bEnum->blockCount(); pi++)
      {
        const DataBlock *enumBlock = att.bEnum->getBlock(pi);
        if (fabsf(enumBlock->getInt("_enumValue", -1) - val) < 1e-3)
        {
          enum_val = enumBlock->getBlockName();
          break;
        }
      }

      att.curValue = val;
      erase_item_by_value(usedRefNids, att.setAssetNid);
      if (DagorAsset *a = DAEDITOR3.getAssetByName(String(0, "%s%s", enum_val, att.attSuffix), animCharEntityClassId))
      {
        wingw::set_busy(true);
        usedRefNids.push_back(att.setAssetNid = usedAssetNames.addNameId(a->getNameTypified()));

        FastNameMap resNameMap;
        resNameMap.addNameId(a->getName());
        ::set_required_res_list_restriction(resNameMap);
        GameResource *attAcRes = ::get_game_resource_ex(GAMERES_HANDLE_FROM_STRING(a->getName()), CharacterGameResClassId);
        AnimCharV20::IAnimCharacter2 *attAc = attAcRes ? ((AnimCharV20::IAnimCharacter2 *)attAcRes)->clone() : NULL;
        ::release_game_resource(attAcRes);
        ::reset_required_res_list_restriction();
        if (attAc)
          init_ref_default_state(attAc, a->props);

        if (attAc && ac->setAttachedChar(att.slotId, a->getNameId(), &attAc->baseComp()) >= 0)
          DAEDITOR3.conNote("attached char <%s> to slotId=%d of animChar <%s>", a->getName(), att.slotId, assetName());
        else
        {
          DAEDITOR3.conError("cannot set attachment char <%s>=%p to slotId=%d of animChar <%s>", a->getName(), attAc, att.slotId,
            assetName());
          destroy_it(attAc);
          ac->setAttachedChar(att.slotId, AnimV20::INVALID_ATTACHMENT_UID, NULL);
          att.setAssetNid = -1;
        }
        replaceAtt(att.slotId, attAc);
        wingw::set_busy(false);
      }
      else
      {
        DAEDITOR3.conNote("reset attachements at slotId=%d of animChar <%s>", att.slotId, assetName());
        ac->setAttachedChar(att.slotId, AnimV20::INVALID_ATTACHMENT_UID, NULL);
        att.setAssetNid = -1;
        replaceAtt(att.slotId, NULL);
      }

      if (!att.configDir.empty() && enum_val && *enum_val)
      {
        String cfg_fn(0, "%s/%s.blk", att.configDir, enum_val);
        simplify_fname(cfg_fn);
        if (dd_file_exists(cfg_fn))
        {
          AnimV20::AnimationGraph *ag = ac->getAnimGraph();
          AnimV20::IAnimStateHolder *as = ac->getAnimState();

          DataBlock blk(cfg_fn);
          const DataBlock &b = *blk.getBlockByNameEx(att.varBlockNm);
          for (int pi = 0; pi < b.paramCount(); pi++)
            if (b.getParamType(pi) == b.TYPE_REAL)
            {
              int param_id = ag->getParamId(b.getParamName(pi), AnimV20::IAnimStateHolder::PT_ScalarParam);
              if (param_id >= 0)
                as->setParam(param_id, b.getReal(pi));
            }
            else if (b.getParamType(pi) == b.TYPE_INT)
            {
              int param_id = ag->getParamId(b.getParamName(pi), AnimV20::IAnimStateHolder::PT_ScalarParamInt);
              if (param_id >= 0)
                as->setParamInt(param_id, b.getInt(pi));
            }
          if (!b.paramCount())
            DAEDITOR3.conWarning("empty block \"%s\" in \"%s\" for slotId=%d of animChar <%s>", att.varBlockNm, cfg_fn, att.slotId,
              assetName());
        }
        else
          DAEDITOR3.conWarning("missing \"%s\" (for %s), slotId=%d of animChar <%s>", cfg_fn, enum_val, att.slotId, assetName());
      }
    }
    for (int i = 0; i < dynAtt.size(); i++)
      if (auto attAc = ac->getAttachedChar(dynAtt[i].slotId))
      {
        if (AnimV20::AnimationGraph *attAnim = attAc->getAnimGraph())
        {
          for (int j = 0; j < dynAtt[i].syncVarsF.size(); j++)
          {
            int param_id = attAnim->getParamId(ac->getAnimGraph()->getParamName(dynAtt[i].syncVarsF[j]));
            if (param_id >= 0)
              attAc->getAnimState()->setParam(param_id, ac->getAnimState()->getParam(dynAtt[i].syncVarsF[j]));
          }
          for (int j = 0; j < dynAtt[i].syncVarsI.size(); j++)
          {
            int param_id = attAnim->getParamId(ac->getAnimGraph()->getParamName(dynAtt[i].syncVarsI[j]));
            if (param_id >= 0)
              attAc->getAnimState()->setParamInt(param_id, ac->getAnimState()->getParamInt(dynAtt[i].syncVarsI[j]));
          }
        }
      }
    for (auto &att : attStor)
      if (att.ac)
      {
        att.ac->setTm(att.ac->getNodeTree().getRootTm());
        att.ac->act(dt, ::grs_cur_view.pos);
      }
  }
  void replaceAtt(int slot_id, AnimCharV20::IAnimCharacter2 *attAc)
  {
    for (auto &att : attStor)
      if (att.slotId == slot_id)
      {
        destroy_it(att.ac);
        att.ac = attAc;
        return;
      }

    if (attAc)
    {
      AttachmentStor &att = attStor.push_back();
      att.slotId = slot_id;
      att.ac = attAc;
    }
  }

public:
  AnimCharEntityPool *pool;
  AnimV20::IAnimCharacter2 *ac;
  Ptr<AnimV20::AnimBlendCtrl_Hub> dbgHub;
  Ptr<AnimV20::AnimBlendCtrl_Fifo3> dbgFifo;
  Ptr<AnimV20::IAnimBlendNode> dbgOldRoot;
  Tab<int> usedRefNids;
  Tab<DynamicAttachement> dynAtt;
  Tab<AttachmentStor> attStor;

  int assetNameId, usedAssetNid;
  BBox3 bbox;
  BSphere3 bsph;
};


class AnimCharEntity : public VirtualAnimCharEntity, public IAnimCharController, public ILodController
{
public:
  AnimCharEntity(int cls) : VirtualAnimCharEntity(cls), idx(MAX_ENTITIES) {}

  void clear()
  {
    clear_and_shrink(dbgAnimNodes);
    VirtualAnimCharEntity::clear();
  }

  virtual void *queryInterfacePtr(unsigned huid)
  {
    RETURN_INTERFACE(huid, IAnimCharController);
    RETURN_INTERFACE(huid, ILodController);
    return NULL;
  }

  virtual void setTm(const TMatrix &_tm)
  {
    if (!ac)
      return;

    ac->setTm(_tm);
  }

  virtual void getTm(TMatrix &m) const { ac ? ac->getTm(m) : VirtualAnimCharEntity::getTm(m); }
  virtual void destroy()
  {
    pool->delEntity(this);
    clear();
  }

  void copyFrom(const VirtualAnimCharEntity &e)
  {
    if (!e.ac)
      return;

    ac = e.ac->clone();
    dbgHub = e.dbgHub;
    dbgFifo = e.dbgFifo;
    dbgOldRoot = e.dbgOldRoot;
    assetNameId = e.assetNameId;
    usedAssetNid = e.usedAssetNid;
    usedRefNids = e.usedRefNids;
    bbox = e.bbox;
    bsph = e.bsph;

    if (aMgr)
    {
      DagorAsset *a = aMgr->findAsset(aMgr->getAssetName(assetNameId), animCharEntityClassId);
      if (a)
      {
        const char *ref_anim = a->props.getStr("ref_anim", NULL);
        if (ref_anim && !ac->getAnimStateDirector())
          setAnim(ref_anim);
      }
    }
  }

  // IAnimCharController
  virtual int getStatesCount() { return getAnimGraph() ? getAnimGraph()->getStateCount() : 0; }
  virtual const char *getStateName(int s_idx) { return getAnimGraph() && s_idx >= 0 ? getAnimGraph()->getStateName(s_idx) : NULL; }
  virtual int getStateIdx(const char *name) { return getAnimGraph() ? getAnimGraph()->getStateIdx(name) : -1; }
  virtual void enqueueState(int s_idx, float force_spd = -1.f)
  {
    if (!getAnimGraph() || s_idx < 0)
      return;
    getAnimGraph()->enqueueState(*getAnimState(), getAnimGraph()->getState(s_idx), -1.f, force_spd);
    if (isPaused())
      fastUpdate();
  }
  virtual bool hasReferencesTo(const DagorAsset *a)
  {
    int nid = usedAssetNames.getNameId(a->getNameTypified());
    if (nid < 0)
    {
      for (int i = 0; i < usedRefNids.size(); i++)
        if (DagorAsset *aref = DAEDITOR3.getAssetByName(usedAssetNames.getName(usedRefNids[i])))
          if (check_asset_depends_on_asset(aref, a))
            return true;
      return false;
    }
    if (nid == usedAssetNid)
      return true;
    return find_value_idx(usedRefNids, nid) >= 0;
  }
  static bool check_asset_depends_on_asset(const DagorAsset *amain, const DagorAsset *a)
  {
    if (IDagorAssetRefProvider *refProvider = amain->getMgr().getAssetRefProvider(amain->getType()))
    {
      Tab<IDagorAssetRefProvider::Ref> refs;
      refs = refProvider->getAssetRefs(*const_cast<DagorAsset *>(amain));
      for (int i = 0; i < refs.size(); ++i)
        if (refs[i].getAsset() == a)
          return true;
      for (int i = 0; i < refs.size(); ++i)
        if (refs[i].getAsset() && check_asset_depends_on_asset(refs[i].getAsset(), a))
          return true;
    }
    return false;
  }

  virtual AnimV20::AnimationGraph *getAnimGraph() const { return ac ? ac->getAnimGraph() : NULL; }
  virtual AnimV20::IAnimStateHolder *getAnimState() const { return ac ? ac->getAnimState() : NULL; }
  virtual AnimV20::IAnimCharacter2 *getAnimChar() const { return ac; }

  virtual bool isPaused() const { return simPaused; }
  virtual void setPaused(bool paused) { simPaused = paused; }
  virtual float getTimeScale() const { return simDtScale; }
  virtual void setTimeScale(float t_scale) { simDtScale = t_scale; }
  virtual void restart()
  {
    if (ac)
    {
      ac->reset();
      DagorAsset *a = aMgr ? aMgr->findAsset(aMgr->getAssetName(assetNameId), animCharEntityClassId) : NULL;
      if (const DataBlock *b = a ? a->props.getBlockByName("ref_states") : NULL)
        for (int i = 0, nid = b->getNameId("state"); i < b->paramCount(); i++)
          if (b->getParamNameId(i) == nid && b->getParamType(i) == b->TYPE_STRING)
            enqueueState(getStateIdx(b->getStr(i)));
      fastUpdate();
    }
  }
  void fastUpdate()
  {
    for (int i = 0; i < 30; i++)
      ac->act(0.03, ::grs_cur_view.pos);
  }

  virtual int getAnimNodeCount()
  {
    fillDbgAnimNodes();
    return dbgAnimNodes.size();
  }
  virtual const char *getAnimNodeName(int a_idx)
  {
    fillDbgAnimNodes();
    return a_idx >= 0 && a_idx < dbgAnimNodes.size() ? dbgAnimNodes[a_idx] : NULL;
  }
  virtual bool enqueueAnimNode(const char *anim_node_nm)
  {
    if (enqueueAnim(anim_node_nm))
      return true;
    enqueueAnim(NULL);
    return false;
  }

  void fillDbgAnimNodes()
  {
    using namespace AnimV20;
    if (dbgAnimNodes.size())
      return;
    dbgAnimNodes.clear();
    if (AnimationGraph *ag = getAnimGraph())
    {
      dbgAnimNodes.reserve(ag->getAnimNodeCount());
      iterate_names(ag->getAnimNodeNames(), [&](int id, const char *name) {
        if (IAnimBlendNode *n = ag->getBlendNodePtr(id))
          if (n != ag->getRoot() && name[0] != '~' && !n->isSubOf(AnimPostBlendAlignCtrlCID) && !n->isSubOf(AnimBlendCtrl_Fifo3CID))
            dbgAnimNodes.push_back(name);
      });
      dbgAnimNodes.shrink_to_fit();
    }
  }

  // ILodController
  virtual int getLodCount() { return ac ? ac->getSceneInstance()->getLodsCount() : 0; }
  virtual void setCurLod(int n) { ac ? ac->getSceneInstance()->setLod(n) : (void)0; }
  virtual real getLodRange(int lod_n) { return ac->getSceneInstance()->getLodsResource()->lods[lod_n].range; }

  // no texture quality level support
  virtual int getTexQLCount() const { return 1; }
  virtual void setTexQL(int ql) {}
  virtual int getTexQL() const { return 0; }

  // no named nodes support
  virtual int getNamedNodeCount() { return 0; }
  virtual const char *getNamedNode(int idx) { return NULL; }
  virtual int getNamedNodeIdx(const char *nm) { return -1; }
  virtual bool getNamedNodeVisibility(int idx) { return true; }
  virtual void setNamedNodeVisibility(int idx, bool vis) {}

public:
  enum
  {
    STEP = 512,
    MAX_ENTITIES = 0x7FFFFFFF
  };

  unsigned idx;
  Tab<const char *> dbgAnimNodes;
};


class AnimCharEntityManagementService : public IEditorService,
                                        public IObjEntityMgr,
                                        public IRenderingService,
                                        public IDagorAssetChangeNotify,
                                        public ILightingChangeClient,
                                        public ILevelResListBuilder,
                                        public IBinaryDataBuilder
{
public:
  AnimCharEntityManagementService()
  {
    animCharEntityClassId = IDaEditor3Engine::get().getAssetTypeId("animChar");
    aMgr = NULL;
    visible = true;
  }
  ~AnimCharEntityManagementService() { usedAssetNames.reset(true); }

  // IEditorService interface
  virtual const char *getServiceName() const { return "_acEntMgr"; }
  virtual const char *getServiceFriendlyName() const { return "(srv) AnimChar entities"; }

  virtual void setServiceVisible(bool vis) { visible = vis; }
  virtual bool getServiceVisible() const { return visible; }

  virtual void actService(float dt)
  {
    if (simPaused)
      return;
    dt *= simDtScale;
    dag::ConstSpan<AnimCharEntity *> ent = objPool.getEntities();
    int st_mask = IObjEntityFilter::getSubTypeMask(IObjEntityFilter::STMASK_TYPE_RENDER);
    uint64_t lh_mask = IObjEntityFilter::getLayerHiddenMask();
    for (int i = 0; i < ent.size(); i++)
      if (ent[i] && ent[i]->isNonVirtual() && ent[i]->checkSubtypeAndLayerHiddenMasks(st_mask, lh_mask))
        ent[i]->update(dt);
  }
  virtual void beforeRenderService() {}
  virtual void renderService() {}
  virtual void renderTransService() {}

  virtual void onBeforeReset3dDevice() {}

  virtual void *queryInterfacePtr(unsigned huid)
  {
    RETURN_INTERFACE(huid, IObjEntityMgr);
    RETURN_INTERFACE(huid, IRenderingService);
    RETURN_INTERFACE(huid, ILightingChangeClient);
    RETURN_INTERFACE(huid, IBinaryDataBuilder);
    RETURN_INTERFACE(huid, ILevelResListBuilder);
    return NULL;
  }

  // ILightingChangeClient
  virtual void onLightingChanged() {}
  virtual void onLightingSettingsChanged() {}

  // IRenderingService interface
  virtual void renderGeometry(Stage stage)
  {
    dag::ConstSpan<AnimCharEntity *> ent = objPool.getEntities();
    int st_mask = IObjEntityFilter::getSubTypeMask(IObjEntityFilter::STMASK_TYPE_RENDER);
    uint64_t lh_mask = IObjEntityFilter::getLayerHiddenMask();
    switch (stage)
    {
      case STG_BEFORE_RENDER:
      {
        mat44f gtm;
        d3d::getglobtm(gtm);
        d3d::settm(TM_WORLD, &Matrix44::IDENT);
        d3d::getglobtm(gtm);
        Driver3dPerspective p;
        AnimCharV20::prepareGlobTm(true, Frustum(gtm), d3d::getpersp(p) ? p.hk : 0, ::grs_cur_view.pos, nullptr);
      }
        for (int i = 0; i < ent.size(); i++)
          if (ent[i] && ent[i]->ac && ent[i]->isNonVirtual() && ent[i]->checkSubtypeAndLayerHiddenMasks(st_mask, lh_mask))
          {
            ent[i]->ac->beforeRender();
            for (auto &att : ent[i]->attStor)
              if (att.ac)
                att.ac->beforeRender();
          }
        break;

      case STG_RENDER_DYNAMIC_OPAQUE:
      case STG_RENDER_SHADOWS:
        for (int i = 0; i < ent.size(); i++)
          if (ent[i] && ent[i]->ac && ent[i]->isNonVirtual() && ent[i]->checkSubtypeAndLayerHiddenMasks(st_mask, lh_mask))
          {
            if (!dynrend::render_in_tools(ent[i]->ac->getSceneInstance(), dynrend::RenderMode::Opaque))
              ent[i]->ac->render(::grs_cur_view.pos);
            for (auto &att : ent[i]->attStor)
              if (att.ac)
                if (!dynrend::render_in_tools(att.ac->getSceneInstance(), dynrend::RenderMode::Opaque))
                  att.ac->render(::grs_cur_view.pos);
          }
        break;

      case STG_RENDER_DYNAMIC_TRANS:
        for (int i = 0; i < ent.size(); i++)
          if (ent[i] && ent[i]->ac && ent[i]->isNonVirtual() && ent[i]->checkSubtypeAndLayerHiddenMasks(st_mask, lh_mask))
          {
            if (!dynrend::render_in_tools(ent[i]->ac->getSceneInstance(), dynrend::RenderMode::Translucent))
              ent[i]->ac->renderTrans(::grs_cur_view.pos);
            for (auto &att : ent[i]->attStor)
              if (att.ac)
                if (!dynrend::render_in_tools(att.ac->getSceneInstance(), dynrend::RenderMode::Translucent))
                  att.ac->renderTrans(::grs_cur_view.pos);
          }
        break;
    }
  }

  // IObjEntityMgr interface
  virtual bool canSupportEntityClass(int entity_class) const
  {
    return animCharEntityClassId >= 0 && animCharEntityClassId == entity_class;
  }

  virtual IObjEntity *createEntity(const DagorAsset &asset, bool virtual_ent)
  {
    if (!aMgr)
      aMgr = &asset.getMgr();
    G_ASSERT(aMgr == &asset.getMgr());

    if (!registeredNotifier)
    {
      registeredNotifier = true;
      asset.getMgr().subscribeUpdateNotify(this, -1, animCharEntityClassId);
    }

    if (virtual_ent)
    {
      VirtualAnimCharEntity *ent = new VirtualAnimCharEntity(animCharEntityClassId);
      ent->setup(asset);
      return ent;
    }

    AnimCharEntity *ent = objPool.allocEntity();
    if (ent)
      ent->clear();
    else
      ent = new AnimCharEntity(animCharEntityClassId);

    ent->setup(asset);
    objPool.addEntity(ent);
    // debug("create ent: %p", ent);
    return ent;
  }

  virtual IObjEntity *cloneEntity(IObjEntity *origin)
  {
    VirtualAnimCharEntity *o = reinterpret_cast<VirtualAnimCharEntity *>(origin);
    AnimCharEntity *ent = objPool.allocEntity();
    if (ent)
      ent->clear();
    else
      ent = new AnimCharEntity(o->getAssetTypeId());

    ent->copyFrom(*o);
    objPool.addEntity(ent);
    // debug("clone ent: %p", ent);
    return ent;
  }

  // IDagorAssetChangeNotify interface
  virtual void onAssetRemoved(int asset_name_id, int asset_type)
  {
    dag::ConstSpan<AnimCharEntity *> ent = objPool.getEntities();
    for (int i = 0; i < ent.size(); i++)
      if (ent[i] && ent[i]->getAssetNameId() == asset_name_id)
        ent[i]->clear();
    EDITORCORE->invalidateViewportCache();
  }
  virtual void onAssetChanged(const DagorAsset &asset, int asset_name_id, int asset_type)
  {
    dag::ConstSpan<AnimCharEntity *> ent = objPool.getEntities();
    for (int i = 0; i < ent.size(); i++)
      if (ent[i] && ent[i]->getAssetNameId() == asset_name_id)
      {
        ent[i]->clear();
        ent[i]->setup(asset);
      }
    EDITORCORE->invalidateViewportCache();
  }

  // IBinaryDataBuilder interface
  virtual bool validateBuild(int target, ILogWriter &log, PropPanel::ContainerPropertyControl *params)
  {
    if (objPool.isEmpty())
      log.addMessage(log.WARNING, "No animChar entities for export");

    if (validateName)
    {
      dag::ConstSpan<AnimCharEntity *> ent = objPool.getEntities();
      for (int i = 0; i < ent.size(); i++)
        if (ent[i])
        {
          int j = 0;
          for (; j < gameDataPath.size(); j++)
          {
            String unitBlkPath(260, "%s/%s.blk", gameDataPath[j], ent[i]->assetName());
            DataBlock unitBlk;
            if (unitBlk.load(unitBlkPath))
              break;
          }
          if (j == gameDataPath.size())
          {
            DAEDITOR3.conWarning("cannot load animChar %s.blk (tried also with %d prefixes)", ent[i]->assetName(), j);
            return false;
          }
        }
    }
    return true;
  }

  virtual bool buildAndWrite(BinDumpSaveCB &main_cwr, const ITextureNumerator &tn, PropPanel::ContainerPropertyControl *)
  {
    dag::Vector<SrcObjsToPlace> objs;
    dag::ConstSpan<AnimCharEntity *> ent = objPool.getEntities();
    int st_mask = IObjEntityFilter::getSubTypeMask(IObjEntityFilter::STMASK_TYPE_EXPORT);
    for (int i = 0; i < ent.size(); i++)
    {
      if (ent[i] && ent[i]->ac && ent[i]->isNonVirtual() && ent[i]->checkSubtypeMask(st_mask))
      {
        int k;
        for (k = 0; k < objs.size(); k++)
          if (ent[i]->getAssetNameId() == objs[k].nameId)
            break;
        if (k == objs.size())
        {
          objs.push_back();
          objs[k].nameId = ent[i]->getAssetNameId();
          objs[k].resName = ent[i]->assetName();
        }
        TMatrix tm;
        ent[i]->getTm(tm);
        objs[k].tm.push_back(tm);
      }
    }
    writeObjsToPlaceDump(main_cwr, make_span(objs), _MAKE4C('AC2'));
    return true;
  }

  virtual bool checkMetrics(const DataBlock &metrics_blk)
  {
    const int maxEntities = metrics_blk.getInt("max_entities", 10);
    if (objPool.getEntities().size() > maxEntities)
    {
      debug("Too much animChars %d > %d entities for export", objPool.getEntities().size(), maxEntities);
      return false;
    }
    return true;
  }

  virtual bool addUsedTextures(ITextureNumerator &tn) { return true; }

  // ILevelResListBuilder
  virtual void addUsedResNames(OAHashNameMap<true> &res_list)
  {
    dag::ConstSpan<AnimCharEntity *> ent = objPool.getEntities();
    for (int i = 0; i < ent.size(); i++)
      if (ent[i])
        res_list.addNameId(ent[i]->assetName());
  }

protected:
  AnimCharEntityPool objPool;
  bool visible;
};


void init_animchar_mgr_service(const DataBlock &app_blk)
{
  if (!IDaEditor3Engine::get().checkVersion())
  {
    DEBUG_CTX("Incorrect version!");
    return;
  }

  const DataBlock *animCharBlk = app_blk.getBlockByNameEx("projectDefaults")->getBlockByNameEx("animChar");
  validateName = animCharBlk->getBool("validateName", false);
  const int paramNameId = animCharBlk->getNameId("searchPrefix");
  for (int paramNo = 0; paramNo < animCharBlk->paramCount(); ++paramNo)
  {
    if (animCharBlk->getParamNameId(paramNo) != paramNameId)
      continue;

    const char *searchPrefix = animCharBlk->getStr(paramNo);
    if (validateName && searchPrefix)
    {
      String &path = gameDataPath.push_back();
      if (searchPrefix[0] == '*')
      {
        path.setStr(EDITORCORE->getBaseWorkspace().getAppDir());
        path.append(searchPrefix + 1);
      }
      else
        path.setStr(searchPrefix);
    }
  }

  ::register_animchar_gameres_factory();
  ::register_geom_node_tree_gameres_factory();
  ::register_character_gameres_factory();
  ::register_a2d_gameres_factory();
  ::register_fast_phys_gameres_factory();

  ltService = EDITORCORE->queryEditorInterface<ISceneLightService>();

  IDaEditor3Engine::get().registerService(new (inimem) AnimCharEntityManagementService);
}
