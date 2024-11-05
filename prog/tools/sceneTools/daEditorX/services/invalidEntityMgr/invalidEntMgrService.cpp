// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <de3_interface.h>
#include <de3_objEntity.h>
#include <de3_entityPool.h>
#include <de3_entityFilter.h>
#include <de3_baseInterfaces.h>
#include <de3_drawInvalidEntities.h>
#include <de3_editorEvents.h>
#include <math/dag_TMatrix.h>

class InvalidEntity;
typedef SingleEntityPool<InvalidEntity> InvalidEntityPool;

class VirtualInvalidEntity : public IObjEntity
{
public:
  VirtualInvalidEntity() : IObjEntity(-1) {}

  virtual void setTm(const TMatrix &_tm) {}
  virtual void getTm(TMatrix &_tm) const { _tm = TMatrix::IDENT; }
  virtual void *queryInterfacePtr(unsigned huid) { return NULL; }
  virtual void destroy() {}

  virtual BSphere3 getBsph() const { return BSphere3(Point3(0, 0, 0), 1.0); }
  virtual BBox3 getBbox() const { return BBox3(Point3(-0.5, -0.5, -0.5), Point3(0.5, 0.5, 0.5)); }
  virtual const char *getObjAssetName() const { return NULL; }
};

class InvalidEntity : public VirtualInvalidEntity
{
public:
  InvalidEntity() : idx(MAX_ENTITIES) {}

  virtual void setTm(const TMatrix &tm_) { tm = tm_; }
  virtual void getTm(TMatrix &tm_) const { tm_ = tm; }
  virtual void destroy() { pool->delEntity(this); }

public:
  enum
  {
    STEP = 1024,
    MAX_ENTITIES = 0x7FFFFFFF
  };

  unsigned idx;
  TMatrix tm;

  static InvalidEntityPool *pool;
};


class InvalidEntityManagementService : public IEditorService, public IObjEntityMgr
{
public:
  InvalidEntityManagementService()
  {
    setServiceVisible(true);
    InvalidEntity::pool = &pool;
  }
  ~InvalidEntityManagementService()
  {
    pool.freeUnusedEntities();
    InvalidEntity::pool = NULL;
  }

  // IEditorService interface
  virtual const char *getServiceName() const { return "_invalidEntMgr"; }
  virtual const char *getServiceFriendlyName() const { return "(srv) Invalid entities"; }

  virtual void setServiceVisible(bool vis) { IObjEntityFilter::setShowInvalidAsset(vis); }
  virtual bool getServiceVisible() const { return IObjEntityFilter::getShowInvalidAsset(); }

  virtual void actService(float) {}
  virtual void beforeRenderService() {}

  virtual void renderService()
  {
    int stType = IObjEntityFilter::getSubTypeMask(IObjEntityFilter::STMASK_TYPE_RENDER);
    uint64_t lh_mask = IObjEntityFilter::getLayerHiddenMask();
    ::render_invalid_entities(pool.getEntities(), stType);
  }

  virtual void renderTransService() {}
  virtual void onBeforeReset3dDevice() {}
  virtual bool catchEvent(unsigned event_huid, void *userData)
  {
    if (event_huid == HUID_DumpEntityStat)
    {
      DAEDITOR3.conNote("InvalidEntMgr: %d entities", pool.getUsedEntityCount());
    }
    return false;
  }

  virtual void *queryInterfacePtr(unsigned huid)
  {
    RETURN_INTERFACE(huid, IObjEntityMgr);
    return NULL;
  }

  // IObjEntityMgr interface
  virtual bool canSupportEntityClass(int entity_class) const { return entity_class == -1; }

  virtual IObjEntity *createEntity(const DagorAsset &, bool virtual_ent)
  {
    if (virtual_ent)
      return &virtualEnt;

    InvalidEntity *ent = pool.allocEntity();
    if (!ent)
      ent = new InvalidEntity;

    pool.addEntity(ent);

    return ent;
  }

  virtual IObjEntity *cloneEntity(IObjEntity *origin)
  {
    InvalidEntity *o = reinterpret_cast<InvalidEntity *>(origin);
    InvalidEntity *ent = pool.allocEntity();
    G_ASSERT(o->getAssetTypeId() == -1);
    if (!ent)
      ent = new InvalidEntity;

    pool.addEntity(ent);

    return ent;
  }

protected:
  InvalidEntityPool pool;
  VirtualInvalidEntity virtualEnt;
};


InvalidEntityPool *InvalidEntity::pool = NULL;

void init_invalid_entity_service()
{
  if (!IDaEditor3Engine::get().checkVersion())
    return;

  IDaEditor3Engine::get().registerService(new (inimem) InvalidEntityManagementService);
}
