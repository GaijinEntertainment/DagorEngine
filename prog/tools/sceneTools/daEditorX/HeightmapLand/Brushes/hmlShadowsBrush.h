// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "hmlBrush.h"

class HmapShadowsBrush : public HmapLandBrush
{
public:
  HmapShadowsBrush(IBrushClient *client, IHmapBrushImage &height_map) : HmapLandBrush(client, height_map) {}

  virtual void fillParams(PropPanel::ContainerPropertyControl &panel);
  virtual void updateToPanel(PropPanel::ContainerPropertyControl &panel);

  virtual bool brushPaintApply(int x, int y, float inc, bool rb) { return true; }

  virtual void onBrushPaintEnd(int buttons, int key_modif);

  virtual IBBox2 getDirtyBox() const
  {
    IBBox2 box;
    return box;
  }

  virtual void saveToBlk(DataBlock &blk) const;
  virtual void loadFromBlk(const DataBlock &blk);

  virtual bool updateFromPanelRef(PropPanel::ContainerPropertyControl &panel, int pid);
};
