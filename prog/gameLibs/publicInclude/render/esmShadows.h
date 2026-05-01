//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <util/dag_inttypes.h>
#include <EASTL/vector.h>
#include <math/integer/dag_IPoint2.h>
#include <3d/dag_resPtr.h>
#include <shaders/dag_postFxRenderer.h>
#include <shaders/dag_overrideStates.h>
#include <shaders/dag_DynamicShaderHelper.h>

class EsmShadows
{
public:
  ~EsmShadows() { close(); }

  void init(int w, int h, int slices, float esm_exp);
  void close();

  void beginRenderSlice(uint32_t slice_id);
  void endRenderSlice();

  ShaderElement *getShader() const { return esmDepthShader.shader.get(); }

private:
  void blur(uint32_t slice);
  void initEsmShadowsStateId();

  UniqueTex esmShadowBlurTmp;
  UniqueTexHolder esmShadowArray;
  PostFxRenderer esmBlurRenderer;
  DynamicShaderHelper esmDepthShader;

  int currentSlice = -1;
  shaders::UniqueOverrideStateId esmShadowsStateId;
};
