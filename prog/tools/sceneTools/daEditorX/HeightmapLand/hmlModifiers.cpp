// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "hmlPlugin.h"
#include "hmlSplineObject.h"

#include <de3_hmapService.h>
#include <de3_interface.h>
#include <coolConsole/coolConsole.h>
#include <perfMon/dag_cpuFreq.h>

bool guard_det_border = false;

static inline real distance_point_to_line_segment_squared(float pt_x, float pt_y, float p1_x, float p1_y, float p2_x, float p2_y,
  float &out_t)
{
  float dp_x = pt_x - p1_x;
  float dp_y = pt_y - p1_y;
  float dir_x = p2_x - p1_x;
  float dir_y = p2_y - p1_y;
  real len2 = dir_x * dir_x + dir_y * dir_y;

  if (len2 == 0)
  {
    out_t = 0;
    return dp_x * dp_x + dp_y * dp_y;
  }

  real t = (dp_x * dir_x + dp_y * dir_y) / len2;

  if (t <= 0)
  {
    out_t = 0;
    return dp_x * dp_x + dp_y * dp_y;
  }
  if (t >= 1)
  {
    out_t = 1;
    float diff_x = pt_x - p2_x;
    float diff_y = pt_y - p2_y;
    return diff_x * diff_x + diff_y * diff_y;
  }

  out_t = t;

  real crossProduct = dp_x * dir_y - dp_y * dir_x;
  return (crossProduct * crossProduct) / len2;
}

struct SplineCache
{
  SplineObject *ptr;
  BBox3 bbox;
  float minDist;
  float maxDist;
  float maxDistSq;
};

struct SplineSegmentInfo
{
  Point3 start, end;
  Point3 segDir;
  real segLen;
  float minDist, maxDist;
  BBox3 bbox;
};

#define HB_SEGMENTS_PER_SPLINE 16
#define HB_SPLINES_PER_CELL    8

struct SpatialSegmentData
{
  uint32_t splineIdx;
  uint16_t segmentIndices[HB_SEGMENTS_PER_SPLINE];
  uint8_t entryCount;
};

// hardcoded 512 is ok, predictable memory
static constexpr int spatialGridSize = 512;

struct SpatialGrid
{
  Tab<SpatialSegmentData> grid;
  Tab<uint8_t> cellCounts; // SpatialSegmentData entries in cell (max is HB_SPLINES_PER_CELL)
  float cellSize;
  Point2 offset;
  int gridSizeX, gridSizeY;

  void init(float cell_size, Point2 off, int grid_size_x, int grid_size_y)
  {
    cellSize = cell_size;
    offset = off;
    gridSizeX = grid_size_x;
    gridSizeY = grid_size_y;

    int totalCells = gridSizeX * gridSizeY;
    grid.resize(totalCells * HB_SPLINES_PER_CELL);
    cellCounts.resize(totalCells);
    memset(cellCounts.data(), 0, cellCounts.size() * sizeof(uint8_t));
  }
};

struct SplineInfluence
{
  real dist;
  float height;
  float weight;
  Point2 projPos;
};

static void AddHeightbakeSplineWithClipCheck(SplineObject *spline, dag::Vector<SplineCache> &delaunaySplines, const IBBox2 &dirty_clip,
  const Point2 &heightMapOffset, float gc_sz)
{
  const Point2 dirtyClipMinWorld(heightMapOffset.x + dirty_clip[0].x * gc_sz, heightMapOffset.y + dirty_clip[0].y * gc_sz);
  const Point2 dirtyClipMaxWorld(heightMapOffset.x + dirty_clip[1].x * gc_sz, heightMapOffset.y + dirty_clip[1].y * gc_sz);

  SplineCache cache;
  cache.ptr = spline;
  spline->getWorldBox(cache.bbox);
  float w1 = spline->getProps().modifParams.width;
  float w2 = w1 + spline->getProps().modifParams.smooth;
  cache.bbox[0] -= Point3(w2, w2, w2);
  cache.bbox[1] += Point3(w2, w2, w2);

  if (cache.bbox[0].x <= dirtyClipMaxWorld.x && cache.bbox[1].x >= dirtyClipMinWorld.x && cache.bbox[0].z <= dirtyClipMaxWorld.y &&
      cache.bbox[1].z >= dirtyClipMinWorld.y)
  {
    cache.minDist = w1;
    cache.maxDist = w2;
    cache.maxDistSq = cache.maxDist * cache.maxDist;

    delaunaySplines.push_back(cache);
  }
}

