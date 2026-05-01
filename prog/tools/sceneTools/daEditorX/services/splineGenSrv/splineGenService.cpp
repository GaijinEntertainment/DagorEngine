// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <de3_splineGenSrv.h>
#include <de3_genObjUtil.h>
#include <de3_entityFilter.h>
#include <de3_genObjData.h>
#include <de3_circularDepsChecker.h>
#include <libTools/staticGeom/geomObject.h>
#include <libTools/staticGeom/staticGeometryContainer.h>
#include <perfMon/dag_cpuFreq.h>
#include <debug/dag_debug3d.h>
#include "spline.h"
#include "poly.h"

static int splineGenEntityClassId = -1;
static int splineSubtypeMask = -1;
static int polyGenEntityClassId = -1;
static int polySubtypeMask = -1;
static int rendEntGeomMask = -1;

template <class GEN_ENTITY_T>
class TraceableSplineEntityCache
{
  int cachedCount;
  int cachedLayer;
  void *cachedPtr;

public:
  Tab<GEN_ENTITY_T *> cache;
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

  bool build(dag::ConstSpan<GEN_ENTITY_T *> entities, int layer)
  {
    if (entities.size() < 50)
      return false;

    bool rebuild = !isValid || entities.size() != cachedCount || (void *)entities.data() != cachedPtr || cachedLayer != layer;
    if (!rebuild)
      return true;

    cache.clear();

    for (GEN_ENTITY_T *e : entities)
      if (e && (layer < 0 || e->layerOrder == layer) && e->hasGeom() && e->mayHaveFoundationGeom() && e->shouldBeTraced())
        cache.push_back(e);

    cachedCount = entities.size();
    cachedPtr = (void *)entities.data();
    cachedLayer = layer;
    isValid = true;
    return true;
  }
};
static TraceableSplineEntityCache<SplineGenEntity> traceable_spline_entity_cache;
static TraceableSplineEntityCache<PolygonGenEntity> traceable_polygon_entity_cache;

// Per-frame SoA cache for renderGeneratedGeom (spline path). The render call is invoked
// O(loft_layers * order_layers) times per frame, and each invocation otherwise dereferences
// every SplineGenEntity for a few scattered filter fields - cache misses dominate.
// Bucketed by layerOrder (0..31) so a layer_order-specific call iterates only its slice.
struct LoftRenderEntry
{
  bbox3f bbox;              // 32 bytes (16-byte aligned)
  GeomObject *loftGeom;     // 8
  uint8_t editLayerIdx;     // 1
  uint8_t hasNonZeroLayers; // 1
  uint16_t _pad0;
  uint32_t _pad1;
};

struct LoftRenderCache
{
  Tab<LoftRenderEntry> entries;
  int bucketStart[33] = {}; // bucketStart[lo] = first entry of layerOrder lo; [32] = total
  bool isValid = false;

  void invalidate() { isValid = false; }

  void rebuild(dag::ConstSpan<SplineGenEntity *> ents)
  {
    int counts[32] = {};
    for (SplineGenEntity *e : ents)
      if (e && e->loftGeom && unsigned(e->layerOrder) < 32u)
        counts[e->layerOrder]++;
    int total = 0;
    for (int i = 0; i < 32; i++)
    {
      bucketStart[i] = total;
      total += counts[i];
    }
    bucketStart[32] = total;
    entries.resize(total);
    int cursor[32];
    memcpy(cursor, bucketStart, sizeof(cursor));
    for (SplineGenEntity *e : ents)
      if (e && e->loftGeom && unsigned(e->layerOrder) < 32u)
      {
        LoftRenderEntry &en = entries[cursor[e->layerOrder]++];
        en.bbox.bmin = v_ldu(&e->loftGeomBox[0].x);
        en.bbox.bmax = v_ldu(&e->loftGeomBox[1].x);
        en.loftGeom = e->loftGeom;
        en.editLayerIdx = e->getEditLayerIdx();
        en.hasNonZeroLayers = e->hasNonZeroLayers;
      }
    isValid = true;
  }

  dag::ConstSpan<LoftRenderEntry> slice(int layer_order) const
  {
    if (layer_order < 0)
      return make_span_const(entries);
    if (unsigned(layer_order) >= 32u)
      return {};
    return make_span_const(entries.data() + bucketStart[layer_order], bucketStart[layer_order + 1] - bucketStart[layer_order]);
  }
};
static LoftRenderCache loft_render_cache;

void invalidate_loft_render_cache() { loft_render_cache.invalidate(); }


