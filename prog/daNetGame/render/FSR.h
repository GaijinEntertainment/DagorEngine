// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <render/antiAliasing_legacy.h>
#include <render/daFrameGraph/nodeHandle.h>

class FSR : public AntiAliasing
{
public:
  FSR(const IPoint2 &outputResolution);

  bool needMotionVectors() const override { return true; };

  void setDt(float dt) { deltaTimeMs = dt * 1000.0f; }

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
  float deltaTimeMs = 0.f;
};
