#include <de3_splineGenSrv.h>
#include <de3_entityPool.h>
#include <de3_randomSeed.h>
#include <de3_baseInterfaces.h>
#include <de3_editorEvents.h>
#include <de3_genObjAlongSpline.h>
#include <de3_assetService.h>
#include <de3_landClassData.h>
#include <de3_lightService.h>
#include <de3_entityFilter.h>
#include <EditorCore/ec_interface.h>
#include <libTools/staticGeom/geomObject.h>
#include <libTools/staticGeom/staticGeometryContainer.h>
#include <libTools/ObjCreator3d/objCreator3d.h>
#include <assets/assetChangeNotify.h>
#include <assets/assetMgr.h>
#include <util/dag_fastIntList.h>
#include <util/dag_string.h>
#include <perfMon/dag_cpuFreq.h>
#include <startup/dag_globalSettings.h>
#include <de3_bitMaskMgr.h>

static int splineGenEntityClassId = -1;
static int rendEntGeomMask = -1;
static int splineSubtypeMask = -1;

class SplineGenEntity;
typedef SingleEntityPool<SplineGenEntity> SplineGenEntityPool;
static OAHashNameMap<true> splNames;

class SplineGenEntity : public IObjEntity, public ISplineGenObj, public IRandomSeedHolder
{
public:
  SplineGenEntity(int cls) : IObjEntity(cls)
  {
    rndSeed = 123;
    tm.identity();
  }

  void setTm(const TMatrix &_tm) override { tm = _tm; }
  void getTm(TMatrix &_tm) const override { _tm = tm; }

  void setSubtype(int t) override { subType = t; }
  void setEditLayerIdx(int t) override
  {
    editLayerIdx = t;
    for (auto &p : entPools)
      for (auto *e : p.entPool)
        if (e)
          e->setEditLayerIdx(t);
  }

  void destroy() override
  {
    pool->delEntity(this);
    clear();
    instSeed = 0;
    rndSeed = 123;
    tm.identity();
    if (IAssetService *srv = EDITORCORE->queryEditorInterface<IAssetService>())
      srv->releaseSplineClassData(splineClass);
    splineClass = nullptr;
  }

  void *queryInterfacePtr(unsigned huid) override
  {
    RETURN_INTERFACE(huid, ISplineGenObj);
    RETURN_INTERFACE(huid, IRandomSeedHolder);
    return NULL;
  }

  BBox3 getBbox() const override { return BBox3(); }
  BSphere3 getBsph() const override { return BSphere3(); }

  const char *getObjAssetName() const override { return splNames.getName(nameId); }

  void setup(const DagorAsset &asset)
  {
    nameId = splNames.addNameId(asset.getName());
    splineClass = nullptr;
    if (IAssetService *srv = EDITORCORE->queryEditorInterface<IAssetService>())
      splineClass = srv->getSplineClassData(asset.getName());
  }
  void copyFrom(const SplineGenEntity &e)
  {
    nameId = e.nameId;
    splineClass = nullptr;
    if (IAssetService *srv = EDITORCORE->queryEditorInterface<IAssetService>())
      splineClass = srv->addRefSplineClassData(e.splineClass);
    clear_and_shrink(entPools);
    loftGeom = nullptr;
    clear_and_shrink(loftSeg);
  }

  bool isNonVirtual() const { return pool; }
  bool mayHaveLayer(int layer) const { return layer == 0 || hasNonZeroLayers; }

  // IRandomSeedHolder interface
  void setSeed(int new_seed) override { rndSeed = new_seed; }
  int getSeed() override { return rndSeed; }
  void setPerInstanceSeed(int seed) override { instSeed = seed; }
  int getPerInstanceSeed() override { return instSeed; }

  // ISplineGenObj interface
  void clearObject() override { clear_and_shrink(entPools); }
  void clearLoftGeom() override { removeLoftGeom(); }
  void clear() override
  {
    clearObject();
    clearLoftGeom();
  }

  void transform(TMatrix &tm) override
  {
    TMatrix m;
    for (auto &p : entPools)
      for (auto *e : p.entPool)
        if (e)
        {
          e->getTm(m);
          e->setTm(tm * m);
        }

    /* too heavy
    if (loftGeom)
    {
      for (StaticGeometryNode *node : loftGeom->getGeometryContainer()->nodes)
        if (node)
          node->wtm = tm*node->wtm;
      loftGeom->notChangeVertexColors(true);
      loftGeom->recompile();
      loftGeom->notChangeVertexColors(false);
      updateLoftBox();
    }
    */
  }

