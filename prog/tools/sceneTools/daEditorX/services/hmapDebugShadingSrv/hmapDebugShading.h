// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <3d/dag_textureIDHolder.h>
#include <math/dag_e3dColor.h>

struct HeightmapColorGradient;

class HeightmapDebugShading
{
public:
  void update(const HeightmapColorGradient *gradient_to_show);

private:
  static E3DCOLOR getGradientColor(const HeightmapColorGradient &gradient, float height);
  void updateGradientdTexture(const HeightmapColorGradient &gradient, float min_height, float max_height);
  void updateGradientShader(const HeightmapColorGradient *gradient_to_show);

  static constexpr int TEXTURE_WIDTH = 256;

  TextureIDHolder gradientTexture;
};
