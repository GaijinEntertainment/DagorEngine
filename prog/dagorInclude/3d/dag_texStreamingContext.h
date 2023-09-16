//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <3d/dag_drv3d.h>

class TexStreamingContext
{
  float multiplicator;

public:
  // Explicitly create context with specified multiplicator, which is (current_perspective.wk * current_rendering_res_width)^2
  // Can be used in two cases:
  // 1. TexStreamingContext(0), when streaming of textures is not needed (shadows, depth-only rendering, etc). Will request lowest mip
  // 2. TexStreamingContext(FLT_MAX) - all used textures will request maximum quality mip level. Should be avoided if possible,
  //    since it reduces effect from texture streaming with computed mip levels. Better try to use some existing context,
  //    built from perspective and width, or create a new one this way.
  TexStreamingContext() : multiplicator(NAN) {}
  TexStreamingContext(float mul) : multiplicator(mul) {}
  // Create a context correctly computing texture mips for streaming
  TexStreamingContext(const Driver3dPerspective &persp, int width);
  int getTexLevel(float texScale, float distSq = 0.0f) const;
};
