// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "swapchain.h"

namespace drv3d_vulkan
{

struct CmdSwapchainImageAcquire
{
  Image *img;
  VulkanSemaphoreHandle acquireSem;
};

struct CmdPresent
{
  WinCritSec *mutex;
  VulkanSwapchainKHRHandle swapchain;
  VulkanSemaphoreHandle frameSem;
  Image *img;
  uint32_t imageIndex;
  uint32_t *currentAcquireId;
  uint32_t submitAcquireId;
  uint32_t *backendAcquireImageIndex;
  VkResult *backendAcquireResult;
  SwapchainImage *images;
  uint32_t enginePresentFrameId;

  bool isFullBackendAcquire() const { return imageIndex == ~0 && !is_null(swapchain); }
};

} // namespace drv3d_vulkan
