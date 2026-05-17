// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "render/antiAliasing_legacy.h"
#include <render/resourceSlot/nodeHandleWithSlotsAccess.h>
#include <shaders/dag_postFxRenderer.h>

class TemporalSuperResolution : public AntiAliasing
{
public:
  enum class Preset
  {
    High = 1
  };

  TemporalSuperResolution(const IPoint2 &outputResolution);
  ~TemporalSuperResolution();

  bool needMotionVectors() const override;
  bool supportsReactiveMask() const override;

  bool supportsDynamicResolution() const override { return true; }

  static TemporalSuperResolution::Preset parse_preset();

  float getLodBias() const override;
  bool isAvailable() const override { return available; }

private:
  bool available;
  dafg::NodeHandle applierNode;
  Preset preset;
};
