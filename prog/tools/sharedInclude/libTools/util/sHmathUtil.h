//
// Dagor Tech 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <math/dag_math3d.h>
#include <math/dag_color.h>
#include <math/dag_SHmath.h>

class SH3Color3TransformCB
{
public:
  virtual Color3 transform(const Color3 &color) = 0;
};

// Transform spherical function represented by SH through arbitrary function.
void transform_sphharm(const Color3 inputSH[SPHHARM_NUM3], Color3 outputSH[SPHHARM_NUM3], SH3Color3TransformCB &callback,
  int subdivisions = 32);

// void add_sphere_light_sphharm(Color4 outputSH[SPHHARM_NUM3],
//   const Point3 &pos, real r2, const Color3 &color);

Color4 recompute_color(const Point3 &n, const Color4 sphHarm[SPHHARM_NUM3]);
