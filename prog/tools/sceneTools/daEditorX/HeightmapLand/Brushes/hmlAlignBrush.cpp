#include <math/dag_mathBase.h>
#include "hmlAlignBrush.h"
#include "../hmlPlugin.h"
#include "../hmlCm.h"
#include <ioSys/dag_dataBlock.h>


void HmapAlignLandBrush::fillParams(PropPanel2 &panel)
{
  HmapLandBrush::fillParams(panel);

  panel.createTrackInt(PID_BRUSH_TRAIL, "Trail:", trail * 100, 1, 100, 1);
  panel.createTrackInt(PID_BRUSH_SIGMA, "Centered average", sigma * 100, 1, 100, 1);

  panel.createCheckBox(PID_BRUSH_ALIGN_SGEOMETRY, "Align to collision", alignCollision);
  panel.createCheckBox(PID_BRUSH_ALIGN_TOP, "Collision top", alignCollisionTop, alignCollision);
  panel.createEditFloat(PID_BRUSH_ALIGN_OFFSET, "Align offset", collisionOffset);
  panel.createCheckBox(PID_BRUSH_CONTINUOS, "Align to first point", !continuos);
}


void HmapAlignLandBrush::updateToPanel(PropPanel2 &panel)
{
  HmapLandBrush::updateToPanel(panel);

  panel.setInt(PID_BRUSH_TRAIL, trail * 100);
  panel.setInt(PID_BRUSH_SIGMA, sigma * 100);

  panel.setBool(PID_BRUSH_ALIGN_SGEOMETRY, alignCollision);
  panel.setBool(PID_BRUSH_ALIGN_TOP, alignCollisionTop);
  panel.setFloat(PID_BRUSH_ALIGN_OFFSET, collisionOffset);
  panel.setBool(PID_BRUSH_CONTINUOS, !continuos);
}


bool HmapAlignLandBrush::updateFromPanelRef(PropPanel2 &panel, int pid)
{
  switch (pid)
  {
    case PID_BRUSH_ALIGN_SGEOMETRY:
    {
      int ar = panel.getBool(pid);
      if (ar < 2)
        alignCollision = ((bool)ar);
      panel.setEnabledById(PID_BRUSH_ALIGN_TOP, alignCollision);
      return true;
    }
    case PID_BRUSH_ALIGN_TOP:
    {
      int ar = panel.getBool(pid);
      if (ar < 2)
        alignCollisionTop = ((bool)ar);
      return true;
    }
    case PID_BRUSH_ALIGN_OFFSET: collisionOffset = panel.getFloat(pid); return true;
    case PID_BRUSH_CONTINUOS:
    {
      int ar = panel.getBool(pid);
      if (ar < 2)
        continuos = !((bool)ar);
      return true;
    }
    case PID_BRUSH_TRAIL: trail = panel.getInt(pid) / 100.0; return true;
    case PID_BRUSH_SIGMA:
    {
      sigma = panel.getInt(pid) / 100.0;
      started = false;
      return true;
    }
    default: return HmapLandBrush::updateFromPanelRef(panel, pid);
  }
}

void HmapAlignLandBrush::brushPaintApplyStart(const IBBox2 &where)
{
  HmapLandBrush::brushPaintApplyStart(where);
  if (where.isEmpty())
    return;
  if (started)
    return;
  if (alignCollision)
    return;
  if (sigma < 0.01)
    sigma = 0.01;

  double calcAverage = 0.0, totalW = 0.0;
  Point2 center = point2(where[1] + where[0]) / 2.0;
  double radius = (point2(where[1]) - center).length();
  double radiusSigma2 = radius * sigma * 2;
  radiusSigma2 *= radiusSigma2;

  for (int y = where[0].y; y <= where[1].y; ++y)
    for (int x = where[0].x; x <= where[1].x; ++x)
    {
      double w = exp(-((double)(x - center.x) * (x - center.x) + (double)(y - center.y) * (y - center.y)) / (radiusSigma2));
      totalW += w;
      calcAverage += heightMap.getBrushImageData(x, y, HmapLandPlugin::self->getEditedChannel()) * w;
    }
  average = calcAverage / totalW;
  if (average > 1000000 || average < -1000000 || _isnan(average) || !_finite(average))
  {
    started = false;
  }
  if (!continuos)
    started = true;
}

void HmapAlignLandBrush::brushPaintApplyEnd() { currentTrail *= trail; }

bool HmapAlignLandBrush::brushPaintApply(int x, int y, float inc, bool rb)
{
  if (inc > 1.0f)
    inc = 1.0f;
  inc *= currentTrail;
  if (alignCollision)
  {
    real maxDist = DAGORED2->getMaxTraceDistance(), dist = maxDist * 2;
    bool hit = false;
    Point3 p(gridCellSize * (x), maxDist, gridCellSize * (y));
    p += Point3(heightMapOffset.x, 0, heightMapOffset.y);
    while (DAGORED2->traceRay(p, Point3(0, -1, 0), dist, NULL, false))
    {
      p.y -= dist + 0.05;
      dist = maxDist;
      hit = true;
      if (alignCollisionTop)
        break;
    }
    if (hit)
    {
      float ht = p.y;
      heightMap.setBrushImageData(x, y,
        inc * (ht + collisionOffset) + heightMap.getBrushImageData(x, y, HmapLandPlugin::self->getEditedChannel()) * (1.0f - inc),
        HmapLandPlugin::self->getEditedChannel());
      return true;
    }
    return false;
  }
  if (average > 1000000 || average < -1000000 || _isnan(average) || !_finite(average))
    return false;
  heightMap.setBrushImageData(x, y,
    inc * average + heightMap.getBrushImageData(x, y, HmapLandPlugin::self->getEditedChannel()) * (1.0f - inc),
    HmapLandPlugin::self->getEditedChannel());
  return true;
}

void HmapAlignLandBrush::saveToBlk(DataBlock &blk) const
{
  HmapLandBrush::saveToBlk(blk);

  blk.addReal("trail", trail);
  blk.addReal("sigma", sigma);
  blk.addBool("continuos", continuos);
  blk.addBool("alignCollision", alignCollision);
  blk.addReal("collisionOffset", collisionOffset);
  blk.addBool("alignCollisionTop", alignCollisionTop);
}

void HmapAlignLandBrush::loadFromBlk(const DataBlock &blk)
{
  HmapLandBrush::loadFromBlk(blk);
  trail = blk.getReal("trail", trail);
  sigma = blk.getReal("sigma", sigma);
  continuos = blk.getBool("continuos", continuos);
  alignCollision = blk.getBool("alignCollision", alignCollision);
  collisionOffset = blk.getReal("collisionOffset", collisionOffset);
  alignCollisionTop = blk.getBool("alignCollisionTop", alignCollisionTop);
  started = false;
  average = 0;
}

namespace heightmap_land
{
HmapLandBrush *getAlignBrush(IBrushClient *client, IHmapBrushImage &height_map)
{
  return new HmapAlignLandBrush(false, client, height_map);
}
}; // namespace heightmap_land