class SplineGenEntityManagementService : public IEditorService, public IObjEntityMgr
{
public:
  // IEditorService interface
  const char *getServiceName() const override { return "_splineGenMgr"; }
  const char *getServiceFriendlyName() const override { return NULL; }

  void setServiceVisible(bool /*vis*/) override {}
  bool getServiceVisible() const override { return true; }

  void actService(float dt) override
  {
    traceable_spline_entity_cache.isValid = false;
    loft_render_cache.invalidate();
  }

  void beforeRenderService() override {}
  void renderService() override {}
  void renderTransService() override {}

  void onBeforeReset3dDevice() override {}
  bool catchEvent(unsigned /*event_huid*/, void * /*userData*/) override { return false; }

  void *queryInterfacePtr(unsigned huid) override
  {
    RETURN_INTERFACE(huid, IObjEntityMgr);
    return NULL;
  }

  // IObjEntityMgr interface
  bool canSupportEntityClass(int entity_class) const override
  {
    return splineGenEntityClassId >= 0 && splineGenEntityClassId == entity_class;
  }

  IObjEntity *createEntity(const DagorAsset &asset, bool virtual_ent) override
  {
    static int cdc_depth_cnt = 0;
    static Tab<const DagorAsset *> cdc_assets_list;

    CircularDependenceChecker chk(cdc_depth_cnt, cdc_assets_list);
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
  dag::ConstSpan<SplineGenEntity *> getEntities() const { return sgPool.getEntities(); }

protected:
  SingleEntityPool<SplineGenEntity> sgPool;
};

class PolygonGenEntityManagementService : public IEditorService, public IObjEntityMgr
{
public:
  // IEditorService interface
  const char *getServiceName() const override { return "_polyGenMgr"; }
  const char *getServiceFriendlyName() const override { return NULL; }

  void setServiceVisible(bool /*vis*/) override {}
  bool getServiceVisible() const override { return true; }

  void actService(float dt) override { traceable_polygon_entity_cache.isValid = false; }

  void beforeRenderService() override {}
  void renderService() override {}
  void renderTransService() override {}

  void onBeforeReset3dDevice() override {}
  bool catchEvent(unsigned /*event_huid*/, void * /*userData*/) override { return false; }

  void *queryInterfacePtr(unsigned huid) override
  {
    RETURN_INTERFACE(huid, IObjEntityMgr);
    return NULL;
  }

  // IObjEntityMgr interface
  bool canSupportEntityClass(int entity_class) const override
  {
    return polyGenEntityClassId >= 0 && polyGenEntityClassId == entity_class;
  }

  IObjEntity *createEntity(const DagorAsset &asset, bool virtual_ent) override
  {
    static int cdc_depth_cnt = 0;
    static Tab<const DagorAsset *> cdc_assets_list;

    CircularDependenceChecker chk(cdc_depth_cnt, cdc_assets_list);
    if (chk.isCyclicError(asset))
      return NULL;

    if (virtual_ent)
      return nullptr; // no need in virtual entities

    traceable_polygon_entity_cache.isValid = false;
    PolygonGenEntity *ent = pgPool.allocEntity();
    if (!ent)
      ent = new PolygonGenEntity(polyGenEntityClassId);

    ent->setup(asset);
    pgPool.addEntity(ent);
    // debug("create ent: %p", ent);
    return ent;
  }

  IObjEntity *cloneEntity(IObjEntity *origin) override
  {
    traceable_polygon_entity_cache.isValid = false;

    PolygonGenEntity *o = reinterpret_cast<PolygonGenEntity *>(origin);
    PolygonGenEntity *ent = pgPool.allocEntity();
    if (!ent)
      ent = new PolygonGenEntity(o->getAssetTypeId());

    ent->copyFrom(*o);
    pgPool.addEntity(ent);
    // debug("clone ent: %p", ent);
    return ent;
  }
  dag::ConstSpan<PolygonGenEntity *> getEntities() const { return pgPool.getEntities(); }

protected:
  SingleEntityPool<PolygonGenEntity> pgPool;
};


struct SplineAndPolygonGenService : public ISplineGenService
{
  SplineGenEntityManagementService *splMgr = nullptr;
  PolygonGenEntityManagementService *polyMgr = nullptr;

