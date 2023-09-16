//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
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
