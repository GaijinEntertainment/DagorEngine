// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <render/antiAliasing.h>
#include <render/daFrameGraph/nodeHandle.h>

class FSR2 : public AntiAliasing
{
public:
  FSR2(const IPoint2 &outputResolution);

  bool needMotionVectors() const override { return true; };

  void setDt(float dt) { deltaTimeMs = dt * 1000.0f; }

  void render(Texture *in_color, Texture *out_color, const AntiAliasing::OptionalInputParams &params);

  static bool is_enabled();

  Point2 update(Driver3dPerspective &perspective);

private:
  int dlss_jitter_offsetVarId = get_shader_glob_var_id("dlss_jitter_offset");

  dafg::NodeHandle applierNode;
  float deltaTimeMs, cameraNear, cameraFar, fov;
  int frameIndex = 0;
};
