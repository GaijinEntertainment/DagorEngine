// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <de3_interface.h>
#include <de3_dataBlockIdHolder.h>
#include <de3_objEntity.h>
#include <de3_entityPool.h>
#include <de3_entityFilter.h>
#include <de3_lightService.h>
#include <de3_baseInterfaces.h>
#include <de3_lodController.h>
#include <de3_entityGatherTex.h>
#include <de3_entityGetSceneLodsRes.h>
#include <de3_randomSeed.h>
#include <oldEditor/de_common_interface.h>
#include <assets/assetChangeNotify.h>
#include <assets/assetMgr.h>
#include <EditorCore/ec_interface.h>
#include <shaders/dag_dynSceneRes.h>
#include <gameRes/dag_gameResSystem.h>
#include <gameRes/dag_stdGameRes.h>
#include <gameRes/dag_collisionResource.h>
#include <startup/dag_globalSettings.h>
#include <drv/3d/dag_driver.h>
#include <3d/dag_render.h>
#include <render/dag_cur_view.h>
#include <3d/dag_texIdSet.h>
#include <math/dag_geomTree.h>
#include <math/dag_TMatrix.h>
#include <osApiWrappers/dag_critSec.h>
#include <debug/dag_debug.h>
#include <debug/dag_debug3d.h>
#include <render/dynmodelRenderer.h>
#include <regExp/regExp.h>

extern bool(__stdcall *external_traceRay_for_rigen)(const Point3 &from, const Point3 &dir, float &current_t, Point3 *out_norm);
static unsigned decal3d_gather_gen = 0, current_dynmodel_gen = 1;
static int decal3dSubtype = -1;
static WinCritSec *ccForTrace = NULL;

static int dynModelEntityClassId = -1;
static int collisionSubtypeMask = -1;
static int decalSubtypeMask = -1;
static ISceneLightService *ltService;
static DagorAssetMgr *aMgr = NULL;

static bool registeredNotifier = false;

extern void ec_stat3d_on_unit_begin();
extern void ec_stat3d_on_unit_end();

static RegExp *check_no_dynrend_shclass_re = nullptr;


class DynModelEntity;

typedef SingleEntityPool<DynModelEntity> DynModelEntityPool;

class VirtualDynModelEntity : public IObjEntity, public IEntityGetDynSceneLodsRes
{
public:
  VirtualDynModelEntity(int cls) : IObjEntity(cls), pool(NULL), res(NULL), sceneInstance(NULL), geomNodeTree(NULL), origSkeleton(NULL)
  {
    bbox = BBox3(Point3(-0.5, -0.5, -0.5), Point3(0.5, 0.5, 0.5));
    bsph = BSphere3(Point3(0, 0, 0), 0.5);
    assetNameId = -1;
    skeletonErrorsReported = false;
  }

  virtual ~VirtualDynModelEntity()
  {
    clear();
    assetNameId = -1;
  }

  void clear()
  {
    noDynRender = false;
    del_it(sceneInstance);

    if (res)
    {
      release_game_resource((GameResource *)res);
      res = NULL;
    }

    geomNodeTree = NULL;
    if (origSkeleton)
    {
      release_game_resource((GameResource *)origSkeleton);
      origSkeleton = NULL;
    }
  }

  virtual void setTm(const TMatrix &_tm)
  {
    if (!sceneInstance)
      return;

    WinAutoLock lock(*ccForTrace);
    if (geomNodeTree)
    {
      geomNodeTree->setRootTmScalar(_tm);
      geomNodeTree->invalidateWtm();
      geomNodeTree->calcWtm();

      bool errors = false;
      iterate_names(res->getNames().node, [&](int id, const char *nodeName) {
        if (auto node = geomNodeTree->findNodeIndex(refSkelNodePrefix + nodeName))
          sceneInstance->setNodeWtm(id, geomNodeTree->getNodeWtmRel(node));
        else
        {
          if (stricmp(nodeName, "@root") != 0)
          {
            if (!skeletonErrorsReported)
              DAEDITOR3.conError("dynmodel <%s>: can't find node '%s%s'", assetName(), refSkelNodePrefix, nodeName);
            errors = true;
          }
          else
            sceneInstance->setNodeWtm(id, geomNodeTree->getRootWtmRel());
        }
      });
      if (errors)
        skeletonErrorsReported = true;
    }
    else
      for (int i = sceneInstance->getNodeCount() - 1; i >= 0; i--)
        sceneInstance->setNodeWtm(i, _tm);
  }

