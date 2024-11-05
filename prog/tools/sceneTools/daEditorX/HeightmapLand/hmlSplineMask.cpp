// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "hmlPlugin.h"
#include "landClassSlotsMgr.h"
#include <de3_genObjUtil.h>
#include <de3_rasterizePoly.h>
#include <de3_splineClassData.h>
#include <de3_genObjData.h>
#include "hmlSplineObject.h"
#include "hmlSplinePoint.h"
#include <EditorCore/ec_IEditorCore.h>
#include <math/dag_bezierPrec.h>
#include <perfMon/dag_cpuFreq.h>
#include <libTools/math/bitrender.h>
#include <sceneRay/dag_sceneRay.h>

using editorcore_extapi::dagGeom;

objgenerator::WorldHugeBitmask bm_loft_mask[LAYER_ORDER_MAX], bm_poly_mask;
//! allocated for this DLL plugin (but should be avoided in future)
objgenerator::WorldHugeBitmask objgenerator::lcmapExcl, objgenerator::splgenExcl;


static void build_loft_sizes(Tab<Point4> &lso, splineclass::LoftGeomGenData *g)
{
  if (!g || !g->loft.size() || !g->foundationGeom)
  {
    lso.resize(0);
    return;
  }

  lso.resize(g->loft.size());
  for (int i = 0; i < lso.size(); i++)
  {
    lso[i] = Point4(g->loft[i].offset.x, g->loft[i].offset.y, 0, 0);
    for (int j = 0; j < g->loft[i].shapePts.size(); j++)
    {
      inplace_min(lso[i].z, g->loft[i].shapePts[j].x);
      inplace_max(lso[i].w, g->loft[i].shapePts[j].x);
    }
  }
}

static float calc_loft_rad(splineclass::LoftGeomGenData *g)
{
  if (!g || !g->loft.size() || !g->foundationGeom)
    return -1;

  float rad = 0;
  for (int i = 0; i < g->loft.size(); i++)
    for (int j = 0; j < g->loft[i].shapePts.size(); j++)
      inplace_max(rad, fabs(g->loft[i].shapePts[j].x) + max(fabs(g->loft[i].offset.x), fabs(g->loft[i].offset.y)));
  return rad;
}

static void saveBmTiff(objgenerator::WorldHugeBitmask &bm, const char *name)
{
  if (!bm.w || !bm.h)
    return;
  IBitMaskImageMgr::BitmapMask img;
  HmapLandPlugin::bitMaskImgMgr->createBitMask(img, (bm.w + 7) & ~7, bm.h, 1);

  for (int y = 0; y < bm.h; y++)
    for (int x = 0; x < bm.w; x++)
      img.setMaskPixel1(x, y, bm.bm->get(x, y) ? 128 : 0);

  HmapLandPlugin::bitMaskImgMgr->saveImage(img, ".", name);
  HmapLandPlugin::bitMaskImgMgr->destroyImage(img);
}

