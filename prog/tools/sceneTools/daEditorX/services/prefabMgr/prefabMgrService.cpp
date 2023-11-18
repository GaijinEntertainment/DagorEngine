#include <de3_interface.h>
#include <de3_objEntity.h>
#include <de3_entityPool.h>
#include <de3_entityFilter.h>
#include <de3_occluderGeomProvider.h>
#include <de3_occludersFromGeom.h>
#include <de3_editorEvents.h>
#include <de3_prefabs.h>
#include <assets/assetChangeNotify.h>
#include <assets/assetMgr.h>
#include <de3_drawInvalidEntities.h>
#include <oldEditor/de_interface.h>
#include <oldEditor/de_clipping.h>
#include <libTools/staticGeom/geomObject.h>
#include <libTools/staticGeom/staticGeometryContainer.h>
#include <regExp/regExp.h>
#include <oldEditor/de_common_interface.h>
#include <scene/dag_occlusionMap.h>
#include <scene/dag_occlusion.h>
#include <3d/dag_drv3d.h>
#include <3d/dag_render.h>
#include <render/dag_cur_view.h>
#include <ioSys/dag_dataBlock.h>
#include <math/dag_TMatrix.h>
#include <math/dag_rayIntersectSphere.h>
#include <util/dag_simpleString.h>
#include <debug/dag_debug.h>
#include <debug/dag_debug3d.h>
#include <coolConsole/coolConsole.h>
#include <de3_collisionPreview.h>
#include <de3_entityCollision.h>
#include <perfMon/dag_statDrv.h>

static int prefabEntityClassId = -1;
static int collisionSubtypeMask = -1;
static int rendEntGeomMask = -1;
static int groundObjectSubtypeMask = -1;
static RegExp *check_clipmap_shclass_re = NULL;
static RegExp *check_water_proj_shclass_re = NULL;
static RegExp *check_loft_mask_shclass_re = NULL;

struct PrefabObjScaleSetup
{
  RegExp *shaderRE;
  unsigned tcMask;
};
static Tab<PrefabObjScaleSetup> obj_scale_setup;

class PrefabEntity;
class PrefabEntityPool;

typedef MultiEntityPool<PrefabEntity, PrefabEntityPool> MultiPrefabEntityPool;
typedef VirtualMpEntity<MultiPrefabEntityPool> VirtualPrefabEntity;

class PrefabEntity : public VirtualPrefabEntity, public IEntityCollisionState
{
public:
  PrefabEntity() : VirtualPrefabEntity(prefabEntityClassId) { tm.identity(); }

  virtual void setTm(const TMatrix &_tm);
  virtual void getTm(TMatrix &_tm) const { _tm = tm; }
  virtual void destroy();

  virtual void *queryInterfacePtr(unsigned huid)
  {
    RETURN_INTERFACE(huid, IEntityCollisionState);
    return NULL;
  }

  // IEntityCollisionState
  virtual void setCollisionFlag(bool f)
  {
    if (f)
      flags |= FLG_CLIP_EDITOR | FLG_CLIP_GAME;
    else
      flags &= ~(FLG_CLIP_EDITOR | FLG_CLIP_GAME);
  }
  virtual bool hasCollision() { return flags & FLG_CLIP_GAME; }
  virtual int getAssetNameId();
  virtual DagorAsset *getAsset();

public:
  static const int STEP = 512;
  enum
  {
    FLG_CLIP_EDITOR = 0x0001,
    FLG_CLIP_GAME = 0x0002,
  };

  TMatrix tm;
};


class PrefabEntityPool : public EntityPool<PrefabEntity>, public IDagorAssetChangeNotify
{
public:
  PrefabEntityPool(const DagorAsset &asset) :
    virtualEntity(prefabEntityClassId), occlBox(midmem), occlQuadV(midmem), geomTmIdent(false)
  {
    assetNameId = asset.getNameId();
    aMgr = &asset.getMgr();
    aMgr->subscribeUpdateNotify(this, assetNameId, asset.getType());
    visRad = 0;
    hasGeomForClipmap = foundationGeom = false;
    renderForClipmapLate = false;
    hasWaterProjGeom = false;
    stageIdx = prefabs::PrefabStage::DEFAULT;

    loadDag(asset);
  }
  ~PrefabEntityPool()
  {
    if (aMgr)
      aMgr->unsubscribeUpdateNotify(this);
    del_it(geom);
    aMgr = NULL;
    assetNameId = -1;
  }

  bool checkEqual(const DagorAsset &asset) { return assetNameId == asset.getNameId(); }
  bool checkStageIdx(int idx) const { return stageIdx == idx; }

  void renderService(int subtype_mask)
  {
    if (!geom)
      ::render_invalid_entities(ent, subtype_mask);

    if (subtype_mask & collisionSubtypeMask)
    {
      begin_draw_cached_debug_lines();

      const E3DCOLOR color(0, 255, 0);

      int entCnt = ent.size();
      for (int k = 0; k < entCnt; k++)
      {
        if (!ent[k] || (ent[k]->getSubtype() == IObjEntity::ST_NOT_COLLIDABLE) ||
            ((ent[k]->getFlags() & PrefabEntity::FLG_CLIP_GAME) != PrefabEntity::FLG_CLIP_GAME))
          continue;

        TMatrix wtm = TMatrix::IDENT;
        ent[k]->getTm(wtm);

        collisionpreview::drawCollisionPreview(collision, wtm, color);
      }
      end_draw_cached_debug_lines();
    }
  }

