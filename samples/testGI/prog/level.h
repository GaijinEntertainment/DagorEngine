// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <heightmap/heightmapHandler.h>
#include "dag_cur_view.h"
class StrmSceneHolder : public BaseStreamingSceneHolder
{
public:
  typedef BaseStreamingSceneHolder base;
  using BaseStreamingSceneHolder::mainBindump;
  HeightmapHandler heightmap;
  enum
  {
    No,
    Loaded,
    Inited
  } state = No;
  void loadHeightmap(const char *fn)
  {
    if (!fn)
      return;
    FullFileLoadCB crd(fn);
    if (!crd.fileHandle)
      logerr("can't open %s", fn);
    else
    {
      if (crd.beginTaggedBlock() != _MAKE4C('HM2'))
      {
        logerr("incorrect block");
      }
      else
      {
        if (!heightmap.loadDump(crd, true, nullptr))
          logerr("can't load %s", fn);
        else
        {
          debug("loaded heightmap from %s", fn);
          state = Loaded;
        }
      }
    }
  }
  void renderHeightmap(const Point3 viewPos)
  {
    if (state != Inited)
    {
      if (state == Loaded)
      {
        heightmap.init();
        state = Inited;
      }
      else
        return;
    }
    TIME_D3D_PROFILE(heightmap);
    float camera_height = 4.0f, hmin = 0, htb = 0;
    if (heightmap.heightmapHeightCulling)
    {
      int lod = floorf(heightmap.heightmapHeightCulling->getLod(128.f));
      heightmap.heightmapHeightCulling->getMinMaxInterpolated(lod, Point2::xz(viewPos), hmin, htb);
      camera_height = max(camera_height, viewPos.y - htb);
    }

    if (heightmap.prepare(viewPos, camera_height))
    {
      heightmap.render(0);
    }
  }
  void render(const VisibilityFinder &vf, bool hmap = true)
  {
    base::render(vf);
    if (hmap)
      renderHeightmap(::grs_cur_view.itm.getcol(3));
  }
  void render(const VisibilityFinder &vf, int render_id, unsigned render_flags_mask)
  {
    base::render(vf, render_id, render_flags_mask);
  }
};
