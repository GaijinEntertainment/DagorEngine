// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <de3_interface.h>
#include <de3_objEntity.h>
#include <de3_effectController.h>
#include <de3_entityPool.h>
#include <de3_entityFilter.h>
#include <de3_baseInterfaces.h>
#include <de3_writeObjsToPlaceDump.h>
#include <de3_entityUserData.h>
#include <de3_editorEvents.h>
#include <oldEditor/de_common_interface.h>
#include <assets/assetMgr.h>
#include <libTools/util/makeBindump.h>
#include <libTools/util/iLogWriter.h>
#include <drv/3d/dag_driver.h>
#include <3d/dag_render.h>
#include <render/dag_cur_view.h>
#include <math/dag_TMatrix.h>
#include <util/dag_fastStrMap.h>
#include <debug/dag_debug.h>

static int fxEntityClassId = -1;
static int efxEntityClassId = -1;
static OAHashNameMap<true> fxNames;

class ExtFxEntity;
typedef SingleEntityPool<ExtFxEntity> ExtFxEntityPool;

class VirtualExtFxEntity : public IObjEntity
{
public:
  VirtualExtFxEntity(int cls) : IObjEntity(cls), pool(NULL), fx(NULL), nameId(-1) {}
  virtual ~VirtualExtFxEntity() { destroy_it(fx); }

  virtual void setTm(const TMatrix &_tm) {}
  virtual void getTm(TMatrix &_tm) const { _tm = TMatrix::IDENT; }
  virtual void destroy() { delete this; }

  virtual BSphere3 getBsph() const { return fx ? fx->getBsph() : BSphere3(Point3(0, 0, 0), 1.0); }
  virtual BBox3 getBbox() const { return fx ? fx->getBbox() : BBox3(Point3(-0.5, -0.5, -0.5), Point3(0.5, 0.5, 0.5)); }

  virtual const char *getObjAssetName() const
  {
    static String buf;
    buf.printf(0, "%s:efx", fxNames.getName(nameId));
    return buf;
  }
  virtual void *queryInterfacePtr(unsigned huid) { return NULL; }

  void setup(const DagorAsset &asset, bool virt)
  {
    const char *a_name = asset.props.getStr("fx", NULL);
    DagorAsset *a = *a_name ? DAEDITOR3.getAssetByName(a_name, fxEntityClassId) : NULL;
    if (!a)
      DAEDITOR3.conError("cannot find asset <%s> in ext.FX <%s>", a_name, asset.getName());

    fx = a ? DAEDITOR3.createEntity(*a, virt) : NULL;
    if (fx)
    {
      nameId = fxNames.addNameId(asset.getName());
      fx->setSubtype(getSubtype());
    }
    else if (a)
      DAEDITOR3.conError("cannot create entity <%s> for ext.FX <%s>", a_name, asset.getName());
  }

  bool isNonVirtual() const { return pool; }

public:
  ExtFxEntityPool *pool;
  IObjEntity *fx;
  int nameId;
};

class ExtFxEntity : public VirtualExtFxEntity, public IObjEntityUserDataHolder, public IEffectController
{
public:
  ExtFxEntity(int cls) : VirtualExtFxEntity(cls), idx(MAX_ENTITIES), userDataBlk(NULL) { tm.identity(); }
  ~ExtFxEntity() { clear(); }

  virtual void setTm(const TMatrix &_tm)
  {
    tm = _tm;
    if (!fx)
      return;
    fx->setTm(tm);
  }
  virtual void getTm(TMatrix &_tm) const { _tm = tm; }
  virtual void setSubtype(int t)
  {
    VirtualExtFxEntity::setSubtype(t);
    if (fx)
      fx->setSubtype(getSubtype());
  }
  virtual void destroy()
  {
    pool->delEntity(this);
    clear();
  }

  virtual void *queryInterfacePtr(unsigned huid)
  {
    RETURN_INTERFACE(huid, IObjEntityUserDataHolder);
    RETURN_INTERFACE(huid, IEffectController);
    return NULL;
  }

  // IObjEntityUserDataHolder
  virtual DataBlock *getUserDataBlock(bool create_if_not_exist)
  {
    if (!userDataBlk && create_if_not_exist)
      userDataBlk = new DataBlock;
    return userDataBlk;
  }
  virtual void resetUserDataBlock() { del_it(userDataBlk); }

