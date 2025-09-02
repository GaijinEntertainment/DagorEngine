//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

struct Driver3dPerspective;

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
  TexStreamingContext() : multiplicator(-1.f) {}
  TexStreamingContext(float mul) : multiplicator(mul) {}
  // Create a context correctly computing texture mips for streaming
  TexStreamingContext(const Driver3dPerspective &persp, int width);
  int getTexLevel(float texScale, float distSq = 0.0f) const;

  constexpr static int MIN_TEX_LEVEL = 1;
  constexpr static int MIN_NON_STUB_LEVEL = MIN_TEX_LEVEL + 1;
  constexpr static int MAX_TEX_LEVEL = 15;
};
