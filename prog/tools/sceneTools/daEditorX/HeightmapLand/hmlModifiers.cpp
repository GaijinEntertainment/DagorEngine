// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "hmlPlugin.h"
#include "hmlSplineObject.h"

#include <de3_hmapService.h>
#include <de3_interface.h>
#include <perfMon/dag_cpuFreq.h>

bool guard_det_border = false;

bool HmapLandPlugin::applyHmModifiers1(HeightMapStorage &hm, float gc_sz, bool gen_colors, bool reset_final, IBBox2 *out_dirty_clip,
  IBBox2 *out_sum_dirty, bool *out_colors_changed)
{
  if (!hm.changed && hm.srcChanged.isEmpty())
    return false;

  IBBox2 dirty_clip = hm.srcChanged;
  int mapSizeX = hm.getMapSizeX();
  int mapSizeY = hm.getMapSizeY();
  bool colors_changed = false;

  if (hm.changed)
  {
    dirty_clip += IPoint2(0, 0);
    dirty_clip += IPoint2(mapSizeX - 1, mapSizeY - 1);
  }
  if (&hm == &heightMapDet)
  {
    detRectC.clipBox(dirty_clip);
    if (reset_final && guard_det_border)
    {
      IBBox2 detRectCI;
      detRectCI = detRectC;
      detRectCI[0].x += detDivisor * 2;
      detRectCI[0].y += detDivisor * 2;
      detRectCI[1].x -= detDivisor * 2;
      detRectCI[1].y -= detDivisor * 2;
      detRectCI.clipBox(dirty_clip);
    }
  }

  if (dirty_clip[0].x < 0)
    dirty_clip[0].x = 0;
  if (dirty_clip[0].y < 0)
    dirty_clip[0].y = 0;
  if (dirty_clip[1].x > mapSizeX - 1)
    dirty_clip[1].x = mapSizeX - 1;
  if (dirty_clip[1].y > mapSizeY - 1)
    dirty_clip[1].y = mapSizeY - 1;
  if (dirty_clip.isEmpty())
    return false;

  hm.changed = false;
  hm.srcChanged.setEmpty();

  int x, y;
  int wss = waterMaskScale, wsd = &hm == &heightMapDet ? detDivisor : 1;

  if (reset_final && hasWaterSurface && waterMask.getW() && waterMask.getH())
  {
    float max_bottom_h = waterSurfaceLevel - minUnderwaterBottomDepth;
    for (y = dirty_clip[1].y; y >= dirty_clip[0].y; --y)
    {
      for (x = dirty_clip[0].x; x <= dirty_clip[1].x; ++x)
      {
        float h = hm.getInitialData(x, y);
        hm.setFinalData(x, y, waterMask.get(x * wss / wsd, y * wss / wsd) && h > max_bottom_h ? max_bottom_h : h);
      }
    }
  }
  else if (reset_final)
    for (y = dirty_clip[1].y; y >= dirty_clip[0].y; --y)
      for (x = dirty_clip[0].x; x <= dirty_clip[1].x; ++x)
        hm.resetFinalData(x, y);

  IBBox2 sum_dirty;
  for (x = 0; x < objEd.splinesCount(); x++)
  {
    SplineObject *spline = objEd.getSpline(x);
    if (spline->isAffectingHmap())
    {
      IBBox2 dirty;

      spline->applyHmapModifier(hm, heightMapOffset, gc_sz, dirty, dirty_clip);

      if (!gen_colors)
        continue;

      dirty_clip.clipBox(dirty);
      if (dirty.isEmpty())
        continue;

      sum_dirty += dirty;
      colors_changed = true;
    }
  }

  if (hasWaterSurface && waterMask.getW() && waterMask.getH())
  {
    float max_bottom_h = waterSurfaceLevel - minUnderwaterBottomDepth;
    for (y = dirty_clip[1].y; y >= dirty_clip[0].y; --y)
    {
      for (x = dirty_clip[0].x; x <= dirty_clip[1].x; ++x)
      {
        float h = heightMap.getFinalData(x, y);
        if (waterMask.get(x * wss / wsd, y * wss / wsd) && h > max_bottom_h)
          heightMap.setFinalData(x, y, max_bottom_h);
      }
    }
  }
  updateHeightMapTex(&hm == &heightMapDet, &dirty_clip);

  if (out_colors_changed)
    *out_colors_changed = colors_changed;
  if (out_dirty_clip)
    *out_dirty_clip = dirty_clip;
  if (out_sum_dirty)
    *out_sum_dirty = sum_dirty;
  return true;
}