  virtual void getTm(TMatrix &_tm) const { _tm = TMatrix::IDENT; }
  virtual void destroy() { delete this; }
  virtual void *queryInterfacePtr(unsigned huid) { return NULL; }

  virtual BSphere3 getBsph() const { return bsph; }
  virtual BBox3 getBbox() const { return bbox; }

  virtual const char *getObjAssetName() const
  {
    static String buf;
    buf.printf(0, "%s:dynModel", assetName());
    return buf;
  }

  virtual DynamicRenderableSceneLodsResource *getSceneLodsRes() { return res; }
  virtual const DynamicRenderableSceneLodsResource *getSceneLodsRes() const { return res; }
  virtual DynamicRenderableSceneInstance *getSceneInstance() override { return sceneInstance; }

  void setup(const DagorAsset &asset)
  {
    const char *name = asset.getName();
    assetNameId = asset.getNameId();

    DAEDITOR3.setFatalHandler(false);
    FastNameMap resNameMap;
    try
    {
      resNameMap.addNameId(name);
      ::set_required_res_list_restriction(resNameMap);
      res = (DynamicRenderableSceneLodsResource *)::get_game_resource_ex(GAMERES_HANDLE_FROM_STRING(name), DynModelGameResClassId);
      ::reset_required_res_list_restriction();
    }
    catch (...)
    {
      logerr("exception while building asset: %s", name);
      res = NULL;
    }
    DAEDITOR3.popFatalHandler();

    if (!res)
    {
      DAEDITOR3.conError("cannot load dynModel: %s", name);
      debug_flush(false);
      sceneInstance = NULL;
      geomNodeTree = NULL;
      origSkeleton = NULL;
      return;
    }

    sceneInstance = new DynamicRenderableSceneInstance(res);

    const char *skeletonName = asset.props.getStr("ref_skeleton", NULL);
    refSkelNodePrefix = asset.props.getStr("ref_skeleton_node_prefix", "");
    if (skeletonName)
    {
      resNameMap.addNameId(skeletonName);
      ::set_required_res_list_restriction(resNameMap);
      geomNodeTree = (GeomNodeTree *)get_game_resource_ex(GAMERES_HANDLE_FROM_STRING(skeletonName), GeomNodeTreeGameResClassId);
      ::reset_required_res_list_restriction();
      if (!geomNodeTree)
        DAEDITOR3.conError("can't find resource GeomNodeTree '%s'", skeletonName);
    }
    else
      geomNodeTree = NULL;
    origSkeleton = geomNodeTree;

    // calculating bbox and bsphere
    setTm(TMatrix::IDENT);
    bbox = sceneInstance->getBoundingBox();
    bsph = sceneInstance->getBoundingSphere();

    if (check_no_dynrend_shclass_re && !res->lods.empty() && res->lods.front().scene)
      for (const auto &rigid : res->lods[0].scene->getRigidsConst())
        if (rigid.mesh && rigid.mesh->getMesh() && !rigid.mesh->getMesh()->getAllElems().empty() &&
            rigid.mesh->getMesh()->getAllElems().front().mat &&
            check_no_dynrend_shclass_re->test(rigid.mesh->getMesh()->getAllElems().front().mat->getShaderClassName()))
        {
          debug("dynModel(%s): shader(%s) -> noDynRender", name,
            rigid.mesh->getMesh()->getAllElems().front().mat->getShaderClassName());
          noDynRender = true;
          break;
        }
  }

  bool isNonVirtual() const { return pool; }
  int getAssetNameId() const { return assetNameId; }
  inline const char *assetName() const { return aMgr ? aMgr->getAssetName(assetNameId) : NULL; }

public:
  DynModelEntityPool *pool;
  DynamicRenderableSceneLodsResource *res;
  DynamicRenderableSceneInstance *sceneInstance;
  GeomNodeTree *geomNodeTree;
  GeomNodeTree *origSkeleton;
  String refSkelNodePrefix;
  int assetNameId;
  BBox3 bbox;
  BSphere3 bsph;
  bool skeletonErrorsReported;
  bool noDynRender = false;
};


