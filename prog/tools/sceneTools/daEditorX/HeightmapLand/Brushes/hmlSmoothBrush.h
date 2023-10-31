#ifndef __GAIJIN_HEIGHTMAPLAND_SMOOTH_BRUSH__
#define __GAIJIN_HEIGHTMAPLAND_SMOOTH_BRUSH__
#pragma once


#include "hmlBrush.h"

class HmapSmoothLandBrush : public HmapLandBrush
{
public:
  HmapSmoothLandBrush(IBrushClient *client, IHmapBrushImage &height_map) :
    HmapLandBrush(client, height_map), halfKernelSize(1), sigma(1)
  {
    makeKernel();
  }

  virtual void fillParams(PropPanel2 &panel);
  virtual void updateToPanel(PropPanel2 &panel);

  virtual bool brushPaintApply(int x, int y, float inc, bool rb);
  virtual void brushPaintApplyStart(const IBBox2 &where);
  virtual void saveToBlk(DataBlock &blk) const;
  virtual void loadFromBlk(const DataBlock &blk);

  virtual bool updateFromPanelRef(PropPanel2 &panel, int pid);

protected:
  void makeKernel();
  float sigma;
  float centerWeight;
  int halfKernelSize;
  IBBox2 where;
  IPoint2 preFilteredSize;

  SmallTab<float, MidmemAlloc> gaussKernel;
  SmallTab<float, TmpmemAlloc> preFiltered;
};


#endif //__GAIJIN_HEIGHTMAP_BRUSH__