bool HmapLandPlugin::rebuildSplinesPolyBitmask(BBox2 &out_dirty_box)
{
  int t0 = get_time_msec();
  objgenerator::WorldHugeBitmask bmPoly;
  int poly_count = 0;

  BBox2 bb_poly;
  for (int i = 0; i < objEd.splinesCount(); i++)
  {
    SplineObject *sobj = objEd.getSpline(i);
    if (!sobj->isPoly())
      continue;
    const objgenerator::LandClassData *lcd = sobj->getLandClass();
    if (!lcd || !lcd->data || !lcd->data->genGeom || !lcd->data->genGeom->foundationGeom)
      continue;
    for (int i = 0; i < sobj->points.size(); i++)
    {
      Point3 p = sobj->points[i]->getPt();
      bb_poly += Point2(p.x - 2, p.z - 2);
      bb_poly += Point2(p.x + 2, p.z + 2);
    }
  }
  if (bb_poly.isempty())
    goto update_map;

  bmPoly.initEven(bb_poly[0].x, bb_poly[0].y, bb_poly[1].x, bb_poly[1].y, 8192, 1.0);
  // DAEDITOR3.conRemark("poly: %dx%d, scale=%.3f", bmPoly.w, bmPoly.h, bmPoly.scale);

  for (int i = 0; i < objEd.splinesCount(); i++)
  {
    SplineObject *sobj = objEd.getSpline(i);
    if (!sobj->isPoly())
      continue;
    const objgenerator::LandClassData *lcd = sobj->getLandClass();
    if (lcd && lcd->data && lcd->data->genGeom && lcd->data->genGeom->foundationGeom)
    {
      rasterize_poly_2(*bmPoly.bm, sobj->points, bmPoly.ox, bmPoly.oz, bmPoly.scale);
      poly_count++;
    }
  }
  if (!poly_count)
    bmPoly.clear();

  // saveBmTiff(bmPoly, "spl_poly_mask.tif");

update_map:
  IBBox2 b;
  if (bm_poly_mask.updateFrom(bmPoly, b))
  {
    out_dirty_box[0].set(b[0].x / bmPoly.scale + bmPoly.ox, b[0].y / bmPoly.scale + bmPoly.oz);
    out_dirty_box[1].set(b[1].x / bmPoly.scale + bmPoly.ox, b[1].y / bmPoly.scale + bmPoly.oz);
    DAEDITOR3.conRemark("new poly map differs (%.1f,%.1f)-(%.1f,%.1f), %d polys for %dms", out_dirty_box[0].x, out_dirty_box[0].y,
      out_dirty_box[1].x, out_dirty_box[1].y, poly_count, get_time_msec() - t0);
    return true;
  }
  else
    DAEDITOR3.conRemark("new poly map is the same, build and compared for %dms", get_time_msec() - t0);
  return false;
}

bool HmapLandPlugin::rebuildSplinesLoftBitmask(int layer, BBox2 &out_dirty_box)
{
  int t0 = get_time_msec();
  objgenerator::WorldHugeBitmask bmLoft;
  Bitrender<objgenerator::WorldHugeBitmask> renderer;
  BezierSpline2d s2d_src;
  Tab<Point2> verts(tmpmem);
  int loft_count = 0;

  BBox2 bb_loft;
  for (int i = 0; i < objEd.splinesCount(); i++)
  {
    SplineObject *sobj = objEd.getSpline(i);
    if (sobj->isPoly() || sobj->getLayer() != layer)
      continue;

    sobj->getSplineXZ(s2d_src);
    BezierSplinePrec2d &s2d = toPrecSpline(s2d_src);

    float splineLen = s2d.getLength(), st;

    const splineclass::AssetData *a = sobj->points[0]->getSplineClass();
    int seg = 0;
    float rad = calc_loft_rad(a ? a->genGeom : NULL);

    for (float s = 0.0; s < splineLen; s += 1)
    {
      int n_seg = s2d.findSegment(s, st);
      if (seg != n_seg)
      {
        seg = n_seg;
        rad = calc_loft_rad(a ? a->genGeom : NULL);
      }
      if (rad > 0)
      {
        Point2 p = s2d.segs[n_seg].point(st);
        bb_loft += p + Point2(rad + 2, rad + 2);
        bb_loft += p - Point2(rad + 2, rad + 2);
      }
    }
  }
  if (bb_loft.isempty())
    goto update_map;

  bmLoft.initEven(bb_loft[0].x, bb_loft[0].y, bb_loft[1].x, bb_loft[1].y, 8192, 1.0);
  // DAEDITOR3.conRemark("loft[%d]: %dx%d, scale=%.3f", layer, bmLoft.w, bmLoft.h, bmLoft.scale);

  renderer.init(bmLoft.w, bmLoft.h, &bmLoft);

  for (int i = 0; i < objEd.splinesCount(); i++)
  {
    SplineObject *sobj = objEd.getSpline(i);
    if (sobj->isPoly() || sobj->getLayer() != layer)
      continue;

    if (sobj->splineChanged)
    {
      objEd.loftGeomCollider.setLoftLayer(layer);
      sobj->updateSpline(sobj->STAGE_GEN_LOFT);
      objEd.loftGeomCollider.setLoftLayer(-1);
    }

    bool has_loft = false;
    for (int j = 0, je = sobj->points.size(); j < je; j++)
    {
      ISplineGenObj *gen = sobj->points[j]->getSplineGen();
      if (!gen || !gen->loftGeom)
        continue;
      GeomObject *geom = gen->loftGeom;
      const splineclass::AssetData *a = sobj->points[j]->getSplineClass();
      if (!a || !a->genGeom || !a->genGeom->foundationGeom)
        continue;

      StaticSceneRayTracer *rt = dagGeom->geomObjectGetRayTracer(*geom);
      if (!rt)
        continue;

      verts.resize(rt->getVertsCount());
      for (int vi = 0; vi < verts.size(); vi++)
        verts[vi].set((rt->verts(vi).x - bmLoft.ox) * bmLoft.scale - 0.5, (rt->verts(vi).z - bmLoft.oz) * bmLoft.scale - 0.5);

      for (int fi = 0, fie = rt->getFacesCount(); fi < fie; fi++)
      {
        StaticSceneRayTracer::RTface &f = rt->faces(fi);
        renderer.draw_indexed_triangle(verts.data(), f.v[0], f.v[1], f.v[2]);
      }
      has_loft = true;
    }
    if (has_loft)
      loft_count++;
  }
  if (!loft_count)
    bmLoft.clear();

  // saveBmTiff(bmLoft, String(32, "spl_loft%d_mask.tif", layer));

update_map:
  IBBox2 b;
  if (bm_loft_mask[layer].updateFrom(bmLoft, b))
  {
    out_dirty_box[0].set(b[0].x / bmLoft.scale + bmLoft.ox, b[0].y / bmLoft.scale + bmLoft.oz);
    out_dirty_box[1].set(b[1].x / bmLoft.scale + bmLoft.ox, b[1].y / bmLoft.scale + bmLoft.oz);
    DAEDITOR3.conRemark("new loft[%d] map differs (%.1f,%.1f)-(%.1f,%.1f), %d polys, for %dms", layer, out_dirty_box[0].x,
      out_dirty_box[0].y, out_dirty_box[1].x, out_dirty_box[1].y, loft_count, get_time_msec() - t0);
    return true;
  }
  else
    DAEDITOR3.conRemark("new loft[%d] map is the same, built and compared for %dms", layer, get_time_msec() - t0);
  return false;
}