  inline bool checkEntityVisible(const Frustum &frustum, const TMatrix &tm)
  {
    BSphere3 bs;
    bs = tm * bsph;
    if (visibility_finder && !visibility_finder->isScreenRatioVisible(bs.c, bs.r2))
      return false;
    if (!frustum.testSphereB(bs.c, bs.r))
      return false;
    if (current_occlusion && current_occlusion->isOccludedSphere(v_ldu(&bs.c.x), v_splats(bs.r)))
      return false;

    return true;
  }

  void render(const Frustum &frustum)
  {
    if (!geom)
      return;
    // debug("render prefabs: %s", assetName());

    TMatrix tm;
    int subtype_mask = IObjEntityFilter::getSubTypeMask(IObjEntityFilter::STMASK_TYPE_RENDER);
    uint64_t lh_mask = IObjEntityFilter::getLayerHiddenMask();

    if (stageIdx == prefabs::PrefabStage::LOFT_MASK_EXPORT && !(subtype_mask & groundObjectSubtypeMask))
      return;

    if (!(sumNodeFlags & StaticGeometryNode::FLG_RENDERABLE))
      return;

    TIME_D3D_PROFILE_NAME(render_prefab, assetName());
    for (int j = 0; j < ent.size(); j++)
      if (ent[j] && ent[j]->checkSubtypeAndLayerHiddenMasks(subtype_mask, lh_mask))
      {
        ent[j]->getTm(tm);
        // debug("  render %d %p: " FMT_P3 "", j, ent[j], P3D(tm.getcol(3)));

        if (!checkEntityVisible(frustum, tm))
          continue;
        geom->setTm(tm);
        geom->render();
      }
    geomTmIdent = false;
  }

  void renderTrans(const Frustum &frustum)
  {
    // debug("render trans prefabs: %s", assetName());
    if (!(sumNodeFlags & StaticGeometryNode::FLG_RENDERABLE))
      return;

    TMatrix tm;
    int subtype_mask = IObjEntityFilter::getSubTypeMask(IObjEntityFilter::STMASK_TYPE_RENDER);
    uint64_t lh_mask = IObjEntityFilter::getLayerHiddenMask();

    if (stageIdx == prefabs::PrefabStage::LOFT_MASK_EXPORT && !(subtype_mask & groundObjectSubtypeMask))
      return;

    for (int j = 0; j < ent.size(); j++)
      if (ent[j] && ent[j]->checkSubtypeAndLayerHiddenMasks(subtype_mask, lh_mask))
      {
        ent[j]->getTm(tm);
        // debug("  render %d %p: " FMT_P3 "", j, ent[j], P3D(tm.getcol(3)));

        if (!checkEntityVisible(frustum, tm))
          continue;
        geom->setTm(tm);
        geom->renderTrans();
      }
    geomTmIdent = false;
  }

