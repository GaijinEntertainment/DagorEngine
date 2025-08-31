// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <drv/3d/dag_shader.h>
#include <drv/3d/dag_texture.h>
#include <drv/3d/dag_vertexIndexBuffer.h>
#include <drv/3d/dag_draw.h>
#include <drv/3d/dag_shaderConstants.h>

#include "swapchain.h"
#include "vulkan_allocation_callbacks.h"
#include "globals.h"
#include "physical_device_set.h"
#include "texture.h"
#include "device_context.h"
#include "timeline_latency.h"

#if _TARGET_ANDROID
#include <osApiWrappers/dag_progGlobals.h>
#include <supp/dag_android_native_app_glue.h>
#include <sys/system_properties.h>
#endif

using namespace drv3d_vulkan;

namespace
{

// This lists the alternative modes for a requested mode, the earlier entries are
// closer match to the requested mode than the later ones.
const VkPresentModeKHR bestPresentModeMatch[4][3] = //
  {
    // VK_PRESENT_MODE_IMMEDIATE_KHR
    {VK_PRESENT_MODE_MAILBOX_KHR, VK_PRESENT_MODE_FIFO_RELAXED_KHR, VK_PRESENT_MODE_FIFO_KHR},
    // row for VK_PRESENT_MODE_MAILBOX_KHR
    {VK_PRESENT_MODE_IMMEDIATE_KHR, VK_PRESENT_MODE_FIFO_RELAXED_KHR, VK_PRESENT_MODE_FIFO_KHR},
    // row for VK_PRESENT_MODE_FIFO_KHR
    {VK_PRESENT_MODE_FIFO_RELAXED_KHR, VK_PRESENT_MODE_MAILBOX_KHR, VK_PRESENT_MODE_IMMEDIATE_KHR},
    // row for VK_PRESENT_MODE_FIFO_RELAXED_KHR
    {VK_PRESENT_MODE_MAILBOX_KHR, VK_PRESENT_MODE_IMMEDIATE_KHR, VK_PRESENT_MODE_FIFO_KHR}
    //
};

} // namespace

bool SwapchainQueryCache::usePredefinedPresentFormat = false;

void SwapchainMode::setPresentModeFromConfig()
{
  const DataBlock *videoCfg = ::dgs_get_settings()->getBlockByNameEx("video");
  bool useVSync = videoCfg->getBool("vsync", false);
  bool useAdaptiveVSync = videoCfg->getBool("adaptive_vsync", false);
  presentMode = get_requested_present_mode(useVSync, useAdaptiveVSync);
}

#if VK_EXT_full_screen_exclusive
bool SwapchainMode::isFullscreenExclusiveAllowed()
{
  // probably need to use vkGetPhysicalDeviceSurfaceCapabilities2KHR for checking support of exclusive modes, but that's not documented
  // well and EXT sample not using it, so just ask it as is without bothering with capability query
  bool fineToUse = Globals::VK::phy.hasExtension<FullScreenExclusiveEXT>();
  fineToUse &= fullscreen > 0;
  return fineToUse;
}
#endif

void SwapchainQueryCache::init()
{
#if _TARGET_ANDROID
  {
    char sdkVerS[PROP_VALUE_MAX + 1];
    int sdkVerI = 33;
    int sdkVerSLen = __system_property_get("ro.build.version.sdk", sdkVerS);
    if (sdkVerSLen >= 0)
      sdkVerI = atoi(sdkVerS);

    // on some exynos samsungs and Snapdragon 8 gen 1 chips with android 13
    // call to vkGetPhysicalDeviceSurfaceFormatsKHR with counts != 0
    // leaves some mutex acquired in gralloc stack, causing application to freeze
    // use format with 100% coverage instead of making a query for them

    // enable by default on all android 13 if not disabled by config
    // just in case if there is more such devices on market
    int minSdk = Globals::cfg.getPerDriverPropertyBlock("androidDeadlockAtQueryPresentFormats")->getInt("androidMinSdkVer", 33);
    usePredefinedPresentFormat = minSdk <= sdkVerI;
    if (usePredefinedPresentFormat)
      debug("vulkan: android: using predefined swapchain format for QueryPresentFormats deadlock workaround");
  }
#endif
}

bool SwapchainQueryCache::fillFormats(VulkanSurfaceKHRHandle surf)
{
  uint32_t count = 0;
  SwapchainFormatStore formats;
  if (usePredefinedPresentFormat)
  {
    // RGBA8 UNORM have 100% coverage, use it instead on asking available ones
    formats.push_back(FormatStore::fromVkFormat(VK_FORMAT_R8G8B8A8_UNORM));
  }
  else if (VULKAN_CHECK_OK(Globals::VK::inst.vkGetPhysicalDeviceSurfaceFormatsKHR(Globals::VK::phy.device, surf, &count, nullptr)))
  {
    SurfaceFormatStore rawFormats;
    rawFormats.resize(count);
    if (VULKAN_CHECK_OK(
          Globals::VK::inst.vkGetPhysicalDeviceSurfaceFormatsKHR(Globals::VK::phy.device, surf, &count, rawFormats.data())))
    {
      for (auto &&i : rawFormats)
        if (FormatStore::canBeStored(i.format))
          formats.push_back(FormatStore::fromVkFormat(i.format));

      if (formats.empty())
      {
        // check for undefined format, it indicates that you can use the format you like
        // most
        auto iter = eastl::find_if(rawFormats.begin(), rawFormats.end(),
          [](const VkSurfaceFormatKHR &fmt) //
          { return fmt.format == VK_FORMAT_UNDEFINED; });
        if (iter != rawFormats.end())
          formats.push_back(FormatStore::fromCreateFlags(TEXFMT_DEFAULT));
      }
    }
  }

  if (formats.empty())
  {
    D3D_ERROR("vulkan: swapchain: no supported formats in query, using default engine format");
    return false;
  }

  if (end(formats) != eastl::find(begin(formats), end(formats), FormatStore{})) // most common format (0 -> argb), do a fast check
    format = FormatStore{};
  else
  {
    // select the format with the most bytes but that is not srgb nor float
    auto rateFormat = [](FormatStore f) -> int //
    { return f.getBytesPerPixelBlock() - (f.isSrgb << 5) - (f.isFloat() << 4); };

    FormatStore selected = formats.front();
    for (auto &&i : formats)
      if (rateFormat(i) > rateFormat(selected))
        selected = i;

    format = selected;
  }
  return true;
}

