// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "spline.h"
#include <assets/asset.h>
#include <de3_assetService.h>
#include <de3_heightmap.h>

int SplineGenEntity::loftChangesCount = 0;
ISplineGenService::LayerIndexList SplineGenEntity::cachedLoftLayers;
OAHashNameMap<true> SplineGenEntity::splNames;

extern void invalidate_loft_render_cache();

static IHeightmap *request_hmap_object()
{
  static IHeightmap *hmap = nullptr;
  static bool requested = false;
  if (hmap || requested)
    return hmap;
  hmap = EDITORCORE->getInterfaceEx<IHeightmap>();
  requested = true;
  return hmap;
}

void SplineGenEntity::setEditLayerIdx(int t)
{
  if (editLayerIdx != t)
    invalidate_loft_render_cache();
  editLayerIdx = t;
  for (auto &p : entPools)
    for (auto *e : p.entPool)
      if (e)
        e->setEditLayerIdx(t);
}

void SplineGenEntity::destroy()
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

void SplineGenEntity::setup(const DagorAsset &asset)
{
  nameId = splNames.addNameId(asset.getName());
  splineClass = nullptr;
  if (IAssetService *srv = EDITORCORE->queryEditorInterface<IAssetService>())
    splineClass = srv->getSplineClassData(asset.getName());
}
void SplineGenEntity::copyFrom(const SplineGenEntity &e)
{
  if (loftGeom)
    invalidate_loft_render_cache();
  nameId = e.nameId;
  splineClass = nullptr;
  if (IAssetService *srv = EDITORCORE->queryEditorInterface<IAssetService>())
    splineClass = srv->addRefSplineClassData(e.splineClass);
  clear_and_shrink(entPools);
  loftGeom = nullptr;
  clear_and_shrink(loftSeg);
}

void SplineGenEntity::transform(TMatrix &tm)
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

BBox3 SplineGenEntity::calcWABB(bool add_geom, bool add_entities) const
{
  BBox3 bb;
  if (add_geom)
    bb = loftGeomBox;
  if (add_entities)
  {
    TMatrix m;
    for (auto &p : entPools)
      for (auto *e : p.entPool)
        if (e)
        {
          e->getTm(m);
          bb += m * e->getBbox();
        }
  }
  return bb;
}

void SplineGenEntity::resetUsedPoolsEntities()
{
  for (auto &p : entPools)
    p.resetUsedEntities();
}
void SplineGenEntity::deleteUnusedPoolsEntities()
{
  for (auto &p : entPools)
    p.deleteUnusedEntities();
}

GeomObject &SplineGenEntity::getClearedLoftGeom()
{
  invalidate_loft_render_cache();
  loftGeomBox.setempty();
  loftSeg.clear();
  if (!loftGeom)
    return *(loftGeom = new (midmem) GeomObject);
  loftGeom->clear();
  return *loftGeom;
}

void SplineGenEntity::removeLoftGeom()
{
  invalidate_loft_render_cache();
  loftSeg.clear();
  del_it(loftGeom);
  updateLoftBox();
  if (hasNonZeroLayers)
    loftChangesCount++;
  hasNonZeroLayers = false;
}

void SplineGenEntity::updateLoftBox()
{
  invalidate_loft_render_cache();
  if (loftGeom)
    loftGeomBox = loftGeom->getBoundBox();
  else
    loftGeomBox.setempty();
}


