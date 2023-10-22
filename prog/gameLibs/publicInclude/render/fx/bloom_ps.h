//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <shaders/dag_postFxRenderer.h>
#include <shaders/dag_overrideStateId.h>
#include <3d/dag_resPtr.h>
#include <3d/dag_textureIDHolder.h>
#include <generic/dag_carray.h>

class ComputeShaderElement;

struct BloomSettings
{
  float upSample = 0.65f, radius = 2.f, mul = 0.8f;
  bool highQuality = false;
};

class BloomPS
{
public:
  BloomPS() : width(0), height(0), mipsCount(0) {}
  ~BloomPS() { close(); }
  void close();
  static void getLumaResolution(uint32_t postfx_width, uint32_t postfx_height, uint32_t &w, uint32_t &h)
  {
    w = postfx_width >> 4;
    h = postfx_height >> 4;
  }
  void init(int w, int h);
  void perform(ManagedTexView downsampled_frame);
  const BloomSettings &getSettings() const { return settings; }
  void setSettings(const BloomSettings &settings_) { settings = settings_; }
  void setOn(bool on_) { on = on_; }
  bool isOn() const { return on; }
  void setForceHighQuality(bool forceHq) { forceHighQuality = forceHq; }

protected:
  PostFxRenderer downsample_13, downsample_4, upSample, blur_4;
  enum
  {
    MAX_MIPS = 7,
    MAX_UI_MIP = 2
  };
  UniqueTexHolder bloomMips; // 1/4 and smaller mips
  UniqueTex bloomLastMip;    // (1/4 or lower) and smaller mips
  int width = 0, height = 0, mipsCount = 0;
  int bloomLastMipStartMip = 100;
  BloomSettings settings;
  bool on = true;
  bool forceHighQuality = false;
};
