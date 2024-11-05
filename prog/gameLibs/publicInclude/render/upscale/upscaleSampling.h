//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <EASTL/unique_ptr.h>
#include <3d/dag_resizableTex.h>

class UpscaleSamplingTex
{
  UniqueTex upscaleTex;

public:
  UpscaleSamplingTex(uint32_t w, uint32_t h, const char *tag = "");
  TEXTUREID getTexId() const { return upscaleTex.getTexId(); }
  ~UpscaleSamplingTex();

  void render(float goffset_x = 0, float goffset_y = 0);
  void onReset() {}
};