bool SplineEntity::SplineData::load(const DataBlock &blk)
{
  static int spline_atype = DAEDITOR3.getAssetTypeId("spline");
  static int land_atype = DAEDITOR3.getAssetTypeId("land");

  isPoly = strcmp(blk.getBlockName(), "polygon") == 0;

  (isPoly ? landAssetName : splAssetName) = blk.getStr("blkGenName", "");
  landClass = *landAssetName ? DAEDITOR3.getAssetByName(landAssetName, land_atype) : nullptr;
  if (landClass)
    splAssetName = landClass->props.getStr("splineClassAssetName", "");

  rndSeed = blk.getInt("rseed", 0);
  perInstSeed = blk.getInt("perInstSeed", 0);

  layerOrder = blk.getInt("layerOrder", 0);
  scaleTcAlong = blk.getReal("scaleTcAlong", 1.0f);

  maySelfCross = blk.getBool("maySelfCross", false);
  placeOnCollision = (blk.getInt("modifType", 0) == 1);

  exportable = blk.getBool("exportable", false);
  cornerType = blk.getInt("cornerType", 0);
  useForNavMesh = blk.getBool("useForNavMesh", false);
  navMeshStripeWidth = blk.getReal("navMeshStripeWidth", 20);

  poly.objOffs = blk.getPoint2("polyObjOffs", Point2(0, 0));
  poly.objRot = blk.getReal("polyObjRot", 0);
  poly.smooth = blk.getBool("polySmooth", false);
  poly.curvStrength = blk.getReal("polyCurv", 1);
  poly.minStep = blk.getReal("polyMinStep", 30);
  poly.maxStep = blk.getReal("polyMaxStep", 1000);
  poly.tiledObjsWorldAnchor = blk.getBool("tiledObjsWorldAnchor", true);
  poly.plantedObjsWorldAnchor = blk.getBool("plantedObjsWorldAnchor", false);

  int nid = blk.getNameId("point");
  pt.reserve(blk.blockCount());
  ptAttr.reserve(blk.blockCount());
  for (int i = 0; i < blk.blockCount(); i++)
    if (blk.getBlock(i)->getBlockNameId() == nid)
    {
      const DataBlock &cb = *blk.getBlock(i);
      ISplineGenObj::SplinePt &p = pt.push_back();
      Attr &pa = ptAttr.push_back();
      const char *scname = cb.getBool("useDefSplineGen", true) ? splAssetName.str() : cb.getStr("blkGenName", "");
      if (isPoly && pt.size() == 1 && *scname)
        splAssetName = scname;

      p.pt = cb.getPoint3("pt", Point3(0, 0, 0));
      p.relIn = cb.getPoint3("in", Point3(0, 0, 0));
      p.relOut = cb.getPoint3("out", Point3(0, 0, 0));
      pa.attr.scale_h = cb.getReal("scale_h", 1.0f);
      pa.attr.scale_w = cb.getReal("scale_w", 1.0f);
      pa.attr.opacity = cb.getReal("opacity", 1.0f);
      pa.attr.tc3u = cb.getReal("tc3u", 1.0f);
      pa.attr.tc3v = cb.getReal("tc3v", 1.0f);
      pa.attr.followOverride = cb.getInt("followOverride", -1);
      pa.attr.roadBhvOverride = cb.getInt("roadBhvOverride", -1);
      p.cornerType = cb.getInt("cornerType", -2);
      pa.splineClass = *scname ? DAEDITOR3.getAssetByName(scname, spline_atype) : nullptr;

      lbb += p.pt;
      lbs += p.pt;
    }
  bool close_spline = (isPoly || blk.getBool("closed", false)) && pt.size() > 2;
  if (blk.getBool("forceCatmullRom", false) && pt.size() > 1)
  {
    BezierSpline3d spline;
    Tab<Point3> pts;
    pts.reserve(pt.size());
    for (auto &p : pt)
      pts.push_back(p.pt);
    spline.calculateCatmullRom(pts.data(), pts.size(), close_spline);

    for (unsigned si = 0; si < spline.segs.size(); si++)
    {
      Point3 v[4];
      spline.segs[si].calculateBack(v);
      pt[si].relOut = v[1] - v[0];
      pt[si].relIn = -pt[si].relOut;
    }
    if (pt.size() == spline.segs.size() + 1)
    {
      Point3 v[4];
      spline.segs.back().calculateBack(v);
      auto &p = pt.back();
      p.relIn = v[2] - v[3];
      p.relOut = -p.relIn;
    }
  }

  if (close_spline && pt.size() > 1)
    pt.push_back(ISplineGenObj::SplinePt(pt[0]));

  pt.shrink_to_fit();
  ptAttr.shrink_to_fit();
  return pt.size() > 1;
}
bool SplineEntity::SplineData::save(DataBlock &blk, const TMatrix &tm) const
{
  blk.setStr("blkGenName", isPoly ? landAssetName : splAssetName);
  blk.setInt("rseed", rndSeed);
  blk.setInt("perInstSeed", perInstSeed);

  blk.setInt("layerOrder", layerOrder);
  blk.setReal("scaleTcAlong", scaleTcAlong);

  blk.setBool("maySelfCross", maySelfCross);
  blk.setInt("modifType", placeOnCollision ? 1 : 0);

  blk.setBool("exportable", exportable);
  blk.setInt("cornerType", cornerType);
  blk.setBool("useForNavMesh", useForNavMesh);
  blk.setReal("navMeshStripeWidth", navMeshStripeWidth);

  blk.setPoint2("polyObjOffs", poly.objOffs);
  blk.setReal("polyObjRot", poly.objRot);
  blk.setBool("polySmooth", poly.smooth);
  blk.setReal("polyCurv", poly.curvStrength);
  blk.setReal("polyMinStep", poly.minStep);
  blk.setReal("polyMaxStep", poly.maxStep);

  if (pt.size() > ptAttr.size() && !isPoly)
    blk.setBool("closed", true);

  for (int i = 0; i < ptAttr.size(); i++)
  {
    DataBlock &cb = *blk.addNewBlock("point");
    const ISplineGenObj::SplinePt &p = pt[i];
    const Attr &pa = ptAttr[i];

    cb.setPoint3("pt", tm * p.pt);
    cb.setPoint3("in", tm % p.relIn);
    cb.setPoint3("out", tm % p.relOut);
    cb.setReal("scale_h", pa.attr.scale_h);
    cb.setReal("scale_w", pa.attr.scale_w);
    cb.setReal("opacity", pa.attr.opacity);
    cb.setReal("tc3u", pa.attr.tc3u);
    cb.setReal("tc3v", pa.attr.tc3v);
    cb.setInt("followOverride", pa.attr.followOverride);
    cb.setInt("roadBhvOverride", pa.attr.roadBhvOverride);
    cb.setInt("cornerType", p.cornerType);
    if (pa.splineClass)
    {
      if (strcmp(pa.splineClass->getName(), splAssetName) != 0 || (isPoly && i == 0))
      {
        cb.setBool("useDefSplineGen", false);
        cb.setStr("blkGenName", pa.splineClass->getName());
      }
    }
    else if (!splAssetName.empty())
    {
      cb.setBool("useDefSplineGen", false);
      cb.setStr("blkGenName", "");
    }
  }

  return true;
}

