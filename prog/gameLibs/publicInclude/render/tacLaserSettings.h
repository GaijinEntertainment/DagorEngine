//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

struct TacLaserRenderSettings
{
  float minIntensityCos = 0.0f, minIntensityMul = 1.0f;
  float maxIntensityCos = 1.0f, maxIntensityMul = 1.0f;
  float fogIntensityMul = 0.0f;
  float fadeDistance = 25.0f;
};
