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

  void setTm(const TMatrix &_tm) override {}
  void getTm(TMatrix &_tm) const override { _tm = TMatrix::IDENT; }
  void *queryInterfacePtr(unsigned huid) override { return NULL; }
  void destroy() override {}

  BSphere3 getBsph() const override { return BSphere3(Point3(0, 0, 0), 1.0); }
  BBox3 getBbox() const override { return BBox3(Point3(-0.5, -0.5, -0.5), Point3(0.5, 0.5, 0.5)); }
  const char *getObjAssetName() const override { return NULL; }
};

class InvalidEntity : public VirtualInvalidEntity
{
public:
  InvalidEntity() : idx(MAX_ENTITIES) {}

  void setTm(const TMatrix &tm_) override { tm = tm_; }
  void getTm(TMatrix &tm_) const override { tm_ = tm; }
  void destroy() override { pool->delEntity(this); }

public:
  enum
  {
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
  ~InvalidEntityManagementService() override
  {
    pool.freeUnusedEntities();
    InvalidEntity::pool = NULL;
  }

  // IEditorService interface
  const char *getServiceName() const override { return "_invalidEntMgr"; }
  const char *getServiceFriendlyName() const override { return "(srv) Invalid entities"; }

  void setServiceVisible(bool vis) override { IObjEntityFilter::setShowInvalidAsset(vis); }
  bool getServiceVisible() const override { return IObjEntityFilter::getShowInvalidAsset(); }

  void actService(float) override {}
  void beforeRenderService() override {}

  void renderService() override
  {
    int stType = IObjEntityFilter::getSubTypeMask(IObjEntityFilter::STMASK_TYPE_RENDER);
    uint64_t lh_mask = IObjEntityFilter::getLayerHiddenMask();
    ::render_invalid_entities(pool.getEntities(), stType);
  }

  void renderTransService() override {}
  void onBeforeReset3dDevice() override {}
  bool catchEvent(unsigned event_huid, void *userData) override
  {
    if (event_huid == HUID_DumpEntityStat)
    {
      DAEDITOR3.conNote("InvalidEntMgr: %d entities", pool.getUsedEntityCount());
    }
    return false;
  }

  void *queryInterfacePtr(unsigned huid) override
  {
    RETURN_INTERFACE(huid, IObjEntityMgr);
    return NULL;
  }

  // IObjEntityMgr interface
  bool canSupportEntityClass(int entity_class) const override { return entity_class == -1; }

  IObjEntity *createEntity(const DagorAsset &, bool virtual_ent) override
  {
    if (virtual_ent)
      return &virtualEnt;

    InvalidEntity *ent = pool.allocEntity();
    if (!ent)
      ent = new InvalidEntity;

    pool.addEntity(ent);

    return ent;
  }

  IObjEntity *cloneEntity(IObjEntity *origin) override
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