bool SwapchainQueryCache::update(VulkanSurfaceKHRHandle surf)
{
  caps = {};
  VULKAN_LOG_CALL(Globals::VK::inst.vkGetPhysicalDeviceSurfaceCapabilitiesKHR(Globals::VK::phy.device, surf, &caps));

  uint32_t count = 0;
  modes.clear();
  if (VULKAN_CHECK_OK(Globals::VK::inst.vkGetPhysicalDeviceSurfacePresentModesKHR(Globals::VK::phy.device, surf, &count, nullptr)))
  {
    modes.resize(count);
    if (VULKAN_CHECK_OK(
          Globals::VK::inst.vkGetPhysicalDeviceSurfacePresentModesKHR(Globals::VK::phy.device, surf, &count, modes.data())))
      modes.resize(count);
    else
      modes.clear();
  }
  if (modes.empty())
  {
    D3D_ERROR("vulkan: swapchain: no available present modes");
    return false;
  }

  return fillFormats(surf);
}

VkPresentModeKHR SwapchainQueryCache::mode(VkPresentModeKHR requested) const
{
  // shortcut for VK_PRESENT_MODE_FIFO_KHR as it is required to be supported
  if (VK_PRESENT_MODE_FIFO_KHR == requested || modes.empty())
    return VK_PRESENT_MODE_FIFO_KHR;

  if (end(modes) != eastl::find(begin(modes), end(modes), requested))
    return requested;

  G_ASSERTF(requested < array_size(bestPresentModeMatch),
    "vulkan: selectPresentMode(..., 'requested' = %u) is a mode that is not fully "
    "supported!");
  if (requested >= array_size(bestPresentModeMatch))
    return VK_PRESENT_MODE_FIFO_KHR;

  for (auto &&mode : bestPresentModeMatch[requested])
    if (end(modes) != eastl::find(begin(modes), end(modes), mode))
      return mode;

  return VK_PRESENT_MODE_FIFO_KHR;
}

void SwapchainImage::shutdown()
{
  Globals::ctx.destroyImage(img);
  if (!is_null(frame))
    VULKAN_LOG_CALL(Globals::VK::dev.vkDestroySemaphore(Globals::VK::dev.get(), frame, VKALLOC(semaphore)));
}

void Swapchain::processAcquireStatus(VkResult rc, bool acquired)
{
  if (acquired)
  {
    if (rc != VK_SUCCESS)
    {
      debug("vulkan: swapchain: queued recreation due to %s", vulkan_error_string(rc));
      queuedRecreate = true;
    }
    return;
  }

  SwapchainMode newMode = currentMode;
  if (rc == VK_ERROR_SURFACE_LOST_KHR)
  {
    newMode.setHeadless();
    newMode.modifySource = "surface lost";
  }
  else if (rc == VK_ERROR_OUT_OF_DATE_KHR)
    newMode.modifySource = "out of date";
  else if (rc == VK_SUBOPTIMAL_KHR)
    newMode.modifySource = "suboptimal";
  else if (rc == VK_ERROR_DEVICE_LOST)
    newMode.modifySource = "device lost";
  else if (rc == VK_ERROR_UNKNOWN)
    newMode.modifySource = "unknown error";
  else if (rc == VK_TIMEOUT || rc == VK_NOT_READY)
  {
    // usually logic should not fall into timeout/not ready
    // yet amount of images needed to work without blocking varies by system
    // in order to conserve memory and at same time not ruin performance
    // dynamically increase amount of images on demand
    ++newMode.extraBusyImages;
    newMode.modifySource = "all images busy";
  }
  else
  {
    logwarn("vulkan: unknown swapchain acquire error code %u", rc);
    newMode.modifySource = "unknown error at acquire";
  }
  setMode(newMode);

  // try to restore surface if it was lost
  if (rc == VK_ERROR_SURFACE_LOST_KHR)
  {
    newMode.surface = init_window_surface(Globals::VK::inst);
    newMode.modifySource = "surface lost restoration";
    setMode(newMode);
    if (is_null(newMode.surface) || is_null(handle))
      DAG_FATAL("vulkan: swapchain: can't recover on lost surface");
  }
}

