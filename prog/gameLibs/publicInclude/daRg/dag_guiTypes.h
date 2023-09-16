//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <debug/dag_debug.h>
#include <math/dag_Point2.h>
#include <math/dag_bounds2.h>
#include <daRg/dag_guiConstants.h>

namespace darg
{


struct ScreenCoord
{
  Point2 relPos, screenPos;
  Point2 size;
  Point2 contentSize;
  Point2 scrollOffs;

  void reset() { memset(this, 0, sizeof(*this)); }

  ScreenCoord() { reset(); }
};


} // namespace darg