class DynModelEntity : public VirtualDynModelEntity,
                       public ILodController,
                       public IEntityGatherTex,
                       public IDataBlockIdHolder,
                       public IRandomSeedHolder
{
public:
  DynModelEntity(int cls) : VirtualDynModelEntity(cls), idx(MAX_ENTITIES)
  {
    tm.identity();
    texQ = 1;
    collision = NULL;
    collisionTried = false;
  }

  virtual void *queryInterfacePtr(unsigned huid)
  {
    RETURN_INTERFACE(huid, ILodController);
    RETURN_INTERFACE(huid, IEntityGatherTex);
    RETURN_INTERFACE(huid, IEntityGetDynSceneLodsRes);
    RETURN_INTERFACE(huid, IDataBlockIdHolder);
    RETURN_INTERFACE(huid, IRandomSeedHolder);
    return NULL;
  }

  virtual void setTm(const TMatrix &_tm)
  {
    VirtualDynModelEntity::setTm(_tm);
    tm = _tm;

    current_dynmodel_gen++;
  }

  virtual void getTm(TMatrix &_tm) const { _tm = tm; }
  virtual void destroy()
  {
    WinAutoLock lock(*ccForTrace);
    pool->delEntity(this);
    clear();
    current_dynmodel_gen++;
  }

  // IDataBlockIdHolder
  virtual void setDataBlockId(unsigned id) override { dataBlockId = id; }
  virtual unsigned getDataBlockId() override { return dataBlockId; }

  void copyFrom(const VirtualDynModelEntity &e)
  {
    if (!e.res)
      return;

    res = e.res;
    assetNameId = e.assetNameId;
    skeletonErrorsReported = e.skeletonErrorsReported;
    bbox = e.bbox;
    bsph = e.bsph;

    game_resource_add_ref((GameResource *)res);
    game_resource_add_ref((GameResource *)e.origSkeleton);
    origSkeleton = geomNodeTree;

    sceneInstance = new DynamicRenderableSceneInstance(res);
    origSkeleton = geomNodeTree = e.origSkeleton;
  }

  bool prepareCollRes()
  {
    if (collision)
      return true;
    if (collisionTried)
      return false;

    FastNameMap resNameMap;
    String resName(0, "%s_collision", assetName());
    try
    {
      resNameMap.addNameId(resName);
      ::set_required_res_list_restriction(resNameMap);
      collision = (CollisionResource *)::get_game_resource_ex(GAMERES_HANDLE_FROM_STRING(resName), CollisionGameResClassId);
      ::reset_required_res_list_restriction();
    }
    catch (...)
    {
      collision = NULL;
    }

    collisionTried = true;
    if (!collision)
      return false;
    collision->collapseAndOptimize(true);
    return true;
  }
  void beforeRender() { sceneInstance->beforeRender(::grs_cur_view.pos); }

  void render()
  {
    dynrend::PerInstanceRenderData renderData;
    Point4 &params = renderData.params.emplace_back();
    memcpy(&params.x, &instanceSeed, sizeof(instanceSeed));

    if (getSubtype() == decal3dSubtype || noDynRender ||
        !dynrend::render_in_tools(sceneInstance, dynrend::RenderMode::Opaque, &renderData))
      sceneInstance->render();
  }

  void renderTrans()
  {
    if (getSubtype() == decal3dSubtype || noDynRender || !dynrend::render_in_tools(sceneInstance, dynrend::RenderMode::Translucent))
      sceneInstance->renderTrans();
  }

  void renderDistortion()
  {
    if (getSubtype() == decal3dSubtype || noDynRender || !dynrend::render_in_tools(sceneInstance, dynrend::RenderMode::Distortion))
      sceneInstance->renderDistortion();
  }

  // ILodController
  virtual int getLodCount() { return sceneInstance ? sceneInstance->getLodsCount() : 0; }
  virtual void setCurLod(int n) { sceneInstance ? sceneInstance->setLod(n) : (void)0; }
  virtual real getLodRange(int lod_n) { return sceneInstance->getLodsResource()->lods[lod_n].range; }

  virtual int getTexQLCount() const { return 2; }
  virtual void setTexQL(int ql)
  {
    if (ql == texQ)
      return;
    texQ = ql;
    del_it(sceneInstance);
    sceneInstance = new DynamicRenderableSceneInstance(res);
    VirtualDynModelEntity::setTm(tm);
  }
  virtual int getTexQL() const { return texQ; }

  virtual int getNamedNodeCount() { return sceneInstance ? sceneInstance->getNodeCount() : 0; }
  virtual const char *getNamedNode(int idx)
  {
    if (!res)
      return NULL;
    const char *nm = res->getNames().node.getNameSlow(idx);
    for (int i = 0; i < res->lods.size(); i++)
    {
      if (res->lods[i].scene->hasRigidMesh(idx))
        return nm;
      if (find_value_idx(res->lods[i].scene->getSkinNodes(), idx) >= 0)
        return nm;
    }
    return NULL;
  }
  virtual int getNamedNodeIdx(const char *nm) { return sceneInstance ? sceneInstance->getNodeId(nm) : -1; }
  virtual bool getNamedNodeVisibility(int idx) { return sceneInstance ? !sceneInstance->isNodeHidden(idx) : true; }
  virtual void setNamedNodeVisibility(int idx, bool vis)
  {
    if (sceneInstance)
      sceneInstance->showNode(idx, vis);
  }

  virtual const char *getNodeName(int idx)
  {
    if (!res)
      return NULL;
    const char *nm = res->getNames().node.getNameSlow(idx);
    return nm;
  }

  virtual const GeomNodeTree *getSkeleton() override { return origSkeleton; }
  virtual void setSkeletonForRender(GeomNodeTree *skel) override { geomNodeTree = skel ? skel : origSkeleton; }

  // IEntityGatherTex interface
  virtual int gatherTextures(TextureIdSet &out_tex_id, int for_lod)
  {
    if (!sceneInstance)
      return 0;
    int st_cnt = out_tex_id.size();
    if (for_lod < 0)
      sceneInstance->getLodsResource()->gatherUsedTex(out_tex_id);
    else if (for_lod < sceneInstance->getLodsResource()->lods.size())
      sceneInstance->getLodsResource()->lods[for_lod].scene->gatherUsedTex(out_tex_id);
    return out_tex_id.size() - st_cnt;
  }

  // IRandomSeedHolder
  virtual void setSeed(int new_seed) {}
  virtual int getSeed() { return 0; }
  virtual void setPerInstanceSeed(int seed) { instanceSeed = seed; }
  virtual int getPerInstanceSeed() { return instanceSeed; }

public:
  static const int STEP = 512;
  static const int MAX_ENTITIES = 0xFFFFFFFF;

  unsigned idx;
  TMatrix tm;
  int texQ;
  CollisionResource *collision;
  bool collisionTried;
  int instanceSeed = 0;
  unsigned short dataBlockId = IDataBlockIdHolder::invalid_id;
};


