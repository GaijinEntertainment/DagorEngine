//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <shaders/dag_postFxRenderer.h>
#include <shaders/dag_computeShaders.h>
#include <EASTL/unique_ptr.h>

class TextureIDPair;
class BaseTexture;
class ComputeShaderElement;

class MipRenderer
{
protected:
  PostFxRenderer mipRenderer;
  eastl::unique_ptr<ComputeShaderElement> mipRendererCS;

public:
  ~MipRenderer();
  MipRenderer();
  MipRenderer(const char *shader) { init(shader); }
  MipRenderer(MipRenderer &&) = default;
  MipRenderer &operator=(MipRenderer &&) = default;
  void close();
  bool init(const char *shader);
  void renderTo(BaseTexture *src, BaseTexture *dst, const IPoint2 &target_size) const;
  void render(BaseTexture *tex, uint8_t max_level = 255) const;
};
