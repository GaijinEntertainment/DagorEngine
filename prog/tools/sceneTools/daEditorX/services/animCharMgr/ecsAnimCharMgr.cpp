// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <de3_entityFilter.h>
#include <de3_baseInterfaces.h>
#include <de3_animCtrl.h>
#include <de3_lodController.h>
#include <de3_objEntity.h>
#include <de3_entityPool.h>
#include <assets/assetChangeNotify.h>
#include <assets/assetRefs.h>
#include <EditorCore/ec_interface.h>
#include <EditorCore/ec_input.h>
#include <anim/dag_animPostBlendCtrl.h>
#include <gameRes/dag_stdGameRes.h>
#include <phys/dag_fastPhys.h>
#include <shaders/dag_dynSceneRes.h>
#include <oldEditor/de_workspace.h>
#include <osApiWrappers/dag_direct.h>
#include <daECS/io/blk.h>
#include <ecs/anim/anim.h>
#include <ecs/anim/animcharUpdateEvent.h>
#include <daECS/core/entityManager.h>
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "common.h"
#include "virtualAnimCharEntityBase.h"

static inline ecs::ComponentsInitializer complist_to_compinitializer(ecs::ComponentsList &&clist)
{
  ecs::ComponentsMap cmap;
  for (auto &&a : clist)
    cmap[ECS_HASH_SLOW(a.first.c_str())] = eastl::move(a.second);
  return ecs::ComponentsInitializer(eastl::move(cmap));
}

static ecs::EntityId create_animchar_entity(const char *name, const DataBlock *asset_props,
  ecs::EntityId parentEid = ecs::INVALID_ENTITY_ID, int attSlotId = -1)
{
  ecs::EntityId eid = ecs::INVALID_ENTITY_ID;
  DAEDITOR3.setFatalHandler(false);
  try
  {
    const char *template_name = "animchar_basic";
    ecs::ComponentsList clist;
    clist.emplace_back("transform", eastl::move(ecs::ChildComponent(TMatrix::IDENT)));
    clist.emplace_back("animchar__res", eastl::move(ecs::ChildComponent(ecs::string(name))));
    clist.emplace_back("animchar__updatable", eastl::move(ecs::ChildComponent(true)));
    clist.emplace_back("animchar_attach__attachedTo", parentEid);
    clist.emplace_back("slot_attach__slotId", attSlotId);
    if (asset_props)
    {
      if (const char *fp_name = asset_props->getStr("ref_fastPhys", NULL))
      {
        clist.emplace_back("animchar_fast_phys__res", eastl::move(ecs::string(fp_name)));
        template_name = "animchar_with_fast_phys";
      }
      if (const DataBlock *b = asset_props->getBlockByName("components"))
        ecs::load_comp_list_from_blk(*g_entity_mgr, *b, clist);
    }

    eid = g_entity_mgr->createEntitySync(template_name, complist_to_compinitializer(eastl::move(clist)));
  }
  catch (...)
  {
    logerr("exception while creating eid: %s", name);
  }
  DAEDITOR3.popFatalHandler();
  return eid;
}

struct AttachmentStor
{
  int slotId = -1;
  ecs::EntityId eid = ecs::INVALID_ENTITY_ID;
};

class AnimCharECSEntity;
using PoolType = SingleEntityPool<AnimCharECSEntity>;
using AnimCharEntityBase = VirtualAnimCharEntityBase<PoolType, AttachmentStor>;

class VirtualAnimCharECSEntity : public AnimCharEntityBase
{
public:
  VirtualAnimCharECSEntity(int cls) : AnimCharEntityBase(cls) {}

  ~VirtualAnimCharECSEntity() { clear(); }

  void clear()
  {
    termDebugFifo();

    if (g_entity_mgr.get())
    {
      g_entity_mgr->destroyEntity(eid);
      for (auto &att : attStor)
        g_entity_mgr->destroyEntity(att.eid);
      g_entity_mgr->tick(true);
    }

    dynAtt.clear();
    attStor.clear();
  }