class DynModelEntityManagementService : public IEditorService,
                                        public IObjEntityMgr,
                                        public IRenderingService,
                                        public IDagorAssetChangeNotify,
                                        public ILightingChangeClient
{
public:
  DynModelEntityManagementService()
  {
    aMgr = NULL;
    self = this;
    dynModelEntityClassId = IDaEditor3Engine::get().getAssetTypeId("dynModel");
    collisionSubtypeMask = 1 << IDaEditor3Engine::get().registerEntitySubTypeId("collision");
    decal3dSubtype = IDaEditor3Engine::get().registerEntitySubTypeId("rend_decal3d_geom");
    decalSubtypeMask = (1 << IDaEditor3Engine::get().registerEntitySubTypeId("rend_decal_geom")) |
                       (1 << IDaEditor3Engine::get().registerEntitySubTypeId("rend_decal3d_geom"));
    visible = true;

    camoMicroNoiseTexId = BAD_TEXTUREID;
    initCamoMicroNoise();

    ccForTrace = new WinCritSec;
    prev_traceRay_for_rigen = external_traceRay_for_rigen;
    external_traceRay_for_rigen = &traceRay_for_rigen;
  }

  ~DynModelEntityManagementService()
  {
    release_managed_tex(camoMicroNoiseTexId);
    external_traceRay_for_rigen = (external_traceRay_for_rigen == &traceRay_for_rigen) ? prev_traceRay_for_rigen : NULL;
    del_it(ccForTrace);
    self = NULL;
  }

  // IEditorService interface
  virtual const char *getServiceName() const { return "_dmEntMgr"; }
  virtual const char *getServiceFriendlyName() const { return "(srv) DynModel entities"; }

  virtual void setServiceVisible(bool vis) { visible = vis; }
  virtual bool getServiceVisible() const { return visible; }

  virtual void actService(float dt) {}
  virtual void beforeRenderService() {}
  virtual void renderService()
  {
    dag::ConstSpan<DynModelEntity *> ent = objPool.getEntities();
    int st_mask = IObjEntityFilter::getSubTypeMask(IObjEntityFilter::STMASK_TYPE_RENDER);
    uint64_t lh_mask = IObjEntityFilter::getLayerHiddenMask();
    if (st_mask & collisionSubtypeMask)
    {
      begin_draw_cached_debug_lines();

      const E3DCOLOR col(0, 255, 0);
      for (int i = 0; i < ent.size(); i++)
        if (ent[i] && ent[i]->res && ent[i]->isNonVirtual() && ent[i]->checkSubtypeAndLayerHiddenMasks(st_mask, lh_mask))
        {
          const TMatrix &tm = ent[i]->tm;
          const BBox3 &bb = ent[i]->bbox;
          set_cached_debug_lines_wtm(tm);
          draw_cached_debug_box(bb, col);
        }

      end_draw_cached_debug_lines();
    }
  }
  virtual void renderTransService() {}

  virtual void onBeforeReset3dDevice() {}

  virtual void *queryInterfacePtr(unsigned huid)
  {
    RETURN_INTERFACE(huid, IObjEntityMgr);
    RETURN_INTERFACE(huid, IRenderingService);
    RETURN_INTERFACE(huid, ILightingChangeClient);
    return NULL;
  }

  // ILightingChangeClient
  virtual void onLightingChanged() {}
  void onLightingSettingsChanged() override {}

  // IRenderingService interface
  virtual void renderGeometry(Stage stage)
  {
    static int camoMicroNoiseTexVarId = get_shader_variable_id("camo_micro_noise_tex", true);
    ShaderGlobal::set_texture(camoMicroNoiseTexVarId, camoMicroNoiseTexId);

    static int editor_dynmodel_render_gvid = ::get_shader_glob_var_id("editor_dynmodel_render", true);
    static int dynmodelObjEntInFlight = 0;
    bool render_now = (stage == STG_RENDER_DYNAMIC_OPAQUE || stage == STG_RENDER_TO_CLIPMAP || stage == STG_RENDER_DYNAMIC_TRANS);
    int st_mask = IObjEntityFilter::getSubTypeMask(IObjEntityFilter::STMASK_TYPE_RENDER);
    uint64_t lh_mask = IObjEntityFilter::getLayerHiddenMask();
    if (stage == STG_RENDER_TO_CLIPMAP && !(st_mask & decalSubtypeMask))
      return;
    if (render_now && dynmodelObjEntInFlight)
      ShaderGlobal::set_int(editor_dynmodel_render_gvid, 1);

    dag::ConstSpan<DynModelEntity *> ent = objPool.getEntities();
    switch (stage)
    {
      case STG_BEFORE_RENDER:
        dynmodelObjEntInFlight = 0;
        for (int i = 0; i < ent.size(); i++)
          if (ent[i] && ent[i]->res && ent[i]->isNonVirtual() && ent[i]->checkSubtypeAndLayerHiddenMasks(st_mask, lh_mask))
          {
            ent[i]->beforeRender();
            dynmodelObjEntInFlight++;
          }
        break;

      case STG_RENDER_DYNAMIC_OPAQUE:
      case STG_RENDER_TO_CLIPMAP:
      case STG_RENDER_SHADOWS:
        for (int i = 0; i < ent.size(); i++)
          if (ent[i] && ent[i]->res && ent[i]->isNonVirtual() && ent[i]->checkSubtypeAndLayerHiddenMasks(st_mask, lh_mask))
          {
            if (stage == STG_RENDER_DYNAMIC_OPAQUE)
              ec_stat3d_on_unit_begin();
            ent[i]->render();
            if (stage == STG_RENDER_DYNAMIC_OPAQUE)
              ec_stat3d_on_unit_end();
          }

        break;

      case STG_RENDER_DYNAMIC_TRANS:
        for (int i = 0; i < ent.size(); i++)
          if (ent[i] && ent[i]->res && ent[i]->isNonVirtual() && ent[i]->checkSubtypeAndLayerHiddenMasks(st_mask, lh_mask))
            ent[i]->renderTrans();
        break;

      case STG_RENDER_DYNAMIC_DISTORTION:
        for (int i = 0; i < ent.size(); i++)
          if (ent[i] && ent[i]->res && ent[i]->isNonVirtual() && ent[i]->checkSubtypeAndLayerHiddenMasks(st_mask, lh_mask))
            ent[i]->renderDistortion();
        break;
    }
    if (render_now && dynmodelObjEntInFlight)
      ShaderGlobal::set_int(editor_dynmodel_render_gvid, 0);
  }


  virtual void initCamoMicroNoise()
  {
    const DataBlock *options_blk = const_cast<DataBlock *>(dgs_get_game_params())->getBlockByNameEx("camoMicroNoise");
    const char *camoMicroNoiseTexName = options_blk->getStr("tex", NULL);
    if (camoMicroNoiseTexName)
    {
      camoMicroNoiseTexId = ::get_tex_gameres(camoMicroNoiseTexName);
      G_ASSERTF(camoMicroNoiseTexId != BAD_TEXTUREID, "'%s' not found", camoMicroNoiseTexName);
    }

    float shipCamoMicroNoiseTile = options_blk->getReal("tile_ship", 2.f);
    float shipCamoMicroNoiseAmplitude = options_blk->getReal("amplitude_ship", 0.25f);
    float tankCamoMicroNoiseTile = options_blk->getReal("tile_tank", 7.f);
    float tankpCamoMicroNoiseAmplitude = options_blk->getReal("amplitude_tank", 0.15f);
    static int camoMicroNoiseParametersVarId = get_shader_variable_id("camo_micro_noise_parameters", true);
    ShaderGlobal::set_color4(camoMicroNoiseParametersVarId, shipCamoMicroNoiseTile, shipCamoMicroNoiseAmplitude,
      tankCamoMicroNoiseTile, tankpCamoMicroNoiseAmplitude);
  }

  // IObjEntityMgr interface
  virtual bool canSupportEntityClass(int entity_class) const
  {
    return dynModelEntityClassId >= 0 && dynModelEntityClassId == entity_class;
  }

  virtual IObjEntity *createEntity(const DagorAsset &asset, bool virtual_ent)
  {
    if (!aMgr)
      aMgr = &asset.getMgr();
    G_ASSERT(aMgr == &asset.getMgr());

    WinAutoLock lock(*ccForTrace);
    if (!registeredNotifier)
    {
      registeredNotifier = true;
      asset.getMgr().subscribeUpdateNotify(this, -1, dynModelEntityClassId);
    }

    if (virtual_ent)
    {
      VirtualDynModelEntity *ent = new VirtualDynModelEntity(dynModelEntityClassId);
      ent->setup(asset);
      return ent;
    }

    DynModelEntity *ent = objPool.allocEntity();
    if (!ent)
      ent = new DynModelEntity(dynModelEntityClassId);

    ent->setup(asset);
    objPool.addEntity(ent);
    // debug("create ent: %p", ent);
    current_dynmodel_gen++;
    return ent;
  }

  virtual IObjEntity *cloneEntity(IObjEntity *origin)
  {
    WinAutoLock lock(*ccForTrace);
    VirtualDynModelEntity *o = reinterpret_cast<VirtualDynModelEntity *>(origin);
    DynModelEntity *ent = objPool.allocEntity();
    if (!ent)
      ent = new DynModelEntity(o->getAssetTypeId());

    ent->copyFrom(*o);
    objPool.addEntity(ent);
    // debug("clone ent: %p", ent);
    current_dynmodel_gen++;
    return ent;
  }

  // IDagorAssetChangeNotify interface
  virtual void onAssetRemoved(int asset_name_id, int asset_type)
  {
    dag::ConstSpan<DynModelEntity *> ent = objPool.getEntities();
    for (int i = 0; i < ent.size(); i++)
      if (ent[i] && ent[i]->getAssetNameId() == asset_name_id)
        ent[i]->clear();
    current_dynmodel_gen++;
    EDITORCORE->invalidateViewportCache();
  }
  virtual void onAssetChanged(const DagorAsset &asset, int asset_name_id, int asset_type)
  {
    dag::ConstSpan<DynModelEntity *> ent = objPool.getEntities();
    for (int i = 0; i < ent.size(); i++)
      if (ent[i] && ent[i]->getAssetNameId() == asset_name_id)
      {
        ent[i]->clear();
        ent[i]->setup(asset);
      }
    current_dynmodel_gen++;
    EDITORCORE->invalidateViewportCache();
  }

  static bool __stdcall traceRay_for_rigen(const Point3 &from, const Point3 &dir, float &current_t, Point3 *out_norm)
  {
    WinAutoLock lock(*ccForTrace);
    if (decal3d_gather_gen != current_dynmodel_gen)
      gather_decal3d();

    bool hit = false;
    for (int i = 0; i < tmpDecalsList.size(); i++)
      if (tmpDecalsList[i]->collision->traceRay(tmpDecalsList[i]->tm, from, dir, current_t, out_norm))
        hit = true;
    if (prev_traceRay_for_rigen && prev_traceRay_for_rigen(from, dir, current_t, out_norm))
      hit = true;
    return hit;
  }
  static void gather_decal3d()
  {
    tmpDecalsList.clear();
    dag::ConstSpan<DynModelEntity *> ent = self->objPool.getEntities();
    for (int i = 0; i < ent.size(); i++)
      if (ent[i] && ent[i]->res && ent[i]->isNonVirtual() && ent[i]->getSubtype() == decal3dSubtype)
        if (ent[i]->prepareCollRes())
          tmpDecalsList.push_back(ent[i]);
    decal3d_gather_gen = current_dynmodel_gen;
  }

protected:
  DynModelEntityPool objPool;
  bool visible;

  TEXTUREID camoMicroNoiseTexId;

  static DynModelEntityManagementService *self;
  static Tab<DynModelEntity *> tmpDecalsList;
  static bool(__stdcall *prev_traceRay_for_rigen)(const Point3 &from, const Point3 &dir, float &current_t, Point3 *out_norm);
};