SplineEntity::~SplineEntity()
{
  for (auto &gp : ptGen)
    if (gp.e)
      gp.e->destroy();
  clear_and_shrink(ptGen);
  destroy_it(polyGen);
  sd = NULL;
}

bool SplineEntity::load(const DataBlock &blk)
{
  // debug("SplineEntity.load(%s)", blk.getStr("name", "n/a"));
  sd = new SplineData;
  if (sd->load(blk))
    return true;
  sd = nullptr;
  return false;
}

void SplineEntity::updateSpline()
{
  int pts_num = sd->pt.size();

  SmallTab<Point3, TmpmemAlloc> pts;
  clear_and_resize(pts, pts_num * 3);

  for (int pi = 0; pi < pts_num; ++pi)
  {
    int wpi = pi % sd->pt.size();
    pts[pi * 3 + 1] = tm * sd->pt[wpi].pt;
    if (sd->isPoly && !sd->poly.smooth)
      pts[pi * 3 + 2] = pts[pi * 3] = pts[pi * 3 + 1];
    else
    {
      pts[pi * 3] = tm * (sd->pt[wpi].pt + SplineGenEntity::getPtEffRelBezierIn(sd->pt, wpi, sd->cornerType));
      pts[pi * 3 + 2] = tm * (sd->pt[wpi].pt + SplineGenEntity::getPtEffRelBezierOut(sd->pt, wpi, sd->cornerType));
    }
  }

  bezierSpline.calculate(pts.data(), pts.size(), false);
}

void SplineEntity::generateData()
{
  using splineclass::Attr;
  static int splineSubTypeId = DAEDITOR3.registerEntitySubTypeId("spline_cls");

  Tab<Attr> splineScales(midmem);
  for (int i = 0; i <= sd->ptAttr.size(); i++)
    splineScales.push_back(sd->ptAttr[i % sd->ptAttr.size()].attr);

  String name(0, "s%p", this);
  if (IPolygonGenObj *gen = polyGen ? polyGen->queryInterface<IPolygonGenObj>() : nullptr)
  {
    Tab<Point3> poly_pts;
    gen->buildSmoothPolyPoints(poly_pts, bezierSpline, sd->poly);
    gen->triangulatePoly(poly_pts, sd->poly, name, nullptr);
    gen->placeObjectsInsidePolygon(make_span(poly_pts), sd->poly, request_hmap_object());
    // clear these since we are not going to show them for debug now
    clear_and_shrink(gen->geom.centerTri);
    clear_and_shrink(gen->geom.borderTri);
    clear_and_shrink(gen->geom.verts);
  }

  for (auto &gp : ptGen)
    if (ISplineGenObj *gen = gp.e->queryInterface<ISplineGenObj>())
      gen->generateLoftSegments(bezierSpline, name, gp.startIdx, gp.endIdx, sd->placeOnCollision,
        make_span_const(splineScales).subspan(gp.startIdx, gp.endIdx - gp.startIdx + 1), sd->scaleTcAlong);

  BezierSpline3d onGndSpline;
  BezierSpline3d &effSpline = sd->placeOnCollision ? onGndSpline : bezierSpline;

  if (sd->placeOnCollision)
  {
    for (auto &gp : ptGen)
      if (ISplineGenObj *gen = gp.e->queryInterface<ISplineGenObj>())
        for (auto &p : gen->entPools)
          p.resetUsedEntities();

    if (ISplineGenService *splSrv = EDITORCORE->queryEditorInterface<ISplineGenService>())
      splSrv->build_ground_spline(onGndSpline, sd->pt, tm, sd->cornerType, sd->isPoly, sd->poly.smooth,
        sd->pt.size() > sd->ptAttr.size());
  }

  for (auto &gp : ptGen)
    if (ISplineGenObj *gen = gp.e->queryInterface<ISplineGenObj>())
      gen->generateObjects(effSpline, gp.startIdx, gp.endIdx, splineSubTypeId, rndSeed, instSeed, nullptr);
}

