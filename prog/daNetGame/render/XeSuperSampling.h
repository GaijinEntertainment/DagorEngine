// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <render/antiAliasing.h>
#include <render/resourceSlot/nodeHandleWithSlotsAccess.h>

class XeSuperSampling : public AntiAliasing
{
public:
  XeSuperSampling(const IPoint2 &outputResolution);

  bool needMotionVectors() const override { return true; };

  static bool is_enabled();

private:
  dabfg::NodeHandle applierNode;
};

void xess_render(Texture *in_color,
  Texture *out_color,
  Point2 input_resolution,
  Point2 jitter_offset,
  const AntiAliasing::OptionalInputParams &params);
