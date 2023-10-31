#include "hmlHillBrush.h"
#include "../hmlPlugin.h"
#include "../hmlCm.h"
#include <ioSys/dag_dataBlock.h>


void HmapHillLandBrush::fillParams(PropPanel2 &panel)
{
  panel.createEditFloat(PID_BRUSH_POWER, "Power(height):", power);
  HmapLandBrush::fillParams(panel);
  panel.createCheckBox(PID_BRUSH_LIMIT_USE, "Use Limit", limitValueUse);
  panel.createEditFloat(PID_BRUSH_LIMIT_VALUE, "Limit", limitValue);
}


void HmapHillLandBrush::updateToPanel(PropPanel2 &panel)
{
  panel.setFloat(PID_BRUSH_POWER, power);
  __super::updateToPanel(panel);
  panel.setBool(PID_BRUSH_LIMIT_USE, limitValueUse);
  panel.setFloat(PID_BRUSH_LIMIT_VALUE, limitValue);
}


bool HmapHillLandBrush::updateFromPanelRef(PropPanel2 &panel, int pid)
{
  switch (pid)
  {
    case PID_BRUSH_LIMIT_USE:
    {
      int ar = panel.getBool(pid);
      if (ar < 2)
        limitValueUse = (bool)ar;
      return true;
    }
    case PID_BRUSH_LIMIT_VALUE: limitValue = panel.getFloat(pid); return true;
    case PID_BRUSH_POWER: power = panel.getFloat(pid); return true;
    default: return HmapLandBrush::updateFromPanelRef(panel, pid);
  }
}

bool HmapHillLandBrush::brushPaintApply(int x, int y, float inc, bool rb)
{
  if (rb)
    inc = -inc;
  if (isDown)
    inc = -inc;
  float result = heightMap.getBrushImageData(x, y, HmapLandPlugin::self->getEditedChannel()) + inc * power;

  if (limitValueUse)
  {
    if (inc > 0)
    {
      if (result > limitValue)
      {
        if (result - inc < limitValue)
          result = limitValue;
        else
          return false;
      }
    }
    else
    {
      if (result < limitValue)
      {
        if (result - inc > limitValue)
          result = limitValue;
        else
          return false;
      }
    }
  }

  heightMap.setBrushImageData(x, y, result, HmapLandPlugin::self->getEditedChannel());
  return true;
}

void HmapHillLandBrush::saveToBlk(DataBlock &blk) const
{
  HmapLandBrush::saveToBlk(blk);

  blk.addReal("limitValue", limitValue);
  blk.addReal("power", power);
  blk.addBool("limitValueUse", limitValueUse);
}

void HmapHillLandBrush::loadFromBlk(const DataBlock &blk)
{
  HmapLandBrush::loadFromBlk(blk);

  power = blk.getReal("power", power);
  limitValue = blk.getReal("limitValue", limitValue);
  limitValueUse = blk.getBool("limitValueUse", limitValueUse);
}

namespace heightmap_land
{
HmapLandBrush *getUpHillBrush(IBrushClient *client, IHmapBrushImage &height_map)
{
  return new HmapHillLandBrush(false, client, height_map);
}

HmapLandBrush *getDownHillBrush(IBrushClient *client, IHmapBrushImage &height_map)
{
  return new HmapHillLandBrush(true, client, height_map);
}
}; // namespace heightmap_land
