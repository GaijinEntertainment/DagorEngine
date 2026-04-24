// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <render/antiAliasing_legacy.h>
#include <render/resourceSlot/nodeHandleWithSlotsAccess.h>
#include <shaders/dag_postFxRenderer.h>
#include <math/integer/dag_IPoint2.h>
#include <render/world/cameraParams.h>
#include <render/daFrameGraph/nodeHandle.h>

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

  void setInputResolution(const IPoint2 &) override { logerr("changing input resolution is not supported"); }

  bool needMotionVectors() const override { return true; };

  float getLodBias() const override;

  Point2 getJitterOffset() const;

  bool isFrameGenerationEnabled() const override;
  bool needsUIBlending() const override { return isFrameGenerationEnabled(); }

  bool needMotionVectorHistory() const override { return isFrameGenerationEnabled(); }
  bool needDepthHistory() const override { return isFrameGenerationEnabled(); }
  bool isAvailable() const override { return available; }

private:
  bool available;
  dafg::NodeHandle applierNode;
  dafg::NodeHandle frameGenerationNode;
  dafg::NodeHandle lifetimeExtenderNode;
  dafg::NodeHandle rayReconstructionPrepareNode;
  dafg::NodeHandle rayReconstructionWaterRenameNode;
  dafg::NodeHandle colorBeforeTransparencyNode;
};
