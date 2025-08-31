// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "render/antiAliasing.h"
#include <render/resourceSlot/nodeHandleWithSlotsAccess.h>
#include <shaders/dag_postFxRenderer.h>

class ComputeShaderElement;

class TemporalSuperResolution : public AntiAliasing
{
public:
  enum class Preset
  {
    Low,
    High
  };

  TemporalSuperResolution(const IPoint2 &outputResolution);

  Point2 update(Driver3dPerspective &perspective) override;

  int getTemporalFrameCount() const override;

  bool needMotionVectors() const override;
  bool supportsReactiveMask() const override;

  float getLodBias() const override { return lodBias; }

  static void load_settings();
  static bool is_enabled();

  bool supportsDynamicResolution() const override { return preset == Preset::High; }

  static TemporalSuperResolution::Preset parse_preset();

private:
  Texture *getDebugRenderTarget();

  eastl::unique_ptr<ComputeShaderElement> computeRenderer;
  UniqueTex debugTex;
  dafg::NodeHandle applierNode;
  Preset preset;
};

void tsr_render(const TextureIDPair &in_color,
  ManagedTexView out_color,
  ManagedTexView history_color,
  ManagedTexView out_confidence,
  ManagedTexView history_confidence,
  Texture *debug_texture,
  const ComputeShaderElement *compute_shader,
  IPoint2 output_resolution);