bool Swapchain::acquireSwapImage()
{
  if (Globals::cfg.bits.headless)
    return false;

  if (is_null(handle) || images.empty() || queuedRecreate)
  {
    SwapchainMode newMode = currentMode;
    newMode.modifySource = "recreate/retry";
    setMode(newMode);
    if (is_null(handle) || images.empty())
      return false;
  }

  VulkanSemaphoreHandle syncSemaphore = semaphoresRing[semaphoresRingIdx];
  VkResult rc = VK_NOT_READY;
  acquiredImageIdx = ~0;
  bool acquired = false;
  bool acquiredByBackend = false;
  uint64_t timeout = 0;

  if (onlyBackendAcquire)
  {
    // allow ignoring some frames if we blocking up
    if (acquireMutex.tryLock())
    {
      rc = backendAcquireResult;
      acquired = backendAcquireImageIndex != 0;
      acquireMutex.unlock();
      processAcquireStatus(rc, acquired);
    }
    return false;
  }

  // backend index biased by 1 to allow passing tri state result via single atomic
  // 0 is fail
  // ~0 is pending or not acquired
  // >0 is (image index + 1)
  auto handleBackendAcquired = [&] {
    acquiredByBackend = true;
    rc = backendAcquireResult;
    if (acquiredImageIdx == 0)
    {
      acquired = false;
    }
    else
    {
      --acquiredImageIdx;
      acquired = true;
    }
  };

  auto doAcquire = [&] {
    acquiredByBackend = false;
    acquiredImageIdx = interlocked_exchange(backendAcquireImageIndex, ~0);
    if (acquiredImageIdx == ~0)
    {
      rc = Globals::VK::dev.vkAcquireNextImageKHR(Globals::VK::dev.get(), handle, timeout, syncSemaphore, VulkanFenceHandle{},
        &acquiredImageIdx);
      acquired = acquiredImageIdx < images.size();
      ++acquireId;
    }
    else
      handleBackendAcquired();
  };

  {
    TIME_PROFILE(vulkan_swapchain_image_acquire);
    if (!acquireMutex.tryLock())
    {
      TIME_PROFILE(vulkan_swapchain_acquire_contention);
      spin_wait([&] { return interlocked_acquire_load(backendAcquireImageIndex) == ~0; });
      acquiredImageIdx = interlocked_exchange(backendAcquireImageIndex, ~0);
      handleBackendAcquired();
    }
    else
    {
      doAcquire();
      acquireMutex.unlock();
    }
  }

  // we try to block here if latency is too big (we don't want too much images)
  // or if windowing system somehow blocks on GPU regardless amount of images in swapchain
  // or if we can't allocate enough images to properly fit into async execution mode constraints
  if (!acquired && (rc == VK_TIMEOUT || rc == VK_NOT_READY) && currentMode.extraBusyImages >= Globals::cfg.swapchainMaxExtraImages)
  {
    TIME_PROFILE(vulkan_swapchain_image_blocked_acquire);
    WinAutoLock lock(acquireMutex);
    timeout = UINT64_MAX;
    doAcquire();
    if ((rc == VK_TIMEOUT || rc == VK_NOT_READY) && acquiredByBackend)
      doAcquire();
  }

  if (acquired)
  {
    SwapchainImage &acquiredImage = images[acquiredImageIdx];
    G_ASSERT(!acquiredImage.acquired);
    acquiredImage.acquired = true;
    if (!acquiredByBackend)
    {
      acquiredImage.acquire = syncSemaphore;
      semaphoresRingIdx = (semaphoresRingIdx + 1) % semaphoresRing.size();
    }
    else
    {
      acquiredImage.acquire = VulkanSemaphoreHandle{};
    }
  }

#if _TARGET_ANDROID
  // ignore suboptimals when prerotation disabled, because it will gone only if rotation enabled
  if (rc == VK_SUBOPTIMAL_KHR && !Globals::cfg.bits.preRotation)
    rc = VK_SUCCESS;
#endif

  if (!acquired && inAcquireRetry)
    return false;

  processAcquireStatus(rc, acquired);

  if (acquired)
    return true;

  // try to acquire image right after mode change to avoid leaving one frame blank
  inAcquireRetry = true;
  bool retryResult = acquireSwapImage();
  inAcquireRetry = false;
  return retryResult;
}

void Swapchain::clearState()
{
  replaceReferences();
  for (SwapchainImage &i : images)
    i.shutdown();
  images.clear();
  semaphoresRingIdx = 0;
  acquiredImageIdx = ~0;
  backendAcquireImageIndex = ~0;
  activeImage = nullptr;
  destroyOffscreenBuffer();
  wrappedTex->image = nullptr;
  rotateNeeded = false;
  queuedRecreate = false;
}

