// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <3d/dag_resPtr.h>
#include <shaders/dag_postFxRenderer.h>
#include <3d/dag_resourcePool.h>
#include <3d/dag_textureIDHolder.h>
#include <render/daBfg/nodeHandle.h>

class AdaptationCS;
class ColorGradingLUT;
class FullColorGradingTonemapLUT;
class WorldRenderer;

namespace ecs
{
class Object;
}

struct PostFxManager
{
  TEXTUREID fireOnScreenTexture = BAD_TEXTUREID;
  TEXTUREID smokeBlackoutTexture = BAD_TEXTUREID;
  eastl::unique_ptr<ColorGradingLUT> gradingLUT;
  eastl::unique_ptr<FullColorGradingTonemapLUT> fullGradingLUT;

  void close();
  void setResolution(const IPoint2 &postfx_resolution, const IPoint2 &rendering_resolution, const WorldRenderer &world_renderer);
  void init(const WorldRenderer &world_renderer);
  void set(const ecs::Object &postFx);
  void prepare(const TextureIDPair &currentAntiAliasedTarget,
    ManagedTexView downsampled_frame,
    ManagedTexView closeDepth,
    ManagedTexView depth,
    float zn,
    float zf,
    float hk);

  ~PostFxManager();
  PostFxManager();
  PostFxManager(const PostFxManager &) = delete;
  PostFxManager(PostFxManager &&) = delete;
  PostFxManager &operator=(const PostFxManager &) = delete;
  PostFxManager &operator=(PostFxManager &&) = delete;
};