  void setup(const DagorAsset &asset, bool is_virtual)
  {
    const char *name = asset.getName();
    assetNameId = asset.getNameId();
    usedAssetNid = usedAssetNames.addNameId(asset.getNameTypified());
    usedRefNids.clear();

    if (is_virtual)
      return;

    // create entity
    eid = create_animchar_entity(name, &asset.props);
    if (eid == ecs::INVALID_ENTITY_ID)
    {
      DAEDITOR3.conError("cannot load animChar: %s", name);
      debug_flush(false);
      return;
    }

    // setup attachments
    for (int i = 0, nid = asset.props.getNameId("ref_attachment"); i < asset.props.blockCount(); i++)
      if (asset.props.getBlock(i)->getBlockNameId() == nid)
      {
        const DataBlock &b = *asset.props.getBlock(i);
        const char *slot_name = b.getStr("slot", NULL);
        int slot_id = AnimCharV20::getSlotId(slot_name);
        if (slot_id < 0)
        {
          DAEDITOR3.conError("attachment bad slot <%s> in animChar <%s>", slot_name, name);
          continue;
        }

        if (const char *attAcNm = b.getStr("animChar", NULL))
        {
          DagorAsset *a = asset.getMgr().findAsset(attAcNm, animCharEntityClassId);
          if (a)
            usedRefNids.push_back(usedAssetNames.addNameId(a->getNameTypified()));

          auto assetProps = a ? &a->props : nullptr;
          ecs::EntityId attEid = create_animchar_entity(attAcNm, assetProps, eid, slot_id);
          if (attEid != ecs::INVALID_ENTITY_ID)
          {
            auto attBaseComp = ECS_GET_COMPONENT(AnimV20::AnimcharBaseComponent, attEid, animchar);
            if (assetProps)
              init_ref_default_state(attBaseComp, *assetProps);
            DAEDITOR3.conNote("attached char <%s> to slot <%s> of animChar <%s>", attAcNm, slot_name, name);
            replaceAtt(slot_id, attEid);
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
    auto baseComp = getAnimCharBase();
    for (int i = 0, nid = asset.props.getNameId("ref_attachment_dynamic"); i < asset.props.blockCount(); i++)
      if (asset.props.getBlock(i)->getBlockNameId() == nid && baseComp && baseComp->getAnimGraph())
      {
        const DataBlock &b = *asset.props.getBlock(i);
        const char *slot_name = b.getStr("slot", NULL);
        const char *var_name = b.getStr("var", NULL);
        const char *enum_cls = b.getStr("enum", NULL);
        AnimV20::AnimationGraph *anim = baseComp->getAnimGraph();

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

        int nid_sv = b.getNameId("syncVar");
        for (int j = 0; j < b.paramCount(); j++)
          if (b.getParamNameId(j) == nid_sv && b.getParamType(j) == b.TYPE_STRING)
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

    // calculating bbox and bsphere
    updateBounds();

    // run default animation
    const char *ref_anim = asset.props.getStr("ref_anim", NULL);
    if (ref_anim)
      setAnim(ref_anim);

    init_ref_default_state(baseComp, asset.props);
  }

  bool setAnim(const char *anim)
  {
    auto baseComp = getAnimCharBase();
    if (!baseComp || !baseComp->getAnimGraph())
    {
      DAEDITOR3.conWarning("cant set ref anim <%s> due to AG is missing in animChar", anim);
      return false;
    }
    if (!enqueueAnim(anim))
    {
      enqueueAnim(NULL);
      return false;
    }

    updateBounds();

    if (anim && *anim)
      DAEDITOR3.conNote("set anim <%s>", anim);
    else
      DAEDITOR3.conNote("reset anim");
    return true;
  }

  void update(float dt)
  {
    auto baseComp = getAnimCharBase();
    if (!baseComp)
      return;

    for (int i = 0; i < dynAtt.size(); i++)
    {
      DynamicAttachement &att = dynAtt[i];
      float val =
        att.varTypeInt ? baseComp->getAnimState()->getParamInt(att.animVarId) : baseComp->getAnimState()->getParam(att.animVarId);
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
        ec_set_busy(true);
        usedRefNids.push_back(att.setAssetNid = usedAssetNames.addNameId(a->getNameTypified()));

        auto assetProps = &a->props;
        auto attachedEid = create_animchar_entity(a->getName(), assetProps, eid, att.slotId);
        baseComp = getAnimCharBase();
        if (attachedEid != ecs::INVALID_ENTITY_ID)
        {
          auto attBaseComp = ECS_GET_COMPONENT(AnimV20::AnimcharBaseComponent, attachedEid, animchar);
          init_ref_default_state(attBaseComp, *assetProps);
          DAEDITOR3.conNote("attached char <%s> to slotId=%d of animChar <%s>", a->getName(), att.slotId, assetName());
        }
        replaceAtt(att.slotId, attachedEid);
        ec_set_busy(false);
      }
      else
      {
        DAEDITOR3.conNote("reset attachements at slotId=%d of animChar <%s>", att.slotId, assetName());
        att.setAssetNid = -1;
        replaceAtt(att.slotId, ecs::INVALID_ENTITY_ID);
      }

      if (!att.configDir.empty() && enum_val && *enum_val)
      {
        String cfg_fn(0, "%s/%s.blk", att.configDir, enum_val);
        simplify_fname(cfg_fn);
        if (dd_file_exists(cfg_fn))
        {
          AnimV20::AnimationGraph *ag = baseComp->getAnimGraph();
          AnimV20::IAnimStateHolder *as = baseComp->getAnimState();

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

    // For some reason in dng based animchar update (animchar.cpp.inl animchar__updater_es)
    // attachment chars are connected to the char only for the duration of animchar update and
    // attachments are released right after. This means we can't get attached chars through
    // getAttachedChar and we can't call initAttachmentTmAndNodeWtm.
    // As a workaround we set attachments here so that we can do what we want.
    for (int i = 0; i < attStor.size(); ++i)
    {
      if (AnimV20::AnimcharBaseComponent *attBaseComp = ECS_GET_COMPONENT(AnimV20::AnimcharBaseComponent, attStor[i].eid, animchar))
        baseComp->setAttachedChar(attStor[i].slotId, ecs::entity_id_t(attStor[i].eid), *attBaseComp, /*recalcable*/ false);
    }

    for (int i = 0; i < dynAtt.size(); i++)
      if (auto attBaseComp = baseComp->getAttachedChar(dynAtt[i].slotId))
      {
        if (AnimV20::AnimationGraph *attAnim = attBaseComp->getAnimGraph())
        {
          for (int j = 0; j < dynAtt[i].syncVarsF.size(); j++)
          {
            int param_id = attAnim->getParamId(baseComp->getAnimGraph()->getParamName(dynAtt[i].syncVarsF[j]));
            if (param_id >= 0)
              attBaseComp->getAnimState()->setParam(param_id, baseComp->getAnimState()->getParam(dynAtt[i].syncVarsF[j]));
          }
          for (int j = 0; j < dynAtt[i].syncVarsI.size(); j++)
          {
            int param_id = attAnim->getParamId(baseComp->getAnimGraph()->getParamName(dynAtt[i].syncVarsI[j]));
            if (param_id >= 0)
              attBaseComp->getAnimState()->setParamInt(param_id, baseComp->getAnimState()->getParamInt(dynAtt[i].syncVarsI[j]));
          }
        }
      }

    if (baseComp)
      for (auto &att : attStor)
      {
        mat44f attach_tm;
        if (att.eid != ecs::INVALID_ENTITY_ID && baseComp->initAttachmentTmAndNodeWtm(att.slotId, attach_tm))
        {
          TMatrix wtm;
          v_mat_43cu_from_mat44(wtm.array, attach_tm);
          g_entity_mgr->set(att.eid, ECS_HASH("transform"), wtm);
        }
      }
  }

  void replaceAtt(int slot_id, ecs::EntityId att_eid)
  {
    for (auto &att : attStor)
      if (att.slotId == slot_id)
      {
        g_entity_mgr->destroyEntity(att.eid);
        att.eid = att_eid;
        return;
      }

    if (att_eid != ecs::INVALID_ENTITY_ID)
    {
      AttachmentStor &att = attStor.push_back();
      att.slotId = slot_id;
      att.eid = att_eid;
    }
  }

protected:
  void updateBounds() override
  {
    auto baseComp = getAnimCharBase();
    if (auto wtm = getAnimCharFinalWTM(); baseComp && wtm)
    {
      baseComp->act(0.001f, true);

      auto rendComp = getAnimCharRend();
      bbox3f wbb;
      rendComp->calcWorldBox(wbb, *wtm, false);
      bbox[0] = as_point3(&wbb.bmin);
      bbox[1] = as_point3(&wbb.bmax);
      bsph = bbox;
    }
  }

  AnimV20::AnimationGraph *getAnimGraph() const override
  {
    auto baseComp = getAnimCharBase();
    return baseComp ? baseComp->getAnimGraph() : NULL;
  }
  AnimV20::IAnimStateHolder *getAnimState() const override
  {
    auto *baseComp = getAnimCharBase();
    return baseComp ? baseComp->getAnimState() : NULL;
  }
  AnimV20::AnimcharBaseComponent *getAnimCharBase() const override
  {
    return g_entity_mgr ? ECS_GET_COMPONENT(AnimV20::AnimcharBaseComponent, eid, animchar) : nullptr;
  }
  AnimV20::AnimcharRendComponent *getAnimCharRend() const override
  {
    return g_entity_mgr ? ECS_GET_COMPONENT(AnimV20::AnimcharRendComponent, eid, animchar_render) : nullptr;
  }
  const AnimV20::AnimcharFinalMat44 *getAnimCharFinalWTM() const
  {
    return g_entity_mgr ? ECS_GET_COMPONENT(AnimcharNodesMat44, eid, animchar_node_wtm) : nullptr;
  }

protected:
  ecs::EntityId eid;
};


class AnimCharECSEntity : public VirtualAnimCharECSEntity, public IAnimCharController, public ILodController
{
public:
  AnimCharECSEntity(int cls) : VirtualAnimCharECSEntity(cls), idx(MAX_ENTITIES) {}

  void clear()
  {
    clear_and_shrink(dbgAnimNodes);
    VirtualAnimCharECSEntity::clear();
  }

  void setTm(const TMatrix &_tm) override { g_entity_mgr->set(eid, ECS_HASH("transform"), _tm); }

  void getTm(TMatrix &_tm) const override
  {
    auto tm = ECS_GET_COMPONENT(TMatrix, eid, transform);
    _tm = tm ? *tm : TMatrix::IDENT;
  }

  void *queryInterfacePtr(unsigned huid) override
  {
    RETURN_INTERFACE(huid, IAnimCharController);
    RETURN_INTERFACE(huid, ILodController);
    return NULL;
  }

  void destroy() override
  {
    pool->delEntity(this);
    clear();
  }

  void copyFrom(const VirtualAnimCharECSEntity &e)
  {
    if (auto a = aMgr->findAsset(e.assetName(), animCharEntityClassId))
    {
      setup(*a, false);
      AnimCharEntityBase::copyFrom(e);
    }
  }

  // IAnimCharController
  int getStatesCount() override { return getAnimGraph() ? getAnimGraph()->getStateCount() : 0; }
  const char *getStateName(int s_idx) override { return getAnimGraph() && s_idx >= 0 ? getAnimGraph()->getStateName(s_idx) : NULL; }
  int getStateIdx(const char *name) override { return getAnimGraph() ? getAnimGraph()->getStateIdx(name) : -1; }
  void enqueueState(int s_idx, float force_spd = -1.f) override
  {
    auto ag = getAnimGraph();
    if (!ag || s_idx < 0)
      return;
    ag->enqueueState(*getAnimState(), ag->getState(s_idx), -1.f, force_spd);
    if (isPaused())
      fastUpdate();
  }
  bool hasReferencesTo(const DagorAsset *a) override { return false; }

  AnimV20::AnimationGraph *getAnimGraph() const override { return VirtualAnimCharECSEntity::getAnimGraph(); }
  AnimV20::IAnimStateHolder *getAnimState() const override { return VirtualAnimCharECSEntity::getAnimState(); }
  AnimV20::AnimcharBaseComponent *getAnimCharBase() const override { return VirtualAnimCharECSEntity::getAnimCharBase(); }
  AnimV20::AnimcharRendComponent *getAnimCharRend() const override { return VirtualAnimCharECSEntity::getAnimCharRend(); }
  const AnimV20::AnimcharFinalMat44 *getAnimCharFinalWTM() const override { return VirtualAnimCharECSEntity::getAnimCharFinalWTM(); }
  BSphere3 getAnimCharBoundingSphere() const override { return bsph; }

  bool isPaused() const override { return simPaused; }
  void setPaused(bool paused) override { simPaused = paused; }
  float getTimeScale() const override { return simDtScale; }
  void setTimeScale(float t_scale) override { simDtScale = t_scale; }
  void restart() override
  {
    if (auto *baseComp = getAnimCharBase())
    {
      baseComp->reset();
      if (auto asset = aMgr ? aMgr->findAsset(assetName(), animCharEntityClassId) : nullptr)
      {
        init_ref_default_state(baseComp, asset->props);
        fastUpdate();
      }
    }
  }
  void fastUpdate() { g_entity_mgr->broadcastEventImmediate(UpdateAnimcharEvent(0.03f, 0.0f)); }

  int getAnimNodeCount() override
  {
    fillDbgAnimNodes();
    return dbgAnimNodes.size();
  }
  const char *getAnimNodeName(int a_idx) override
  {
    fillDbgAnimNodes();
    return a_idx >= 0 && a_idx < dbgAnimNodes.size() ? dbgAnimNodes[a_idx] : NULL;
  }
  bool enqueueAnimNode(const char *anim_node_nm) override
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
  int getLodCount() override
  {
    auto rendComp = getAnimCharRend();
    return rendComp ? rendComp->getSceneInstance()->getLodsCount() : 0;
  }
  void setCurLod(int n) override
  {
    if (auto rendComp = getAnimCharRend())
      rendComp->getSceneInstance()->setLod(n);
  }
  real getLodRange(int lod_n) override
  {
    auto rendComp = getAnimCharRend();
    return rendComp->getSceneInstance()->getLodsResource()->lods[lod_n].range;
  }

  // no named nodes support
  int getNamedNodeCount() override { return 0; }
  const char *getNamedNode(int idx) override { return NULL; }
  int getNamedNodeIdx(const char *nm) override { return -1; }
  bool getNamedNodeVisibility(int idx) override { return true; }
  void setNamedNodeVisibility(int idx, bool vis) override {}

public:
  enum
  {
    MAX_ENTITIES = 0x7FFFFFFF
  };

  unsigned idx;
  Tab<const char *> dbgAnimNodes;
};


class AnimCharECSEntityManagementService : public IEditorService,
                                           public IObjEntityMgr,
                                           public IDagorAssetChangeNotify,
                                           public ILightingChangeClient,
                                           public ILevelResListBuilder,
                                           public IBinaryDataBuilder
{
public:
  AnimCharECSEntityManagementService()
  {
    animCharEntityClassId = IDaEditor3Engine::get().getAssetTypeId("animChar");
    aMgr = NULL;
    visible = true;
  }
  ~AnimCharECSEntityManagementService() override { usedAssetNames.reset(true); }

  // IEditorService interface
  const char *getServiceName() const override { return "_acEntMgr"; }
  const char *getServiceFriendlyName() const override { return "(srv) AnimChar entities"; }

  void setServiceVisible(bool vis) override { visible = vis; }
  bool getServiceVisible() const override { return visible; }

  void actService(float dt) override
  {
    if (simPaused)
      return;
    dt *= simDtScale;

    g_entity_mgr->broadcastEventImmediate(UpdateAnimcharEvent(dt, 0.0f));

    dag::ConstSpan<AnimCharECSEntity *> ent = objPool.getEntities();
    int st_mask = IObjEntityFilter::getSubTypeMask(IObjEntityFilter::STMASK_TYPE_RENDER);
    const LayerHiddenMask lh_mask = IObjEntityFilter::getLayerHiddenMask();
    for (int i = 0; i < ent.size(); i++)
      if (ent[i] && ent[i]->isNonVirtual() && ent[i]->checkSubtypeAndLayerHiddenMasks(st_mask, lh_mask))
        ent[i]->update(dt);
  }
  void beforeRenderService() override {}
  void renderService() override {}
  void renderTransService() override {}

  void onBeforeReset3dDevice() override {}

  void *queryInterfacePtr(unsigned huid) override
  {
    RETURN_INTERFACE(huid, IObjEntityMgr);
    RETURN_INTERFACE(huid, ILightingChangeClient);
    RETURN_INTERFACE(huid, IBinaryDataBuilder);
    RETURN_INTERFACE(huid, ILevelResListBuilder);
    return NULL;
  }

  // ILightingChangeClient
  void onLightingChanged() override {}
  void onLightingSettingsChanged() override {}

  // IObjEntityMgr interface
  bool canSupportEntityClass(int entity_class) const override
  {
    return animCharEntityClassId >= 0 && animCharEntityClassId == entity_class;
  }

  IObjEntity *createEntity(const DagorAsset &asset, bool virtual_ent) override
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
      VirtualAnimCharECSEntity *ent = new VirtualAnimCharECSEntity(animCharEntityClassId);
      ent->setup(asset, true);
      return ent;
    }

    AnimCharECSEntity *ent = objPool.allocEntity();
    if (ent)
      ent->clear();
    else
      ent = new AnimCharECSEntity(animCharEntityClassId);

    ent->setup(asset, virtual_ent);
    objPool.addEntity(ent);
    // debug("create ent: %p", ent);
    return ent;
  }

  IObjEntity *cloneEntity(IObjEntity *origin) override
  {
    auto o = reinterpret_cast<VirtualAnimCharECSEntity *>(origin);
    auto ent = objPool.allocEntity();
    if (ent)
      ent->clear();
    else
      ent = new AnimCharECSEntity(o->getAssetTypeId());

    ent->copyFrom(*o);
    objPool.addEntity(ent);
    // debug("clone ent: %p", ent);
    return ent;
  }

  // IDagorAssetChangeNotify interface
  void onAssetRemoved(int asset_name_id, int asset_type) override { EDITORCORE->invalidateViewportCache(); }
  void onAssetChanged(const DagorAsset &asset, int asset_name_id, int asset_type) override { EDITORCORE->invalidateViewportCache(); }

  // IBinaryDataBuilder interface
  bool validateBuild(int target, ILogWriter &log, PropPanel::ContainerPropertyControl *params) override { return true; }

  bool buildAndWrite(BinDumpSaveCB &main_cwr, const ITextureNumerator &tn, PropPanel::ContainerPropertyControl *) override
  {
    return true;
  }

  bool checkMetrics(const DataBlock &metrics_blk) override { return true; }

  bool addUsedTextures(ITextureNumerator &tn) override { return true; }

  // ILevelResListBuilder
  void addUsedResNames(OAHashNameMap<true> &res_list) override
  {
    dag::ConstSpan<AnimCharECSEntity *> ent = objPool.getEntities();
    for (int i = 0; i < ent.size(); i++)
      if (ent[i])
        res_list.addNameId(ent[i]->assetName());
  }

protected:
  PoolType objPool;
  bool visible;
};


void init_ecs_animchar_mgr_service(const DataBlock &app_blk)
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

  IDaEditor3Engine::get().registerService(new (inimem) AnimCharECSEntityManagementService);
}
