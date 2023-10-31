#ifndef __GAIJIN_HEIGHTMAPLAND_SMOOTH_BRUSH__
#define __GAIJIN_HEIGHTMAPLAND_SMOOTH_BRUSH__
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

  virtual void fillParams(PropPanel2 &panel);
  virtual void updateToPanel(PropPanel2 &panel);

  virtual void onRBBrushPaintStart(int buttons, int key_modif) { onBrushPaintStart(buttons, key_modif); }
  virtual void onBrushPaintStart(int buttons, int key_modif)
  {
    HmapLandBrush::onBrushPaintStart(buttons, key_modif);
    currentTrail = 1;
    started = false;
  }
  virtual bool brushPaintApply(int x, int y, float inc, bool rb);
  virtual void brushPaintApplyStart(const IBBox2 &where);
  virtual void brushPaintApplyEnd();
  virtual void saveToBlk(DataBlock &blk) const;
  virtual void loadFromBlk(const DataBlock &blk);

  virtual bool updateFromPanelRef(PropPanel2 &panel, int pid);

protected:
  float trail, sigma;
  float average, currentTrail;
  bool started, continuos, alignCollision, alignCollisionTop;
  float collisionOffset;
};


#endif //__GAIJIN_HEIGHTMAP_BRUSH__
