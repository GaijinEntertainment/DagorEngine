//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <3d/dag_resPtr.h>
#include <shaders/dag_postFxRenderer.h>

enum
{
  PREINTEGRATE_QUALITY_DEFAULT = 0,
  PREINTEGRATE_QUALITY_MAX = 2,

  PREINTEGRATE_SPECULAR_ONLY = 0,
  PREINTEGRATE_SPECULAR_DIFFUSE = 1,
  PREINTEGRATE_SPECULAR_DIFFUSE_QUALITY_MAX = PREINTEGRATE_SPECULAR_DIFFUSE | PREINTEGRATE_QUALITY_MAX,
};

class MultiFramePGF
{
public:
  MultiFramePGF() = default;
  MultiFramePGF(uint32_t preintegrate_type, uint32_t frames = 128, const char *name = "preIntegratedGF",
    const char *shaderName = "preintegrateEnvi");
  void init(uint32_t preintegrate_type = PREINTEGRATE_SPECULAR_DIFFUSE, uint32_t _frames = 128, const char *name = "preIntegratedGF",
    const char *shaderName = "preintegrateEnvi");
  void reset();
  void update()
  {
    if (frame < frames)
      updateFrame();
  }
  void close();
  void setFramesCount(uint32_t f)
  {
    f = f <= 1u ? 1u : f > 1024u ? 1024u : f;
    if (f == frames)
      return;
    frames = f;
    reset();
  }

private:
  void updateFrame();
  uint32_t frames = 1, frame = 0;
  PostFxRenderer shader;
  UniqueTexHolder preIntegratedGF;
};

TexPtr create_preintegrated_fresnel_GGX(const char *name, uint32_t preintegrate_type);
void render_preintegrated_fresnel_GGX(Texture *preIntegratedGF, PostFxRenderer *preintegrate_envi, uint32_t frame, uint32_t frames);

TexPtr render_preintegrated_fresnel_GGX(const char *name = "preIntegratedGF", uint32_t preintegrate_type = PREINTEGRATE_SPECULAR_ONLY,
  PostFxRenderer *preintegrate_envi = nullptr);
