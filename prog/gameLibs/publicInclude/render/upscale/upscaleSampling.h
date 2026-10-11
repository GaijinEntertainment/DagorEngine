//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <EASTL/unique_ptr.h>
#include <3d/dag_resizableTex.h>
#include <resourcePool/resourcePool.h>
#include <math/integer/dag_IPoint2.h>

class BaseTexture;

class UpscaleSamplingRenderer
{
public:
  UpscaleSamplingRenderer();
  ~UpscaleSamplingRenderer();
  UpscaleSamplingRenderer(const UpscaleSamplingRenderer &) = delete;
  UpscaleSamplingRenderer &operator=(const UpscaleSamplingRenderer &) = delete;

  uint32_t getTexFlags() const;
  void render(BaseTexture *target, const IPoint2 &resolution, const IPoint2 &gbuffer_offset = IPoint2::ZERO) const;
};

class UpscaleSamplingTex
{
  RTargetPool::Ptr upscaleTexRTPool;
  RTarget::Ptr upscaleTex;
  int upscaleSamplingTexVarId;
  UpscaleSamplingRenderer renderer;

public:
  UpscaleSamplingTex(uint32_t w, uint32_t h, const char *tag = "");
  TEXTUREID getTexId(bool optional = false) const
  {
    G_ASSERT_RETURN(optional || upscaleTex, BAD_TEXTUREID);
    return upscaleTex ? upscaleTex->getTexId() : BAD_TEXTUREID;
  }

  void render(float goffset_x = 0, float goffset_y = 0);
  void onReset() {}
  void releaseRTs();
};
