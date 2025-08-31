// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "vulkan_loader.h"
#include "os.h"
#include <osApiWrappers/dag_dynLib.h>
#include <EASTL/sort.h>

using namespace drv3d_vulkan;

dag::Vector<VkLayerProperties> VulkanLoader::getLayers()
{
  dag::Vector<VkLayerProperties> result;
  uint32_t count = 0;
  // have to loop until the second call reports an error or success
  // in theory it can return incomplete if new layers where added inbetween calls
  for (;;)
  {
    VULKAN_EXIT_ON_FAIL(vkEnumerateInstanceLayerProperties(&count, nullptr));
    result.resize(count);
    VkResult errorCode = VULKAN_CHECK_RESULT(vkEnumerateInstanceLayerProperties(&count, result.data()));
    VULKAN_EXIT_ON_FAIL(errorCode);
    if (errorCode == VK_SUCCESS)
    {
      result.resize(count);
      break;
    }
  }
  return result;
}

dag::Vector<VkExtensionProperties> VulkanLoader::getExtensions()
{
  dag::Vector<VkExtensionProperties> result;
  uint32_t count = 0;
  // have to loop until the second call reports an error or success
  // in theory it can return incomplete if new extensions where added inbetween calls
  for (;;)
  {
    uint32_t oldCount = count;
    VULKAN_EXIT_ON_FAIL(vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr));
    debug("vulkan: found %u instance extensions", count);
    // prevent possible endless loop
    if ((count == -1) || (oldCount >= count))
    {
      debug("vulkan: no instance extensions or system is bugged (count %u old %u)", count, oldCount);
      break;
    }
    result.resize(count);
    VkResult errorCode = VULKAN_CHECK_RESULT(vkEnumerateInstanceExtensionProperties(nullptr, &count, result.data()));
    VULKAN_EXIT_ON_FAIL(errorCode);
    if (errorCode == VK_SUCCESS)
    {
      result.resize(count);
      break;
    }
  }

  // It can happen that extensions are listed more than once, this can happen when multiple
  // layers implement the same extension and add them self into the list.
  // Need to filter them out first that later algorithms can safely assume each extension
  // is unique.
  eastl::sort(begin(result), end(result),
    [](const VkExtensionProperties &l, const VkExtensionProperties &r) //
    { return strcmp(l.extensionName, r.extensionName) < 0; });

  auto newEnd = eastl::unique(begin(result), end(result),
    [](const VkExtensionProperties &l, const VkExtensionProperties &r) //
    { return 0 == strcmp(r.extensionName, l.extensionName); });
  result.erase(newEnd, end(result));

  return result;
}

#if USE_STREAMLINE_FOR_DLSS
bool VulkanLoader::initStreamlineAdapter()
{
  if (streamlineInterposer)
  {
    StreamlineAdapter::SupportOverrideMap supportOverride;
    if (!StreamlineAdapter::init(streamlineAdapter, StreamlineAdapter::RenderAPI::Vulkan, supportOverride))
    {
      streamlineInterposer.reset();
      return false;
    }
    else
    {
      // old device or non NV device, unload interposer and adapter to avoid hooking issues
      // as we can't control code executed in hooked methods
      bool noSupport = streamlineAdapter->isDlssSupported() != nv::SupportState::Supported ||
                       streamlineAdapter->isReflexSupported() != nv::SupportState::Supported;
      if (noSupport)
      {
        streamlineAdapter.reset();
        streamlineInterposer.reset();
        return false;
      }
      else
        return true;
    }
  }
  return false;
}
#endif

bool VulkanLoader::load(const char *name, bool validate)
{
#if !_TARGET_C3
#if USE_STREAMLINE_FOR_DLSS
  streamlineInterposer = StreamlineAdapter::loadInterposer();
  if (initStreamlineAdapter())
    libHandle = streamlineInterposer.get();
#endif
  if (!libHandle)
  {
    libHandle = os_dll_load(name);
    if (!libHandle)
      return false;
  }
#if VULKAN_DO_SIGN_CHECK
  // on windows the lib is digitaly signed to make
  // sure we get a std conformant loader
  if (validate)
  {
    if (!validate_vulkan_signature(libHandle))
    {
      unload();
      return false;
    }
  }
#else
  (void)validate;
#endif

#define VK_DEFINE_ENTRY_POINT(name)                                        \
  *reinterpret_cast<void **>(&name) = os_dll_get_symbol(libHandle, #name); \
  if (!name)                                                               \
  {                                                                        \
    unload();                                                              \
    return false;                                                          \
  }
  VK_LOADER_ENTRY_POINT_LIST
#undef VK_DEFINE_ENTRY_POINT

#else


#endif

  return true;
}

void VulkanLoader::unload()
{
#if !_TARGET_C3
#if USE_STREAMLINE_FOR_DLSS
  if (streamlineInterposer)
  {
    streamlineInterposer.reset();
    libHandle = nullptr;
  }
#endif
  if (libHandle)
  {
    os_dll_close(libHandle);
    libHandle = nullptr;
  }
#define VK_DEFINE_ENTRY_POINT(name) name = nullptr;
  VK_LOADER_ENTRY_POINT_LIST
#undef VK_DEFINE_ENTRY_POINT

#endif
}