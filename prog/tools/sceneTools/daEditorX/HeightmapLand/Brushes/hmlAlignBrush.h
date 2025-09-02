// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "hmlBrush.h"

class HmapAlignLandBrush : public HmapLandBrush
{
public:
  HmapAlignLandBrush(bool continuos, IBrushClient *client, IHmapBrushImage &height_map) :
    HmapLandBrush(client, height_map),
    trail(1),
    currentTrail(1),
    average(0),
    continuos(false),
    sigma(1),
    started(false),
    alignCollision(false),
    collisionOffset(-0.1)
  {}

  void fillParams(PropPanel::ContainerPropertyControl &panel) override;
  void updateToPanel(PropPanel::ContainerPropertyControl &panel) override;

  void onRBBrushPaintStart(int buttons, int key_modif) override { onBrushPaintStart(buttons, key_modif); }
  void onBrushPaintStart(int buttons, int key_modif) override
  {
    HmapLandBrush::onBrushPaintStart(buttons, key_modif);
    currentTrail = 1;
    started = false;
  }
  bool brushPaintApply(int x, int y, float inc, bool rb) override;
  void brushPaintApplyStart(const IBBox2 &where) override;
  void brushPaintApplyEnd() override;
  void saveToBlk(DataBlock &blk) const override;
  void loadFromBlk(const DataBlock &blk) override;

  bool updateFromPanelRef(PropPanel::ContainerPropertyControl &panel, int pid) override;

protected:
  float trail, sigma;
  float average, currentTrail;
  bool started, continuos, alignCollision, alignCollisionTop;
  float collisionOffset;
};