static IBBox2 applyHeightBake(HeightMapStorage &hm, dag::Vector<SplineCache> &delaunaySplines, float gc_sz,
  const Point2 &heightMapOffset, const IBBox2 &dirty_clip, HmapBitmap *bmp)
{
  if (delaunaySplines.empty())
    return {};

  int64_t reft = ref_time_ticks_qpc();

  IBBox2 dirty;
  dirty.setEmpty();

  const Point2 clipMinWorld(heightMapOffset.x + dirty_clip[0].x * gc_sz, heightMapOffset.y + dirty_clip[0].y * gc_sz);
  const Point2 clipMaxWorld(heightMapOffset.x + dirty_clip[1].x * gc_sz, heightMapOffset.y + dirty_clip[1].y * gc_sz);

  Tab<Tab<SplineSegmentInfo>> splineSegments(tmpmem);
  splineSegments.resize(delaunaySplines.size());

  for (int splineIdx = 0; splineIdx < delaunaySplines.size(); ++splineIdx)
  {
    const SplineCache &spline = delaunaySplines[splineIdx];
    Tab<SplineSegmentInfo> &segments = splineSegments[splineIdx];

    const BezierSpline3d &bezierSpline = spline.ptr->getBezierSpline();
    float maxLen = bezierSpline.getLength();

    float maxSizeFloat = spline.maxDist;
    float step = maxSizeFloat / 2;
    if (step < gc_sz / 4)
      step = gc_sz / 4;
    if (step > maxLen)
      step = maxLen;

    Point3 pt = bezierSpline.segs[0].point(0);
    int segId = 0;

    const int numSteps = int(maxLen / step) + 1;
    for (int stepIdx = 0; stepIdx < numSteps; ++stepIdx)
    {
      float len = stepIdx * step;
      if (len > maxLen)
        break;

      float locT;
      int next_segId = bezierSpline.findSegment(len, locT);
      Point3 pt_next = bezierSpline.segs[next_segId].point(locT);

      SplineSegmentInfo segInfo;
      segInfo.start = pt;
      segInfo.end = pt_next;
      segInfo.minDist = spline.minDist;
      segInfo.maxDist = spline.maxDist;

      Point3 segDir = pt_next - pt;
      real segLen = segDir.length();
      if (segLen > 1e-6f)
      {
        segDir /= segLen;
        segInfo.segDir = segDir;
        segInfo.segLen = segLen;

        segInfo.bbox[0] = Point3(min(pt.x, pt_next.x) - spline.maxDist,
          0.f, // not needed
          min(pt.z, pt_next.z) - spline.maxDist);
        segInfo.bbox[1] = Point3(max(pt.x, pt_next.x) + spline.maxDist,
          0.f, // not needed
          max(pt.z, pt_next.z) + spline.maxDist);

        if (segInfo.bbox[0].x <= clipMaxWorld.x && segInfo.bbox[1].x >= clipMinWorld.x && segInfo.bbox[0].z <= clipMaxWorld.y &&
            segInfo.bbox[1].z >= clipMinWorld.y)
          segments.push_back(segInfo);
      }

      pt = pt_next;
      segId = next_segId;
    }
  }

  int mapSizeX = hm.getMapSizeX();
  int mapSizeY = hm.getMapSizeY();

  SpatialGrid spatialGrid;
  const float spatialGridCellSize = max((mapSizeX * gc_sz) / spatialGridSize, (mapSizeY * gc_sz) / spatialGridSize);

  spatialGrid.init(spatialGridCellSize, heightMapOffset, spatialGridSize, spatialGridSize);

  const int minCellX = max(0, int((clipMinWorld.x - heightMapOffset.x) / spatialGridCellSize));
  const int maxCellX = min(spatialGridSize - 1, int((clipMaxWorld.x - heightMapOffset.x) / spatialGridCellSize));
  const int minCellY = max(0, int((clipMinWorld.y - heightMapOffset.y) / spatialGridCellSize));
  const int maxCellY = min(spatialGridSize - 1, int((clipMaxWorld.y - heightMapOffset.y) / spatialGridCellSize));

  for (int splineIdx = 0; splineIdx < delaunaySplines.size(); ++splineIdx)
  {
    const Tab<SplineSegmentInfo> &segments = splineSegments[splineIdx];

    for (int segmentIdx = 0; segmentIdx < segments.size(); ++segmentIdx)
    {
      const SplineSegmentInfo &seg = segments[segmentIdx];

      int segMinCellX = max(minCellX, int((seg.bbox[0].x - heightMapOffset.x) / spatialGridCellSize));
      int segMaxCellX = min(maxCellX, int((seg.bbox[1].x - heightMapOffset.x) / spatialGridCellSize));
      int segMinCellY = max(minCellY, int((seg.bbox[0].z - heightMapOffset.y) / spatialGridCellSize));
      int segMaxCellY = min(maxCellY, int((seg.bbox[1].z - heightMapOffset.y) / spatialGridCellSize));

      for (int cellY = segMinCellY; cellY <= segMaxCellY; ++cellY)
      {
        for (int cellX = segMinCellX; cellX <= segMaxCellX; ++cellX)
        {
          int cellIdx = cellY * spatialGridSize + cellX;

          bool added = false;
          int cellDataOffset = cellIdx * HB_SPLINES_PER_CELL;
          uint8_t &cellCount = spatialGrid.cellCounts[cellIdx];

          for (int dataIdx = 0; dataIdx < cellCount; ++dataIdx)
          {
            SpatialSegmentData &data = spatialGrid.grid[cellDataOffset + dataIdx];
            if (data.splineIdx == splineIdx && data.entryCount < HB_SEGMENTS_PER_SPLINE)
            {
              data.segmentIndices[data.entryCount] = segmentIdx;
              data.entryCount++;
              added = true;
              break;
            }
          }

          if (!added && cellCount < HB_SPLINES_PER_CELL)
          {
            SpatialSegmentData &newData = spatialGrid.grid[cellDataOffset + cellCount];
            memset(&newData, 0, sizeof(newData));
            newData.splineIdx = splineIdx;
            newData.segmentIndices[0] = segmentIdx;
            newData.entryCount = 1;
            cellCount++;
          }
        }
      }
    }
  }

  Tab<SplineInfluence> influences(tmpmem);
  influences.resize(666);
  SplineInfluence *influencesPtr = influences.data();
  int influencesCount = 0;

  Point3 worldPos;
  worldPos.y = 0.0f;
  Point3 bestProjPoint;
  Point2 dir1Unnorm(0, 0), dir2Unnorm(0, 0);

  SplineInfluence *bestInf = nullptr;
  SplineInfluence *bestOpposite = nullptr;

  for (int cellY = minCellY; cellY <= maxCellY; ++cellY) // iterate over cells that cover dirty_clip
  {
    for (int cellX = minCellX; cellX <= maxCellX; ++cellX)
    {
      int cellIdx = cellY * spatialGrid.gridSizeX + cellX;
      uint8_t cellCount = spatialGrid.cellCounts[cellIdx];

      if (cellCount == 0)
        continue;

      float cellMinX = spatialGrid.offset.x + cellX * spatialGrid.cellSize;
      float cellMaxX = cellMinX + spatialGrid.cellSize;
      float cellMinZ = spatialGrid.offset.y + cellY * spatialGrid.cellSize;
      float cellMaxZ = cellMinZ + spatialGrid.cellSize;

      int pxFrom = max(0, int((cellMinX - heightMapOffset.x) / gc_sz));
      int pxTo = min(mapSizeX, int((cellMaxX - heightMapOffset.x) / gc_sz) + 1);
      int pyFrom = max(0, int((cellMinZ - heightMapOffset.y) / gc_sz));
      int pyTo = min(mapSizeY, int((cellMaxZ - heightMapOffset.y) / gc_sz) + 1);

      for (int py = pyFrom; py < pyTo; ++py) // iterate over hmap texels in this cell
      {
        for (int px = pxFrom; px < pxTo; ++px)
        {
          worldPos.x = heightMapOffset.x + px * gc_sz;
          worldPos.z = heightMapOffset.y + py * gc_sz;

          influencesCount = 0; // reset
          bestInf = nullptr;
          float bestWeight = 0.0f;
          float bestDist = MAX_REAL;

          int cellDataOffset = cellIdx * HB_SPLINES_PER_CELL;

          for (int dataIdx = 0; dataIdx < cellCount; ++dataIdx)
          {
            const SpatialSegmentData &data = spatialGrid.grid[cellDataOffset + dataIdx];

            int splineIdx = data.splineIdx;
            const SplineCache &spline = delaunaySplines[splineIdx];

            real minDistSq = MAX_REAL;

            for (int slotIdx = 0; slotIdx < data.entryCount; ++slotIdx)
            {
              int segmentIdx = data.segmentIndices[slotIdx];
              const SplineSegmentInfo &seg = splineSegments[splineIdx][segmentIdx];

              float line_t;
              real distSq =
                distance_point_to_line_segment_squared(worldPos.x, worldPos.z, seg.start.x, seg.start.z, seg.end.x, seg.end.z, line_t);
              Point3 projPoint = seg.start + (seg.end - seg.start) * line_t;

              if (distSq < minDistSq)
              {
                minDistSq = distSq;
                bestProjPoint = projPoint;
              }
            }

            if (minDistSq < spline.maxDistSq)
            {
              real dist = sqrtf(minDistSq);
              float weight =
                (dist <= spline.minDist) ? 1.0f : 1.0f - saturate((dist - spline.minDist) / (spline.maxDist - spline.minDist));

              if ((weight > bestWeight) || (fabsf(weight - bestWeight) < 1e-5f && dist < bestDist))
              {
                bestWeight = weight;
                bestDist = dist;
                bestInf = &influencesPtr[influencesCount];
              }

              influencesPtr[influencesCount].dist = dist;
              influencesPtr[influencesCount].height = bestProjPoint.y;
              influencesPtr[influencesCount].projPos.x = bestProjPoint.x;
              influencesPtr[influencesCount].projPos.y = bestProjPoint.z;
              influencesPtr[influencesCount].weight = weight;

              influencesCount++;
            }
          }

          if (bestInf)
          {
            float h1 = 0.0f, h2 = 0.0f;
            float w1 = 0.0f, w2 = 0.0f;

            h1 = bestInf->height;
            w1 = bestInf->weight;

            Point2 pixelPos(worldPos.x, worldPos.z);
            dir1Unnorm = bestInf->projPos - pixelPos;

            float bestOppositeWeight = 0.0f;
            bestOpposite = nullptr;

            for (int i = 0; i < influencesCount; ++i)
            {
              if (&influencesPtr[i] == bestInf)
                continue;

              dir2Unnorm = influencesPtr[i].projPos - pixelPos;

              if (dir1Unnorm * dir2Unnorm < 0.0f && influencesPtr[i].weight > bestOppositeWeight)
              {
                bestOppositeWeight = influencesPtr[i].weight;
                bestOpposite = &influencesPtr[i];
              }
            }

            float origH = hm.getInitialData(px, py);
            if (bestOpposite) // two splines
            {
              h2 = bestOpposite->height;
              w2 = bestOpposite->weight;

              float totalWeight = w1 + w2;
              float finalH = (h1 * w1 + h2 * w2) / totalWeight;

              if (totalWeight < 1.0f)
              {
                finalH = finalH * totalWeight + origH * (1.0f - totalWeight);
              }

              hm.setFinalData(px, py, finalH);
              dirty += IPoint2(px, py);
              if (bmp)
                bmp->set(px, py);
            }
            else // single spline
            {
              float finalH = h1 * w1 + origH * (1.0f - w1);

              hm.setFinalData(px, py, finalH);
              dirty += IPoint2(px, py);
              if (bmp)
                bmp->set(px, py);
            }
          }
        }
      }
    }
  }

  DAEDITOR3.conNote("Bake heightbake splines to hmap in %.1f seconds (%d splines)", get_time_usec_qpc(reft) / 1e6,
    delaunaySplines.size());
  return dirty;
}

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
  dag::Vector<SplineCache> delaunaySplines = {};

  for (x = 0; x < objEd.splinesCount(); x++)
  {
    SplineObject *spline = objEd.getSpline(x);

    if (spline->isHeightBake())
    {
      if (applyHeightBakeSplines)
        AddHeightbakeSplineWithClipCheck(spline, delaunaySplines, dirty_clip, heightMapOffset, gc_sz);
    }
    else if (spline->isAffectingHmap())
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

  if (!delaunaySplines.empty())
  {
    IBBox2 hbDirty = applyHeightBake(hm, delaunaySplines, gc_sz, heightMapOffset, dirty_clip, nullptr);
    if (gen_colors)
    {
      dirty_clip.clipBox(hbDirty);
      sum_dirty += hbDirty;
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

#undef HB_SEGMENTS_PER_SPLINE
#undef HB_SPLINES_PER_CELL

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

void HmapLandPlugin::convertDelaunaySplinesToHeightBake()
{
  const float texelHalfDiag = gridCellSize * 0.7071f;
  for (int x = 0; x < objEd.splinesCount(); x++)
  {
    SplineObject *spline = objEd.getSpline(x);

    const char *splineClass = spline->getBlkGenName();
    const bool isDelaunaySpline = spline->getModifType() == MODIF_HEIGHTBAKE || strstr(splineClass, "delan") ||
                                  strstr(splineClass, "delone") || strstr(splineClass, "delaun");

    if (isDelaunaySpline)
    {
      if (spline->getModifType() != MODIF_HEIGHTBAKE)
        spline->setModifType(MODIF_HEIGHTBAKE);
      if (spline->getProps().modifParams.width < texelHalfDiag)
        spline->setModifWidth(texelHalfDiag);
    }
  }
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
  Tab<int> prevModifType;

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

    prevModifType.resize(spl.size());
    for (int i = 0; i < spl.size(); i++)
    {
      if (spl[i]->isPoly())
        spl[i]->setPolyHmapAlign(false);
      else
      {
        prevModifType[i] = spl[i]->getModifType();
        spl[i]->setModifType(MODIF_NONE);
      }
    }
  }

  void restore(bool save_redo) override
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

    int prevModifSize = prevModifType.size();
    G_ASSERT(prevModifSize == spl.size());
    for (int i = 0; i < spl.size(); i++)
    {
      if (spl[i]->isPoly())
        spl[i]->setPolyHmapAlign(true);
      else
      {
        int modifType = i < prevModifSize ? prevModifType[i] : MODIF_HMAP;
        spl[i]->setModifType(modifType);
      }
    }
    HmapLandPlugin::self->invalidateObjectProps();
  }

  void redo() override
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
    prevModifType.resize(spl.size());
    for (int i = 0; i < spl.size(); i++)
    {
      if (spl[i]->isPoly())
        spl[i]->setPolyHmapAlign(false);
      else
      {
        prevModifType[i] = spl[i]->getModifType();
        spl[i]->setModifType(MODIF_NONE);
      }
    }
    HmapLandPlugin::self->invalidateObjectProps();
  }

  size_t size() override
  {
    return data_size(spl) + data_size(hmap[0].oldHt) + data_size(hmap[0].newHt) + data_size(hmap[1].oldHt) + data_size(hmap[1].newHt);
  }
  void accepted() override {}
  void get_description(String &s) override { s = "UndoCollapse"; }
};

