// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "landMinMax_api.h"
#include <heightmap/heightmapHandler.h>
#include <landMesh/lmeshManager.h>
#include <math/dag_bounds2.h>

namespace rendinst::gen
{
void land_min_max(LandMeshManager *lmesh, const BBox2 &bbox_xz, float &out_min, float &out_max)
{
  const float LAND_HEIGHT_DELTA = 100.0f;
  out_min = MAX_REAL;
  out_max = MIN_REAL;
  if (lmesh && lmesh->getNumCellsX() > 0 && lmesh->getNumCellsY() > 0)
  {
    const float eps = 1e-3;
    for (int z = floorf(bbox_xz.lim[0].y / lmesh->getLandCellSize() + eps);
         z <= floorf(bbox_xz.lim[1].y / lmesh->getLandCellSize() - eps); z++)
    {
      for (int x = floorf(bbox_xz.lim[0].x / lmesh->getLandCellSize() + eps);
           x <= floorf(bbox_xz.lim[1].x / lmesh->getLandCellSize() - eps); x++)
      {
        BBox3 bbox = lmesh->getBBox(x, z);
        out_min = min(out_min, bbox.lim[0].y);
        out_max = max(out_max, bbox.lim[1].y);
      }
    }
  }
  else if (HeightmapHandler *hmap = lmesh ? lmesh->getHmapHandler() : nullptr)
  {
    float hmin, hmax;
    if (hmap->getHeightmapHeightMinMaxInChunk(bbox_xz.lim[0], max(bbox_xz.width().x, bbox_xz.width().y), hmin, hmax))
    {
      out_min = hmin;
      out_max = hmax;
    }
  }

  if (out_min > out_max)
  {
    const float land_height_fallback = 8192.f;
    out_min = 0.f;
    out_max = land_height_fallback;
  }
  else
  {
    out_min -= LAND_HEIGHT_DELTA / 5.0f;
    out_max += LAND_HEIGHT_DELTA;
  }
}
} // namespace rendinst::gen
