// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <drv/3d/dag_driverDesc.h>
#include <generic/dag_expected.h>

#include <EASTL/unique_ptr.h>
#include <EASTL/string.h>

struct ID3D12CommandList;

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
  uint32_t inInputWidth;
  uint32_t inInputHeight;
  uint32_t inColorDepthOffsetX;
  uint32_t inColorDepthOffsetY;
  bool inReset;

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

struct XessFgParamsDx12ResourceStates
{
  uint32_t inColorHudlessState;
  uint32_t inUiState;
  uint32_t inDepthState;
  uint32_t inMotionVectorsState;
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
  void getXeSSRenderResolution(int &w, int &h, int &minw, int &minh, int &maxw, int &maxh) const;
  bool evaluateXess(void *context, const void *dlss_params);
  void setVelocityScale(float x, float y);
  bool isXessQualityAvailableAtResolution(uint32_t target_width, uint32_t target_height, int xess_quality) const;
  dag::Expected<eastl::string, ErrorKind> getVersion() const;
  XessWrapper();
  ~XessWrapper();

  void startDump(const char *path, uint32_t numberOfFrames);

  bool isFrameGenerationSupported() const;
  bool isFrameGenerationEnabled() const;
  void enableFrameGeneration(bool enable);
  void suppressFrameGeneration(bool suppress);
  void doScheduleGeneratedFrames(const XessFgParamsDx12 &fgArgs, const XessFgParamsDx12ResourceStates &resourceStates,
    ID3D12CommandList *d3dCommandList);
  int getPresentedFrameCount();
  uint64_t getMemoryUsage() const;

private:
  eastl::unique_ptr<XessWrapperImpl> pImpl;
};
} // namespace drv3d_dx12
