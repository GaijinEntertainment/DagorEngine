// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "hmlBrush.h"

class HmapHillLandBrush : public HmapLandBrush
{
public:
  HmapHillLandBrush(bool down, IBrushClient *client, IHmapBrushImage &height_map) :
    HmapLandBrush(client, height_map), limitValue(0), isDown(down), limitValueUse(false), power(1)
  {}

  void fillParams(PropPanel::ContainerPropertyControl &panel) override;
  void updateToPanel(PropPanel::ContainerPropertyControl &panel) override;

  bool brushPaintApply(int x, int y, float inc, bool rb) override;
  void saveToBlk(DataBlock &blk) const override;
  void loadFromBlk(const DataBlock &blk) override;

  bool updateFromPanelRef(PropPanel::ContainerPropertyControl &panel, int pid) override;

protected:
  float limitValue;
  float power;
  bool isDown, limitValueUse;
};
