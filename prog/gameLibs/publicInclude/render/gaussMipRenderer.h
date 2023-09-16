//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
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
  bool init() { return mipRenderer.init("gaussian_mipchain"); }
  void render(BaseTexture *tex) { mipRenderer.render(tex); }
};