void HmapLandPlugin::applyHmModifiers(bool gen_colors, bool reset_final, bool finished)
{
  bool colors_changed = false;
  IBBox2 dirty_clip, sum_dirty;
  int64_t reft = ref_time_ticks_qpc();

  bool changed_main = applyHmModifiers1(heightMap, gridCellSize, gen_colors, reset_final, &dirty_clip, &sum_dirty, &colors_changed);
  bool changed_det =
    !detDivisor ? false : applyHmModifiers1(heightMapDet, gridCellSize / detDivisor, false, reset_final, NULL, NULL, NULL);

  if (get_time_usec_qpc(reft) > 1000000)
    DAEDITOR3.conWarning("applyHmModifiers(gen_colors=%d, reset_final=%d, finished=%d) took %.1f sec to process %@", (int)gen_colors,
      (int)reset_final, (int)finished, get_time_usec_qpc(reft) / 1e6, sum_dirty);
  if (!changed_main && !changed_det)
    return;

  if (colors_changed)
  {
    generateLandColors(&sum_dirty);
    recalcLightingInRect(sum_dirty);
    // resetRenderer();
    hmlService->invalidateClipmap(false);
    DAGORED2->invalidateViewportCache();
  }
  if (finished)
    onLandRegionChanged(dirty_clip[0].x * lcmScale, dirty_clip[0].y * lcmScale, dirty_clip[1].x * lcmScale, dirty_clip[1].y * lcmScale,
      false, finished);
}

void HmapLandPlugin::invalidateFinalBox(const BBox3 &box)
{
  IBBox2 box2(IPoint2(floor((box[0].x - heightMapOffset.x) / gridCellSize), floor((box[0].z - heightMapOffset.y) / gridCellSize)),
    IPoint2(ceil((box[1].x - heightMapOffset.x) / gridCellSize), ceil((box[1].z - heightMapOffset.y) / gridCellSize)));
  heightMap.srcChanged += box2;
  if (detDivisor)
  {
    box2[0] *= detDivisor;
    box2[1] *= detDivisor;
    detRectC.clipBox(box2);
    heightMapDet.srcChanged += box2;
  }
}

struct UndoCollapse : public UndoRedoObject
{
  PtrTab<SplineObject> spl;
  struct HmapHeights
  {
    HmapBitmap bmp;
    Tab<float> oldHt, newHt;
    HeightMapStorage *stor;
  } hmap[2];

  UndoCollapse(HeightMapStorage *hmap_main, HeightMapStorage *hmap_det, dag::ConstSpan<SplineObject *> _spl)
  {
    spl.Tab<Ptr<SplineObject>>::operator=(make_span_const((const Ptr<SplineObject> *)_spl.data(), _spl.size()));
    hmap[0].stor = hmap_main;
    hmap[1].stor = hmap_det;
    for (int i = 0; i < 2; i++)
      if (hmap[i].stor)
        hmap[i].bmp.resize(hmap[i].stor->getMapSizeX(), hmap[i].stor->getMapSizeY());
  }
  void finalizeChanges(const IBBox2 coll_area[2], bool use_bmp)
  {
    for (int i = 0; i < 2; i++)
    {
      HmapHeights &hh = hmap[i];
      if (!hh.stor)
        continue;
      int map_x = hh.stor->getMapSizeX(), map_y = hh.stor->getMapSizeY();
      IBBox2 bb = coll_area[i];
      if (bb[1].x + 1 < map_x)
        bb[1].x++;
      if (bb[1].y + 1 < map_y)
        bb[1].y++;

      hh.oldHt.reserve(bb.width().x * bb.width().y);
      hh.newHt.reserve(bb.width().x * bb.width().y);
      for (int y = bb[0].y; y < bb[1].y; y++)
        for (int x = bb[0].x; x < bb[1].x; x++)
        {
          if (use_bmp && !hh.bmp.get(x, y))
            continue;
          if (!use_bmp && hh.stor->getInitialData(x, y) == hh.stor->getFinalData(x, y))
            continue;

          hh.bmp.set(x, y);
          hh.oldHt.push_back(hh.stor->getInitialData(x, y));
          hh.newHt.push_back(hh.stor->getFinalData(x, y));
          hh.stor->setInitialData(x, y, hh.stor->getFinalData(x, y));
        }
      hh.oldHt.shrink_to_fit();
      hh.newHt.shrink_to_fit();
    }

    for (int i = 0; i < spl.size(); i++)
    {
      if (spl[i]->isPoly())
        spl[i]->setPolyHmapAlign(false);
      else
        spl[i]->setModifType(MODIF_NONE);
    }
  }

