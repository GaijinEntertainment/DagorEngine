// Copyright (C) Gaijin Games KFT.  All rights reserved.
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

  void fillParams(PropPanel::ContainerPropertyControl &panel) override;
  void updateToPanel(PropPanel::ContainerPropertyControl &panel) override;

  bool brushPaintApply(int x, int y, float inc, bool rb) override;
  void brushPaintApplyStart(const IBBox2 &where) override;
  void saveToBlk(DataBlock &blk) const override;
  void loadFromBlk(const DataBlock &blk) override;

  bool updateFromPanelRef(PropPanel::ContainerPropertyControl &panel, int pid) override;

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