  void init()
  {
    splineGenEntityClassId = IDaEditor3Engine::get().getAssetTypeId("spline");
    splineSubtypeMask = 1 << IDaEditor3Engine::get().registerEntitySubTypeId("spline_cls");
    polyGenEntityClassId = IDaEditor3Engine::get().getAssetTypeId("land");
    polySubtypeMask = 1 << IDaEditor3Engine::get().registerEntitySubTypeId("poly_tile");
    rendEntGeomMask = 1 << IDaEditor3Engine::get().registerEntitySubTypeId("rend_ent_geom");

    if (!IDaEditor3Engine::get().registerService(splMgr = new SplineGenEntityManagementService))
      splMgr = nullptr;
    if (!IDaEditor3Engine::get().registerService(polyMgr = new PolygonGenEntityManagementService))
      polyMgr = nullptr;
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

  void recalcLighting(GeomObject *go) override
  {
    if (go)
      SplineGenEntity::recalcGeomLighting(*go->getGeometryContainer());
  }

  IObjEntity *createVirtualSplineEntity(const DataBlock &blk) override
  {
    traceable_spline_entity_cache.isValid = false;
    traceable_polygon_entity_cache.isValid = false;
    SplineEntity *e = new SplineEntity;
    if (e->load(blk))
      return e;
    delete e;
    return nullptr;
  }

  void gatherLoftLayers(LayerIndexList &loft_layers, bool /*only_visible*/) const override
  {
    if (SplineGenEntity::loftChangesCount)
    {
      int64_t reft = ref_time_ticks();
      int passed = 0;

      SplineGenEntity::cachedLoftLayers.reset();
      for (SplineGenEntity *e : splMgr->getEntities())
        if (e && e->loftGeom && e->hasNonZeroLayers)
        {
          passed++;
          for (StaticGeometryNode *node : e->loftGeom->getGeometryContainer()->nodes)
            if (node)
              if (int l = node->layer)
                SplineGenEntity::cachedLoftLayers.set(l);
        }

      debug("frame #%d: gatherLoftLayers ent.count=%d ent.passed=%d -> %d usec (%d layers), due to loftChangesCount=%d",
        dagor_frame_no(), splMgr->getEntities().size(), passed, get_time_usec(reft),
        __popcount(SplineGenEntity::cachedLoftLayers.getMask()), SplineGenEntity::loftChangesCount);
      SplineGenEntity::loftChangesCount = 0;
    }

    loft_layers.add(SplineGenEntity::cachedLoftLayers);
  }
  void gatherGeneratedGeomOrderLayers(LayerIndexList &order_layers) const override
  {
    for (SplineGenEntity *e : splMgr->getEntities())
      if (e && e->hasGeom())
        order_layers.set(e->layerOrder);
    for (PolygonGenEntity *e : polyMgr->getEntities())
      if (e && e->hasGeom())
        order_layers.set(e->layerOrder);
  }

  void gatherGeneratedGeomTags(OAHashNameMap<true> &tags) const override
  {
    for (SplineGenEntity *e : splMgr->getEntities())
      if (e)
        gatherGeomLayerTags(e->loftGeom, tags);
    for (PolygonGenEntity *e : polyMgr->getEntities())
      if (e)
      {
        gatherGeomLayerTags(e->geom.mainMesh, tags);
        gatherGeomLayerTags(e->geom.borderMesh, tags);
      }
  }
  static void gatherGeomLayerTags(GeomObject *g, OAHashNameMap<true> &tags)
  {
    if (g)
      for (StaticGeometryNode *node : g->getGeometryContainer()->nodes)
        if (node)
          if (const char *t = node->script.getStr("layerTag", NULL))
            tags.addNameId(t);
  }

  void gatherStaticGeometry(StaticGeometryContainer &cont, int flags, bool collision, int layer, int stage, int layer_order,
    LayerHiddenMask lh_mask) const override
  {
    int st_mask =
      DAEDITOR3.getEntitySubTypeMask(collision ? IObjEntityFilter::STMASK_TYPE_COLLISION : IObjEntityFilter::STMASK_TYPE_EXPORT);


    if (collision)
      flags |= StaticGeometryNode::FLG_COLLIDABLE;

    if (st_mask & splineSubtypeMask)
      for (SplineGenEntity *e : splMgr->getEntities())
        if (e && e->loftGeom && e->mayHaveLayer(layer) && (layer_order < 0 || e->layerOrder == layer_order) &&
            e->checkSubtypeAndLayerHiddenMasks(~0u, lh_mask))
          gatherGeom(cont, e->loftGeom, flags, layer, stage, layer_order, e);

    if ((st_mask & polySubtypeMask) && layer == 0)
      for (PolygonGenEntity *e : polyMgr->getEntities())
        if (e && !e->altGeom && (layer_order < 0 || e->layerOrder == layer_order) && e->checkSubtypeAndLayerHiddenMasks(~0u, lh_mask))
        {
          gatherGeom(cont, e->geom.mainMesh, flags, layer, stage, layer_order, e);
          gatherGeom(cont, e->geom.borderMesh, flags, layer, stage, layer_order, e);
        }
  }
  static void gatherGeom(StaticGeometryContainer &cont, const GeomObject *g, int flags, int layer, int stage, int layer_order, void *e)
  {
    if (const StaticGeometryContainer *geom = g ? g->getGeometryContainer() : nullptr)
      for (int ni = 0; ni < geom->nodes.size(); ++ni)
      {
        StaticGeometryNode *node = geom->nodes[ni];
        if (node && node->stage != stage)
          continue;

        if (node && node->layer == layer && (!flags || (node->flags & flags)))
        {
          StaticGeometryNode *n = new (tmpmem) StaticGeometryNode(*node);
          n->name = (ni == 0) ? String(0, "%s_%p", node->name, e) : String(0, "%s_%p_%d", node->name, e, ni);
          if (layer_order >= 0)
            n->script.setInt("splineLayer", layer_order); // Use this and layer:i from loft to split materials by layers.
          cont.addNode(n);
        }
      }
  }

  void renderGeneratedGeom(int layer, bool opaque, const Frustum &frustum, int layer_order, bool asHeightmapPatch = false) override
  {
    int st_mask = DAEDITOR3.getEntitySubTypeMask(IObjEntityFilter::STMASK_TYPE_RENDER);
    const LayerHiddenMask lh_mask = IObjEntityFilter::getLayerHiddenMask();

    if ((st_mask & polySubtypeMask) && layer == 0)
      for (PolygonGenEntity *e : polyMgr->getEntities())
        if (e && (layer_order < 0 || e->layerOrder == layer_order))
          if (!lh_mask.isHidden(e->getEditLayerIdx()) && frustum.testBoxB(e->geomBox))
          {
            if (e->geom.mainMesh)
              renderGeom(e->geom.mainMesh, layer, opaque, asHeightmapPatch);
            if (e->geom.borderMesh)
              renderGeom(e->geom.borderMesh, layer, opaque, asHeightmapPatch);
          }

    if (st_mask & splineSubtypeMask)
    {
      if (!loft_render_cache.isValid)
        loft_render_cache.rebuild(splMgr->getEntities());
      for (const LoftRenderEntry &en : loft_render_cache.slice(layer_order))
        if ((layer == 0 || en.hasNonZeroLayers) && !lh_mask.isHidden(en.editLayerIdx) && frustum.testBoxB(en.bbox.bmin, en.bbox.bmax))
          renderGeom(en.loftGeom, layer, opaque, asHeightmapPatch);
    }
  }
  static void renderGeom(GeomObject *geom, int layer, bool opaque, bool asHeightmapPatch)
  {
    bool changed = false;
    bool isPatch = false;
    if (asHeightmapPatch)
    {
      for (int name_idx = 0; name_idx < geom->getShaderNamesCount(); name_idx++)
      {
        const char *shaderName = geom->getShaderName(name_idx);
        if (strstr(shaderName, "height_patch"))
          isPatch = true;
      }
    }
    for (StaticGeometryNode *node : geom->getGeometryContainer()->nodes)
      if (node)
      {
        if (node->layer != layer || node->stage != 0)
        {
          changed = true;
          node->flags |= StaticGeometryNode::FLG_OPERATIVE_HIDE;
        }
      }

    if (!asHeightmapPatch || isPatch)
      opaque ? geom->render() : geom->renderTrans();

    if (changed)
      for (StaticGeometryNode *node : geom->getGeometryContainer()->nodes)
        if (node)
          node->flags &= ~StaticGeometryNode::FLG_OPERATIVE_HIDE;
  }

  bool traceRayFoundationLoftGeom(int layer, const Point3 &p, const Point3 &dir, real &maxt, Point3 *norm) const override
  {
    dag::ConstSpan<SplineGenEntity *> entityList =
      traceable_spline_entity_cache.build(splMgr->getEntities(), layer) ? traceable_spline_entity_cache.cache : splMgr->getEntities();

    bool hit = false;
    for (SplineGenEntity *e : entityList)
      if (e && (layer < 0 || e->layerOrder == layer) && e->loftGeom && e->mayHaveFoundationGeom())
        if (e->loftGeom->traceRay(p, dir, maxt, norm))
          hit = true;
    return hit;
  }
  bool traceRayFoundationPolyGeom(int layer, const Point3 &p, const Point3 &dir, real &maxt, Point3 *norm) const override
  {
    dag::ConstSpan<PolygonGenEntity *> entityList = traceable_polygon_entity_cache.build(polyMgr->getEntities(), layer)
                                                      ? traceable_polygon_entity_cache.cache
                                                      : polyMgr->getEntities();

    bool hit = false;
    for (PolygonGenEntity *e : entityList)
      if (e && (layer < 0 || e->layerOrder == layer) && e->mayHaveFoundationGeom())
      {
        if (auto *go = e->geom.mainMesh)
          if (go->traceRay(p, dir, maxt, norm))
            hit = true;
        if (auto *go = e->geom.borderMesh)
          if (go->traceRay(p, dir, maxt, norm))
            hit = true;
      }
    return hit;
  }
  bool shadowRayFoundationLoftGeomHitTest(int layer, const Point3 &p, const Point3 &dir, real &maxt) const override
  {
    for (SplineGenEntity *e : splMgr->getEntities())
      if (e && (layer < 0 || e->layerOrder == layer) && e->loftGeom && e->mayHaveFoundationGeom())
        if (e->loftGeom->shadowRayHitTest(p, dir, maxt))
          return true;
    return false;
  }
  bool shadowRayFoundationPolyGeomHitTest(int layer, const Point3 &p, const Point3 &dir, real &maxt) const override
  {
    for (PolygonGenEntity *e : polyMgr->getEntities())
      if (e && (layer < 0 || e->layerOrder == layer) && e->mayHaveFoundationGeom())
      {
        if (auto *go = e->geom.mainMesh)
          if (go->shadowRayHitTest(p, dir, maxt))
            return true;
        if (auto *go = e->geom.borderMesh)
          if (go->shadowRayHitTest(p, dir, maxt))
            return true;
      }
    return false;
  }

  static void copyMask(objgenerator::WorldHugeBitmask &dest_bm, const objgenerator::WorldHugeBitmask &src_bm)
  {
    dest_bm.bm = nullptr;
    if (!src_bm.bm)
      return dest_bm.clear();
    dest_bm.initExplicit(src_bm.ox, src_bm.oz, src_bm.scale, src_bm.bm->getW(), src_bm.bm->getH());
    *dest_bm.bm = *src_bm.bm;
  }
  void setSweepMaskForSplines(const objgenerator::WorldHugeBitmask &bm) override { copyMask(objgenerator::splgenExcl, bm); }
  void setSweepMaskForPolygons(const objgenerator::WorldHugeBitmask &bm) override { copyMask(objgenerator::lcmapExcl, bm); }
  void resetSweepMasks() override
  {
    objgenerator::splgenExcl.bm = nullptr;
    objgenerator::splgenExcl.clear();
    objgenerator::lcmapExcl.bm = nullptr;
    objgenerator::lcmapExcl.clear();
  }

  void buildSmoothPolyPoints(Tab<Point3> &out_smooth_poly, const BezierSpline3d &bezierSpline,
    const IPolygonGenObj::Props &props) override
  {
    PolygonGenEntity::build_smooth_poly(out_smooth_poly, bezierSpline, props);
  }
  bool buildInnerSpline(Tab<Point3> &out_pts, dag::ConstSpan<Point3> smooth_poly, float offset, float pts_y, float eps) override
  {
    return PolygonGenEntity::build_inner_spline(out_pts, smooth_poly, offset, pts_y, eps);
  }

  void renderDebugPolyEdges(const IPolygonGenObj::Geom &g) override
  {
    if (!g.verts.size())
      return;

    for (int i = 0; i < g.centerTri.size(); i++)
    {
      ::draw_cached_debug_line(g.verts[g.centerTri[i][0]], g.verts[g.centerTri[i][1]], E3DCOLOR(255, 255, 0, 100));
      ::draw_cached_debug_line(g.verts[g.centerTri[i][1]], g.verts[g.centerTri[i][2]], E3DCOLOR(255, 255, 0, 100));
      ::draw_cached_debug_line(g.verts[g.centerTri[i][2]], g.verts[g.centerTri[i][0]], E3DCOLOR(255, 255, 0, 100));
    }

    for (int i = 0; i < g.borderTri.size(); i++)
    {
      ::draw_cached_debug_line(g.verts[g.borderTri[i][0]], g.verts[g.borderTri[i][1]], E3DCOLOR(255, 0, 100, 100));
      ::draw_cached_debug_line(g.verts[g.borderTri[i][1]], g.verts[g.borderTri[i][2]], E3DCOLOR(255, 0, 100, 100));
      ::draw_cached_debug_line(g.verts[g.borderTri[i][2]], g.verts[g.borderTri[i][0]], E3DCOLOR(255, 0, 100, 100));
    }
  }
};

static SplineAndPolygonGenService splSrv;

extern void init_spline_gen_mgr_service() { splSrv.init(); }
extern void *get_generic_spline_gen_service() { return &splSrv; }
