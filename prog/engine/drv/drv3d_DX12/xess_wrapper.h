// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <drv/3d/dag_consts.h>

#include <EASTL/unique_ptr.h>

namespace drv3d_dx12
{
class XessWrapperImpl;
class Image;

struct XessParamsDx12
{
  Image *inColor;
  Image *inDepth;
  Image *inMotionVectors;
  float inJitterOffsetX;
  float inJitterOffsetY;
  float inInputWidth;
  float inInputHeight;
  int inColorDepthOffsetX;
  int inColorDepthOffsetY;

  Image *outColor;
};

class XessWrapper
{
public:
  bool xessInit(void *device);
  bool xessCreateFeature(int quality, uint32_t target_width, uint32_t target_height);
  bool xessShutdown();

  XessState getXessState() const;
  void getXeSSRenderResolution(int &w, int &h) const;
  bool evaluateXess(void *context, const void *dlss_params);
  void setVelocityScale(float x, float y);
  bool isXessQualityAvailableAtResolution(uint32_t target_width, uint32_t target_height, int xess_quality) const;
  XessWrapper();
  ~XessWrapper();

  void startDump(const char *path, int numberOfFrames);

private:
  eastl::unique_ptr<XessWrapperImpl> pImpl;
};
} // namespace drv3d_dx12
