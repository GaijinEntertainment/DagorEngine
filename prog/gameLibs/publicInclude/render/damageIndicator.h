//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <math/dag_color.h>

struct DamageIndicatorDrawConfig
{
  bool rotateTowardsCamera = true;
  bool drawOutline = true;
  bool drawSilhouette = false;
  bool drawTargetingSightLine = false;
  float rotationOffset = 0.0f;
  Color4 modulateColor = Color4(1, 1, 1, 1);
};