void HmapLandPlugin::copyFinalHmapToInitial()
{
  Tab<SplineObject *> spl;
  for (int i = 0; i < objEd.objectCount(); i++)
    if (SplineObject *o = RTTI_cast<SplineObject>(objEd.getObject(i)))
      if (o->isAffectingHmap() || spl[i]->isHeightBake())
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

  IBBox2 fullMapBbox = IBBox2(IPoint2(0, 0), IPoint2(hw - 1, hh - 1));
  dag::Vector<SplineCache> delaunaySplines = {};
  for (int i = 0; i < collapse_splines.size(); i++)
  {
    if (collapse_splines[i]->isHeightBake())
    {
      AddHeightbakeSplineWithClipCheck(collapse_splines[i], delaunaySplines, fullMapBbox, heightMapOffset, gridCellSize);
      continue;
    }
    IBBox2 dirty;
    collapse_splines[i]->applyHmapModifier(heightMap, heightMapOffset, gridCellSize, dirty, fullMapBbox, &undo->hmap[0].bmp);
    collArea[0] += dirty;

    if (detDivisor)
    {
      collapse_splines[i]->applyHmapModifier(heightMapDet, heightMapOffset, gridCellSize / detDivisor, dirty, detRectC,
        &undo->hmap[1].bmp);
      collArea[1] += dirty;
    }
  }

  if (!delaunaySplines.empty())
  {
    IBBox2 hbDirty = applyHeightBake(heightMap, delaunaySplines, gridCellSize, heightMapOffset, fullMapBbox, &undo->hmap[0].bmp);
    collArea[0] += hbDirty;
    if (detDivisor)
    {
      IBBox2 hbDirtyDet =
        applyHeightBake(heightMapDet, delaunaySplines, gridCellSize / detDivisor, heightMapOffset, detRectC, &undo->hmap[1].bmp);
      collArea[1] += hbDirtyDet;
    }
  }

  undo->finalizeChanges(collArea, true);
  objEd.getUndoSystem()->put(undo);
  objEd.getUndoSystem()->accept("Collapse modifier(s)");

  updateHeightMapTex(false, &collArea[0]);
  if (detDivisor)
    updateHeightMapTex(true, &collArea[1]);
}
