// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <de3_interface.h>
#include <de3_objEntity.h>
#include <de3_entityPool.h>
#include <de3_entityFilter.h>
#include <de3_lightService.h>
#include <de3_baseInterfaces.h>
#include <de3_writeObjsToPlaceDump.h>
#include <de3_entityUserData.h>
#include <de3_randomSeed.h>
#include <oldEditor/de_common_interface.h>
#include <assets/assetChangeNotify.h>
#include <assets/assetMgr.h>
#include <EditorCore/ec_interface.h>
#include <libTools/util/makeBindump.h>
#include <libTools/util/iLogWriter.h>
#include <phys/dag_physResource.h>
#include <gameRes/dag_gameResSystem.h>
#include <gameRes/dag_stdGameRes.h>
#include <drv/3d/dag_driver.h>
#include <3d/dag_render.h>
#include <shaders/dag_dynSceneRes.h>
#include <render/dag_cur_view.h>
#include <math/dag_geomTree.h>
#include <math/dag_TMatrix.h>
#include <debug/dag_debug.h>
#include <debug/dag_debug3d.h>
#include <math/dag_capsule.h>
#include <render/dynmodelRenderer.h>

static int collisionSubtypeMask = -1;
static int physobjEntityClassId = -1;
static int rendEntGeomMask = -1;
static bool registeredNotifier = false;
static ISceneLightService *ltService;
static OAHashNameMap<true> physObjNames;

class PhysObjEntity;
typedef SingleEntityPool<PhysObjEntity> PhysObjEntityPool;

class VirtualPhysObjEntity : public IObjEntity, public IObjEntityUserDataHolder
{
public:
  VirtualPhysObjEntity(int cls) : IObjEntity(cls), pool(NULL), pd(NULL), nameId(-1), userDataBlk(NULL)
  {
    assetNameId = -1;
    bbox = BBox3(Point3(-0.5, -0.5, -0.5), Point3(0.5, 0.5, 0.5));
    bsph.setempty();
    bsph = bbox;
  }
  virtual ~VirtualPhysObjEntity()
  {
    clear();
    assetNameId = -1;
  }

  void clear()
  {
    for (int i = 0; i < scenes.size(); i++)
      delete scenes[i];
    clear_and_shrink(scenes);
    if (pd)
    {
      release_game_resource((GameResource *)(DynamicPhysObjectData *)pd);
      pd = NULL;
    }
    del_it(userDataBlk);
  }

  virtual void setTm(const TMatrix &_tm) {}
  virtual void getTm(TMatrix &_tm) const { _tm = TMatrix::IDENT; }
  virtual void destroy() { delete this; }
  virtual void *queryInterfacePtr(unsigned huid)
  {
    RETURN_INTERFACE(huid, IObjEntityUserDataHolder);
    return NULL;
  }

  virtual BSphere3 getBsph() const { return bsph; }
  virtual BBox3 getBbox() const { return bbox; }

  virtual const char *getObjAssetName() const
  {
    static String buf;
    buf.printf(0, "%s:physObj", physObjNames.getName(nameId));
    return buf;
  }

  void setup(const DagorAsset &asset)
  {
    const char *name = asset.getName();
    assetNameId = asset.getNameId();

    pd = (DynamicPhysObjectData *)::get_one_game_resource_ex(GAMERES_HANDLE_FROM_STRING(name), PhysObjGameResClassId);
    if (!pd)
    {
      DAEDITOR3.conError("cannot load physObj: %s", name);
      return;
    }

    nameId = physObjNames.addNameId(name);

    int cnt = 0;
    for (int i = 0; i < pd->models.size(); i++)
      if (pd->models[i])
        cnt++;

    if (!cnt)
    {
      release_game_resource((GameResource *)(DynamicPhysObjectData *)pd);
      pd = NULL;
      return;
    }

    clear_and_resize(scenes, cnt);
    cnt = 0;

    bbox.setempty();

    for (int i = 0; i < pd->models.size(); i++)
      if (pd->models[i])
      {
        scenes[cnt] = new DynamicRenderableSceneInstance(pd->models[i]);
        scenes[cnt]->setCurrentLod(pd->models[i]->lods.size() - 1);
        bbox += pd->models[i]->getLocalBoundingBox();
        cnt++;
      }

    bsph.setempty();
    bsph = bbox;
  }