  // IEffectController
  virtual bool isActive() const override
  {
    if (!fx)
      return false;

    IEffectController *effectController = fx->queryInterface<IEffectController>();
    return effectController && effectController->isActive();
  }

  virtual void setSpawnRate(float rate) override
  {
    if (!fx)
      return;

    IEffectController *effectController = fx->queryInterface<IEffectController>();
    if (effectController)
      effectController->setSpawnRate(rate);
  }

  void copyFrom(const VirtualExtFxEntity &e)
  {
    nameId = e.nameId;
    fx = e.fx ? DAEDITOR3.cloneEntity(e.fx) : NULL;
    del_it(userDataBlk);
    if (fx)
    {
      fx->setSubtype(getSubtype());
      fx->setTm(tm);
    }
  }
  void clear()
  {
    destroy_it(fx);
    del_it(userDataBlk);
  }

public:
  enum
  {
    STEP = 512,
    MAX_ENTITIES = 0x7FFFFFFF
  };

  unsigned idx;
  TMatrix tm;
  DataBlock *userDataBlk;
};


class ExtFxEntityManagementService : public IEditorService,
                                     public IObjEntityMgr,
                                     public IBinaryDataBuilder,
                                     public ILevelResListBuilder
{
public:
  ExtFxEntityManagementService()
  {
    fxEntityClassId = IDaEditor3Engine::get().getAssetTypeId("fx");
    efxEntityClassId = IDaEditor3Engine::get().getAssetTypeId("efx");

    visible = true;
  }

  // IEditorService interface
  virtual const char *getServiceName() const { return "_efxEntMgr"; }
  virtual const char *getServiceFriendlyName() const { return "(srv) Ext.FX entities"; }

  virtual void setServiceVisible(bool vis) { visible = vis; }
  virtual bool getServiceVisible() const { return visible; }

  virtual void actService(float dt) {}
  virtual void beforeRenderService() {}
  virtual void renderService() {}
  virtual void renderTransService() {}

  virtual void onBeforeReset3dDevice() {}
  virtual bool catchEvent(unsigned event_huid, void *userData)
  {
    if (event_huid == HUID_DumpEntityStat)
      DAEDITOR3.conNote("ExtFxMgr: %d entities", efxPool.getUsedEntityCount());
    return false;
  }

  virtual void *queryInterfacePtr(unsigned huid)
  {
    RETURN_INTERFACE(huid, IObjEntityMgr);
    RETURN_INTERFACE(huid, IBinaryDataBuilder);
    RETURN_INTERFACE(huid, ILevelResListBuilder);
    return NULL;
  }

  // IObjEntityMgr interface
  virtual bool canSupportEntityClass(int entity_class) const { return efxEntityClassId >= 0 && efxEntityClassId == entity_class; }

  virtual IObjEntity *createEntity(const DagorAsset &asset, bool virtual_ent)
  {
    if (virtual_ent)
    {
      VirtualExtFxEntity *ent = new VirtualExtFxEntity(efxEntityClassId);
      ent->setup(asset, virtual_ent);
      return ent;
    }

    ExtFxEntity *ent = efxPool.allocEntity();
    if (!ent)
      ent = new ExtFxEntity(efxEntityClassId);

    ent->setup(asset, virtual_ent);
    efxPool.addEntity(ent);
    // debug("create ent: %p", ent);
    return ent;
  }

  virtual IObjEntity *cloneEntity(IObjEntity *origin)
  {
    VirtualExtFxEntity *o = reinterpret_cast<VirtualExtFxEntity *>(origin);
    ExtFxEntity *ent = efxPool.allocEntity();
    if (!ent)
      ent = new ExtFxEntity(o->getAssetTypeId());

    ent->copyFrom(*o);
    efxPool.addEntity(ent);
    // debug("clone ent: %p", ent);
    return ent;
  }

  // IBinaryDataBuilder interface
  virtual bool validateBuild(int target, ILogWriter &log, PropPanel::ContainerPropertyControl *params)
  {
    if (!efxPool.calcEntities(IObjEntityFilter::getSubTypeMask(IObjEntityFilter::STMASK_TYPE_EXPORT)))
      log.addMessage(log.WARNING, "No ext.fx entities for export");
    return true;
  }

  virtual bool addUsedTextures(ITextureNumerator &tn) { return true; }

  virtual bool buildAndWrite(BinDumpSaveCB &cwr, const ITextureNumerator &tn, PropPanel::ContainerPropertyControl *)
  {
    dag::Vector<SrcObjsToPlace> objs;

    dag::ConstSpan<ExtFxEntity *> ent = efxPool.getEntities();
    int st_mask = IObjEntityFilter::getSubTypeMask(IObjEntityFilter::STMASK_TYPE_EXPORT);
    for (int i = 0; i < ent.size(); i++)
      if (ent[i] && ent[i]->fx && ent[i]->isNonVirtual() && ent[i]->checkSubtypeMask(st_mask))
      {
        int k;
        for (k = 0; k < objs.size(); k++)
          if (ent[i]->nameId == objs[k].nameId)
            break;
        if (k == objs.size())
        {
          objs.push_back();
          objs[k].nameId = ent[i]->nameId;
          objs[k].resName = fxNames.getName(ent[i]->nameId);
        }
        objs[k].tm.push_back(ent[i]->tm);
        if (ent[i]->userDataBlk)
          objs[k].addBlk.addNewBlock(ent[i]->userDataBlk, "");
        else
          objs[k].addBlk.addNewBlock("");
      }

    writeObjsToPlaceDump(cwr, make_span(objs), _MAKE4C('eFX'));
    return true;
  }

  virtual bool checkMetrics(const DataBlock &metrics_blk)
  {
    int subtype_mask = IObjEntityFilter::getSubTypeMask(IObjEntityFilter::STMASK_TYPE_EXPORT);
    int cnt = efxPool.calcEntities(subtype_mask);
    int maxCount = metrics_blk.getInt("max_entities", 0);

    if (cnt > maxCount)
    {
      DAEDITOR3.conError("Metrics validation failed: ext.FX count=%d  > maximum allowed=%d.", cnt, maxCount);
      return false;
    }
    return true;
  }

  // ILevelResListBuilder
  virtual void addUsedResNames(OAHashNameMap<true> &res_list)
  {
    /*== for now efx is nor gameres and is not exported
    dag::ConstSpan<ExtFxEntity*> ent = efxPool.getEntities();
    int st_mask = IObjEntityFilter::getSubTypeMask(IObjEntityFilter::STMASK_TYPE_EXPORT);
    SmallTab<bool, TmpmemAlloc> used_mark;
    used_mark.resize(fxNames.nameCount());
    mem_set_0(used_mark);

    for ( int i = 0; i < ent.size(); i ++ )
      if ( ent[i] && ent[i]->fx && ent[i]->isNonVirtual() && ent[i]->checkSubtypeMask(st_mask))
      {
        if (used_mark[ent[i]->nameId])
          continue;
        used_mark[ent[i]->nameId] = 1;
        res_list.addNameId(fxNames.getName(ent[i]->nameId));
      }
    */
  }

protected:
  ExtFxEntityPool efxPool;
  bool visible;
};


