//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <math/dag_Point2.h>
#include <math/integer/dag_IPoint2.h>
#include <3d/dag_resPtr.h>
#include <shaders/dag_overrideStateId.h>
#include <EASTL/unique_ptr.h>

class PostFxRenderer;

class HeatHazeRenderer
{
public:
  using RenderHazeParticlesCallback = eastl::function<void()>;
  using RenderCustomHazeCallback = eastl::function<void()>;
  using BeforeApplyHazeCallback = eastl::function<void()>;
  using AfterApplyHazeCallback = eastl::function<void()>;

  struct RenderTargets
  {
    TEXTUREID backBufferId = BAD_TEXTUREID;
    Texture *backBuffer = nullptr;
    const RectInt *backBufferArea = nullptr;
    TEXTUREID depthId = BAD_TEXTUREID;
    TEXTUREID resolvedDepthId = BAD_TEXTUREID;
    Texture *hazeTemp = nullptr;
    TEXTUREID hazeTempId = BAD_TEXTUREID;
    Texture *hazeOffset = nullptr;
    Texture *hazeDepth = nullptr;
    Texture *hazeColor = nullptr;
  };

  HeatHazeRenderer(int haze_resolution_divisor);
  ~HeatHazeRenderer();

  void renderHazeParticles(Texture *haze_depth, Texture *haze_offset, TEXTUREID depth_tex_id, int depth_tex_lod,
    RenderHazeParticlesCallback render_haze_particles, RenderCustomHazeCallback render_ri_haze, Texture *stencil = nullptr);
  void renderColorHaze(Texture *haze_color, RenderCustomHazeCallback render_haze_particles, RenderCustomHazeCallback render_ri_haze,
    Texture *stencil = nullptr);
  void applyHaze(double total_time, Texture *back_buffer, const RectInt *back_buffer_area, TEXTUREID back_buffer_id,
    TEXTUREID resolve_depth_tex_id, Texture *haze_temp, TEXTUREID haze_temp_id, const IPoint2 &back_buffer_resolution);

  void render(double total_time, const RenderTargets &targets, const IPoint2 &back_buffer_resolution, int depth_tex_lod,
    RenderHazeParticlesCallback render_haze_particles, RenderCustomHazeCallback render_ri_haze,
    BeforeApplyHazeCallback before_apply_haze = nullptr, AfterApplyHazeCallback after_apply_haze = nullptr);

  void clearTargets(Texture *haze_color, Texture *haze_offset, Texture *haze_depth);
  int getHazeResolutionDivisor() const { return hazeResolutionDivisor; }
  bool isHazeAppliedManual() const;

private:
  void setUp(int haze_resolution_divisor);
  void tearDown();

  bool areShadersValid();

  int hazeFxId;
  int hazeResolutionDivisor;

  TEXTUREID hazeNoiseTexId;

  Point2 hazeNoiseScale;
  Point2 hazeLuminanceScale;

  eastl::unique_ptr<PostFxRenderer> hazeFxRenderer;
  shaders::UniqueOverrideStateId zDisabledStateId, zDisabledBlendMinStateId;

  d3d::SamplerHandle sourceLinearTexSampler;
  d3d::SamplerHandle sourcePointTexSampler;
};
