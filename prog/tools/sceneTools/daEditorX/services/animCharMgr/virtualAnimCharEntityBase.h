// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <de3_interface.h>
#include <de3_objEntity.h>
#include <anim/dag_animBlendCtrl.h>
#include <animChar/dag_animCharacter2.h>

template <class EntityPool, class AttachmentStor>
class VirtualAnimCharEntityBase : public IObjEntity
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

  VirtualAnimCharEntityBase(int cls) : IObjEntity(cls), pool(NULL)
  {
    bbox = BBox3(Point3(-0.5, -0.5, -0.5), Point3(0.5, 0.5, 0.5));
    bsph = BSphere3(Point3(0, 0, 0), 0.5);
    assetNameId = -1;
    usedAssetNid = -1;
  }

  virtual ~VirtualAnimCharEntityBase() { assetNameId = -1; }

  void setTm(const TMatrix &_tm) override {}
  void getTm(TMatrix &_tm) const override { _tm = TMatrix::IDENT; }
  void destroy() override { delete this; }
  void *queryInterfacePtr(unsigned huid) override { return NULL; }

  BSphere3 getBsph() const override { return bsph; }
  BBox3 getBbox() const override { return bbox; }

  const char *getObjAssetName() const override { return usedAssetNames.getName(usedAssetNid); }

  bool isNonVirtual() const { return pool; }
  int getAssetNameId() const { return assetNameId; }
  inline const char *assetName() const { return aMgr ? aMgr->getAssetName(assetNameId) : NULL; }

  void copyFrom(const VirtualAnimCharEntityBase &e)
  {
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
      auto baseComp = getAnimCharBase();
      if (baseComp && a)
      {
        const char *ref_anim = a->props.getStr("ref_anim", NULL);
        if (ref_anim)
          setAnim(ref_anim);
      }
    }
  }

  AnimV20::AnimBlendCtrl_Fifo3 *initDebugFifo(bool set_anim)
  {
    AnimV20::AnimationGraph *ag = getAnimGraph();
    if (!ag)
      return NULL;
    AnimV20::IAnimStateHolder *as = getAnimState();
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
    getAnimCharBase()->reset();
    dbgHub->setBlendNodeWt(*as, dbgOldRoot, set_anim ? 0.0f : 1.0f);
    dbgHub->setBlendNodeWt(*as, dbgFifo, set_anim ? 1.0f : 0.0f);
    return dbgFifo;
  }

  void termDebugFifo()
  {
    AnimV20::AnimationGraph *ag = getAnimGraph();
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
      getAnimCharBase()->reset();
    }
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

  void init_ref_default_state(AnimV20::AnimcharBaseComponent *baseComp, const DataBlock &props)
  {
    AnimV20::AnimationGraph *ag = baseComp->getAnimGraph();
    if (!ag)
      return;
    AnimV20::IAnimStateHolder &st = *baseComp->getAnimState();
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
          if (int s_idx = ag->getStateIdx(b->getStr(i)); s_idx >= 0)
            ag->enqueueState(st, ag->getState(s_idx), -1);
  }

  bool enqueueAnim(const char *anim)
  {
    AnimV20::AnimationGraph *ag = getAnimGraph();
    if (!ag)
      return false;
    if (anim && !*anim)
      anim = NULL;

    AnimV20::IAnimStateHolder *as = getAnimState();
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
      fifo->enqueueState(*as, n, 0.15);
    }
    return true;
  }

  bool setAnim(const char *anim)
  {
    auto ag = getAnimCharBase();
    if (!ag)
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

protected:
  virtual void updateBounds() = 0;

  virtual AnimV20::AnimationGraph *getAnimGraph() const = 0;
  virtual AnimV20::IAnimStateHolder *getAnimState() const = 0;
  virtual AnimV20::AnimcharBaseComponent *getAnimCharBase() const = 0;
  virtual AnimV20::AnimcharRendComponent *getAnimCharRend() const = 0;

public:
  EntityPool *pool;
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