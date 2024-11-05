// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <cstdint>

struct RENDERDOC_API_1_5_0;

namespace drv3d_vulkan
{
class RenderDocCaptureModule
{
public:
  RenderDocCaptureModule() = default;
  RenderDocCaptureModule(RenderDocCaptureModule &&) = default;
  RenderDocCaptureModule &operator=(RenderDocCaptureModule &&) = default;
  ~RenderDocCaptureModule() {}

  void init();
  void shutdown();

  void triggerCapture(uint32_t count);
  void beginCapture();
  void endCapture();

private:
  const char *getLibName();
  void loadRenderDocAPI();

  void *docLib = nullptr;
  RENDERDOC_API_1_5_0 *docAPI = nullptr;
};
} // namespace drv3d_vulkan