  bool isNonVirtual() const { return pool; }
  int getAssetNameId() const { return assetNameId; }


  // IObjEntityUserDataHolder
  virtual DataBlock *getUserDataBlock(bool create_if_not_exist)
  {
    if (!userDataBlk && create_if_not_exist)
      userDataBlk = new DataBlock;
    return userDataBlk;
  }
  virtual void resetUserDataBlock() { del_it(userDataBlk); }

public:
  DataBlock *userDataBlk;
  PhysObjEntityPool *pool;
  DynamicPhysObjectData *pd;
  BBox3 bbox;
  BSphere3 bsph;
  int nameId;
  int assetNameId;
  SmallTab<DynamicRenderableSceneInstance *, MidmemAlloc> scenes;
};

class PhysObjEntity : public VirtualPhysObjEntity, public IRandomSeedHolder
{
public:
  PhysObjEntity(int cls) : VirtualPhysObjEntity(cls), idx(MAX_ENTITIES), nodeTree(NULL) { tm.identity(); }
  ~PhysObjEntity() { clear(); }

  virtual void *queryInterfacePtr(unsigned huid)
  {
    RETURN_INTERFACE(huid, IRandomSeedHolder);
    return NULL;
  }

  void clear()
  {
    del_it(nodeTree);
    VirtualPhysObjEntity::clear();
  }

  virtual void setTm(const TMatrix &_tm)
  {
    tm = _tm;
    if (!pd)
      return;

    if (scenes.size() == 1 && pd->physRes->getBodies().size() == 1 && scenes[0])
    {
      // single-node model;  in fact, during real simulation matrix must be:
      //  tm*physSys->getBody(0)->getTm()*pd->physRes->getBodies()[0].tmInvert);
      scenes[0]->setNodeWtm(0, tm);
      if (nodeTree)
      {
        nodeTree->setRootTmScalar(tm);
        nodeTree->invalidateWtm();
        nodeTree->calcWtm();
      }
      if (scenes[0]->getCurSceneResource() && scenes[0]->getCurSceneResource()->getNames().node.nameCount() == 1)
        return;
    }

    for (int i = 0; i < scenes.size(); i++)
    {
      DynamicRenderableSceneInstance *scene = scenes[i];
      if (!scene || !scene->getCurSceneResource())
        continue;

      scene->setDisableAutoChooseLod(true);
      iterate_names(scene->getCurSceneResource()->getNames().node, [&](int nodeId, const char *nodeName) {
        if (nodeId == 0)
          return;

        TMatrix tmHelperTm;
        auto n = nodeTree ? nodeTree->findINodeIndex(nodeName) : dag::Index16();

        if (pd->physRes->getTmHelperWtm(nodeName, tmHelperTm))
        {
          TMatrix wtm = tm * tmHelperTm;
          scene->setNodeWtm(nodeId, wtm);
          if (n)
          {
            nodeTree->setNodeWtmRelScalar(n, wtm);
            nodeTree->invalidateWtm(n);
            nodeTree->calcWtm();
          }
        }
        else if (n)
          scene->setNodeWtm(nodeId, nodeTree->getNodeWtmRel(n));
      });
    }
  }
  virtual void getTm(TMatrix &_tm) const { _tm = tm; }
  virtual void destroy()
  {
    pool->delEntity(this);
    clear();
  }

  void setup(const DagorAsset &asset)
  {
    VirtualPhysObjEntity::setup(asset);
    nodeTree = pd && pd->nodeTree ? new GeomNodeTree(*pd->nodeTree) : NULL;
  }

  void copyFrom(const PhysObjEntity &e)
  {
    if (!e.pd)
      return;

    pd = e.pd;
    bbox = e.bbox;
    bsph = e.bsph;

    nameId = e.nameId;
    game_resource_add_ref((GameResource *)(DynamicPhysObjectData *)pd);
    nodeTree = pd->nodeTree ? new GeomNodeTree(*pd->nodeTree) : NULL;

    clear_and_resize(scenes, e.scenes.size());
    for (int i = 0, cnt = 0; i < pd->models.size(); i++)
      if (pd->models[i])
      {
        scenes[cnt] = new DynamicRenderableSceneInstance(pd->models[i]);
        scenes[cnt]->setCurrentLod(pd->models[i]->lods.size() - 1);
        cnt++;
      }
  }