  void generateObjects(BezierSpline3d &effSpline, int start_seg, int end_seg, int splineSubTypeId, int editLayerIdx, int rndSeed,
    int perInstSeed, Tab<cable_handle_t> *cablesPool) override;
  void generateLoftSegments(BezierSpline3d &effSpline, const char *name, int start_idx, int end_idx, bool place_on_collision,
    dag::ConstSpan<splineclass::Attr> splineScales, float scaleTcAlong) override;
  void gatherLoftLandPts(Tab<Point3> &loft_pt_cloud, BezierSpline3d &effSpline, BezierSpline2d &splineXZ, const char *name,
    int start_idx, int end_idx, bool place_on_collision, dag::ConstSpan<splineclass::Attr> splineScales,
    Tab<Point3> &water_border_polys, Tab<Point2> &hmap_sweep_polys, bool water_surf, float water_level) override;

  static void build_corner_spline(BezierSplinePrec3d &loftSpl, BezierSplinePrec3d &baseSpl, dag::ConstSpan<splineclass::SegData> seg,
    int ss, int es);
  static void build_ground_spline(BezierSpline3d &gndSpl, dag::ConstSpan<ISplineGenObj::SplinePt> points, const TMatrix &tm,
    int corner_type, bool poly, bool poly_smooth, bool is_closed);

public:
  enum
  {
    STEP = 512,
    MAX_ENTITIES = 0x7FFFFFFF
  };
  TMatrix tm;
  int rndSeed;
  int instSeed = 0;
  SplineGenEntityPool *pool = nullptr;
  unsigned idx = 0xffffffffu;
  int nameId = -1;
  bool hasNonZeroLayers = false;

  static int loftChangesCount;
  static FastIntList cachedLoftLayers;

  void resetUsedPoolsEntities()
  {
    for (auto &p : entPools)
      p.resetUsedEntities();
  }
  void deleteUnusedPoolsEntities()
  {
    for (auto &p : entPools)
      p.deleteUnusedEntities();
  }

  GeomObject &getClearedLoftGeom()
  {
    loftGeomBox.setempty();
    loftSeg.clear();
    if (!loftGeom)
      return *(loftGeom = new (midmem) GeomObject);
    loftGeom->clear();
    return *loftGeom;
  }

  void removeLoftGeom()
  {
    loftSeg.clear();
    del_it(loftGeom);
    updateLoftBox();
    if (hasNonZeroLayers)
      loftChangesCount++;
    hasNonZeroLayers = false;
  }

  void updateLoftBox()
  {
    if (loftGeom)
      loftGeomBox = loftGeom->getBoundBox();
    else
      loftGeomBox.setempty();
  }
};
int SplineGenEntity::loftChangesCount = 0;
FastIntList SplineGenEntity::cachedLoftLayers;

#include "genObjs.cpp"
#include "genLoft.cpp"
#include "spline.cpp"

static class TraceableSplineEntityCache
{
  int cachedCount;
  int cachedLayer;
  void *cachedPtr;

public:
  Tab<SplineGenEntity *> cache;
  bool isValid;

  TraceableSplineEntityCache() { reset(); }

  void reset()
  {
    isValid = false;
    cache.clear();
    cachedCount = INT_MAX;
    cachedLayer = INT_MAX;
    cachedPtr = nullptr;
  }

  bool build(dag::ConstSpan<SplineGenEntity *> entities, int layer)
  {
    if (entities.size() < 50)
      return false;

    bool rebuild = !isValid || entities.size() != cachedCount || (void *)entities.data() != cachedPtr || cachedLayer != layer;
    if (!rebuild)
      return true;

    cache.clear();

    for (SplineGenEntity *e : entities)
      if (e && (layer < 0 || e->splineLayer == layer) && e->loftGeom)
      {
        const splineclass::AssetData *a = e->splineClass;
        if (!a || !a->genGeom || !a->genGeom->foundationGeom)
          continue;

        if (!e->loftGeom || !e->loftGeom->hasCollision())
          continue;

        cache.push_back(e);
      }

    cachedCount = entities.size();
    cachedPtr = (void *)entities.data();
    cachedLayer = layer;
    isValid = true;
    return true;
  }
} traceable_spline_entity_cache;


