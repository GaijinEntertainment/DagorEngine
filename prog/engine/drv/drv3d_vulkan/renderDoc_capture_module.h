#pragma once

#include <cstdint>

struct RENDERDOC_API_1_5_0;

namespace drv3d_vulkan
{
class RenderDocCaptureModule
{
public:
  RenderDocCaptureModule();
  ~RenderDocCaptureModule();

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