VkResult Swapchain::checkAcquireExclusive(VkSwapchainCreateInfoKHR &sci)
{
  VkResult ret = VK_SUCCESS;

  if (acquireExclusiveTestRunned)
    return ret;

  if (!Globals::cfg.bits.useSwapchainAcquireExclusiveTest)
  {
    acquireExclusiveTestRunned = true;
    return ret;
  }

  if (Globals::cfg.bits.forceSwapchainOnlyBackendAcquire)
  {
    onlyBackendAcquire = true;
    debug("vulkan: swapchain: forced only backend acquire mode by config");
  }
  else
  {
    bool allPassed = true;
    bool anyPassed = false;
    bool anyTimeoutOrNotReady = false;

    VulkanFenceHandle fence;
    VkFenceCreateInfo fci;
    fci.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fci.pNext = NULL;
    fci.flags = 0;
    VULKAN_EXIT_ON_FAIL(Globals::VK::dev.vkCreateFence(Globals::VK::dev.get(), &fci, VKALLOC(fence), ptr(fence)));
    for (uint32_t i = 0; i < sci.minImageCount * 2; ++i)
    {
      uint32_t idx = ~0;
      VkResult rc = Globals::VK::dev.vkAcquireNextImageKHR(Globals::VK::dev.get(), handle, 0, VulkanSemaphoreHandle{}, fence, &idx);
      debug("vulkan: swapchain: check acquire %u %s %u", i, vulkan_error_string(rc), idx);
      allPassed &= rc >= 0;
      anyPassed |= rc >= 0;
      anyTimeoutOrNotReady |= (rc == VK_TIMEOUT || rc == VK_NOT_READY);
      if (idx != ~0)
      {
        VULKAN_LOG_CALL(Globals::VK::dev.vkWaitForFences(Globals::VK::dev.get(), 1, ptr(fence), VK_TRUE, GENERAL_GPU_TIMEOUT_NS));
        VULKAN_LOG_CALL(Globals::VK::dev.vkResetFences(Globals::VK::dev.get(), 1, ptr(fence)));
      }
    }
    VULKAN_LOG_CALL(Globals::VK::dev.vkDestroyFence(Globals::VK::dev.get(), fence, VKALLOC(fence)));
    if (!anyTimeoutOrNotReady && allPassed)
    {
      debug("vulkan: swapchain: checkAcquireExclusive shows no timeout/not ready, using backend only acquire");
      onlyBackendAcquire = true;
    }
    else if (!allPassed)
    {
      // swapchain died somewhere inside test, will be recreated
      debug("vulkan: swapchain: checkAcquireExclusive will retry");
      return VK_SUCCESS;
    }
    if (anyPassed)
    {
      // recreate swapchain because we acquired some images that will never be presented
      destroySwapchainHandle(handle);
      ret = Globals::VK::dev.vkCreateSwapchainKHR(Globals::VK::dev.get(), &sci, VKALLOC(swapchain), ptr(handle));
      debug("vulkan: swapchain: checkAcquireExclusive recreation result %s", vulkan_error_string(ret));
    }

    if (anyTimeoutOrNotReady && allPassed)
    {
      debug("vulkan: swapchain: checkAcquireExclusive shows timeout/not ready, using default acquire logic");
      onlyBackendAcquire = false;
    }
  }
  acquireExclusiveTestRunned = true;
  return ret;
}

