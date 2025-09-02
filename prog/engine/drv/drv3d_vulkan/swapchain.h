// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <drv/3d/dag_renderStates.h>
#include <EASTL/fixed_vector.h>
#include <osApiWrappers/dag_critSec.h>
#include "vulkan_device.h"


namespace drv3d_vulkan
{
class Image;
struct BaseTex;
struct CmdPresent;
struct CmdBlitImage;

// describes swapchain mode
// used to represent frontend, backend & other intermediate modes
struct SwapchainMode
{
  VulkanSurfaceKHRHandle surface;
  VkPresentModeKHR presentMode;
  VkExtent2D extent;
  FormatStore format;
  uint8_t extraBusyImages;
  uint8_t enableSrgb : 1;
  uint8_t fullscreen : 1;
  bool mutableFormat : 1;
  bool mismatchedExtents : 1;
  const char *modifySource;

  bool isFullscreenExclusiveAllowed();
  bool isVsyncOn() const { return VK_PRESENT_MODE_IMMEDIATE_KHR != presentMode; }
  void setHeadless() { surface = VulkanSurfaceKHRHandle{}; }

  void setPresentModeFromConfig();
};

struct SwapchainImage
{
  Image *img;
  // According to the latest version of the vulkan validator, the spec should
  // be understood as requiring us to have separate present semaphores for each
  // swapchain image. This makes sense, as acquiring an image that was previously
  // presented is an implicit "join" operation, and until it is performed, it is not
  // safe to reuse the semaphore that is being operated on concurrently by the
  // presentation engine.
  VulkanSemaphoreHandle frame;
  VulkanSemaphoreHandle acquire;
  bool acquired : 1;

  void shutdown();
};

struct SwapchainQueryCache
{
  // around 5'ish formats are reported usually
  static constexpr uint32_t usualSwapchainFormatCount = 5;
  // there are currently 6 types of present modes, no drivers supports usually more than 4
  static constexpr uint32_t usualPresentModeCount = 4;
  typedef eastl::fixed_vector<VkPresentModeKHR, usualPresentModeCount, true> PresentModeStore;
  typedef eastl::fixed_vector<VkSurfaceFormatKHR, usualSwapchainFormatCount, true> SurfaceFormatStore;
  typedef eastl::fixed_vector<FormatStore, usualSwapchainFormatCount, true> SwapchainFormatStore;

  VkSurfaceCapabilitiesKHR caps;
  PresentModeStore modes;
  FormatStore format;

  void init();
  bool update(VulkanSurfaceKHRHandle surf);
  VkPresentModeKHR mode(VkPresentModeKHR requested) const;

private:
  static bool usePredefinedPresentFormat;
  bool fillFormats(VulkanSurfaceKHRHandle surf);
};

// NOTE: every mode switch is done lazily at the frame start
class Swapchain
{

private:
  static constexpr int MAX_ACQUIRED_IMAGES = GPU_TIMELINE_HISTORY_SIZE + MAX_PENDING_REPLAY_ITEMS;

  WinCritSec acquireMutex;
  VulkanSwapchainKHRHandle handle = {};

  // when swapchain change fails consequtively, trigger fatal
  static constexpr int updateConsequtiveFailuresLimit = 16;
  uint32_t updateConsequtiveFailures = 0;
  enum ObjectUpdateResult
  {
    OK,
    RETRY,
    FAIL
  };
  ObjectUpdateResult lastUpdateResult = ObjectUpdateResult::OK;

  SwapchainMode currentMode{};

  dag::Vector<VulkanSemaphoreHandle> semaphoresRing;
  uint32_t semaphoresRingIdx = 0;
  dag::Vector<SwapchainImage> images;
  VkResult backendAcquireResult = VK_SUCCESS;
  uint32_t backendAcquireImageIndex = ~0;
  uint32_t acquiredImageIdx = ~0;
  uint32_t acquireId = 0;
  Image *activeImage = nullptr;
  Image *offscreenBuffer = nullptr;
  BaseTex *wrappedTex = nullptr;
  bool usePredefinedPresentFormat = false;
  bool queuedRecreate = false;
  bool inAcquireRetry = false;
  // used for implementations that bug out on multiple acquires --
  // giving same indexes without present submits instead of properly reporting timeout/not ready status
  bool onlyBackendAcquire = false;
  bool acquireExclusiveTestRunned = false;
  VkResult checkAcquireExclusive(VkSwapchainCreateInfoKHR &sci);

  // prerotation stuff
  BaseTex *wrappedRotatedTex = nullptr;
  bool rotateNeeded = false;
  shaders::DriverRenderStateId rotateRenderState;

  // still image for system preview of non focused application
  Image *stillImage = nullptr;
  bool needStillImage = false;

  int surfaceTransformToAngle(VkSurfaceTransformFlagBitsKHR bit)
  {
    if (bit == VK_SURFACE_TRANSFORM_ROTATE_90_BIT_KHR)
      return 90;
    else if (bit == VK_SURFACE_TRANSFORM_ROTATE_180_BIT_KHR)
      return 180;
    else if (bit == VK_SURFACE_TRANSFORM_ROTATE_270_BIT_KHR)
      return 270;
    return 0;
  }
  SwapchainQueryCache query;

  ObjectUpdateResult updateObject();
  bool acquireSwapImage();
  bool updateImageList(const VkSwapchainCreateInfoKHR &sci);
  void fillSemaphoresRing();

  void ensureOffscreenBuffer();
  void destroyOffscreenBuffer();
  void replaceReferences();
  void destroySwapchainHandle(VulkanSwapchainKHRHandle &sc_handle);
  void clearState();

  void rotateFromOffscreen();
  CmdBlitImage blitImage(Image *src, Image *dst);

  void processAcquireStatus(VkResult rc, bool acquired);

public:
  Swapchain() = default;
  ~Swapchain() = default;

  Swapchain(const Swapchain &) = delete;
  Swapchain &operator=(const Swapchain &) = delete;

  Swapchain(Swapchain &&) = delete;
  Swapchain &operator=(Swapchain &&) = delete;

  const SwapchainMode &getMode() const { return currentMode; }
  void setMode(const SwapchainMode &new_mode);

  bool init();
  void shutdown();

  void nextFrame();
  void prePresent();
  CmdPresent present();

  Image *getCurrentTarget()
  {
    G_ASSERT(activeImage); // nextFrame must be called before getting target
    return activeImage;
  }

  BaseTex *getCurrentTargetTex()
  {
    G_ASSERT(wrappedTex); // avoid calling on dead swapchain
    return wrappedTex;
  }

  bool blitStillImage();
};

} // namespace drv3d_vulkan