void HmapLandPlugin::rebuildSplinesBitmask(bool auto_regen)
{
  int t0 = get_time_msec();
  BBox2 bb_changed[1 + LAYER_ORDER_MAX], bb_full;
  bool poly_changed = rebuildSplinesPolyBitmask(bb_changed[0]);
  bool loft_changed = false;

  bb_full = bb_changed[0];
  for (int layer = 0; layer < LAYER_ORDER_MAX; ++layer)
  {
    if (loft_changed || poly_changed)
    {
      objEd.loftGeomCollider.setLoftLayer(layer);
      for (int i = 0; i < objEd.splinesCount(); ++i)
      {
        SplineObject *s = objEd.getSpline(i);
        if (!s || !s->isCreated() || s->isPoly() || s->getLayer() != layer)
          continue;

        for (int j = 0; j <= layer; j++)
          if (!bb_changed[j].isempty() && s->intersects(bb_changed[j], true))
          {
            s->splineChanged = true;
            s->pointChanged(-1);
            s->updateSpline(s->STAGE_GEN_LOFT);
            break;
          }
      }
      objEd.loftGeomCollider.setLoftLayer(-1);
    }
    if (rebuildSplinesLoftBitmask(layer, bb_changed[layer + 1]))
    {
      loft_changed = true;
      bb_full += bb_changed[layer + 1];
    }
  }

  int t1 = get_time_msec();
  objgenerator::WorldHugeBitmask bmExcl;
  objgenerator::WorldHugeBitmask bm2Excl;
  BezierSpline2d s2d_src;
  int spl_count = 0;
  int spl2_count = 0;

  const float sx = 0.5;
  bmExcl.initExplicit(heightMapOffset.x, heightMapOffset.y, sx, sx * gridCellSize * heightMap.getMapSizeX(),
    sx * gridCellSize * heightMap.getMapSizeY());
  bm2Excl.initExplicit(heightMapOffset.x, heightMapOffset.y, sx, sx * gridCellSize * heightMap.getMapSizeX(),
    sx * gridCellSize * heightMap.getMapSizeY());

  for (int i = 0; i < objEd.splinesCount(); i++)
  {
    SplineObject *sobj = objEd.getSpline(i);
    if (sobj->isPoly())
      continue;

    sobj->getSplineXZ(s2d_src);
    BezierSplinePrec2d &s2d = toPrecSpline(s2d_src);
    float splineLen = s2d.getLength(), st;

    const splineclass::AssetData *a = sobj->points[0]->getSplineClass();
    int seg = 0, hw0 = (a && a->sweepWidth > 0) ? int(a->sweepWidth / 2 * sx) : -1;
    int hw1 = (a && a->sweepWidth + a->addFuzzySweepHalfWidth > 0) ? int((a->sweepWidth / 2 + a->addFuzzySweepHalfWidth) * sx) : -1;
    bool has_width = false;

    for (float s = 0.0; s < splineLen; s += 0.5 / sx)
    {
      int n_seg = s2d.findSegment(s, st);
      if (seg != n_seg)
      {
        a = sobj->points[n_seg]->getSplineClass();
        hw0 = (a && a->sweepWidth > 0) ? int(a->sweepWidth / 2 * sx) : -1;
        hw1 = (a && a->sweepWidth + a->addFuzzySweepHalfWidth > 0) ? int((a->sweepWidth / 2 + a->addFuzzySweepHalfWidth) * sx) : -1;
        seg = n_seg;
      }

      if (hw1 < 0)
        continue;

      Point2 p = s2d.segs[n_seg].point(st);
      Point2 t = normalize(s2d.segs[n_seg].tang(st));
      Point2 n(t.y / sx, -t.x / sx);
      for (int i = -hw0 + 1; i <= hw0 - 1; i++)
        bmExcl.mark3x3(p.x + n.x * i, p.y + n.y * i);

      for (int i = hw0 + 1; i <= hw1; i++)
      {
        int thres = 0x2000 * (i - hw0) / (hw1 - hw0) + 0x5F00;
        int px = p.x + n.x * i, py = p.y + n.y * i;
        int seed = py * bmExcl.w + px;

        if (_rnd(seed) > thres)
          bmExcl.mark(px, py);

        px = p.x - n.x * i, py = p.y - n.y * i;
        seed = py * bmExcl.w + px;
        if (_rnd(seed) > thres)
          bmExcl.mark(px, py);
      }
      has_width = true;
    }
    if (has_width)
      spl_count++;

    a = sobj->points[0]->getSplineClass();
    seg = 0, hw0 = (a && a->sweep2Width > 0) ? int(a->sweep2Width / 2 * sx) : -1;
    hw1 = (a && a->sweep2Width + a->addFuzzySweep2HalfWidth > 0) ? int((a->sweep2Width / 2 + a->addFuzzySweep2HalfWidth) * sx) : -1;
    has_width = false;

    for (float s = 0.0; s < splineLen; s += 0.5 / sx)
    {
      int n_seg = s2d.findSegment(s, st);
      if (seg != n_seg)
      {
        a = sobj->points[n_seg]->getSplineClass();
        hw0 = (a && a->sweep2Width > 0) ? int(a->sweep2Width / 2 * sx) : -1;
        hw1 =
          (a && a->sweep2Width + a->addFuzzySweep2HalfWidth > 0) ? int((a->sweep2Width / 2 + a->addFuzzySweep2HalfWidth) * sx) : -1;
        seg = n_seg;
      }

      if (hw1 < 0)
        continue;

      Point2 p = s2d.segs[n_seg].point(st);
      Point2 t = normalize(s2d.segs[n_seg].tang(st));
      Point2 n(t.y / sx, -t.x / sx);
      for (int i = -hw0 + 1; i <= hw0 - 1; i++)
        bm2Excl.mark3x3(p.x + n.x * i, p.y + n.y * i);

      for (int i = hw0 + 1; i <= hw1; i++)
      {
        int thres = 0x2000 * (i - hw0) / (hw1 - hw0) + 0x5F00;
        int px = p.x + n.x * i, py = p.y + n.y * i;
        int seed = py * bm2Excl.w + px;

        if (_rnd(seed) > thres)
          bm2Excl.mark(px, py);

        px = p.x - n.x * i, py = p.y - n.y * i;
        seed = py * bm2Excl.w + px;
        if (_rnd(seed) > thres)
          bm2Excl.mark(px, py);
      }
      has_width = true;
    }
    if (has_width)
      spl2_count++;
  }
  DAEDITOR3.conRemark("rebuilt excl map %dx%d of %d(%d)/%d splines for %dms (%dms total for all maps)", bmExcl.w, bmExcl.h, spl_count,
    spl2_count, objEd.splinesCount(), get_time_msec() - t1, get_time_msec() - t0);

  if (!spl_count)
    bmExcl.clear();
  if (!spl2_count)
    bm2Excl.clear();
  // saveBmTiff(bmExcl, "spl_sweep_mask.tif");
  // saveBmTiff(bm2Excl, "spl_sweep2_mask.tif");

  if (IRendInstGenService *rigenSrv = DAGORED2->queryEditorInterface<IRendInstGenService>())
    rigenSrv->setSweepMask(nullptr, 0, 0, 1);

  t0 = get_time_msec();
  IBBox2 b;
  bool excl_changed = objgenerator::lcmapExcl.updateFrom(bmExcl, b);
  if (excl_changed)
  {
    bb_full += Point2(b[0].x / bmExcl.scale + bmExcl.ox, b[0].y / bmExcl.scale + bmExcl.oz);
    bb_full += Point2(b[1].x / bmExcl.scale + bmExcl.ox, b[1].y / bmExcl.scale + bmExcl.oz);
  }
  if (objgenerator::splgenExcl.updateFrom(bm2Excl, b))
  {
    bb_full += Point2(b[0].x / bm2Excl.scale + bmExcl.ox, b[0].y / bm2Excl.scale + bm2Excl.oz);
    bb_full += Point2(b[1].x / bm2Excl.scale + bmExcl.ox, b[1].y / bm2Excl.scale + bm2Excl.oz);
    excl_changed = true;
  }
  if (IRendInstGenService *rigenSrv = DAGORED2->queryEditorInterface<IRendInstGenService>())
    rigenSrv->setSweepMask(objgenerator::lcmapExcl.bm, objgenerator::lcmapExcl.ox, objgenerator::lcmapExcl.oz,
      objgenerator::lcmapExcl.scale);
  if (ISplineGenService *splSrv = EDITORCORE->queryEditorInterface<ISplineGenService>())
    splSrv->setSweepMask(objgenerator::splgenExcl.bm, objgenerator::splgenExcl.ox, objgenerator::splgenExcl.oz,
      objgenerator::splgenExcl.scale);

  if (excl_changed || poly_changed || loft_changed)
  {
    DAEDITOR3.conRemark("compared for %dms", get_time_msec() - t0);
    if (!auto_regen)
      return;

    t0 = get_time_msec();

    float s = landClsMap.getMapSizeX() / gridCellSize / heightMap.getMapSizeX();
    b[0].x = (bb_full[0].x - heightMapOffset.x) * s;
    b[0].y = (bb_full[0].y - heightMapOffset.y) * s;
    b[1].x = (bb_full[1].x - heightMapOffset.x) * s;
    b[1].y = (bb_full[1].y - heightMapOffset.y) * s;

    onLandRegionChanged(0, 0, landClsMap.getMapSizeX(), landClsMap.getMapSizeY(), true);
    DAEDITOR3.conRemark("regenerated objects in (%d,%d)-(%d,%d) for %dms", b[0].x, b[0].y, b[1].x, b[1].y, get_time_msec() - t0);
  }
  else
    DAEDITOR3.conRemark("new excl map is the same, compared for %dms", get_time_msec() - t0);
}