void SplineEntity::setTm(const TMatrix &_tm)
{
  if (gizmoEnabled)
  {
    TMatrix transf;
    transf = _tm * inverse(tm);
    tm = _tm;
    for (auto &gp : ptGen)
      if (ISplineGenObj *gen = gp.e->queryInterface<ISplineGenObj>())
        gen->transform(transf);
    if (IPolygonGenObj *gen = polyGen ? polyGen->queryInterface<IPolygonGenObj>() : nullptr)
      gen->transform(transf);
    return;
  }
  tm = _tm;
  updateSpline();
  generateData();
}

void SplineEntity::setEditLayerIdx(int t)
{
  editLayerIdx = t;
  if (polyGen)
    polyGen->setEditLayerIdx(t);
  for (auto &gp : ptGen)
    gp.e->setEditLayerIdx(t);
}
void SplineEntity::setGizmoTranformMode(bool enabled)
{
  if (gizmoEnabled == enabled)
    return;
  gizmoEnabled = enabled;
  if (!enabled)
  {
    updateSpline();
    generateData();
  }
}

IObjEntity *SplineEntity::createSplineEntityInstance()
{
  if (!sd)
    return nullptr;

  SplineEntity *e = new SplineEntity;
  e->sd = sd;
  e->rndSeed = sd->rndSeed;
  e->instSeed = sd->perInstSeed;
  e->ptGen.reserve(sd->ptAttr.size());

  DagorAsset *pa = nullptr;
  for (int i = 0; i < (int)sd->pt.size() - 1; i++)
    if (pa != sd->ptAttr[i].splineClass)
    {
      pa = sd->ptAttr[i].splineClass;
      if (IObjEntity *gen = pa ? DAEDITOR3.createEntity(*pa) : nullptr)
      {
        GenPart &gp = e->ptGen.push_back();
        gp.e = gen;
        gp.e->setEditLayerIdx(editLayerIdx);
        gp.startIdx = i;
        gp.endIdx = i + 1;
      }
    }
    else if (sd->ptAttr[i].splineClass)
      e->ptGen.back().endIdx++;

  if (sd->isPoly && !sd->landAssetName.empty())
    if ((e->polyGen = DAEDITOR3.createEntity(*sd->landClass)) != nullptr)
      e->polyGen->setEditLayerIdx(editLayerIdx);

  // for (auto &gp : e->ptGen)
  //   debug("%p.genPart %p: %d,%d (%s)", e, gp.e, gp.startIdx, gp.endIdx, sd->ptAttr[gp.startIdx].splineClass->getName());
  // debug("isPoly=%d landAssetName=%s -> e=%p polyGen=%p", sd->isPoly, sd->landAssetName, e, e->polyGen);
  return e;
}
bool SplineEntity::saveSplineTo(DataBlock &out_blk) const
{
  if (!sd)
    return false;
  out_blk.clearData();
  return sd->save(*out_blk.addBlock(sd->isPoly ? "polygon" : "spline"), tm);
}
BBox3 SplineEntity::calcWABB(bool spline_path, bool geom, bool entities) const
{
  BBox3 bb;
  if (spline_path)
    bb = tm * getBbox();
  if (IPolygonGenObj *gen = polyGen ? polyGen->queryInterface<IPolygonGenObj>() : nullptr)
    bb += gen->calcWABB(geom, entities);
  for (auto &gp : ptGen)
    if (ISplineGenObj *gen = gp.e ? gp.e->queryInterface<ISplineGenObj>() : nullptr)
      bb += gen->calcWABB(geom, entities);
  return bb;
}