DynModelEntityManagementService *DynModelEntityManagementService::self = NULL;
Tab<DynModelEntity *> DynModelEntityManagementService::tmpDecalsList;
bool(__stdcall *DynModelEntityManagementService::prev_traceRay_for_rigen)(const Point3 &from, const Point3 &dir, float &current_t,
  Point3 *out_norm) = NULL;

void init_dynmodel_mgr_service(const DataBlock &app_blk)
{
  if (!IDaEditor3Engine::get().checkVersion())
  {
    DEBUG_CTX("Incorrect version!");
    return;
  }

  ::register_dynmodel_gameres_factory();
  ::register_geom_node_tree_gameres_factory();

  if (const char *re_src =
        app_blk.getBlockByNameEx("projectDefaults")->getBlockByNameEx("dynModelMgr")->getStr("noDynRendShaderRE", NULL))
  {
    check_no_dynrend_shclass_re = new RegExp;
    if (!check_no_dynrend_shclass_re->compile(re_src, ""))
    {
      logerr("failed to compile %s{%s{%s:t=%s", "projectDefaults", "dynModelMgr", "noDynRendShaderRE", re_src);
      del_it(check_no_dynrend_shclass_re);
    }
  }

  ltService = EDITORCORE->queryEditorInterface<ISceneLightService>();

  IDaEditor3Engine::get().registerService(new (inimem) DynModelEntityManagementService);
}
