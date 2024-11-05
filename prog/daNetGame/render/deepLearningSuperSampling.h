// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <render/antiAliasing.h>
#include <render/resourceSlot/nodeHandleWithSlotsAccess.h>
#include <shaders/dag_postFxRenderer.h>
#include <math/integer/dag_IPoint2.h>
#include <render/world/cameraParams.h>
#include <render/daBfg/nodeHandle.h>

class DeepLearningSuperSampling : public AntiAliasing
{
public:
  enum class Preset
  {
    ULTRA_PERFORMANCE = 3,
    PERFORMANCE = 0,
    BALANCED = 1,
    QUALITY = 2,
    DLAA = 5
  };

  DeepLearningSuperSampling(const IPoint2 &outputResolution);
  ~DeepLearningSuperSampling();

  void setInputResolution(const IPoint2 &) override { logerr("changing input resolution is not supported"); }

  bool needMotionVectors() const override { return true; };

  float getLodBias() const override;

  static void load_settings();
  static bool is_enabled();

  Point2 getJitterOffset() const;

  bool isFrameGenerationEnabled() const;

  bool needMotionVectorHistory() const override { return isFrameGenerationEnabled(); }
  bool needDepthHistory() const override { return isFrameGenerationEnabled(); }

private:
  resource_slot::NodeHandleWithSlotsAccess applierNode;
  dabfg::NodeHandle frameGenerationNode;
  dabfg::NodeHandle lifetimeExtenderNode;
};

void dlss_render(Texture *in_color,
  Texture *out_color,
  Point2 jitter_offset,
  const AntiAliasing::OptionalInputParams &params,
  const CameraParams &camera,
  const CameraParams &prev_camera);
