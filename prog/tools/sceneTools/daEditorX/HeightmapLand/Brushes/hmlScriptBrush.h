#ifndef __GAIJIN_HEIGHTMAPLAND_SCRIPT_BRUSH__
#define __GAIJIN_HEIGHTMAPLAND_SCRIPT_BRUSH__

#pragma once

#include "hmlBrush.h"
#include "../hmlPlugin.h"

#include <debug/dag_debug.h>

class HmapScriptBrush : public HmapLandBrush
{
public:
  HmapScriptBrush(IBrushClient *client, IHmapBrushImage &height_map) : HmapLandBrush(client, height_map) {}

  virtual void fillParams(PropPanel2 &panel);
  virtual void updateToPanel(PropPanel2 &panel);

  virtual bool brushPaintApply(int x, int y, float inc, bool rb) { return true; }

  virtual void saveToBlk(DataBlock &blk) const;
  virtual void loadFromBlk(const DataBlock &blk);

  virtual bool updateFromPanelRef(PropPanel2 &panel, int pid);
};

#endif //__GAIJIN_HEIGHTMAPLAND_SCRIPT_BRUSH__
