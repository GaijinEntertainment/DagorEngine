// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <drv/3d/dag_consts.h>
#include <generic/dag_expected.h>

#include <EASTL/unique_ptr.h>
#include <EASTL/string.h>

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

struct XessFgParamsDx12
{
  float viewTm[16];
  float projTm[16];
  Image *inColorHudless;
  Image *inUi;
  Image *inDepth;
  Image *inMotionVectors;
  float inJitterOffsetX;
  float inJitterOffsetY;
  float inMotionVectorScaleX;
  float inMotionVectorScaleY;
  uint32_t inFrameIndex;
  bool inReset;
};

class XessWrapper
{
public:
  enum class ErrorKind
  {
    UnsupportedDevice,
    UnsupportedDriver,
    Uninitialized,
    InvalidArgument,
    DeviceOutOfMemory,
    Device,
    NotImplemented,
    InvalidContext,
    OperationInProgress,
    Unsupported,
    CantLoadLibrary,
    Unknown
  };
  static eastl::string errorKindToString(ErrorKind kind);

  bool xessInit(void *device);
  bool xessCreateFeature(int quality, uint32_t target_width, uint32_t target_height);
  bool xessShutdown();

  XessState getXessState() const;
  void getXeSSRenderResolution(int &w, int &h) const;
  bool evaluateXess(void *context, const void *dlss_params);
  void setVelocityScale(float x, float y);
  bool isXessQualityAvailableAtResolution(uint32_t target_width, uint32_t target_height, int xess_quality) const;
  dag::Expected<eastl::string, ErrorKind> getVersion() const;
  XessWrapper();
  ~XessWrapper();

  void startDump(const char *path, int numberOfFrames);

  bool isFrameGenerationSupported() const;
  bool isFrameGenerationEnabled() const;
  void enableFrameGeneration(bool enable);
  void suppressFrameGeneration(bool suppress);
  void doScheduleGeneratedFrames(const void *args, void *command_list);
  int getPresentedFrameCount();

private:
  eastl::unique_ptr<XessWrapperImpl> pImpl;
};
} // namespace drv3d_dx12
