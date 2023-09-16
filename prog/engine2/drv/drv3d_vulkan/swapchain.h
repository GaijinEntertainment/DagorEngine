#pragma once

#include <EASTL/vector.h>
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
  uint8_t enableSrgb : 1;
  // TODO: VK_EXT_full_screen_exclusive support
  // uint8_t fullscreen : 1;

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

    return true;
  }

  void setPresentModeFromConfig();
};

// NOTE: every mode switch is done lazily at the frame start
class Swapchain
{

private:
  Device &device;
  VulkanDevice &vkDev;

  VulkanSwapchainKHRHandle handle;

  // when we change swapchain, old one will be still used by display/gpu
  // instead of making deletion queue, for them
  // simply wait in headless mode for amount of images used in swapchain
  uint32_t changeDelay = 0;

  SwapchainMode activeMode{};
  SwapchainMode frontMode{};

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
  static constexpr int SURFACE_ROTATION_POLLING_INTERVAL = 120;
  int surfaceRotationPolling = 0;
  bool usePredefinedPresentFormat;
#endif

#if ENABLE_SWAPPY
  int swappyTarget = 0;
  bool swappyInitialized = false;
#endif

  Image *offscreenBuffer = nullptr;
  Image *depthStencilImage = nullptr;
  Image *lastRenderedImage = nullptr;

  // around 5'ish formats are reported usually
  static constexpr uint32_t usualSwapchainFormatCount = 5;
  // there are currently 6 types of present modes, no drivers supports usually more than 4
  static constexpr uint32_t usualPresentModeCount = 4;
  typedef eastl::fixed_vector<VkPresentModeKHR, usualPresentModeCount, true> PresentModeStore;
  typedef eastl::fixed_vector<VkSurfaceFormatKHR, usualSwapchainFormatCount, true> SurfaceFormatStore;
  typedef eastl::fixed_vector<FormatStore, usualSwapchainFormatCount, true> SwapchainFormatStore;

  VkSurfaceCapabilitiesKHR querySurfaceCaps();
  PresentModeStore queryPresentModeList();
  SwapchainFormatStore queryPresentFormats();

  static VkPresentModeKHR selectPresentMode(const PresentModeStore &modes, VkPresentModeKHR requested);
  static FormatStore selectPresentFormat(const SwapchainFormatStore &formats);
  bool acquireSwapImage(FrameInfo &frame);

  bool changeSwapchain(FrameInfo &frame);
  bool updateSwapchainImageList(const VkSwapchainCreateInfoKHR &sci);

  // NOTE: if from and to have same extent, then blit is a copy per vulkan spec
  void doBlit(VulkanCommandBufferHandle cmd, Image *from, Image *to);
  void makeReadyForBlit(VulkanCommandBufferHandle cmd, Image *from, Image *to);
  void makeReadyForPresent(VulkanCommandBufferHandle cmd, Image *img);

  void createOffscreenBuffer();
  void destroyOffscreenBuffer(FrameInfo &frame);
  void createLastRenderedImage(const VkSwapchainCreateInfoKHR &sci);
  void destroyLastRenderedImage(FrameInfo &frame);

  bool checkVkSwapchainError(VkResult rc, const char *source_call);

public:
  Swapchain() = delete;
  ~Swapchain() = default;

  Swapchain(const Swapchain &) = delete;
  Swapchain &operator=(const Swapchain &) = delete;

  Swapchain(Swapchain &&) = delete;
  Swapchain &operator=(Swapchain &&) = delete;

  Swapchain(Device &dvc);

  const SwapchainMode &getMode() const { return frontMode; }
  void setMode(const SwapchainMode &new_mode);

  bool init(const SwapchainMode &initial_mode);
  void shutdown(FrameInfo &frame);
  int getPreRotationAngle();

  // following methods should not be called
  // outside of execution context
  // frame should not be intermixed too
  Image *getDepthStencilImageForExtent(VkExtent2D ext, FrameInfo &frame);
  Image *getColorImage() const { return colorTarget; }
  const SwapchainMode &getActiveMode() const { return activeMode; }

  void holdPreRotatedStateForOneFrame() { keepRotatedScreen = true; }
  void preRotateStart(uint32_t offscreen_binding_idx, FrameInfo &frame, ExecutionContext &ctx);
  bool inPreRotate() { return currentState == SWP_PRE_ROTATE; }
  bool needPreRotationFIFOWorkaround();

  // methods that can change internal swapchain state
  void changeSwapchainMode(FrameInfo &frame, const SwapchainMode &new_mode);
  void prePresent(VulkanCommandBufferHandle cmd, FrameInfo &frame);
  void present(FrameInfo &frame);
  void onFrameBegin(FrameInfo &frame);

  void setSwappyTargetFrameRate(int rate);
  void getSwappyStatus(int *status);
};

} // namespace drv3d_vulkan