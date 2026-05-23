// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <drv/3d/dag_driverDesc.h>
#include <generic/dag_expected.h>

#include <EASTL/unique_ptr.h>
#include <EASTL/string.h>
#include <EASTL/vector.h>
#include "vulkan_api.h"

struct _xess_context_handle_t;

namespace drv3d_vulkan
{

struct CmdExecuteXESS;
class Image;

struct XessFgParamsVulkan
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

struct XessFgParamsVulkanResourceStates
{
  uint32_t inColorHudlessState;
  uint32_t inUiState;
  uint32_t inDepthState;
  uint32_t inMotionVectorsState;
};

class Xess
{
  mutable eastl::vector<eastl::string> requiredDeviceExtensions;
  mutable eastl::vector<eastl::string> requiredInstanceExtensions;
  mutable uint32_t reqApiVersion;
  _xess_context_handle_t *context = nullptr;
  XessState state = XessState::DISABLED;
  uint32_t desiredOutputResolutionX = 0, desiredOutputResolutionY = 0;
  uint32_t optimalRenderResolutionX = 0, optimalRenderResolutionY = 0;
  uint32_t minRenderResolutionX = 0, minRenderResolutionY = 0;
  uint32_t maxRenderResolutionX = 0, maxRenderResolutionY = 0;
  mutable bool requiredDeviceExtPass = false;
  mutable bool requiredInstanceExtPass = false;

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

  void load();
  void unload();
  bool init();
  bool createFeature(int quality, uint32_t target_width, uint32_t target_height);
  bool shutdown();
  void failDeviceExt();
  void failInstanceExt();

  XessState getState() const;
  void getRenderResolution(int &w, int &h, int &minw, int &minh, int &maxw, int &maxh) const;
  bool evaluate(const CmdExecuteXESS &params);
  void setVelocityScale(float x, float y);
  bool isQualityAvailableAtResolution(uint32_t target_width, uint32_t target_height, int xess_quality) const;
  dag::Expected<eastl::string, ErrorKind> getVersion() const;
  Xess();
  ~Xess();

  void startDump(const char *path, uint32_t numberOfFrames);

  bool isFrameGenerationSupported() const;
  bool isFrameGenerationEnabled() const;
  void enableFrameGeneration(bool enable);
  void suppressFrameGeneration(bool suppress);
  void doScheduleGeneratedFrames(const XessFgParamsVulkan &fgArgs, const XessFgParamsVulkanResourceStates &resourceStates);
  int getPresentedFrameCount();
  uint64_t getMemoryUsage() const;

  const eastl::vector<eastl::string> &getRequiredDeviceExtensions() const;
  const eastl::vector<eastl::string> &getRequiredInstanceExtensions() const;
  void injectRequiredFeatures(VkPhysicalDeviceFeatures2KHR &features2) const;
  void patchApiVersion(uint32_t &api_ver);
};

} // namespace drv3d_vulkan
