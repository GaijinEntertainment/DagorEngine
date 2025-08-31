// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "d3d_initialSettings.h"

#include <startup/dag_globalSettings.h>
#include <ioSys/dag_dataBlock.h>


D3dInitialSettings::D3dInitialSettings(int screen_w, int screen_h) :
  resolution(screen_w, screen_h), vsync(false), allowRetinaRes(false)
{
  const DataBlock &blk_video = *dgs_get_settings()->getBlockByNameEx("video");
  vsync = blk_video.getBool("vsync", vsync);
}