Swapchain::ObjectUpdateResult Swapchain::updateObject()
{
  TIME_PROFILE(vulkan_swapchain_update);
  if (!is_null(handle))
    Globals::ctx.wait();

  uint32_t fittedWidth = currentMode.extent.width;
  uint32_t fittedHeight = currentMode.extent.height;

  // handle minimized state early to avoid resource recreations
  if (!is_null(currentMode.surface) && !Globals::cfg.bits.headless)
  {
    if (!query.update(currentMode.surface))
      return ObjectUpdateResult::FAIL;

    auto fitExtents = [](uint32_t mode_value, uint32_t cap_value, uint32_t cap_min, uint32_t cap_max) {
      if (cap_value == -1)
        return clamp<uint32_t>(mode_value, cap_min, cap_max);
      return cap_value;
    };
    fittedWidth = fitExtents(currentMode.extent.width, query.caps.currentExtent.width, query.caps.minImageExtent.width,
      query.caps.maxImageExtent.width);
    fittedHeight = fitExtents(currentMode.extent.height, query.caps.currentExtent.height, query.caps.minImageExtent.height,
      query.caps.maxImageExtent.height);

    // this can happen if the window was minimized
    if (!fittedWidth || !fittedHeight)
      return ObjectUpdateResult::RETRY;
  }

  bool imageWasAcquired = (acquiredImageIdx != ~0) || (onlyBackendAcquire && (backendAcquireImageIndex - 1 < images.size()));
  imageWasAcquired &= Globals::cfg.bits.recreateSwapchainWhenImageAcquired > 0;
  clearState();

  // no surface - no swapchain, same goes for headless
  if (is_null(currentMode.surface) || Globals::cfg.bits.headless)
  {
    destroySwapchainHandle(handle);
    return ObjectUpdateResult::RETRY;
  }

  VkSwapchainCreateInfoKHR sci;
  sci.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  sci.pNext = nullptr;
  sci.imageExtent.width = fittedWidth;
  sci.imageExtent.height = fittedHeight;

  VkImageFormatListCreateInfoKHR iflci;
  VkFormat formatList[2] = {query.format.asVkFormat(), query.format.getSRGBFormat()};
  currentMode.mutableFormat = currentMode.enableSrgb && Globals::VK::dev.hasExtension<SwapchainMutableFormatKHR>() &&
                              Globals::VK::dev.hasExtension<ImageFormatListKHR>() && Globals::VK::dev.hasExtension<Maintenance2KHR>();
  if (currentMode.mutableFormat)
  {
    sci.flags = VK_SWAPCHAIN_CREATE_MUTABLE_FORMAT_BIT_KHR;
    iflci.sType = VK_STRUCTURE_TYPE_IMAGE_FORMAT_LIST_CREATE_INFO_KHR;
    iflci.pNext = nullptr;
    iflci.viewFormatCount = 2;
    iflci.pViewFormats = formatList;
    chain_structs(sci, iflci);
  }
  else
    sci.flags = 0;

  sci.surface = currentMode.surface;
  sci.imageArrayLayers = 1;
  sci.imageUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                   VK_IMAGE_USAGE_SAMPLED_BIT;
  sci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
  sci.queueFamilyIndexCount = 0;
  sci.pQueueFamilyIndices = nullptr;
  sci.clipped = VK_FALSE;
  sci.oldSwapchain = handle;

  if (query.caps.supportedTransforms & VK_SURFACE_TRANSFORM_INHERIT_BIT_KHR)
    sci.preTransform = VK_SURFACE_TRANSFORM_INHERIT_BIT_KHR;
  else
    sci.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;

  if (Globals::cfg.bits.preRotation)
  {
    rotateNeeded =
      query.caps.currentTransform != sci.preTransform && query.caps.currentTransform != VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    sci.preTransform = query.caps.currentTransform;
  }
  else
    rotateNeeded = false;

  if (query.caps.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR)
    sci.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  else
    sci.compositeAlpha = VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;

  sci.imageUsage &= query.caps.supportedUsageFlags;

  sci.minImageCount =
    max<uint32_t>(GPU_TIMELINE_HISTORY_SIZE, query.caps.minImageCount) + (MAX_ACQUIRED_IMAGES - 1) + currentMode.extraBusyImages;
  if (query.caps.maxImageCount)
    sci.minImageCount = min<uint32_t>(sci.minImageCount, query.caps.maxImageCount);

  currentMode.mismatchedExtents =
    currentMode.extent.width != sci.imageExtent.width || currentMode.extent.height != sci.imageExtent.height;
  if (currentMode.mismatchedExtents)
    debug("vulkan: swapchain: mode (%u,%u) and surface (%u, %u) extends mismatch, using scaling blit", currentMode.extent.width,
      currentMode.extent.height, sci.imageExtent.width, sci.imageExtent.height);

  // swap extents to match device transform
  if ((sci.preTransform & VK_SURFACE_TRANSFORM_ROTATE_90_BIT_KHR) || (sci.preTransform & VK_SURFACE_TRANSFORM_ROTATE_270_BIT_KHR))
    eastl::swap(sci.imageExtent.width, sci.imageExtent.height);
  currentMode.format = query.format;
  sci.imageFormat = currentMode.format.asVkFormat();
  sci.imageColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
  sci.presentMode = query.mode(currentMode.presentMode);

  debug("vulkan: swapchain: using min %u:%s images, mode %s (asked %s) rotation: %u srgb: %s", sci.minImageCount,
    currentMode.format.getNameString(), formatPresentMode(sci.presentMode), formatPresentMode(currentMode.presentMode),
    rotateNeeded ? surfaceTransformToAngle(sci.preTransform) : 0,
    currentMode.enableSrgb ? (currentMode.mutableFormat ? "mutable" : "blit") : "off");

  if (Globals::cfg.bits.preRotation)
  {
    wrappedRotatedTex->pars.w = sci.imageExtent.width;
    wrappedRotatedTex->pars.h = sci.imageExtent.height;
    wrappedRotatedTex->pars.flg = TEXCF_RTARGET | currentMode.format.asTexFlags();
    wrappedRotatedTex->fmt = FormatStore::fromCreateFlags(wrappedRotatedTex->pars.flg);
    wrappedRotatedTex->setInitialImageViewState();
  }

#if VK_EXT_full_screen_exclusive
  VkSurfaceFullScreenExclusiveInfoEXT fsei = {
    VK_STRUCTURE_TYPE_SURFACE_FULL_SCREEN_EXCLUSIVE_INFO_EXT, nullptr, VK_FULL_SCREEN_EXCLUSIVE_ALLOWED_EXT};
  if (currentMode.isFullscreenExclusiveAllowed())
  {
    chain_structs(sci, fsei);
    debug("vulkan: swapchain: full screen exclusive allowed");
  }
#endif
  VulkanSwapchainKHRHandle oldHandle = handle;
  if (imageWasAcquired)
  {
    debug("vulkan: swapchain: destroy old swapchain because image was acquired");
    sci.oldSwapchain = VulkanSwapchainKHRHandle{};
    destroySwapchainHandle(oldHandle);
  }
  // clear handle for sanity
  handle = VulkanSwapchainKHRHandle();

  VkResult createResult = Globals::VK::dev.vkCreateSwapchainKHR(Globals::VK::dev.get(), &sci, VKALLOC(swapchain), ptr(handle));
  destroySwapchainHandle(oldHandle);

  if (createResult == VK_SUCCESS)
    createResult = checkAcquireExclusive(sci);

  if (createResult != VK_SUCCESS)
  {
    logwarn("vulkan: swapchain: failed to create due to error %u[%s]", createResult, vulkan_error_string(createResult));
    return ObjectUpdateResult::FAIL;
  }

  if (!updateImageList(sci))
  {
    destroySwapchainHandle(handle);
    return ObjectUpdateResult::FAIL;
  }
  fillSemaphoresRing();

  return ObjectUpdateResult::OK;
}

void Swapchain::fillSemaphoresRing()
{
  VkSemaphoreCreateInfo sci;
  sci.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
  sci.pNext = NULL;
  sci.flags = 0;
  while (semaphoresRing.size() < (max<uint32_t>(TimelineLatency::replayToGPUCompletionRingBufferSize, images.size())))
  {
    VulkanSemaphoreHandle sem;
    VULKAN_EXIT_ON_FAIL(Globals::VK::dev.vkCreateSemaphore(Globals::VK::dev.get(), &sci, VKALLOC(semaphore), ptr(sem)));
    Globals::Dbg::naming.setSemaphoreName(sem, "swapchain_acquire_semaphore");
    semaphoresRing.push_back(sem);
  }
}

