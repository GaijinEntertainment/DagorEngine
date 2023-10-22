#pragma once

#include <math/integer/dag_IPoint2.h>

struct D3dInitialSettings
{
  IPoint2 nonaaZbufSize, aaZbufSize;
  IPoint2 resolution;
  int maxThreads;
  bool vsync;
  bool allowRetinaRes;
  bool useMpGLEngine;
  uint32_t max_genmip_tex_sz;

  D3dInitialSettings(int screen_x, int screen_y);
};