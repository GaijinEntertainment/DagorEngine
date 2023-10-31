#include "hmlScriptBrush.h"
#include "../hmlPlugin.h"
#include "../hmlCm.h"
#include <ioSys/dag_dataBlock.h>

#include <debug/dag_debug.h>


namespace heightmap_land
{
HmapLandBrush *getScriptBrush(IBrushClient *client, IHmapBrushImage &height_map) { return new HmapScriptBrush(client, height_map); }
}; // namespace heightmap_land


void HmapScriptBrush::fillParams(PropPanel2 &panel) { panel.createEditFloat(PID_BRUSH_RADIUS, "Brush radius:", radius); }


void HmapScriptBrush::updateToPanel(PropPanel2 &panel) { panel.setFloat(PID_BRUSH_RADIUS, radius); }


void HmapScriptBrush::saveToBlk(DataBlock &blk) const { blk.addReal("radius", getRadius()); }


void HmapScriptBrush::loadFromBlk(const DataBlock &blk) { setRadius(blk.getReal("radius", getRadius())); }


bool HmapScriptBrush::updateFromPanelRef(PropPanel2 &panel, int pid)
{
  if (pid == PID_BRUSH_RADIUS)
  {
    const real rad = panel.getFloat(pid);

    const real sRad = setRadius(rad);
    if (sRad != rad)
      panel.setFloat(pid, sRad);

    return true;
  }

  return false;
}