class SplineGenEntityManagementService : public IEditorService, public IObjEntityMgr, public ISplineGenService
{
public:
  static SplineGenEntityManagementService *self;

  SplineGenEntityManagementService()
  {
    self = this;
    splineGenEntityClassId = IDaEditor3Engine::get().getAssetTypeId("spline");
    rendEntGeomMask = 1 << IDaEditor3Engine::get().registerEntitySubTypeId("rend_ent_geom");
    splineSubtypeMask = 1 << IDaEditor3Engine::get().registerEntitySubTypeId("spline_cls");
  }
  ~SplineGenEntityManagementService()
  {
    if (self == this)
      self = nullptr;
  }

  // IEditorService interface
  const char *getServiceName() const override { return "_splineGenMgr"; }
  const char *getServiceFriendlyName() const override { return NULL; }

  void setServiceVisible(bool /*vis*/) override {}
  bool getServiceVisible() const override { return true; }

  void actService(float dt) override { traceable_spline_entity_cache.isValid = false; }

  void beforeRenderService() override {}
  void renderService() override {}
  void renderTransService() override {}

  void onBeforeReset3dDevice() override {}
  bool catchEvent(unsigned event_huid, void *userData) override
  {
    // if (event_huid == HUID_BeforeMainLoop) {}
    return false;
  }

  void *queryInterfacePtr(unsigned huid) override
  {
    RETURN_INTERFACE(huid, IObjEntityMgr);
    RETURN_INTERFACE(huid, ISplineGenService);
    return NULL;
  }

  // IObjEntityMgr interface
  bool canSupportEntityClass(int entity_class) const override
  {
    return splineGenEntityClassId >= 0 && splineGenEntityClassId == entity_class;
  }

  IObjEntity *createEntity(const DagorAsset &asset, bool virtual_ent) override
  {
    CircularDependenceChecker chk;

    if (chk.isCyclicError(asset))
      return NULL;

    if (virtual_ent)
      return nullptr; // no need in virtual entities

    traceable_spline_entity_cache.isValid = false;
    SplineGenEntity *ent = sgPool.allocEntity();
    if (!ent)
      ent = new SplineGenEntity(splineGenEntityClassId);

    ent->setup(asset);
    sgPool.addEntity(ent);
    // debug("create ent: %p", ent);
    return ent;
  }

  IObjEntity *cloneEntity(IObjEntity *origin) override
  {
    traceable_spline_entity_cache.isValid = false;

    SplineGenEntity *o = reinterpret_cast<SplineGenEntity *>(origin);
    SplineGenEntity *ent = sgPool.allocEntity();
    if (!ent)
      ent = new SplineGenEntity(o->getAssetTypeId());

    ent->copyFrom(*o);
    sgPool.addEntity(ent);
    // debug("clone ent: %p", ent);
    return ent;
  }

  // ISplineGenService
  void build_corner_spline(BezierSplinePrec3d &loftSpl, BezierSplinePrec3d &baseSpl, dag::ConstSpan<splineclass::SegData> seg, int ss,
    int es) const override
  {
    traceable_spline_entity_cache.isValid = false;
    SplineGenEntity::build_corner_spline(loftSpl, baseSpl, seg, ss, es);
  }
  void build_ground_spline(BezierSpline3d &gndSpl, dag::ConstSpan<ISplineGenObj::SplinePt> points, const TMatrix &tm, int corner_type,
    bool poly, bool poly_smooth, bool is_closed) const override
  {
    traceable_spline_entity_cache.isValid = false;
    SplineGenEntity::build_ground_spline(gndSpl, points, tm, corner_type, poly, poly_smooth, is_closed);
  }

  IObjEntity *createVirtualSplineEntity(const DataBlock &blk) override
  {
    traceable_spline_entity_cache.isValid = false;
    SplineEntity *e = new SplineEntity;
    if (e->load(blk))
      return e;
    delete e;
    return nullptr;
  }