bool Swapchain::updateImageList(const VkSwapchainCreateInfoKHR &sci)
{
  ImageCreateInfo ii;
  ii.type = VK_IMAGE_TYPE_2D;
  ii.mips = 1;
  ii.usage = sci.imageUsage;
  ii.size.width = sci.imageExtent.width;
  ii.size.height = sci.imageExtent.height;
  ii.size.depth = 1;
  ii.arrays = 1;
  ii.samples = VK_SAMPLE_COUNT_1_BIT;
  ii.flags = currentMode.mutableFormat ? (VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT | VK_IMAGE_CREATE_EXTENDED_USAGE_BIT) : 0;

  uint32_t count = 0;
  if (VULKAN_CHECK_FAIL(Globals::VK::dev.vkGetSwapchainImagesKHR(Globals::VK::dev.get(), handle, &count, nullptr)))
    return false;

  debug("vulkan: swapchain: %u images", count);

  dag::Vector<VulkanImageHandle> imageList;
  imageList.resize(count);
  if (VULKAN_CHECK_FAIL(Globals::VK::dev.vkGetSwapchainImagesKHR(Globals::VK::dev.get(), handle, &count, ary(imageList.data()))))
    return false;

  images.reserve(imageList.size());
  {
    VkSemaphoreCreateInfo seci;
    seci.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    seci.pNext = NULL;
    seci.flags = 0;

    WinAutoLock lk(Globals::Mem::mutex);
    for (auto &&image : imageList)
    {
      Image::Description desc = {{ii.flags, VK_IMAGE_TYPE_2D, ii.size, 1, 1, ii.usage, VkSampleCountFlagBits::VK_SAMPLE_COUNT_1_BIT},
        Image::MEM_NOT_EVICTABLE, currentMode.format, VK_IMAGE_LAYOUT_UNDEFINED};
      Image *newImage = Globals::Mem::res.alloc<Image>(desc, false);
      newImage->setDerivedHandle(image);
      Globals::Dbg::naming.setTexName(newImage, String(64, "swapchainImage%u", &image - &imageList[0]));
      images.push_back();
      images.back().img = newImage;
      VULKAN_EXIT_ON_FAIL(
        Globals::VK::dev.vkCreateSemaphore(Globals::VK::dev.get(), &seci, VKALLOC(semaphore), ptr(images.back().frame)));
      Globals::Dbg::naming.setSemaphoreName(images.back().frame, "swapchain_frame_semaphore");
    }
  }

  return true;
}

void Swapchain::ensureOffscreenBuffer()
{
  if (offscreenBuffer)
    return;

  ImageCreateInfo ii;
  ii.type = VK_IMAGE_TYPE_2D;
  ii.mips = 1;
  ii.size.width = currentMode.extent.width;
  ii.size.height = currentMode.extent.height;
  ii.size.depth = 1;
  ii.arrays = 1;
  ii.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
             VK_IMAGE_USAGE_SAMPLED_BIT;
  ii.format = currentMode.format;
  ii.flags = currentMode.enableSrgb ? VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT : 0;
  ii.memFlags = Image::MEM_NOT_EVICTABLE | Image::MEM_SWAPCHAIN;
  ii.samples = VK_SAMPLE_COUNT_1_BIT;
  offscreenBuffer = Image::create(ii);
  Globals::Dbg::naming.setTexName(offscreenBuffer, "swapchain-offscreen-buffer");
}

void Swapchain::destroyOffscreenBuffer()
{
  if (offscreenBuffer)
  {
    Globals::ctx.destroyImage(offscreenBuffer);
    offscreenBuffer = nullptr;
  }
}

void Swapchain::replaceReferences()
{
  if (images.empty())
    return;
  ensureOffscreenBuffer();
  for (SwapchainImage &i : images)
    Frontend::State::pipe.replaceImage(i.img, offscreenBuffer);
}

bool Swapchain::init()
{
  query.init();
  wrappedTex = allocate_texture(1, 1, 1, 1, D3DResourceType::TEX, TEXCF_RTARGET, "swapchain_wrapped_image");
  if (Globals::cfg.bits.preRotation)
    wrappedRotatedTex = allocate_texture(1, 1, 1, 1, D3DResourceType::TEX, TEXCF_RTARGET, "swapchain_wrapped_rotated_image");
  clearState();
  rotateRenderState = shaders::DriverRenderStateId(~0);
  return true;
}

