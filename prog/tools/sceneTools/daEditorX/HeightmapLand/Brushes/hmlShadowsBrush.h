// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "hmlBrush.h"

class HmapShadowsBrush : public HmapLandBrush
{
public:
  HmapShadowsBrush(IBrushClient *client, IHmapBrushImage &height_map) : HmapLandBrush(client, height_map) {}

  void fillParams(PropPanel::ContainerPropertyControl &panel) override;
  void updateToPanel(PropPanel::ContainerPropertyControl &panel) override;

  bool brushPaintApply(int x, int y, float inc, bool rb) override { return true; }

  void onBrushPaintEnd(int buttons, int key_modif) override;

  IBBox2 getDirtyBox() const override
  {
    IBBox2 box;
    return box;
  }

  void saveToBlk(DataBlock &blk) const override;
  void loadFromBlk(const DataBlock &blk) override;

  bool updateFromPanelRef(PropPanel::ContainerPropertyControl &panel, int pid) override;
};