  void gatherLoftLayers(FastIntList &loft_layers, bool /*only_visible*/) const override
  {
    if (SplineGenEntity::loftChangesCount)
    {
      int64_t reft = ref_time_ticks();
      int passed = 0;

      SplineGenEntity::cachedLoftLayers.reset();
      for (SplineGenEntity *e : sgPool.getEntities())
        if (e && e->loftGeom && e->hasNonZeroLayers)
        {
          passed++;
          for (StaticGeometryNode *node : e->loftGeom->getGeometryContainer()->nodes)
            if (node)
              if (int l = node->script.getInt("layer", 0))
                SplineGenEntity::cachedLoftLayers.addInt(l);
        }

      debug("frame #%d: gatherLoftLayers ent.count=%d ent.passed=%d -> %d usec (%d layers), due to loftChangesCount=%d",
        dagor_frame_no(), sgPool.getEntities().size(), passed, get_time_usec(reft), SplineGenEntity::cachedLoftLayers.size(),
        SplineGenEntity::loftChangesCount);
      SplineGenEntity::loftChangesCount = 0;
    }

    if (!loft_layers.size())
      loft_layers = SplineGenEntity::cachedLoftLayers;
    else
      for (int ll : SplineGenEntity::cachedLoftLayers.getList())
        loft_layers.addInt(ll);
  }
  void gatherLoftTags(OAHashNameMap<true> &tags) const override
  {
    for (SplineGenEntity *e : sgPool.getEntities())
      if (e && e->loftGeom)
      {
        for (StaticGeometryNode *node : e->loftGeom->getGeometryContainer()->nodes)
          if (node)
            if (const char *t = node->script.getStr("layerTag", NULL))
              tags.addNameId(t);
      }
  }
  void gatherStaticGeometry(StaticGeometryContainer &cont, int flags, bool collision, int layer, int stage, int spl_layer,
    uint64_t lh_mask) const override
  {
    int st_mask =
      DAEDITOR3.getEntitySubTypeMask(collision ? IObjEntityFilter::STMASK_TYPE_COLLISION : IObjEntityFilter::STMASK_TYPE_EXPORT);
    if (!(st_mask & splineSubtypeMask))
      return;

    if (collision)
      flags |= StaticGeometryNode::FLG_COLLIDABLE;

    for (SplineGenEntity *e : sgPool.getEntities())
      if (e && e->loftGeom && e->mayHaveLayer(layer) && (spl_layer < 0 || e->splineLayer == spl_layer) &&
          e->checkSubtypeAndLayerHiddenMasks(~0u, lh_mask))
        if (const StaticGeometryContainer *geom = e->loftGeom->getGeometryContainer())
          for (int ni = 0; ni < geom->nodes.size(); ++ni)
          {
            StaticGeometryNode *node = geom->nodes[ni];
            if (node && node->script.getInt("stage", 0) != stage)
              continue;

            if (node && node->script.getInt("layer", 0) == layer && (!flags || (node->flags & flags)))
            {
              StaticGeometryNode *n = new (tmpmem) StaticGeometryNode(*node);
              n->name = String(1024, "%p_%d", e, ni);
              if (spl_layer >= 0)
                n->script.setInt("splineLayer", e->splineLayer); // Use this and layer:i from loft to split materials by layers.
              cont.addNode(n);
            }
          }
  }

  void renderLoftGeom(int layer, bool opaque, const Frustum &frustum, int spl_layer, bool asHeightmapPatch = false) override
  {
    int st_mask = DAEDITOR3.getEntitySubTypeMask(IObjEntityFilter::STMASK_TYPE_RENDER);
    uint64_t lh_mask = IObjEntityFilter::getLayerHiddenMask();
    if (!(st_mask & splineSubtypeMask))
      return;

    for (SplineGenEntity *e : sgPool.getEntities())
      if (e && e->loftGeom && e->mayHaveLayer(layer) && (spl_layer < 0 || e->splineLayer == spl_layer))
      {
        if (lh_mask & (uint64_t(1) << e->getEditLayerIdx()))
          continue;
        if (!frustum.testBoxB(e->loftGeomBox))
          continue;
        bool changed = false;
        bool isPatch = false;
        for (int name_idx = 0; name_idx < e->loftGeom->getShaderNamesCount(); name_idx++)
        {
          const char *shaderName = e->loftGeom->getShaderName(name_idx);
          if (strstr(shaderName, "height_patch"))
            isPatch = true;
        }
        for (StaticGeometryNode *node : e->loftGeom->getGeometryContainer()->nodes)
          if (node)
          {
            int l = node->script.getInt("layer", 0);
            if (l != layer || node->script.getInt("stage", 0) != 0)
            {
              changed = true;
              node->flags |= StaticGeometryNode::FLG_OPERATIVE_HIDE;
            }
          }

        if (!asHeightmapPatch || isPatch)
          opaque ? e->loftGeom->render() : e->loftGeom->renderTrans();

        if (changed)
          for (StaticGeometryNode *node : e->loftGeom->getGeometryContainer()->nodes)
            if (node)
              node->flags &= ~StaticGeometryNode::FLG_OPERATIVE_HIDE;
      }
  }