void init_efxmgr_service()
{
  if (!IDaEditor3Engine::get().checkVersion())
  {
    DEBUG_CTX("Incorrect version!");
    return;
  }

  IDaEditor3Engine::get().registerService(new (inimem) ExtFxEntityManagementService);
}

void mount_efx_assets(DagorAssetMgr &aMgr, const char *fx_blk_fname)
{
  int atype = aMgr.getAssetTypeId("efx");
  if (atype < 0)
    return;

  DataBlock blk;
  if (!blk.load(fx_blk_fname))
  {
    DAEDITOR3.conError("cannot load EFX blk: %s", fx_blk_fname);
    return;
  }
  if (!blk.blockCount())
    return;

  FastStrMap nm;
  for (int i = 0; i < blk.blockCount(); i++)
    if (blk.getBlock(i)->getStr("fx", NULL))
    {
      const char *fx_nm = blk.getBlock(i)->getBlockName();
      if (nm.addStrId(fx_nm, i) != i)
        DAEDITOR3.conError("%s has duplicate FX <%s> at %d and %d block", fx_blk_fname, fx_nm, i, nm.getStrId(fx_nm));
    }

  aMgr.startMountAssetsDirect("ext.FX", 0);
  for (int i = 0; i < nm.getMapRaw().size(); i++)
    aMgr.makeAssetDirect(nm.getMapRaw()[i].name, *blk.getBlock(nm.getMapRaw()[i].id), atype);
  aMgr.stopMountAssetsDirect();
}
