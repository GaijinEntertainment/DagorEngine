//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

class IRenderWorld
{
public:
  virtual void render() = 0;
  virtual void renderTrans() = 0;
  virtual void renderToShadowMap() = 0;
};