  void beforeRender()
  {
    for (int n = 0; n < scenes.size(); n++)
      scenes[n]->chooseLodByDistSq(lengthSq(grs_cur_view.pos - tm.getcol(3)));
  }
  void render()
  {
    dynrend::PerInstanceRenderData renderData;
    Point4 &params = renderData.params.emplace_back();
    memcpy(&params.x, &instanceSeed, sizeof(instanceSeed));

    for (int n = 0; n < scenes.size(); n++)
      if (!dynrend::render_in_tools(scenes[n], dynrend::RenderMode::Opaque, &renderData))
        scenes[n]->render();
  }
  void renderTrans()
  {
    for (int n = 0; n < scenes.size(); n++)
      if (!dynrend::render_in_tools(scenes[n], dynrend::RenderMode::Translucent))
        scenes[n]->renderTrans();
  }

  void renderCollision()
  {
    if (!pd || !pd->physRes)
      return;

    begin_draw_cached_debug_lines();
    set_cached_debug_lines_wtm(tm);

    const E3DCOLOR color(0, 255, 0);

    int cnt = pd->physRes->getBodies().size();
    for (int i = 0; i < cnt; i++)
    {
      const PhysicsResource::Body &body = pd->physRes->getBodies()[i];
      const TMatrix &t = body.tm;

      int scnt = body.sphColl.size();
      for (int j = 0; j < scnt; j++)
      {
        Point3 p = t * body.capColl[j].center;
        draw_cached_debug_sphere(p, body.capColl[j].radius, color);
      }

      int bcnt = body.boxColl.size();
      for (int j = 0; j < bcnt; j++)
      {
        TMatrix m = t * body.boxColl[j].tm;
        const Point3 &s = body.boxColl[j].size;
        Point3 ax = m.getcol(0) * s.x;
        Point3 ay = m.getcol(1) * s.y;
        Point3 az = m.getcol(2) * s.z;

        draw_cached_debug_box(m.getcol(3) - (ax + ay + az) / 2.f, ax, ay, az, color);
      }

      int ccnt = body.capColl.size();
      for (int j = 0; j < ccnt; j++)
      {
        const Point3 &p = body.capColl[j].center;
        const real &r = body.capColl[j].radius;
        const Point3 e = body.capColl[j].extent - r * normalize(body.capColl[j].extent);

        Capsule c;
        c.set(p - e, p + e, r);
        c.transform(t);

        draw_cached_debug_capsule_w(c, color);
      }
    }

    end_draw_cached_debug_lines();
  }

  // IRandomSeedHolder
  virtual void setSeed(int new_seed) {}
  virtual int getSeed() { return 0; }
  virtual void setPerInstanceSeed(int seed) { instanceSeed = seed; }
  virtual int getPerInstanceSeed() { return instanceSeed; }

public:
  enum
  {
    STEP = 512,
    MAX_ENTITIES = 0x7FFFFFFF
  };

  unsigned idx;
  TMatrix tm;
  GeomNodeTree *nodeTree;
  int instanceSeed = 0;
};


