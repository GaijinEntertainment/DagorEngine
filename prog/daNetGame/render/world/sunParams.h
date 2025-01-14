// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <math/dag_Point3.h>
#include <math/dag_color.h>

struct SunParams
{
  Point3 dirToSun;
  Color3 sunColor;
  // The sun's position in the sky is subject to a reprojection while panorama sky is used.
  // It's calculated at the current camera position.
  // This direction points towards the sun as it appears on the sky (for non-panorama, it's the same as dirToSun)
  Point3 panoramaDirToSun;
};