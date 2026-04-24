//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <render/mipRenderer.h>

enum class WaterReflectionBlurMode : uint8_t
{
  SimpleGauss = 0,
  Vertical = 1
};

class WaterReflectionsMipRenderer
{
protected:
  MipRenderer simpleMipRenderer;
  MipRenderer mipRenderer;

public:
  ~WaterReflectionsMipRenderer() {}
  WaterReflectionsMipRenderer() {}
  void close()
  {
    simpleMipRenderer.close();
    mipRenderer.close();
  }
  bool init(d3d::AddressMode addressMode, d3d::FilterMode filterMode)
  {
    bool init_simple_mips = simpleMipRenderer.init("water_blur_reflection_mips_simple", addressMode, filterMode);
    bool init_mips = mipRenderer.init("water_blur_reflection_mips", addressMode, filterMode);
    return init_simple_mips && init_mips;
  }
  void render(BaseTexture *tex, WaterReflectionBlurMode mode)
  {
    if (mode == WaterReflectionBlurMode::SimpleGauss)
      simpleMipRenderer.render(tex);
    else
      mipRenderer.render(tex);
  }
};