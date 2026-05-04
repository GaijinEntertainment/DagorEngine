//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <math/dag_TMatrix4.h>
#include <render/viewDependentResource.h>
#include <render/daFrameGraph/nodeHandle.h>
#include <resourcePool/resourcePool.h>
#include <math/integer/dag_IPoint2.h>
#include <math/dag_rectInt.h>
#include <shaders/dag_postFxRenderer.h>
#include <shaders/dag_computeShaders.h>

#include <EASTL/optional.h>

class TextureIDPair;

class TemporalSuperResolution
{
public:
  enum class Preset
  {
    Low,
    High,
    Vr
  };

  static Preset parse_preset(bool vr_mode);

  TemporalSuperResolution(const IPoint2 &input_resolution, const IPoint2 &output_resolution, bool vr_mode = false);
  void releaseShaderResources();
  void setExtraTexFlags(unsigned int flags) { extraTexFlags = flags; }
  dafg::NodeHandle createApplierNode(const char *input_name);

  void apply(Texture *in_color, Texture *out_color, Texture *history_color, Texture *out_confidence, Texture *history_confidence,
    Texture *reactive_tex, Texture *debug_texture, const Point4 &uv_transform, bool reset = false,
    Point2 jitterPixelOffset = Point2::ZERO, Texture *vrs_mask = nullptr, IPoint2 input_resolution = IPoint2::ZERO);

  TextureIDPair getDebugRenderTarget();

  bool isValid() const { return true; }
  const IPoint2 &getInputResolution() const { return inputResolution; }
  bool isUpsampling() const { return inputResolution != outputResolution; }
  float getLodBias() const { return lodBias; }

private:
  UniqueTex debugTex;

  const IPoint2 inputResolution;
  const IPoint2 outputResolution;

  eastl::unique_ptr<ComputeShaderElement> render_cs;
  eastl::unique_ptr<ComputeShaderElement> render_vr_cs;

  UniqueBuf filterWeightsBuf;

  float lodBias;

  unsigned int extraTexFlags = 0;

  Preset preset;
};
