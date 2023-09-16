#include "vulkan_loader.h"
#include "os.h"
#include <osApiWrappers/dag_dynLib.h>
#include <EASTL/sort.h>

using namespace drv3d_vulkan;

eastl::vector<VkLayerProperties> VulkanLoader::getLayers()
{
  eastl::vector<VkLayerProperties> result;
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

eastl::vector<VkExtensionProperties> VulkanLoader::getExtensions()
{
  eastl::vector<VkExtensionProperties> result;
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

bool VulkanLoader::load(const char *name, bool validate)
{
#if !_TARGET_C3
  libHandle = os_dll_load(name);
  if (!libHandle)
    return false;

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