// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "hmlShadowsBrush.h"
#include "../hmlPlugin.h"
#include "../hmlCm.h"
#include <ioSys/dag_dataBlock.h>

#include <debug/dag_debug.h>


namespace heightmap_land
{
HmapLandBrush *getShadowsBrush(IBrushClient *client, IHmapBrushImage &height_map) { return new HmapShadowsBrush(client, height_map); }
}; // namespace heightmap_land


void HmapShadowsBrush::onBrushPaintEnd(int buttons, int key_modif)
{
  HmapLandPlugin::self->calcGoodLandLightingInBox(dirtyBrushBox);

  HmapLandBrush::onBrushPaintEnd(buttons, key_modif);
}


void HmapShadowsBrush::fillParams(PropPanel::ContainerPropertyControl &panel)
{
  panel.createEditFloat(PID_BRUSH_RADIUS, "Brush radius:", radius);
}


void HmapShadowsBrush::updateToPanel(PropPanel::ContainerPropertyControl &panel) { panel.setFloat(PID_BRUSH_RADIUS, radius); }


void HmapShadowsBrush::saveToBlk(DataBlock &blk) const { blk.addReal("radius", getRadius()); }


void HmapShadowsBrush::loadFromBlk(const DataBlock &blk) { setRadius(blk.getReal("radius", getRadius())); }


bool HmapShadowsBrush::updateFromPanelRef(PropPanel::ContainerPropertyControl &panel, int pid)
{
  if (pid == PID_BRUSH_RADIUS)
  {
    real rad = panel.getFloat(pid);

    const real sRad = setRadius(rad);
    if (sRad != rad)
      panel.setFloat(pid, sRad);

    return true;
  }

  return false;
}
