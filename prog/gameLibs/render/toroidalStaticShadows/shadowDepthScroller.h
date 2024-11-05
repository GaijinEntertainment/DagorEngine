// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <shaders/dag_postFxRenderer.h>
#include <shaders/dag_overrideStateId.h>
#include <3d/dag_textureIDHolder.h>
#include <EASTL/unique_ptr.h>

struct ShadowDepthScroller
{
  eastl::unique_ptr<PostFxRenderer> readDepthPS, writeDepthPS;
  TextureIDHolderWithVar tileTex;
  int tileSizeW = 512, tileSizeH = 256;
  shaders::UniqueOverrideStateId scrollStateId;
  bool translateDepth(Texture *tex, int layer, float scale, float ofs);
  void init();
};
