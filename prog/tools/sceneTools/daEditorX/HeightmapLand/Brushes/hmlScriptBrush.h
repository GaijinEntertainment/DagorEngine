// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "hmlBrush.h"
#include "../hmlPlugin.h"

#include <debug/dag_debug.h>

class HmapScriptBrush : public HmapLandBrush
{
public:
  HmapScriptBrush(IBrushClient *client, IHmapBrushImage &height_map) : HmapLandBrush(client, height_map) {}

  virtual void fillParams(PropPanel::ContainerPropertyControl &panel);
  virtual void updateToPanel(PropPanel::ContainerPropertyControl &panel);

  virtual bool brushPaintApply(int x, int y, float inc, bool rb) { return true; }

  virtual void saveToBlk(DataBlock &blk) const;
  virtual void loadFromBlk(const DataBlock &blk);

  virtual bool updateFromPanelRef(PropPanel::ContainerPropertyControl &panel, int pid);
};