void Swapchain::setMode(const SwapchainMode &new_mode)
{
  bool hasChanges = false;
  bool extentFittingChange = false;
  VulkanSurfaceKHRHandle oldSurface;
  if (currentMode.surface != new_mode.surface)
  {
    debug("vulkan: swapchain mode change request: surface %016llX -> %016llX", generalize(currentMode.surface),
      generalize(new_mode.surface));
    hasChanges = true;
    oldSurface = currentMode.surface;
  }

  if (currentMode.extent != new_mode.extent)
  {
    debug("vulkan: swapchain mode change request: extent [%u, %u] -> [%u, %u]", currentMode.extent.width, currentMode.extent.height,
      new_mode.extent.width, new_mode.extent.height);
    hasChanges = true;
    // we are not propragating surface extent change back to engine right away, so vulkan can see extend change early than engine
    // causing two setMode calls, one from vulkan acquire, one from engine
    // one from engine should processed without swapchain recreation if it is fitting properly
    if (images.size() && new_mode.extent.width == images[0].img->getBaseExtent2D().width &&
        new_mode.extent.height == images[0].img->getBaseExtent2D().height)
    {
      extentFittingChange = true;
      hasChanges = false;
      debug("vulkan: swapchain mode change request: extent fitting to surface extent");
    }
  }

  if (currentMode.presentMode != new_mode.presentMode)
  {
    debug("vulkan: swapchain mode change request: mode %s -> %s", formatPresentMode(currentMode.presentMode),
      formatPresentMode(new_mode.presentMode));
    hasChanges = true;
  }

  if (currentMode.extraBusyImages != new_mode.extraBusyImages)
  {
    debug("vulkan: swapchain mode change request: busy images %u -> %u", currentMode.extraBusyImages, new_mode.extraBusyImages);
    hasChanges = true;
  }

  // only turn it on, never turn it off again
  if (currentMode.enableSrgb && !new_mode.enableSrgb)
  {
    debug("vulkan: swapchain mode change request: ignored srgb turnoff");
  }
  else if (currentMode.enableSrgb != new_mode.enableSrgb)
  {
    debug("vulkan: swapchain mode change request: enable srgb");
    hasChanges = true;
  }

  if (new_mode.modifySource)
  {
    if (lastUpdateResult != ObjectUpdateResult::RETRY)
      debug("vulkan: swapchain mode change request: %s", new_mode.modifySource);
    hasChanges = true;
  }

  currentMode = new_mode;
  currentMode.modifySource = nullptr;

  if (!hasChanges && !extentFittingChange)
  {
    debug("vulkan: no changes in swapchain mode set");
    return;
  }

  if (extentFittingChange && !hasChanges && lastUpdateResult == ObjectUpdateResult::OK)
  {
    debug("vulkan: swapchain: no object recreation mode change");
    currentMode.mismatchedExtents = false;
    destroyOffscreenBuffer();
  }
  else
  {
    lastUpdateResult = updateObject();
    if (lastUpdateResult == ObjectUpdateResult::OK)
    {
      updateConsequtiveFailures = 0;
      debug("vulkan: swapchain: presenting to display");
    }
    else if (lastUpdateResult != ObjectUpdateResult::RETRY)
    {
      ++updateConsequtiveFailures;
      // if we crash here, we must know what happened
      // D3D_ERROR on limit and crash on limit * 2, so error is sent to stats or saved to crash
      if (updateConsequtiveFailures == updateConsequtiveFailuresLimit)
      {
        D3D_ERROR("vulkan: can't init swapchain, setting device lost status");
        dagor_d3d_force_driver_reset = true;
      }
      if (updateConsequtiveFailures > updateConsequtiveFailuresLimit * 2)
        DAG_FATAL("vulkan: can't init swapchain, crash");
    }

    // surface is owned by swapchain object, so must be cleaned on change
    if (!is_null(oldSurface))
      VULKAN_LOG_CALL(Globals::VK::inst.vkDestroySurfaceKHR(Globals::VK::inst.get(), oldSurface, nullptr));
  }

  wrappedTex->pars.w = currentMode.extent.width;
  wrappedTex->pars.h = currentMode.extent.height;
  wrappedTex->pars.flg = TEXCF_RTARGET | currentMode.format.asTexFlags();
  wrappedTex->fmt = FormatStore::fromCreateFlags(wrappedTex->pars.flg);
  wrappedTex->setInitialImageViewState();

  ensureOffscreenBuffer();
  activeImage = offscreenBuffer;
  wrappedTex->image = activeImage;
}

void Swapchain::nextFrame()
{
  // already at next frame
  if (images.size() && acquiredImageIdx < images.size())
    return;

  if (!acquireSwapImage())
  {
    ensureOffscreenBuffer();
    activeImage = offscreenBuffer;
  }
  else
  {
    activeImage = images[acquiredImageIdx].img;
    Globals::ctx.dispatchCommand<CmdSwapchainImageAcquire>({activeImage, images[acquiredImageIdx].acquire});

    if ((currentMode.enableSrgb && !currentMode.mutableFormat) || rotateNeeded || currentMode.mismatchedExtents)
    {
      ensureOffscreenBuffer();
      activeImage = offscreenBuffer;
    }
    else
      destroyOffscreenBuffer();
  }

  wrappedTex->image = activeImage;
}

CmdBlitImage Swapchain::blitImage(Image *src, Image *dst)
{
  VkImageBlit blit;
  blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  blit.srcSubresource.mipLevel = 0;
  blit.srcSubresource.baseArrayLayer = 0;
  blit.srcSubresource.layerCount = 1;
  blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  blit.dstSubresource.mipLevel = 0;
  blit.dstSubresource.baseArrayLayer = 0;
  blit.dstSubresource.layerCount = 1;
  blit.srcOffsets[0].x = 0;
  blit.srcOffsets[0].y = 0;
  blit.srcOffsets[0].z = 0;
  blit.srcOffsets[1] = toOffset(src->getMipExtents(0));
  if (blit.srcOffsets[1].z < 1)
    blit.srcOffsets[1].z = 1;
  blit.dstOffsets[0].x = 0;
  blit.dstOffsets[0].y = 0;
  blit.dstOffsets[0].z = 0;
  blit.dstOffsets[1] = toOffset(dst->getMipExtents(0));
  if (blit.dstOffsets[1].z < 1)
    blit.dstOffsets[1].z = 1;
  CmdBlitImage cmd{src, dst, blit, true};
  return cmd;
}

