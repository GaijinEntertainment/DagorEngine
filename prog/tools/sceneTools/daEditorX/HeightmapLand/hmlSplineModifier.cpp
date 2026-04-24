// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "hmlSplineObject.h"
#include "hmlSplinePoint.h"
#include "hmlPlugin.h"
#include <math/dag_math2d.h>

void SplineObject::applyHmapModifier(HeightMapStorage &hm, Point2 hm_ofs, float hm_cell_size, IBBox2 &dirty, const IBBox2 &dirty_clip,
  HmapBitmap *bmp)
{
  if (!points.size() || (props.modifParams.width <= 0 && props.modifParams.smooth <= 0))
    return;

  // Early bbox rejection: skip splines that don't overlap the dirty region
  float addr = (props.modifParams.width + props.modifParams.smooth) * props.modifParams.maxWidthScale;
  {
    int splMinX = (int)floorf((splBox[0].x - addr - hm_ofs.x) / hm_cell_size);
    int splMaxX = (int)ceilf((splBox[1].x + addr - hm_ofs.x) / hm_cell_size);
    int splMinY = (int)floorf((splBox[0].z - addr - hm_ofs.y) / hm_cell_size);
    int splMaxY = (int)ceilf((splBox[1].z + addr - hm_ofs.y) / hm_cell_size);
    if (splMinX > dirty_clip[1].x || splMaxX < dirty_clip[0].x || splMinY > dirty_clip[1].y || splMaxY < dirty_clip[0].y)
      return;
  }

  float maxLen = bezierSpline.getLength();

  real minY = points[0]->getPt().y;
  if (poly)
    for (int i = 1; i < points.size(); i++)
      if (points[i]->getPt().y < minY)
        minY = points[i]->getPt().y;

  float dY = 0;
  if (props.modifParams.additive)
    for (int i = 0; i < points.size(); i++)
    {
      int xs = (int)floorf((points[i]->getPt().x - hm_ofs.x) / hm_cell_size);
      int ys = (int)floorf((points[i]->getPt().z - hm_ofs.y) / hm_cell_size);
      float dh = points[i]->getPt().y - hm.getFinalData(clamp(xs, 0, hm.getMapSizeX()), clamp(ys, 0, hm.getMapSizeY()));
      if (i == 0 || dY > dh)
        dY = dh;
    }
  minY -= 0.5;

  Tab<SmallTab<Point3, TmpmemAlloc>> heightmapDistance(tmpmem);
  int elemSize = 32;
  int xSize = hm.getMapSizeX() / elemSize + 1;
  int ySize = hm.getMapSizeY() / elemSize + 1;
  heightmapDistance.resize(xSize * ySize);

  float splineWidth = props.modifParams.width / hm_cell_size;
  float splineSmooth = props.modifParams.smooth / hm_cell_size;
  bool usePerPointWidth = props.modifParams.usePerPointWidth;
  float maxWidthScale = props.modifParams.maxWidthScale;
  float maxScale = usePerPointWidth ? maxWidthScale : 1.0f;
  float maxSizeFloat = (splineWidth + splineSmooth) * maxScale;
  Point3 updir = Point3(0, 1, 0);
  float step = maxSizeFloat / 2;
  if (step < hm_cell_size / 4)
    step = hm_cell_size / 4;
  if (step > maxLen)
    step = maxLen;
  int maxSize = ceilf(maxSizeFloat + step / 2);
  float maxSizeFloatSq = maxSizeFloat * maxSizeFloat;

  // debug("maxLen=%.3f maxSizeFloat=%.3f poly=%d hm_cell_size=%.3f step=%.3f", maxLen, maxSizeFloat, poly, hm_cell_size, step);
  auto getScaleWAt = [&](int bezSegId, float locT) -> float {
    int numPts = points.size();
    if (bezSegId < 0 || bezSegId >= numPts)
      return 1.0f;
    float s0 = max(points[bezSegId]->getProps().attr.scale_w, 1e-3f);
    int nextPtIdx = min(bezSegId + 1, numPts - 1);
    float s1 = max(points[nextPtIdx]->getProps().attr.scale_w, 1e-3f);
    return s0 + (s1 - s0) * locT;
  };

  Point3 pt = bezierSpline.segs[0].point(0) - Point3::x0y(hm_ofs);
  Point3 tang = normalize(bezierSpline.segs[0].tang(0));
  int segId = 0;
  float prevScaleW = usePerPointWidth ? getScaleWAt(0, 0.0f) : 1.0f;
  pt.x /= hm_cell_size;
  pt.z /= hm_cell_size;
  for (float len = step; len <= maxLen; len += step)
  {
    float locT;
    int next_segId = bezierSpline.findSegment(len, locT);
    Point3 pt_next = bezierSpline.segs[next_segId].point(locT) - Point3::x0y(hm_ofs);
    pt_next.x /= hm_cell_size;
    pt_next.z /= hm_cell_size;

    float nextScaleW = usePerPointWidth ? getScaleWAt(next_segId, locT) : 1.0f;

    Point3 side = updir % tang;
    side.normalize();
    float sideX = side.x, sideY = side.y, sideZ = side.z;

    int xsStart = max(int(floorf((pt.x + pt_next.x) / 2)) - maxSize, 0);
    int xsEnd = min(int(ceilf((pt.x + pt_next.x) / 2)) + maxSize, xSize * elemSize - 1);
    int ysStart = max(int(floorf((pt.z + pt_next.z) / 2)) - maxSize, 0);
    int ysEnd = min(int(ceilf((pt.z + pt_next.z) / 2)) + maxSize, ySize * elemSize - 1);
    for (int ys = ysStart; ys <= ysEnd; ys++)
    {
      int yElems = ys / elemSize;
      for (int xs = xsStart; xs <= xsEnd; xs++)
      {
        real line_t;
        real distSq = sq_distance_point_to_line_segment(Point2(xs, ys), Point2::xz(pt), Point2::xz(pt_next), line_t);
        if (distSq > maxSizeFloatSq)
          continue;

        int xElems = xs / elemSize;
        int elemId = xElems + yElems * xSize;
        if (!heightmapDistance[elemId].size())
        {
          clear_and_resize(heightmapDistance[elemId], elemSize * elemSize);
          for (int j = 0; j < heightmapDistance[elemId].size(); ++j)
            heightmapDistance[elemId][j].x = 1.1e12f;
        }
        int subIndex = xs - xElems * elemSize + (ys - yElems * elemSize) * elemSize;
        if (heightmapDistance[elemId][subIndex].x <= distSq)
          continue;
        heightmapDistance[elemId][subIndex].x = distSq;

        real k =
          Point2(xs - pt.x * (1 - line_t) - pt_next.x * line_t, ys - pt.z * (1 - line_t) - pt_next.z * line_t) * Point2(sideX, sideZ);

        heightmapDistance[elemId][subIndex].y = (poly ? minY : pt.y * (1 - line_t) + pt_next.y * line_t) + k * sideY * 2;
        heightmapDistance[elemId][subIndex].z = prevScaleW + (nextScaleW - prevScaleW) * line_t;
      }
    }
    tang = normalize(bezierSpline.segs[next_segId].tang(locT));
    pt = pt_next;
    segId = next_segId;
    prevScaleW = nextScaleW;
  }

  if (poly)
  {
    Point3 diff = points[0]->getPt() - points.back()->getPt();
    real diffLength = diff.length();

    pt = points[0]->getPt() - Point3::x0y(hm_ofs);
    pt.x /= hm_cell_size;
    pt.z /= hm_cell_size;
    for (float len = 0; len < diffLength; len += step)
    {
      Point3 pt_next =
        points[0]->getPt() * (1 - (len / diffLength)) + points.back()->getPt() * (len / diffLength) - Point3::x0y(hm_ofs);
      pt_next.x /= hm_cell_size;
      pt_next.z /= hm_cell_size;

      int xsStart2 = max(int(floorf((pt.x + pt_next.x) / 2)) - maxSize, 0);
      int xsEnd2 = min(int(ceilf((pt.x + pt_next.x) / 2)) + maxSize, xSize * elemSize - 1);
      int ysStart2 = max(int(floorf((pt.z + pt_next.z) / 2)) - maxSize, 0);
      int ysEnd2 = min(int(ceilf((pt.z + pt_next.z) / 2)) + maxSize, ySize * elemSize - 1);
      for (int ys = ysStart2; ys <= ysEnd2; ys++)
      {
        int yElems = ys / elemSize;
        for (int xs = xsStart2; xs <= xsEnd2; xs++)
        {
          int xElems = xs / elemSize;
          int elemId = xElems + yElems * xSize;
          real line_t;
          real distSq = sq_distance_point_to_line_segment(Point2(xs, ys), Point2::xz(pt), Point2::xz(pt_next), line_t);

          if (distSq >= maxSizeFloatSq)
            continue;
          if (!heightmapDistance[elemId].size())
          {
            clear_and_resize(heightmapDistance[elemId], elemSize * elemSize);
            for (int j = 0; j < heightmapDistance[elemId].size(); ++j)
              heightmapDistance[elemId][j].x = 1.1e12f;
          }
          int subIndex = xs - xElems * elemSize + (ys - yElems * elemSize) * elemSize;
          if (heightmapDistance[elemId][subIndex].x <= distSq)
            continue;
          heightmapDistance[elemId][subIndex].x = distSq;
          heightmapDistance[elemId][subIndex].y = minY;
          heightmapDistance[elemId][subIndex].z = 1.0f;
        }
      }
      pt = pt_next;
    }
  }

  dirty.setEmpty();
  IBBox2 eBox;
  eBox[0].x = dirty_clip[0].x / elemSize;
  eBox[0].y = dirty_clip[0].y / elemSize;
  eBox[1].x = (dirty_clip[1].x + elemSize - 1) / elemSize;
  eBox[1].y = (dirty_clip[1].y + elemSize - 1) / elemSize;
  if (eBox[1].x >= xSize)
    eBox[1].x = xSize - 1;
  if (eBox[1].y >= ySize)
    eBox[1].y = ySize - 1;

  for (int elemId = eBox[0].y * xSize + eBox[0].x, ye = eBox[0].y, eStep = xSize - eBox.width().x - 1; ye <= eBox[1].y;
       ++ye, elemId += eStep)
    for (int xe = eBox[0].x; xe <= eBox[1].x; ++xe, ++elemId)
    {
      if (!heightmapDistance[elemId].size())
        continue;
      for (int subId = 0, ys = ye * elemSize; ys < ye * elemSize + elemSize; ++ys)
        for (int xs = xe * elemSize; xs < xe * elemSize + elemSize; ++xs, ++subId)
        {
          if (!(dirty_clip & IPoint2(xs, ys)))
            continue;

          float distSq = heightmapDistance[elemId][subId].x;

          if (distSq >= 1e12f)
            continue;

          float dist = sqrtf(distSq);

          float ht = heightmapDistance[elemId][subId].y;
          float scaleW = heightmapDistance[elemId][subId].z;
          float effSplineWidth = splineWidth * scaleW;
          float effSplineSmooth = splineSmooth * scaleW;
          float effMaxSize = effSplineWidth + effSplineSmooth;
          dirty += IPoint2(xs, ys);

          if (bmp)
            bmp->set(xs, ys);

          if (dist > effMaxSize)
            continue;

          if (dist > effSplineWidth)
          {
            ht += props.modifParams.offset[1];
            float strength = (effSplineSmooth > 1e-9f) ? (dist - effSplineWidth) / effSplineSmooth : 1.0f;
            float tx = xs * hm_cell_size + hm_ofs.x;
            float ty = ys * hm_cell_size + hm_ofs.y;
            if (poly && pointInsidePoly(Point2(tx, ty)))
              strength = 0;
            if (strength < 1.0f)
            {
              hm.setFinalData(xs, ys,
                props.modifParams.additive ? hm.getFinalData(xs, ys) + dY * (1.0f - strength)
                                           : hm.getFinalData(xs, ys) * strength + ht * (1.0f - strength));
            }
          }
          else
          {
            if (props.modifParams.additive)
              ht = hm.getFinalData(xs, ys) + dY;
            float interp = (effSplineWidth > 1e-9f) ? powf(dist / effSplineWidth, props.modifParams.offsetPow) : 0.0f;
            ht += props.modifParams.offset[0] * (1.0f - interp) + props.modifParams.offset[1] * interp;
            hm.setFinalData(xs, ys, ht);
          }
        }
    }

  IBBox2 polyBox(IPoint2(floor((splBox[0].x - hm_ofs.x) / hm_cell_size), floor((splBox[0].z - hm_ofs.y) / hm_cell_size)),
    IPoint2(ceil((splBox[1].x - hm_ofs.x) / hm_cell_size), ceil((splBox[1].z - hm_ofs.y) / hm_cell_size)));

  dirty_clip.clipBox(polyBox);

  if (poly && !polyBox.isEmpty())
    for (int ys = polyBox[0].y; ys <= polyBox[1].y; ++ys)
      for (int xs = polyBox[0].x; xs <= polyBox[1].x; ++xs)
      {
        float tx = xs * hm_cell_size + hm_ofs.x;
        float ty = ys * hm_cell_size + hm_ofs.y;

        if (pointInsidePoly(Point2(tx, ty)))
        {
          if (props.modifParams.additive)
          {
            int xe = xs / elemSize, ye = ys / elemSize;
            if (xe < 0 || ye < 0 || xe >= xSize || ye >= ySize)
              continue;

            int hmd_idx = xe + ye * xSize;
            if (hmd_idx >= 0 && hmd_idx < heightmapDistance.size())
            {
              dag::ConstSpan<Point3> hmd = heightmapDistance[hmd_idx];
              if (hmd.size() && hmd[(ys % elemSize) * elemSize + (xs % elemSize)].x < 1e12f)
                continue;
            }
            hm.setFinalData(xs, ys, hm.getFinalData(xs, ys) + dY);
          }
          else
            hm.setFinalData(xs, ys, minY);
        }

        dirty += IPoint2(xs, ys);
        if (bmp)
          bmp->set(xs, ys);
      }

  IBBox2 sum = dirty;
  if (HmapLandPlugin::self->getHeightmapSizeX() == hm.getMapSizeX() && HmapLandPlugin::self->getHeightmapSizeY() == hm.getMapSizeY())
  {
    sum += lastModifArea;
    lastModifArea = dirty;
  }
  else
  {
    sum += lastModifAreaDet;
    lastModifAreaDet = dirty;
  }
  dirty = sum;
}
