//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <math/dag_e3dColor.h>

namespace game_dbg
{
struct TextStyle
{
  enum class Anchor : uint8_t
  {
    CENTER,
    TOP_RIGHT
  };

  Anchor anchor = Anchor::TOP_RIGHT;
  float offsetX = 0.f;
  float offsetY = 0.f;
  E3DCOLOR color = E3DCOLOR_MAKE(255, 255, 255, 255);
};

} // namespace game_dbg