void Swapchain::rotateFromOffscreen()
{
  // expect that state here is not needed to be restored
  SwapchainImage &acquiredImage = images[acquiredImageIdx];
  wrappedRotatedTex->image = acquiredImage.img;
  d3d::set_render_target(0, wrappedRotatedTex, 0, 0);
  d3d::set_tex(STAGE_PS, 0, wrappedTex);
  d3d::set_sampler(STAGE_PS, 0, d3d::request_sampler({}));
  d3d::set_program(Globals::shaderProgramDatabase.getRotateProgram(query.caps.currentTransform).get());
  d3d::setvsrc(0, 0, 0);
  if ((uint32_t)rotateRenderState == ~0)
    rotateRenderState = d3d::create_render_state(shaders::RenderState());
  d3d::set_render_state(rotateRenderState);
  float fv[4] = {(float)currentMode.extent.width, (float)currentMode.extent.height, 1.0f / currentMode.extent.width,
    1.0f / currentMode.extent.height};
  d3d::set_const(STAGE_PS, 0, fv, 1);
  d3d::draw(PRIM_TRILIST, 0, 1);
  wrappedRotatedTex->image = nullptr;
}

void Swapchain::prePresent()
{
  if (acquiredImageIdx >= images.size())
    return;

  if (rotateNeeded)
    rotateFromOffscreen();

  if (needStillImage && !stillImage)
  {
    ImageCreateInfo ii;
    ii.type = VK_IMAGE_TYPE_2D;
    ii.mips = 1;
    ii.size.width = currentMode.extent.width;
    ii.size.height = currentMode.extent.height;
    ii.size.depth = 1;
    ii.arrays = 1;
    ii.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
               VK_IMAGE_USAGE_SAMPLED_BIT;
    ii.format = currentMode.format;
    ii.flags = currentMode.enableSrgb ? VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT : 0;
    ii.memFlags = Image::MEM_NOT_EVICTABLE;
    ii.samples = VK_SAMPLE_COUNT_1_BIT;
    stillImage = Image::create(ii);
    Globals::ctx.dispatchCommand(blitImage(activeImage, stillImage));
  }
  else if (!needStillImage && stillImage)
  {
    Globals::ctx.destroyImage(stillImage);
    stillImage = nullptr;
  }
  needStillImage = false;
}

CmdPresent Swapchain::present()
{
  CmdPresent ret{};
  if (onlyBackendAcquire)
  {
    ret.swapchain = handle;
    ret.mutex = &acquireMutex;
    ret.backendAcquireImageIndex = &backendAcquireImageIndex;
    ret.backendAcquireResult = &backendAcquireResult;
    ret.img = activeImage;
    ret.imageIndex = ~0;
    ret.images = images.data();
    return ret;
  }

  if (acquiredImageIdx >= images.size())
    return ret;
  SwapchainImage &acquiredImage = images[acquiredImageIdx];
  G_ASSERT(acquiredImage.acquired);

  if (((currentMode.enableSrgb && !currentMode.mutableFormat) || currentMode.mismatchedExtents) && !rotateNeeded)
    Globals::ctx.dispatchCommandNoLock(blitImage(offscreenBuffer, acquiredImage.img));

  ret.swapchain = handle;
  ret.frameSem = acquiredImage.frame;
  ret.imageIndex = acquiredImageIdx;
  ret.img = acquiredImage.img;
  ret.mutex = &acquireMutex;
  ret.currentAcquireId = &acquireId;
  ret.submitAcquireId = acquireId;
  ret.backendAcquireImageIndex = &backendAcquireImageIndex;
  ret.backendAcquireResult = &backendAcquireResult;

  acquiredImage.acquired = false;
  acquiredImage.acquire = VulkanSemaphoreHandle{};
  acquiredImageIdx = ~0;

  return ret;
}

void Swapchain::shutdown()
{
  SwapchainMode newMode = currentMode;
  newMode.setHeadless();
  setMode(newMode);
  clearState();
  if (stillImage)
  {
    Globals::ctx.destroyImage(stillImage);
    stillImage = nullptr;
  }
  wrappedTex->destroy();
  wrappedTex = nullptr;
  if (wrappedRotatedTex)
  {
    wrappedRotatedTex->destroy();
    wrappedRotatedTex = nullptr;
  }
  for (VulkanSemaphoreHandle i : semaphoresRing)
    VULKAN_LOG_CALL(Globals::VK::dev.vkDestroySemaphore(Globals::VK::dev.get(), i, VKALLOC(semaphore)));
  semaphoresRing.clear();
}

void Swapchain::destroySwapchainHandle(VulkanSwapchainKHRHandle &sc_handle)
{
  if (!is_null(sc_handle))
  {
    VULKAN_LOG_CALL(Globals::VK::dev.vkDestroySwapchainKHR(Globals::VK::dev.get(), sc_handle, VKALLOC(swapchain)));
    sc_handle = VulkanNullHandle();
  }
}

bool Swapchain::blitStillImage()
{
  needStillImage = true;
  if (!stillImage)
    return false;

  Globals::ctx.dispatchCommand(blitImage(stillImage, activeImage));
  return true;
}
