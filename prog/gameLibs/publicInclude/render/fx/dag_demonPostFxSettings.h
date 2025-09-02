//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <string.h> // memset
#include <math/dag_color.h>
#include <math/dag_Point3.h>

struct DemonPostFxSettings
{
  DemonPostFxSettings() { memset(this, 0, sizeof(*this)); }
  uint32_t debugFlags;
  enum
  {
    NO_SCENE = 1 << 0,
    NO_VFOG = 1 << 1,
    NO_GLOW = 1 << 2,
    NO_STARS = 1 << 3
  };
  float hueShift;
  Color3 saturationColor;
  float saturation;
  Color3 grayColor;
  Color3 contrastColor;
  float contrast;
  Color3 contrastPivotColor;
  float contrastPivot;
  Color3 brightnessColor;
  float brightness;

  // Glow
  float glowRadius;

  float hdrDarkThreshold;
  float hdrGlowPower;
  float hdrGlowMul;

  // Fog
  float volfogRange;
  float volfogMul;
  Color3 volfogColor;
  Point3 sunDir; // must be normalized
  // bool useRawSkyMask;

  float volfogFade;
  float volfogMaxAngle;

  Point3 filmGrainParams;

  void setSunDir(const Point3 &dir2sun) { sunDir = dir2sun; }
};
