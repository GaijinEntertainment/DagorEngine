//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

namespace darg
{

struct RenderState
{
  float opacity;
  bool transformActive;

  void reset()
  {
    opacity = 1.0f;
    transformActive = false;
  }
};


} // namespace darg
