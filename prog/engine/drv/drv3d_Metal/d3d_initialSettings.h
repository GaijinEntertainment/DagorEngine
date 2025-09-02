// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <math/integer/dag_IPoint2.h>

struct D3dInitialSettings
{
  IPoint2 resolution;
  bool vsync;
  bool allowRetinaRes;

  D3dInitialSettings(int screen_x, int screen_y);
};