  virtual void restore(bool save_redo)
  {
    for (int i = 0; i < 2; i++)
    {
      HmapHeights &hh = hmap[i];
      if (!hh.stor)
        continue;
      for (int y = 0, ord = 0; y < hh.stor->getMapSizeY(); y++)
        for (int x = 0; x < hh.stor->getMapSizeX(); x++)
          if (hh.bmp.get(x, y))
            hh.stor->setInitialData(x, y, hh.oldHt[ord++]);
    }
    for (int i = 0; i < spl.size(); i++)
    {
      if (spl[i]->isPoly())
        spl[i]->setPolyHmapAlign(true);
      else
        spl[i]->setModifType(MODIF_HMAP);
    }
    HmapLandPlugin::self->invalidateObjectProps();
  }

  virtual void redo()
  {
    for (int i = 0; i < 2; i++)
    {
      HmapHeights &hh = hmap[i];
      if (!hh.stor)
        continue;
      for (int y = 0, ord = 0; y < hh.stor->getMapSizeY(); y++)
        for (int x = 0; x < hh.stor->getMapSizeX(); x++)
          if (hh.bmp.get(x, y))
            hh.stor->setInitialData(x, y, hh.newHt[ord++]);
    }
    for (int i = 0; i < spl.size(); i++)
    {
      if (spl[i]->isPoly())
        spl[i]->setPolyHmapAlign(false);
      else
        spl[i]->setModifType(MODIF_NONE);
    }
    HmapLandPlugin::self->invalidateObjectProps();
  }

  virtual size_t size()
  {
    return data_size(spl) + data_size(hmap[0].oldHt) + data_size(hmap[0].newHt) + data_size(hmap[1].oldHt) + data_size(hmap[1].newHt);
  }
  virtual void accepted() {}
  virtual void get_description(String &s) { s = "UndoCollapse"; }
};

void HmapLandPlugin::copyFinalHmapToInitial()
{
  Tab<SplineObject *> spl;
  for (int i = 0; i < objEd.objectCount(); i++)
    if (SplineObject *o = RTTI_cast<SplineObject>(objEd.getObject(i)))
      if (o->isAffectingHmap())
        spl.push_back(o);

  objEd.getUndoSystem()->begin();
  UndoCollapse *undo = new UndoCollapse(&heightMap, &heightMapDet, spl);

  IBBox2 collArea[2];
  collArea[0][0].set(0, 0);
  collArea[0][1].set(heightMap.getMapSizeX(), heightMap.getMapSizeY());
  collArea[1] = detRectC;
  undo->finalizeChanges(collArea, false);

  objEd.getUndoSystem()->put(undo);
  objEd.getUndoSystem()->accept("Copy final HMAP to initial");

  updateHeightMapTex(false);
  if (detDivisor)
    updateHeightMapTex(true);
}

void HmapLandPlugin::collapseModifiers(dag::ConstSpan<SplineObject *> collapse_splines)
{
  UndoCollapse *undo = new UndoCollapse(&heightMap, &heightMapDet, collapse_splines);
  objEd.getUndoSystem()->begin();

  IBBox2 collArea[2];
  int hw = heightMap.getMapSizeX();
  int hh = heightMap.getMapSizeY();

  for (int i = 0; i < collapse_splines.size(); i++)
  {
    IBBox2 dirty;
    collapse_splines[i]->applyHmapModifier(heightMap, heightMapOffset, gridCellSize, dirty,
      IBBox2(IPoint2(0, 0), IPoint2(hw - 1, hh - 1)), &undo->hmap[0].bmp);
    collArea[0] += dirty;

    if (detDivisor)
    {
      collapse_splines[i]->applyHmapModifier(heightMapDet, heightMapOffset, gridCellSize / detDivisor, dirty, detRectC,
        &undo->hmap[1].bmp);
      collArea[1] += dirty;
    }
  }

  undo->finalizeChanges(collArea, true);
  objEd.getUndoSystem()->put(undo);
  objEd.getUndoSystem()->accept("Collapse modifier(s)");

  updateHeightMapTex(false, &collArea[0]);
  if (detDivisor)
    updateHeightMapTex(true, &collArea[1]);
}