class PhysObjEntityManagementService : public IEditorService,
                                       public IObjEntityMgr,
                                       public ILightingChangeClient,
                                       public IBinaryDataBuilder,
                                       public ILevelResListBuilder,
                                       public IDagorAssetChangeNotify,
                                       public IRenderingService
{
public:
  PhysObjEntityManagementService()
  {
    physobjEntityClassId = IDaEditor3Engine::get().getAssetTypeId("physObj");
    collisionSubtypeMask = 1 << IDaEditor3Engine::get().registerEntitySubTypeId("collision");
    rendEntGeomMask = 1 << IDaEditor3Engine::get().registerEntitySubTypeId("rend_ent_geom");
    visible = true;
  }

  // IEditorService interface
  virtual const char *getServiceName() const { return "_poEntMgr"; }
  virtual const char *getServiceFriendlyName() const { return "(srv) Phys object entities"; }

  virtual void setServiceVisible(bool vis) { visible = vis; }
  virtual bool getServiceVisible() const { return visible; }

  virtual void actService(float dt) {}
  virtual void beforeRenderService() {}
  virtual void renderService()
  {
    int subtypeMask = IObjEntityFilter::getSubTypeMask(IObjEntityFilter::STMASK_TYPE_RENDER);
    uint64_t lh_mask = IObjEntityFilter::getLayerHiddenMask();
    if (subtypeMask & collisionSubtypeMask)
    {
      dag::ConstSpan<PhysObjEntity *> ent = objPool.getEntities();
      for (int i = 0; i < ent.size(); i++)
        if (ent[i] && ent[i]->pd && ent[i]->isNonVirtual() && ent[i]->checkSubtypeAndLayerHiddenMasks(subtypeMask, lh_mask))
          ent[i]->renderCollision();
    }
  }
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

  // IRenderingService interface
  virtual void renderGeometry(Stage stage)
  {
    int st_mask = IObjEntityFilter::getSubTypeMask(IObjEntityFilter::STMASK_TYPE_RENDER);
    uint64_t lh_mask = IObjEntityFilter::getLayerHiddenMask();
    if ((st_mask & rendEntGeomMask) != rendEntGeomMask)
      return;

    dag::ConstSpan<PhysObjEntity *> ent = objPool.getEntities();
    switch (stage)
    {
      case STG_BEFORE_RENDER:
        for (int i = 0; i < ent.size(); i++)
          if (ent[i] && ent[i]->pd && ent[i]->isNonVirtual() && ent[i]->checkSubtypeAndLayerHiddenMasks(st_mask, lh_mask))
            ent[i]->beforeRender();
        break;

      case STG_RENDER_DYNAMIC_OPAQUE:
        for (int i = 0; i < ent.size(); i++)
          if (ent[i] && ent[i]->pd && ent[i]->isNonVirtual() && ent[i]->checkSubtypeAndLayerHiddenMasks(st_mask, lh_mask))
            ent[i]->render();
        break;

      case STG_RENDER_DYNAMIC_TRANS:
        for (int i = 0; i < ent.size(); i++)
          if (ent[i] && ent[i]->pd && ent[i]->isNonVirtual() && ent[i]->checkSubtypeAndLayerHiddenMasks(st_mask, lh_mask))
            ent[i]->renderTrans();
        break;

      case STG_RENDER_SHADOWS:
        for (int i = 0; i < ent.size(); i++)
          if (ent[i] && ent[i]->pd && ent[i]->isNonVirtual() && ent[i]->checkSubtypeAndLayerHiddenMasks(st_mask, lh_mask))
          {
            ent[i]->render();
            // ent[i]->renderTrans();
          }
        break;
    }
  }

  // IObjEntityMgr interface
  virtual bool canSupportEntityClass(int entity_class) const
  {
    return physobjEntityClassId >= 0 && physobjEntityClassId == entity_class;
  }

  virtual IObjEntity *createEntity(const DagorAsset &asset, bool virtual_ent)
  {
    if (!registeredNotifier)
    {
      registeredNotifier = true;
      asset.getMgr().subscribeUpdateNotify(this, -1, physobjEntityClassId);
    }

    if (virtual_ent)
    {
      VirtualPhysObjEntity *ent = new VirtualPhysObjEntity(physobjEntityClassId);
      ent->setup(asset);
      return ent;
    }

    PhysObjEntity *ent = objPool.allocEntity();
    if (!ent)
      ent = new PhysObjEntity(physobjEntityClassId);

    ent->setup(asset);
    objPool.addEntity(ent);
    // debug("create ent: %p", ent);
    return ent;
  }

  virtual IObjEntity *cloneEntity(IObjEntity *origin)
  {
    PhysObjEntity *o = reinterpret_cast<PhysObjEntity *>(origin);
    PhysObjEntity *ent = objPool.allocEntity();
    if (!ent)
      ent = new PhysObjEntity(o->getAssetTypeId());

    ent->copyFrom(*o);
    objPool.addEntity(ent);
    // debug("clone ent: %p", ent);
    return ent;
  }

  // IDagorAssetChangeNotify interface
  virtual void onAssetRemoved(int asset_name_id, int asset_type)
  {
    dag::ConstSpan<PhysObjEntity *> ent = objPool.getEntities();
    for (int i = 0; i < ent.size(); i++)
      if (ent[i] && ent[i]->getAssetNameId() == asset_name_id)
        ent[i]->clear();
    EDITORCORE->invalidateViewportCache();
  }
  virtual void onAssetChanged(const DagorAsset &asset, int asset_name_id, int asset_type)
  {
    dag::ConstSpan<PhysObjEntity *> ent = objPool.getEntities();
    for (int i = 0; i < ent.size(); i++)
      if (ent[i] && ent[i]->getAssetNameId() == asset_name_id)
      {
        ent[i]->clear();
        ent[i]->setup(asset);
      }
    EDITORCORE->invalidateViewportCache();
  }

  // ILightingChangeClient
  virtual void onLightingChanged() {}
  void onLightingSettingsChanged() override {}

  // IBinaryDataBuilder interface
  virtual bool validateBuild(int target, ILogWriter &log, PropPanel::ContainerPropertyControl *params)
  {
    if (!objPool.calcEntities(IObjEntityFilter::getSubTypeMask(IObjEntityFilter::STMASK_TYPE_EXPORT)))
      log.addMessage(log.WARNING, "No physObj entities for export");
    return true;
  }

  virtual bool addUsedTextures(ITextureNumerator &tn) { return true; }

  virtual bool buildAndWrite(BinDumpSaveCB &cwr, const ITextureNumerator &tn, PropPanel::ContainerPropertyControl *)
  {
    dag::Vector<SrcObjsToPlace> objs;

    dag::ConstSpan<PhysObjEntity *> ent = objPool.getEntities();
    int st_mask = IObjEntityFilter::getSubTypeMask(IObjEntityFilter::STMASK_TYPE_EXPORT);
    for (int i = 0; i < ent.size(); i++)
      if (ent[i] && ent[i]->pd && ent[i]->isNonVirtual() && ent[i]->checkSubtypeMask(st_mask))
      {
        int k;
        for (k = 0; k < objs.size(); k++)
          if (ent[i]->nameId == objs[k].nameId)
            break;
        if (k == objs.size())
        {
          objs.push_back();
          objs[k].nameId = ent[i]->nameId;
          objs[k].resName = physObjNames.getName(ent[i]->nameId);
        }

        objs[k].tm.push_back(ent[i]->tm);
        if (ent[i]->userDataBlk)
          objs[k].addBlk.addNewBlock(ent[i]->userDataBlk, "");
        else
          objs[k].addBlk.addNewBlock("");
      }

    writeObjsToPlaceDump(cwr, make_span(objs), _MAKE4C('PhO'));
    return true;
  }

  virtual bool checkMetrics(const DataBlock &metrics_blk)
  {
    int subtype_mask = IObjEntityFilter::getSubTypeMask(IObjEntityFilter::STMASK_TYPE_EXPORT);
    int cnt = objPool.calcEntities(subtype_mask);
    int maxCount = metrics_blk.getInt("max_entities", 0);

    if (cnt > maxCount)
    {
      DAEDITOR3.conError("Metrics validation failed: PhysObj count=%d  > maximum allowed=%d.", cnt, maxCount);
      return false;
    }
    return true;
  }

  // ILevelResListBuilder
  virtual void addUsedResNames(OAHashNameMap<true> &res_list)
  {
    dag::ConstSpan<PhysObjEntity *> ent = objPool.getEntities();
    int st_mask = IObjEntityFilter::getSubTypeMask(IObjEntityFilter::STMASK_TYPE_EXPORT);
    SmallTab<bool, TmpmemAlloc> used_mark;
    clear_and_resize(used_mark, physObjNames.nameCount());
    mem_set_0(used_mark);

    for (int i = 0; i < ent.size(); i++)
      if (ent[i] && ent[i]->pd && ent[i]->isNonVirtual() && ent[i]->checkSubtypeMask(st_mask))
      {
        if (used_mark[ent[i]->nameId])
          continue;
        used_mark[ent[i]->nameId] = 1;
        res_list.addNameId(physObjNames.getName(ent[i]->nameId));
      }
  }

protected:
  PhysObjEntityPool objPool;
  bool visible;
};


void init_physobj_mgr_service()
{
  if (!IDaEditor3Engine::get().checkVersion())
  {
    DEBUG_CTX("Incorrect version!");
    return;
  }

  ::register_dynmodel_gameres_factory();
  ::register_geom_node_tree_gameres_factory();
  ::register_phys_sys_gameres_factory();
  ::register_phys_obj_gameres_factory();

  ltService = EDITORCORE->queryEditorInterface<ISceneLightService>();

  IDaEditor3Engine::get().registerService(new (inimem) PhysObjEntityManagementService);
}