  bool traceRayFoundationGeom(int layer, const Point3 &p, const Point3 &dir, real &maxt, Point3 *norm) const override
  {
    dag::ConstSpan<SplineGenEntity *> entityList =
      traceable_spline_entity_cache.build(sgPool.getEntities(), layer) ? traceable_spline_entity_cache.cache : sgPool.getEntities();

    bool hit = false;
    for (SplineGenEntity *e : entityList)
      if (e && (layer < 0 || e->splineLayer == layer) && e->loftGeom)
      {
        const splineclass::AssetData *a = e->splineClass;
        if (!a || !a->genGeom || !a->genGeom->foundationGeom)
          continue;
        if (e->loftGeom->traceRay(p, dir, maxt, norm))
          hit = true;
      }
    return hit;
  }
  bool shadowRayFoundationGeomHitTest(int layer, const Point3 &p, const Point3 &dir, real &maxt) const override
  {
    for (SplineGenEntity *e : sgPool.getEntities())
      if (e && (layer < 0 || e->splineLayer == layer) && e->loftGeom)
      {
        const splineclass::AssetData *a = e->splineClass;
        if (!a || !a->genGeom || !a->genGeom->foundationGeom)
          continue;
        if (e->loftGeom->shadowRayHitTest(p, dir, maxt))
          return true;
      }
    return false;
  }

  void setSweepMask(const EditableHugeBitmask *bm, float ox, float oz, float scale) override
  {
    if (!bm)
    {
      objgenerator::splgenExcl.clear();
      return;
    }
    objgenerator::splgenExcl.initExplicit(ox, oz, scale, bm->getW(), bm->getH());
    *objgenerator::splgenExcl.bm = *bm;
  }

protected:
  SplineGenEntityPool sgPool;
  bool visible = true;

  struct CircularDependenceChecker
  {
    CircularDependenceChecker()
    {
      if (depthCnt == 0)
        assets.clear();
      depthCnt++;
    }
    ~CircularDependenceChecker()
    {
      G_ASSERT(depthCnt > 0);
      depthCnt--;
      assets.pop_back();
    }

    bool isCyclicError(const DagorAsset &a)
    {
      for (int i = assets.size() - 1; i >= 0; i--)
        if (assets[i] == &a)
        {
          DAEDITOR3.conError("asset <%s> contains circular dependencies, e.g. to %s", assets[0]->getNameTypified(),
            a.getNameTypified());
          assets.push_back(NULL);
          return true;
        }
      assets.push_back(&a);
      return false;
    }

    static int depthCnt;
    static Tab<const DagorAsset *> assets;
  };
};
SplineGenEntityManagementService *SplineGenEntityManagementService::self = nullptr;
int SplineGenEntityManagementService::CircularDependenceChecker::depthCnt = 0;
Tab<const DagorAsset *> SplineGenEntityManagementService::CircularDependenceChecker::assets(tmpmem);

void init_spline_gen_mgr_service()
{
  if (!IDaEditor3Engine::get().checkVersion())
  {
    debug_ctx("Incorrect version!");
    return;
  }

  IDaEditor3Engine::get().registerService(new (inimem) SplineGenEntityManagementService);
}
extern void *get_generic_spline_gen_service()
{
  if (SplineGenEntityManagementService::self)
    return SplineGenEntityManagementService::self->queryInterfacePtr(ISplineGenService::HUID);
  return nullptr;
}
