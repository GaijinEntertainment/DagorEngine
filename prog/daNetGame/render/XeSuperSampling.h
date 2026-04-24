// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <render/antiAliasing_legacy.h>
#include <render/resourceSlot/nodeHandleWithSlotsAccess.h>

class XeSuperSampling : public AntiAliasing
{
public:
  XeSuperSampling(const IPoint2 &outputResolution);

  bool needMotionVectors() const override { return true; };

  float getLodBias() const override;

  bool isFrameGenerationEnabled() const override;

  bool needMotionVectorHistory() const override { return isFrameGenerationEnabled(); }
  bool needDepthHistory() const override { return isFrameGenerationEnabled(); }
  bool isAvailable() const override { return available; }

private:
  bool available;
  dafg::NodeHandle applierNode;
  dafg::NodeHandle frameGenerationNode;
  dafg::NodeHandle lifetimeExtenderNode;
};