  static int processNodeWithObjectScale(StaticGeometryNode *n, float objectScale)
  {
    const StaticGeometryMesh &m = *n->mesh;
    if (!obj_scale_setup.size())
      return 0;

    Bitarray tvmask[NUMMESHTCH];
    int sum = 0;
    for (int i = 0; i < m.mats.size(); i++)
    {
      unsigned tc_mask = 0;
      for (int j = 0; j < obj_scale_setup.size(); j++)
        if (obj_scale_setup[j].shaderRE->test(m.mats[i]->className))
        {
          tc_mask = obj_scale_setup[j].tcMask;
          break;
        }

      if (!tc_mask)
        continue;
      for (int j = 0; j < NUMMESHTCH; j++)
        if (tc_mask & (1 << j))
        {
          if (tvmask[j].size() != m.mesh.getTVert(j).size())
          {
            tvmask[j].resize(m.mesh.getTVert(j).size());
            tvmask[j].reset();
          }
          else if (!m.mesh.getTVert(j).size())
            tc_mask &= ~(1 << j);
        }


      dag::ConstSpan<Face> f = m.mesh.getFace();
      for (int fi = 0; fi < f.size(); fi++)
        if (f[fi].mat == i)
          for (int j = 0; j < NUMMESHTCH; j++)
            if (tc_mask & (1 << j))
              for (int k = 0; k < 3; k++)
              {
                tvmask[j].set(m.mesh.getTFace(j)[fi].t[k]);
                sum++;
              }
    }
    if (!sum)
      return 0;

    // make mesh copy since we are going to modify it!
    n->mesh = new StaticGeometryMesh(m);
    for (int j = 0; j < NUMMESHTCH; j++)
      if (tvmask[j].size())
      {
        TVertTab &t = n->mesh->mesh.getMeshData().tvert[j];
        for (int k = 0; k < t.size(); k++)
          if (tvmask[j].get(k))
            t[k] *= objectScale;
      }
    return sum;
  }
  void addGeometry(StaticGeometryContainer &cont, int node_flags, int entity_flags, int st_mask, uint64_t lh_mask = 0)
  {
    if (!geom)
      return;
    if (!(sumNodeFlags & node_flags))
      return;

    const StaticGeometryContainer *g = geom->getGeometryContainer();
    Tab<StaticGeometryNode *> nodes(tmpmem);

    for (int i = 0; i < g->nodes.size(); ++i)
    {
      StaticGeometryNode *node = g->nodes[i];

      if (node && (node->flags & node_flags) == node_flags)
        nodes.push_back(node);
    }


    for (int j = 0; j < ent.size(); j++)
      if (ent[j] && ent[j]->checkSubtypeAndLayerHiddenMasks(st_mask, lh_mask) && (ent[j]->getFlags() & entity_flags) == entity_flags)
        for (int i = 0; i < nodes.size(); i++)
        {
          StaticGeometryNode &node = *nodes[i];
          StaticGeometryNode *n = new (midmem) StaticGeometryNode(node);

          n->wtm = ent[j]->tm * node.wtm;
          n->flags = node.flags;
          n->visRange = node.visRange;
          n->lighting = node.lighting;

          n->name = String(256, "%s_%d_%s", assetName(), j, node.name.str());

          if (node.topLodName.length())
            n->topLodName = String(256, "%s_%d_%s", assetName(), j, node.topLodName.str());

          if (node_flags == StaticGeometryNode::FLG_RENDERABLE)
          {
            float objectScale = max(1.0f, sqrtf(0.5 * (n->wtm.getcol(1).lengthSq() + n->wtm.getcol(2).lengthSq())));
            if (objectScale > 1.01)
              if (int tv_scaled_count = processNodeWithObjectScale(n, objectScale))
                debug("prefab <%s>: scaled %d TCs with object scale %.2f", n->name, tv_scaled_count, objectScale);
          }
          cont.addNode(n);
        }
  }
  bool traceRay(const Point3 &p, const Point3 &dir, real &maxt, Point3 *norm)
  {
    if (!foundationGeom)
      return false;

    if (maxt <= 0)
      return false;

    if (!geomTmIdent)
    {
      geom->setTm(TMatrix::IDENT);
      geomTmIdent = true;
    }

    TMatrix tm, iwtm;
    BSphere3 wbs;

    const int subtype_mask = IObjEntityFilter::getSubTypeMask(IObjEntityFilter::STMASK_TYPE_COLLISION);
    int hit_id = -1;
    for (int j = 0; j < ent.size(); j++)
      if (ent[j] && (ent[j]->getSubtype() != IObjEntity::ST_NOT_COLLIDABLE) && ent[j]->checkSubtypeMask(subtype_mask))
      {
        ent[j]->getTm(tm);
        wbs = tm * bsph;
        if (!notNormalizedRayIntersectSphere(p, dir, wbs.c, wbs.r2))
          continue;

        float tm_det = tm.det();
        if (fabsf(tm_det) < 1e-12)
          continue;
        iwtm = inverse(tm, tm_det);
        Point3 transformedP = iwtm * p;
        Point3 transformedP2 = iwtm * (p + dir * maxt);
        Point3 transformedDir = transformedP2 - transformedP;

        real t = 1;

        if (geom->traceRay(transformedP, transformedDir, t, norm))
        {
          maxt *= t;
          hit_id = j;
        }
      }

    if (hit_id >= 0)
    {
      if (norm)
      {
        ent[hit_id]->getTm(tm);
        *norm = normalize(tm % *norm);
      }
      return true;
    }
    return false;
  }
  bool shadowRayHitTest(const Point3 &p, const Point3 &dir, real &maxt)
  {
    if (!(sumNodeFlags & StaticGeometryNode::FLG_RENDERABLE))
      return false;
    if (!(sumNodeFlags & StaticGeometryNode::FLG_CASTSHADOWS))
      return false;

    if (maxt <= 0)
      return false;

    if (!geomTmIdent)
    {
      geom->setTm(TMatrix::IDENT);
      geomTmIdent = true;
    }

    TMatrix tm, iwtm;
    BSphere3 wbs;

    const int subtype_mask = IObjEntityFilter::getSubTypeMask(IObjEntityFilter::STMASK_TYPE_COLLISION);
    for (int j = 0; j < ent.size(); j++)
      if (ent[j] && ent[j]->getSubtype() != IObjEntity::ST_NOT_COLLIDABLE && ent[j]->checkSubtypeMask(subtype_mask))
      {
        ent[j]->getTm(tm);
        wbs = tm * bsph;
        if (!notNormalizedRayIntersectSphere(p, dir, wbs.c, wbs.r2))
          continue;

        float tm_det = tm.det();
        if (fabsf(tm_det) < 1e-12)
          continue;
        iwtm = inverse(tm, tm_det);
        Point3 transformedP = iwtm * p;
        Point3 transformedP2 = iwtm * (p + dir * maxt);
        Point3 transformedDir = transformedP2 - transformedP;

        if (geom->shadowRayHitTest(transformedP2, -transformedDir, 1, 0))
          return true;
      }
    return false;
  }
  void gatherOccluders(Tab<TMatrix> &occl_boxes, Tab<IOccluderGeomProvider::Quad> &occl_quads)
  {
    if (occlBox.size() + occlQuadV.size() == 0)
      return;

    int bnum = occlBox.size(), qnum = occlQuadV.size() / 4;
    TMatrix tm;

    for (int j = 0; j < ent.size(); j++)
      if (ent[j])
      {
        ent[j]->getTm(tm);

        if (bnum)
        {
          int base = append_items(occl_boxes, bnum);
          for (int i = 0; i < bnum; i++)
            occl_boxes[base + i] = tm * occlBox[i];
        }
        if (qnum)
        {
          int base = append_items(occl_quads, qnum);
          for (int i = 0; i < qnum * 4; i++)
            occl_quads[base + i / 4].v[i % 4] = tm * occlQuadV[i];
        }
      }
  }

