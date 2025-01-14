// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/vector.h>
#include <EASTL/fixed_vector.h>
#include "vulkan_device.h"

namespace drv3d_vulkan
{
class Image;
class Device;
class ExecutionContext;
struct FrameInfo;

// describes swapchain mode
// used to represent frontend, backend & other intermediate modes
struct SwapchainMode
{
  VulkanSurfaceKHRHandle surface;
  VkPresentModeKHR presentMode;
  VkExtent2D extent;
  FormatStore colorFormat;
  uint32_t recreateRequest;
  uint8_t enableSrgb : 1;
  uint8_t fullscreen : 1;

  bool isVsyncOn() const { return VK_PRESENT_MODE_IMMEDIATE_KHR != presentMode; }

  // consexpr?
  FormatStore getDepthStencilFormat() const { return FormatStore::fromCreateFlags(TEXFMT_DEPTH24); }

  bool compare(const SwapchainMode &r) const
  {
    // ordered by change possibility
    if (extent.height != r.extent.height)
      return false;

    if (extent.width != r.extent.width)
      return false;

    if (presentMode != r.presentMode)
      return false;

    if (surface != r.surface)
      return false;

    if (enableSrgb != r.enableSrgb)
      return false;

    if (colorFormat != r.colorFormat)
      return false;

    if (recreateRequest != r.recreateRequest)
      return false;

    return true;
  }

  void setPresentModeFromConfig();
};

// NOTE: every mode switch is done lazily at the frame start
class Swapchain
{

private:
  VulkanSwapchainKHRHandle handle;

  // when we change swapchain, old one will be still used by display/gpu
  // instead of making deletion queue, for them
  // simply wait in headless mode for amount of images used in swapchain
  uint32_t changeDelay = 0;
  // when swapchain change fails consequtively, trigger fatal
  static constexpr int changeConsequtiveFailuresLimit = 16;
  uint32_t changeConsequtiveFailures = 0;
  enum ChangeResult
  {
    OK,
    RETRY,
    FAIL
  };

  SwapchainMode activeMode{};
  SwapchainMode frontMode{};
  uint32_t recreateRequest = 0;

  // defines internal swapchain state
  // that enforces & checks state validity & sequencing
  // so we can't do something bad
  enum SwapStateId
  {
    SWP_INITIAL,
    SWP_HEADLESS,
    SWP_DELAYED_ACQUIRE,
    SWP_EARLY_ACQUIRE,
    SWP_PRESENT,
    SWP_PRE_PRESENT,
    SWP_PRE_ROTATE,
  };

  SwapStateId currentState = SWP_INITIAL;
  void validateSwapState();
  void changeSwapState(SwapStateId new_state);
  void destroySwapchainHandle(VulkanSwapchainKHRHandle &sc_handle);

  eastl::vector<Image *> swapImages;
  uint32_t colorTargetIndex = 0;
  Image *colorTarget = nullptr;

  // pre rotation stuff
  bool preRotation = false;
  bool preRotationProcessed = false;
  bool keepRotatedScreen = false;
  uint32_t savedOffscreenBinding = 0;
  volatile int preRotationAngle = 0;
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
#if _TARGET_ANDROID
  static constexpr int SURFACE_POLLING_INTERVAL = 120;
  int surfacePolling = 0;
  bool usePredefinedPresentFormat;
#endif

  // saved global configuration
  bool reuseHandle = true;

  Image *offscreenBuffer = nullptr;
  Image *lastRenderedImage = nullptr;

  // around 5'ish formats are reported usually
  static constexpr uint32_t usualSwapchainFormatCount = 5;
  // there are currently 6 types of present modes, no drivers supports usually more than 4
  static constexpr uint32_t usualPresentModeCount = 4;
  typedef eastl::fixed_vector<VkPresentModeKHR, usualPresentModeCount, true> PresentModeStore;
  typedef eastl::fixed_vector<VkSurfaceFormatKHR, usualSwapchainFormatCount, true> SurfaceFormatStore;
  typedef eastl::fixed_vector<FormatStore, usualSwapchainFormatCount, true> SwapchainFormatStore;

  bool isFullscreenExclusiveAllowed();
  VkSurfaceCapabilitiesKHR querySurfaceCaps();
  PresentModeStore queryPresentModeList();
  SwapchainFormatStore queryPresentFormats();

  static VkPresentModeKHR selectPresentMode(const PresentModeStore &modes, VkPresentModeKHR requested);
  static FormatStore selectPresentFormat(const SwapchainFormatStore &formats);
  bool acquireSwapImage(FrameInfo &frame);

  ChangeResult changeSwapchain(FrameInfo &frame);
  bool updateSwapchainImageList(const VkSwapchainCreateInfoKHR &sci);

  // NOTE: if from and to have same extent, then blit is a copy per vulkan spec
  void doBlit(Image *from, Image *to);
  void makeReadyForBlit(Image *from, Image *to);
  void makeReadyForPresent(Image *img);

  void createOffscreenBuffer();
  void destroyOffscreenBuffer(FrameInfo &frame);
  void createLastRenderedImage(const VkSwapchainCreateInfoKHR &sci);
  void destroyLastRenderedImage(FrameInfo &frame);

  bool checkVkSwapchainError(VkResult rc, const char *source_call);

public:
  Swapchain() = default;
  ~Swapchain() = default;

  Swapchain(const Swapchain &) = delete;
  Swapchain &operator=(const Swapchain &) = delete;

  Swapchain(Swapchain &&) = delete;
  Swapchain &operator=(Swapchain &&) = delete;

  const SwapchainMode &getMode() const { return frontMode; }
  void setMode(const SwapchainMode &new_mode);
  void forceRecreate() { ++frontMode.recreateRequest; }

  bool init(const SwapchainMode &initial_mode);
  void shutdown(FrameInfo &frame);
  int getPreRotationAngle();

  // following methods should not be called
  // outside of execution context
  // frame should not be intermixed too
  Image *getColorImage() const { return colorTarget; }
  const SwapchainMode &getActiveMode() const { return activeMode; }

  void holdPreRotatedStateForOneFrame() { keepRotatedScreen = true; }
  void preRotateStart(uint32_t offscreen_binding_idx, FrameInfo &frame, ExecutionContext &ctx);
  bool inPreRotate() { return currentState == SWP_PRE_ROTATE; }
  bool needPreRotationFIFOWorkaround();

  // methods that can change internal swapchain state
  void changeSwapchainMode(ExecutionContext &ctx, const SwapchainMode &new_mode);
  void prePresent();
  void present(VulkanSemaphoreHandle present_signal);
  void onFrameBegin(ExecutionContext &ctx);
};

} // namespace drv3d_vulkan