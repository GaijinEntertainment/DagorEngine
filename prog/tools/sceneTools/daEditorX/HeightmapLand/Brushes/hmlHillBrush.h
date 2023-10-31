#ifndef __GAIJIN_HEIGHTMAPLAND_HILL_BRUSH__
#define __GAIJIN_HEIGHTMAPLAND_HILL_BRUSH__
#pragma once


#include "hmlBrush.h"

class HmapHillLandBrush : public HmapLandBrush
{
public:
  HmapHillLandBrush(bool down, IBrushClient *client, IHmapBrushImage &height_map) :
    HmapLandBrush(client, height_map), limitValue(0), isDown(down), limitValueUse(false), power(1)
  {}

  virtual void fillParams(PropPanel2 &panel);
  virtual void updateToPanel(PropPanel2 &panel);

  virtual bool brushPaintApply(int x, int y, float inc, bool rb);
  virtual void saveToBlk(DataBlock &blk) const;
  virtual void loadFromBlk(const DataBlock &blk);

  virtual bool updateFromPanelRef(PropPanel2 &panel, int pid);

protected:
  float limitValue;
  float power;
  bool isDown, limitValueUse;
};


#endif //__GAIJIN_HEIGHTMAP_BRUSH__