  void setupVirtualEnt(MultiPrefabEntityPool *p, int idx)
  {
    virtualEntity.pool = p;
    virtualEntity.poolIdx = idx;
  }
  IObjEntity *getVirtualEnt() { return &virtualEntity; }

  // IDagorAssetChangeNotify interface
  virtual void onAssetRemoved(int asset_name_id, int asset_type)
  {
    del_it(geom);
    sumNodeFlags = 0;
    occlBox.clear();
    occlQuadV.clear();
    EDITORCORE->invalidateViewportCache();
  }
  virtual void onAssetChanged(const DagorAsset &asset, int asset_name_id, int asset_type)
  {
    del_it(geom);
    bbox.setempty();
    bsph.setempty();

    loadDag(asset);
    EDITORCORE->invalidateViewportCache();
  }

  bool isCollidable() const { return (sumNodeFlags & StaticGeometryNode::FLG_COLLIDABLE); }
  bool needsRenderToClipmap() const { return hasGeomForClipmap; }
  bool needsRenderToClipmap(bool late) const { return (late == renderForClipmapLate) && hasGeomForClipmap; }
  bool needsRenderAsHeightmapPatch() const { return isPatchingHeightmap; }
  bool needsRenderToWater() const { return hasWaterProjGeom; }
  const char *assetName() const { return aMgr ? aMgr->getAssetName(assetNameId) : NULL; }

  const char *getObjAssetName() const
  {
    static String buf;
    buf.printf(0, "%s:prefab", assetName());
    return buf;
  }
  int getAssetNameId() const { return assetNameId; }
  DagorAsset *getAsset() const
  {
    if (!aMgr)
      return NULL;
    dag::ConstSpan<DagorAsset *> a = aMgr->getAssets();
    for (int i = 0; i < a.size(); i++)
      if (a[i]->getNameId() == assetNameId && a[i]->getType() == prefabEntityClassId)
        return a[i];
    return NULL;
  }

protected:
  DagorAssetMgr *aMgr;
  GeomObject *geom;
  VirtualPrefabEntity virtualEntity;
  Tab<TMatrix> occlBox;
  Tab<Point3> occlQuadV;
  float visRad;

  collisionpreview::Collision collision;

  int assetNameId;
  int sumNodeFlags;
  bool geomTmIdent;
  bool hasGeomForClipmap;
  bool renderForClipmapLate;
  bool hasWaterProjGeom;
  bool foundationGeom;
  bool isPatchingHeightmap;
  int stageIdx;

  void loadDag(const DagorAsset &asset)
  {
    SimpleString prefabName(asset.getTargetFilePath());

    sumNodeFlags = 0;
    geom = new (midmem) GeomObject;
    visRad = 0;
    if (geom->loadFromDag(prefabName, &EDITORCORE->getConsole()))
    {
      bbox = geom->getBoundBox(true);
      bsph = bbox;
      visRad = bsph.r + length(bsph.c);
    }
    else
    {
      DAEDITOR3.conError("can't load prefab %s: %s", asset.getName(), prefabName.str());
      bbox = bsph = BSphere3(Point3(0, 0, 0), 0.3);
      del_it(geom);
      return;
    }

    ::getOcclFromGeom(*geom->getGeometryContainer(), asset.getName(), occlBox, occlQuadV);
    parsePrefab(asset);
  }

