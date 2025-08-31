// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "hmlBrush.h"
#include "../hmlPlugin.h"

#include <debug/dag_debug.h>

class HmapScriptBrush : public HmapLandBrush
{
public:
  HmapScriptBrush(IBrushClient *client, IHmapBrushImage &height_map) : HmapLandBrush(client, height_map) {}

  void fillParams(PropPanel::ContainerPropertyControl &panel) override;
  void updateToPanel(PropPanel::ContainerPropertyControl &panel) override;

  bool brushPaintApply(int x, int y, float inc, bool rb) override { return true; }

  void saveToBlk(DataBlock &blk) const override;
  void loadFromBlk(const DataBlock &blk) override;

  bool updateFromPanelRef(PropPanel::ContainerPropertyControl &panel, int pid) override;
};
