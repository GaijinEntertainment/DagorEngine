//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <math/dag_mathBase.h>
#include <math/dag_color.h>


enum
{
  SPHHARM_00 = 0,
  SPHHARM_1m1,
  SPHHARM_10,
  SPHHARM_1p1,
  SPHHARM_2m2,
  SPHHARM_2m1,
  SPHHARM_20,
  SPHHARM_2p1,
  SPHHARM_2p2,
  SPHHARM_NUM3
};

#define SPHHARM_COEF_0  0.282095f
#define SPHHARM_COEF_1  0.488603f
#define SPHHARM_COEF_21 1.092548f
#define SPHHARM_COEF_20 0.315392f
#define SPHHARM_COEF_22 0.546274f

// Rotates SH from world space to local, rotation matrix must be orthonormal.
// NOTE: orthonormal matrix inverse is transpose.
void rotate_sphharm(const Color3 worldSH[SPHHARM_NUM3], Color3 localSH[SPHHARM_NUM3], const TMatrix &local2world);

// Calculate SH of hemisphere filled with single color and add it to outputSH.
void add_hemisphere_sphharm(Color3 outputSH[SPHHARM_NUM3], const Point3 &dir, real cos_alpha, const Color3 &color);

// Calculate SH of solid sphere light and add it to outputSH.
void add_sphere_light_sphharm(Color3 outputSH[SPHHARM_NUM3], const Point3 &relative_position, real radius_squared,
  const Color3 &color);