  void parsePrefab(const DagorAsset &asset)
  {
    const StaticGeometryContainer *g = geom->getGeometryContainer();

    stageIdx = -1;
    int clipmap_late = -1;
    if (asset.props.paramExists("clipmapLate"))
      clipmap_late = asset.props.getBool("clipmapLate") ? 1 : 0;
    for (int i = 0; i < g->nodes.size(); ++i)
      if (g->nodes[i])
      {
        sumNodeFlags |= g->nodes[i]->flags;
        int st = g->nodes[i]->script.getInt("stage", -1);
        if (st >= 0)
        {
          if (stageIdx < 0)
            stageIdx = st;
          else
            DAEDITOR3.conError("prefab %s: multiple stage:i= used in nodes, will use first stage=%d", asset.getName(), stageIdx);
        }
        if (g->nodes[i]->script.paramExists("clipmapLate"))
        {
          if (clipmap_late < 0)
            clipmap_late = g->nodes[i]->script.getBool("clipmapLate") ? 1 : 0;
          else
            DAEDITOR3.conError("prefab %s: multiple clipmapLate:i= used in nodes, will use first stage=%d", asset.getName(),
              clipmap_late);
        }
      }
    if (stageIdx < 0)
      stageIdx = prefabs::PrefabStage::DEFAULT;
    else if (stageIdx != prefabs::PrefabStage::DEFAULT && stageIdx != prefabs::PrefabStage::LOFT_MASK_EXPORT)
      DAEDITOR3.conError("prefab %s: unknown stage:i=%d used in nodes", asset.getName(), stageIdx);

    const char *loftMaskShaderName = nullptr;
    if (check_loft_mask_shclass_re)
      for (int i = 0, e = geom->getShaderNamesCount(); i < e; ++i)
        if (check_loft_mask_shclass_re->test(geom->getShaderName(i)))
        {
          loftMaskShaderName = geom->getShaderName(i);
          break;
        }
    if (loftMaskShaderName && stageIdx != prefabs::PrefabStage::LOFT_MASK_EXPORT)
      DAEDITOR3.conError("prefab %s: loft mask shader '%s' used without stage:i=%d", asset.getName(), loftMaskShaderName,
        prefabs::PrefabStage::LOFT_MASK_EXPORT);

    renderForClipmapLate = clipmap_late > 0;

    if (asset.props.getBool("no_collision", false))
      sumNodeFlags &= ~StaticGeometryNode::FLG_COLLIDABLE;

    if (asset.props.getBool("no_render", false))
      sumNodeFlags &= ~StaticGeometryNode::FLG_RENDERABLE;

    if (sumNodeFlags & StaticGeometryNode::FLG_COLLIDABLE)
      collisionpreview::assembleCollisionFromNodes(collision, g->nodes);
    else
      collision.clear();

    isPatchingHeightmap = false;
    for (int i = 0; i < g->nodes.size(); ++i)
      if (g->nodes[i] && g->nodes[i]->mesh.get() && (g->nodes[i]->flags & StaticGeometryNode::FLG_RENDERABLE))
      {
        PtrTab<StaticGeometryMaterial> &mats = g->nodes[i]->mesh->mats;
        for (int j = 0; j < mats.size(); j++)
          if (strstr(mats[j]->className, "height_patch"))
          {
            isPatchingHeightmap = true;
            break;
          }
        if (isPatchingHeightmap)
          break;
      }

    hasGeomForClipmap = check_clipmap_shclass_re ? false : true;
    if (check_clipmap_shclass_re)
      for (int i = 0; i < g->nodes.size(); ++i)
        if (g->nodes[i] && g->nodes[i]->mesh.get() && (g->nodes[i]->flags & StaticGeometryNode::FLG_RENDERABLE))
        {
          PtrTab<StaticGeometryMaterial> &mats = g->nodes[i]->mesh->mats;
          for (int j = 0; j < mats.size(); j++)
            if (check_clipmap_shclass_re->test(mats[j]->className))
            {
              hasGeomForClipmap = true;
              break;
            }
          if (hasGeomForClipmap)
            break;
        }
    debug("prefab [%s]: %s for clipmap render %s", asset.getName(), hasGeomForClipmap ? "USE" : "DONT use",
      hasGeomForClipmap ? (renderForClipmapLate ? "(stage_late)" : "(stage_normal)") : "");


    hasWaterProjGeom = check_water_proj_shclass_re ? false : true;
    if (check_water_proj_shclass_re)
      for (int i = 0; i < g->nodes.size(); ++i)
        if (g->nodes[i] && g->nodes[i]->mesh.get() && (g->nodes[i]->flags & StaticGeometryNode::FLG_RENDERABLE))
        {
          PtrTab<StaticGeometryMaterial> &mats = g->nodes[i]->mesh->mats;
          for (int j = 0; j < mats.size(); j++)
            if (check_water_proj_shclass_re->test(mats[j]->className))
            {
              hasWaterProjGeom = true;
              break;
            }
          if (hasWaterProjGeom)
            break;
        }

    if (sumNodeFlags & StaticGeometryNode::FLG_COLLIDABLE)
    {
      foundationGeom = hasGeomForClipmap;
      for (int i = 0; i < g->nodes.size(); ++i)
        if (g->nodes[i])
          if (g->nodes[i]->script.getBool("foundationGeom", false))
            foundationGeom = true;
    }
    else
      foundationGeom = false;
    debug("prefab [%s]: %s foundation geom", asset.getName(), foundationGeom ? "HAS" : "has NOT");
  }
};

