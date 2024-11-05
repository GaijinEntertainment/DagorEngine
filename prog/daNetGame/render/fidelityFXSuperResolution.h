// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <render/antiAliasing.h>
#include <render/resourceSlot/nodeHandleWithSlotsAccess.h>

namespace amd
{
struct FSR;
}

class FidelityFXSuperResolution : public AntiAliasing
{
public:
  FidelityFXSuperResolution(const IPoint2 &outputResolution, bool enableHdr);

  virtual ~FidelityFXSuperResolution();

  bool needMotionVectors() const override { return true; };
  bool supportsReactiveMask() const override { return true; }

  void setDt(float dt) { deltaTimeMs = dt * 1000.0f; }

  void render(Texture *in_color, Texture *out_color, const AntiAliasing::OptionalInputParams &params);

  static bool is_enabled();

private:
  eastl::unique_ptr<amd::FSR> fsr;
  int dlss_jitter_offsetVarId = get_shader_glob_var_id("dlss_jitter_offset");
  resource_slot::NodeHandleWithSlotsAccess applierNode;
  float deltaTimeMs;
};
