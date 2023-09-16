//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <EASTL/vector.h>
#include <math/integer/dag_IPoint2.h>
#include <3d/dag_resPtr.h>
#include <shaders/dag_postFxRenderer.h>
#include <shaders/dag_overrideStates.h>

class EsmShadows
{
public:
  ~EsmShadows() { close(); }

  void init(int w, int h, int slices, float esm_exp);
  void close();

  void beginRenderSlice(int slice_id);
  void endRenderSlice();

private:
  void blur(int slice);
  void initEsmShadowsStateId();

  UniqueTex esmShadowBlurTmp;
  UniqueTexHolder esmShadowArray;
  PostFxRenderer esmBlurRenderer;

  int currentSlice = -1;
  shaders::UniqueOverrideStateId esmShadowsStateId;
};
