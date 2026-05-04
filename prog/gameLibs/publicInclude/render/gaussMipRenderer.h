//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <render/mipRenderer.h>

class GaussMipRenderer
{
protected:
  MipRenderer mipRenderer;

public:
  ~GaussMipRenderer() {}
  GaussMipRenderer() {}
  void close() { mipRenderer.close(); }
  bool init(d3d::AddressMode addressMode, bool alpha = false)
  {
    return mipRenderer.init(alpha ? "gaussian_mipchain_alpha" : "gaussian_mipchain", addressMode);
  }
  void render(BaseTexture *tex) { mipRenderer.render(tex); }
};