void PrefabEntity::setTm(const TMatrix &_tm)
{
  tm = _tm;
  if (DAGORED2 && pool->getPools()[poolIdx]->needsRenderToClipmap())
    DAGORED2->spawnEvent(HUID_InvalidateClipmap, (void *)false);
}
void PrefabEntity::destroy()
{
  if (DAGORED2 && pool->getPools()[poolIdx]->needsRenderToClipmap())
    DAGORED2->spawnEvent(HUID_InvalidateClipmap, (void *)false);
  pool->delEntity(this);
}
int PrefabEntity::getAssetNameId() { return pool->getPools()[poolIdx]->getAssetNameId(); }
DagorAsset *PrefabEntity::getAsset() { return pool->getPools()[poolIdx]->getAsset(); }

class PrefabEntityManagementService : public IEditorService,
                                      public IObjEntityMgr,
                                      public IGatherStaticGeometry,
                                      public IDagorEdCustomCollider,
                                      public IOccluderGeomProvider,
                                      public IRenderingService
{
public:
  PrefabEntityManagementService()
  {
    prefabEntityClassId = IDaEditor3Engine::get().getAssetTypeId("prefab");
    rendEntGeomMask = 1 << IDaEditor3Engine::get().registerEntitySubTypeId("rend_ent_geom");
    collisionSubtypeMask = 1 << IDaEditor3Engine::get().registerEntitySubTypeId("collision");
    groundObjectSubtypeMask = 1 << IDaEditor3Engine::get().registerEntitySubTypeId("ground_obj");
    visible = true;
    if (DAGORED2)
      DAGORED2->registerCustomCollider(this);
  }
  ~PrefabEntityManagementService()
  {
    if (DAGORED2)
      DAGORED2->unregisterCustomCollider(this);
  }

  // IEditorService interface
  virtual const char *getServiceName() const { return "_prefabEntMgr"; }
  virtual const char *getServiceFriendlyName() const { return "(srv) Prefab entities"; }

  virtual void setServiceVisible(bool vis) { visible = vis; }
  virtual bool getServiceVisible() const { return visible; }

  virtual void actService(float dt) {}
  virtual void beforeRenderService() {}
  virtual void renderService()
  {
    dag::ConstSpan<PrefabEntityPool *> p = prefabPool.getPools();
    if (!p.size())
      return;

    int subtypeMask = IObjEntityFilter::getSubTypeMask(IObjEntityFilter::STMASK_TYPE_RENDER);
    uint64_t lh_mask = IObjEntityFilter::getLayerHiddenMask();

    for (int i = 0; i < p.size(); i++)
      p[i]->renderService(subtypeMask);
  }
  virtual void renderTransService() {}

  virtual void onBeforeReset3dDevice() {}
  virtual bool catchEvent(unsigned event_huid, void *userData)
  {
    if (event_huid == HUID_DumpEntityStat)
    {
      int ccnt = 0;
      dag::ConstSpan<PrefabEntityPool *> p = prefabPool.getPools();
      for (int i = 0; i < p.size(); i++)
        if (p[i] && p[i]->isCollidable())
          ccnt += p[i]->getUsedEntityCount();
      DAEDITOR3.conNote("PrefabMgr: %d entities (%d collidable)", prefabPool.getUsedEntityCount(), ccnt);
    }
    return false;
  }

  virtual void *queryInterfacePtr(unsigned huid)
  {
    RETURN_INTERFACE(huid, IObjEntityMgr);
    RETURN_INTERFACE(huid, IRenderingService);
    RETURN_INTERFACE(huid, IGatherStaticGeometry);
    RETURN_INTERFACE(huid, IOccluderGeomProvider);
    return NULL;
  }

  // IRenderingService interface
  virtual void renderGeometry(Stage stage)
  {
    int st_mask = IObjEntityFilter::getSubTypeMask(IObjEntityFilter::STMASK_TYPE_RENDER);
    if ((st_mask & rendEntGeomMask) != rendEntGeomMask)
      return;

    if (stage != STG_RENDER_STATIC_OPAQUE && stage != STG_RENDER_STATIC_TRANS && stage != STG_RENDER_SHADOWS &&
        stage != STG_RENDER_WATER_PROJ && stage != STG_RENDER_TO_CLIPMAP && stage != STG_RENDER_TO_CLIPMAP_LATE &&
        stage != STG_RENDER_HEIGHT_PATCH && stage != STG_RENDER_GRASS_MASK)
      return;

    dag::ConstSpan<PrefabEntityPool *> p = prefabPool.getPools();
    if (!p.size())
      return;
    TIME_D3D_PROFILE_NAME(render_prefab, "prefab");
    static int landCellShortDecodeXZ = ::get_shader_variable_id("landCellShortDecodeXZ", true);
    static int landCellShortDecodeY = ::get_shader_variable_id("landCellShortDecodeY", true);
    static int prefabs_render_modeVarId = ::get_shader_variable_id("prefabs_render_mode", true);
    static Color4 dec_xz_0(0, 0, 0, 0), dec_y_0(0, 0, 0, 0);
    if (landCellShortDecodeXZ != -1)
    {
      dec_xz_0 = ShaderGlobal::get_color4(landCellShortDecodeXZ);
      ShaderGlobal::set_color4(landCellShortDecodeXZ, 1.0f / 32767.f, 0, 0, 0);
    }
    if (landCellShortDecodeY != -1)
    {
      dec_y_0 = ShaderGlobal::get_color4(landCellShortDecodeY);
      ShaderGlobal::set_color4(landCellShortDecodeY, 1.0f / 32767.f, 0, 0, 0);
    }

    TMatrix4 projtm, viewtm;
    d3d::gettm(TM_PROJ, &projtm);
    d3d::gettm(TM_VIEW, &viewtm);
    Frustum frustum(viewtm * projtm);
    switch (stage)
    {
      case STG_RENDER_STATIC_OPAQUE:
        ShaderGlobal::set_int(prefabs_render_modeVarId, 1); // render to gbuffer
        for (int i = 0; i < p.size(); i++)
          p[i]->render(frustum);
        ShaderGlobal::set_int(prefabs_render_modeVarId, 0); // restore render to loft mask
        break;

      case STG_RENDER_TO_CLIPMAP:
      case STG_RENDER_TO_CLIPMAP_LATE:
        for (int i = 0; i < p.size(); i++)
          if (p[i]->needsRenderToClipmap(stage == STG_RENDER_TO_CLIPMAP_LATE))
            p[i]->render(frustum);
        break;

      case STG_RENDER_GRASS_MASK:
        for (int i = 0; i < p.size(); i++)
          if (p[i]->needsRenderToClipmap(false))
            p[i]->render(frustum);
        for (int i = 0; i < p.size(); i++)
          if (p[i]->needsRenderToClipmap(true))
            p[i]->render(frustum);
        break;

      case STG_RENDER_WATER_PROJ:
        for (int i = 0; i < p.size(); i++)
          if (p[i]->needsRenderToWater())
            p[i]->render(frustum);
        break;

      case STG_RENDER_STATIC_TRANS:
        for (int i = 0; i < p.size(); i++)
          p[i]->renderTrans(frustum);
        break;

      case STG_RENDER_SHADOWS:
        for (int i = 0; i < p.size(); i++)
        {
          p[i]->render(frustum);
          // p[i]->renderTrans(frustum);
        }
        break;
      case STG_RENDER_HEIGHT_PATCH:
        for (int i = 0; i < p.size(); i++)
          if (p[i]->needsRenderAsHeightmapPatch())
            p[i]->render(frustum);
    }
    if (landCellShortDecodeXZ != -1)
      ShaderGlobal::set_color4(landCellShortDecodeXZ, dec_xz_0);
    if (landCellShortDecodeY != -1)
      ShaderGlobal::set_color4(landCellShortDecodeY, dec_y_0);
  }

  // IObjEntityMgr interface
  virtual bool canSupportEntityClass(int entity_class) const
  {
    return prefabEntityClassId >= 0 && prefabEntityClassId == entity_class;
  }

  virtual IObjEntity *createEntity(const DagorAsset &asset, bool virtual_ent)
  {
    int pool_idx = prefabPool.findPool(asset);
    if (pool_idx < 0)
      pool_idx = prefabPool.addPool(new PrefabEntityPool(asset));

    if (virtual_ent)
      return prefabPool.getVirtualEnt(pool_idx);

    if (!prefabPool.canAddEntity(pool_idx))
      return NULL;

    PrefabEntity *ent = prefabPool.allocEntity();
    if (!ent)
      ent = new PrefabEntity;

    ent->setFlags(PrefabEntity::FLG_CLIP_GAME | PrefabEntity::FLG_CLIP_EDITOR);
    prefabPool.addEntity(ent, pool_idx);
    // debug("create ent: %p", ent);
    return ent;
  }

  virtual IObjEntity *cloneEntity(IObjEntity *origin)
  {
    PrefabEntity *o = reinterpret_cast<PrefabEntity *>(origin);
    if (!prefabPool.canAddEntity(o->poolIdx))
      return NULL;

    PrefabEntity *ent = prefabPool.allocEntity();
    if (!ent)
      ent = new PrefabEntity;

    ent->setFlags(PrefabEntity::FLG_CLIP_GAME | PrefabEntity::FLG_CLIP_EDITOR);
    prefabPool.addEntity(ent, o->poolIdx);
    // debug("clone ent: %p", ent);
    return ent;
  }

  // IGatherStaticGeometry interface
  virtual void gatherStaticVisualGeometry(StaticGeometryContainer &cont) { gatherStaticVisualGeometryForStage(cont, 0); }
  virtual void gatherStaticVisualGeometryForStage(StaticGeometryContainer &cont, int stage_idx)
  {
    dag::ConstSpan<PrefabEntityPool *> p = prefabPool.getPools();
    int st_mask = IObjEntityFilter::getSubTypeMask(IObjEntityFilter::STMASK_TYPE_EXPORT);
    uint64_t lh_mask = stage_idx == prefabs::PrefabStage::LOFT_MASK_EXPORT ? IObjEntityFilter::getLayerHiddenMask() : 0;
    for (int i = 0; i < p.size(); i++)
      if (p[i]->checkStageIdx(stage_idx))
        p[i]->addGeometry(cont, StaticGeometryNode::FLG_RENDERABLE, 0, st_mask, lh_mask);
  }
  virtual void gatherStaticEnviGeometry(StaticGeometryContainer &cont) {}
  virtual void gatherStaticCollisionGeomGame(StaticGeometryContainer &cont)
  {
    dag::ConstSpan<PrefabEntityPool *> p = prefabPool.getPools();
    int st_mask = IObjEntityFilter::getSubTypeMask(IObjEntityFilter::STMASK_TYPE_COLLISION);
    for (int i = 0; i < p.size(); i++)
      p[i]->addGeometry(cont, StaticGeometryNode::FLG_COLLIDABLE, PrefabEntity::FLG_CLIP_GAME, st_mask);
  }
  virtual void gatherStaticCollisionGeomEditor(StaticGeometryContainer &cont)
  {
    dag::ConstSpan<PrefabEntityPool *> p = prefabPool.getPools();
    int st_mask = IObjEntityFilter::getSubTypeMask(IObjEntityFilter::STMASK_TYPE_COLLISION);
    for (int i = 0; i < p.size(); i++)
      p[i]->addGeometry(cont, StaticGeometryNode::FLG_COLLIDABLE, PrefabEntity::FLG_CLIP_EDITOR, st_mask);
  }

  // IDagorEdCustomCollider interface
  virtual bool traceRay(const Point3 &p0, const Point3 &dir, real &maxt, Point3 *norm)
  {
    if (maxt <= 0)
      return false;

    dag::ConstSpan<PrefabEntityPool *> p = prefabPool.getPools();
    bool ret = false;
    for (int i = 0; i < p.size(); i++)
      if (p[i]->traceRay(p0, dir, maxt, norm))
        ret = true;
    return ret;
  }
  virtual bool shadowRayHitTest(const Point3 &p0, const Point3 &dir, real maxt)
  {
    if (maxt <= 0)
      return false;

    dag::ConstSpan<PrefabEntityPool *> p = prefabPool.getPools();
    for (int i = 0; i < p.size(); i++)
      if (p[i]->shadowRayHitTest(p0, dir, maxt))
        return true;
    return false;
  }
  virtual const char *getColliderName() const { return getServiceFriendlyName(); }
  virtual bool isColliderVisible() const { return visible; }

  // IOccluderGeomProvider
  virtual void gatherOccluders(Tab<TMatrix> &occl_boxes, Tab<Quad> &occl_quads)
  {
    dag::ConstSpan<PrefabEntityPool *> p = prefabPool.getPools();
    for (int i = 0; i < p.size(); i++)
      p[i]->gatherOccluders(occl_boxes, occl_quads);
  }
  virtual void renderOccluders(const Point3 &camPos, float max_dist) {}

protected:
  MultiPrefabEntityPool prefabPool;
  bool visible;
};


