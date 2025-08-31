//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <render/viewDependentResource.h>
#include <resourcePool/resourcePool.h>
#include <math/integer/dag_IPoint2.h>
#include <shaders/dag_postFxRenderer.h>
#include <3d/dag_textureIDHolder.h>
#include <drv/3d/dag_renderPass.h>

class SMAA
{
public:
  SMAA(const IPoint2 &resolution);
  void apply(const TextureIDPair &source, const TextureIDPair &destination);

private:
  PostFxRenderer edge_detect, blend_weights, apply_smaa;
  TextureIDHolderWithVar edgeDetect, blendWeights;
  SharedTexHolder areaTex, searchTex;
  TextureIDHolder depthStencilTex;
  const IPoint2 resolution;
};