//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <EASTL/unique_ptr.h>
#include <3d/dag_resizableTex.h>
#include <shaders/dag_postFxRenderer.h>
#include <shaders/dag_computeShaders.h>

class UpscaleSamplingTex
{
  UniqueBufHolder upscaleWeightsBuffer;
  UniqueTexHolder upscaleTex;
  PostFxRenderer upscaleRenderer;
  eastl::unique_ptr<ComputeShaderElement> upscaleRendererCS;

  void uploadWeights();

public:
  UpscaleSamplingTex(uint32_t w, uint32_t h);

  void render(float goffset_x = 0, float goffset_y = 0);
  void onReset();
};