void init_prefabmgr_service(const DataBlock &app_blk)
{
  if (!IDaEditor3Engine::get().checkVersion())
  {
    debug_ctx("Incorrect version!");
    return;
  }
  if (const char *re_src = app_blk.getBlockByNameEx("projectDefaults")->getBlockByNameEx("prefabMgr")->getStr("clipmapShaderRE", NULL))
  {
    check_clipmap_shclass_re = new RegExp;
    check_clipmap_shclass_re->compile(re_src, "");
  }

  if (
    const char *re_src = app_blk.getBlockByNameEx("projectDefaults")->getBlockByNameEx("prefabMgr")->getStr("loftMaskShaderRE", NULL))
  {
    check_loft_mask_shclass_re = new RegExp;
    check_loft_mask_shclass_re->compile(re_src, "");
  }

  if (
    const char *re_src = app_blk.getBlockByNameEx("projectDefaults")->getBlockByNameEx("prefabMgr")->getStr("waterProjShaderRE", NULL))
  {
    check_water_proj_shclass_re = new RegExp;
    check_water_proj_shclass_re->compile(re_src, "");
  }
  if (const DataBlock *b = app_blk.getBlockByNameEx("projectDefaults")->getBlockByName("prefabMgr"))
    for (int i = 0, nid = app_blk.getNameId("applyObjScale"); i < b->blockCount(); i++)
      if (b->getBlock(i)->getBlockNameId() == nid)
      {
        PrefabObjScaleSetup &s = obj_scale_setup.push_back();
        s.shaderRE = new RegExp;
        if (!s.shaderRE->compile(b->getBlock(i)->getStr("shaderRE", "")))
        {
          DAEDITOR3.conError("bad shaderRE=\"%s\" in application.blk", b->getBlock(i)->getStr("shaderRE", ""));
          del_it(s.shaderRE);
          obj_scale_setup.pop_back();
          continue;
        }
        s.tcMask = 0;
        for (int j = 0; j < 8; j++)
          if (b->getBlock(i)->getBool(String(0, "tc%d", j), false))
            s.tcMask |= 1 << j;
        if (!s.tcMask)
        {
          DAEDITOR3.conError("shaderRE=\"%s\" in application.blk is not applied to any TC", b->getBlock(i)->getStr("shaderRE", ""));
          del_it(s.shaderRE);
          obj_scale_setup.pop_back();
          continue;
        }
      }

  IDaEditor3Engine::get().registerService(new (inimem) PrefabEntityManagementService);
}
