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
struct BloomAdaptationParams
{
  bool isFixedExposure = false;
  BaseTexture *lumaTex;
  BloomAdaptationParams(bool is_fixed_exposure, BaseTexture *luma_tex) : lumaTex(luma_tex), isFixedExposure(is_fixed_exposure) {}
};

struct BloomSettings
{
  float threshold = 0.8f, upSample = 0.65f, radius = 2.f, mul = 0.8f;
  bool highQuality = false;
};

class BloomPS
{
public:
  BloomPS() : width(0), height(0), mipsCount(0) {}
  ~BloomPS() { close(); }
  void close();
  void getLumaResolution(int &w, int &h);
  void init(int w, int h);
  void downsample();
  void perform(BloomAdaptationParams *adaptation_params, bool forceHq);
  const BloomSettings &getSettings() const { return settings; }
  void setSettings(const BloomSettings &settings_) { settings = settings_; }
  TextureIDPair getDownsampledFrame() { return TextureIDPair(downsampledFrame.getTex2D(), downsampledFrame.getTexId()); }
  void setOn(bool on_) { on = on_; }
  bool isOn() const { return on; }

protected:
  PostFxRenderer downsample_first, downsample_13, downsample_4, upSample, blur_4;
  enum
  {
    MAX_MIPS = 7,
    MAX_UI_MIP = 2
  };
  UniqueTex downsampledFrame; // 1/2
  UniqueTexHolder bloomMips;  // 1/4 and smaller mips
  UniqueTex bloomLastMip;     // (1/4 or lower) and smaller mips
  int width = 0, height = 0, mipsCount = 0;
  int bloomLastMipStartMip = 100;
  BloomSettings settings;
  bool on = true;
};
