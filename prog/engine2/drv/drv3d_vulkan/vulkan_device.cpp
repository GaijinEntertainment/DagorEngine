#include "vulkan_device.h"
#include <EASTL/algorithm.h>
#include <EASTL/sort.h>
#include <ioSys/dag_dataBlock.h>

using namespace drv3d_vulkan;

typedef MutualExclusiveExtensions
#if VK_AMD_negative_viewport_height || VK_KHR_maintenance1
  <ExtensionHasNever // just filler to make the #if stuff simpler
#if VK_KHR_maintenance1
    ,
    ExtensionAdaptor<Maintenance1KHR>
#endif
#if VK_AMD_negative_viewport_height
    ,
    ExtensionAdaptor<NegativeViewportHeightAMD>
#endif
#else
  <ExtensionHasAlways
#endif
    >
    InvertedViewportExtensions;

#if VK_NV_dedicated_allocation
typedef ExtensionAdaptor<DedicatedAllocationNV> VulkanDeviceDedicatedAllocationNVAdaptor;
#else
typedef ExtensionHasNever VulkanDeviceDedicatedAllocationNVAdaptor;
#endif

typedef RequiresAllExtensions<ExtensionAdaptor<DedicatedAllocationKHR>, ExtensionAdaptor<GetMemoryRequirements2KHR>>
  DedicatedAllocationAndMemoryInfoExtensions;

typedef MutualExclusiveExtensions<DedicatedAllocationAndMemoryInfoExtensions, VulkanDeviceDedicatedAllocationNVAdaptor>
  DedicaltedAllocationExtensions;

typedef RequiresAllExtensions<ExtensionHasAlways
#if VK_KHR_shader_draw_parameters
  // required to make vertex id work properly (DX11 does not add base vertex, ogl/vk does)
  ,
  ExtensionAdaptor<ShaderDrawParametersKHR>
#endif
  >
  MiscRequiredExtensions;

bool drv3d_vulkan::has_all_required_extension(dag::ConstSpan<const char *> ext_list)
{
  if (!InvertedViewportExtensions::has(ext_list))
    return false;
  if (!MiscRequiredExtensions::has(ext_list))
    return false;

  return true;
}

int drv3d_vulkan::remove_mutual_exclusive_extensions(dag::Span<const char *> ext_list)
{
  eastl::sort(begin(ext_list), end(ext_list));
  uint32_t count = eastl::unique(begin(ext_list), end(ext_list)) - begin(ext_list);
  count = InvertedViewportExtensions::filter(ext_list.data(), count);
  count = DedicaltedAllocationExtensions::filter(ext_list.data(), count);
  count = MiscRequiredExtensions::filter(ext_list.data(), count);
  return count;
}

bool drv3d_vulkan::device_has_all_required_extensions(VulkanDevice &device, void (*clb)(const char *name))
{
  // don't early exit, list all possible missing extensions
  // just do it like for the vulkan instance version to make it simple to add new stuff
  bool hasAll = true;
  if (!InvertedViewportExtensions::hasExtensions(device))
  {
    InvertedViewportExtensions::enumerate(clb);
    hasAll = false;
  }
  if (!MiscRequiredExtensions::hasExtensions(device))
  {
    MiscRequiredExtensions::enumerate(clb);
    hasAll = false;
  }

  return hasAll;
}

StaticTab<const char *, VulkanDevice::ExtensionCount> drv3d_vulkan::get_enabled_device_extensions(const DataBlock *list)
{
  StaticTab<const char *, VulkanDevice::ExtensionCount> result;

  result = make_span_const(VulkanDevice::ExtensionNames, VulkanDevice::ExtensionCount);
  if (list)
  {
    auto newEnd = end(result);
    for (int i = 0; i < list->paramCount(); ++i)
    {
      if (!list->getBool(i))
      {
        newEnd = eastl::remove_if(begin(result), newEnd,
          [pName = list->getParamName(i)](const char *name) { return 0 == strcmp(pName, name); });
      }
    }
    auto cnt = end(result) - newEnd;
    erase_items(result, result.size() - cnt, cnt);
  }

  return result;
}


uint32_t drv3d_vulkan::fill_device_extension_list(Tab<const char *> &result,
  const StaticTab<const char *, VulkanDevice::ExtensionCount> &white_list,
  const eastl::vector<VkExtensionProperties> &device_extensions)
{
  result.resize(VulkanDevice::ExtensionCount);
  uint32_t count = 0;
  debug("vulkan: Device Extensions...");
  for (auto &&ext : device_extensions)
  {
    auto extRef = eastl::find_if(begin(white_list), end(white_list),
      [&](const char *name) //
      { return 0 == strcmp(name, ext.extensionName); });
    if (extRef != end(white_list))
    {
      debug("vulkan: ...recognized %s v %u...", ext.extensionName, ext.specVersion);
      result[count++] = ext.extensionName;
    }
    else
    {
      debug("vulkan: ...unrecognized %s v %u...", ext.extensionName, ext.specVersion);
    }
  }
  auto list = make_span(result).first(count);
  // also display all extensions we support but not the device
  for (auto &&ext : white_list)
  {
    auto extRef = eastl::find_if(begin(list), end(list), [=](const char *name) { return 0 == strcmp(name, ext); });
    if (extRef == end(list))
    {
      debug("vulkan: ...missing %s...", ext);
    }
  }
  // some extensions are mutual exclusive, so we need to remove them first
  return remove_mutual_exclusive_extensions(list);
}

MemoryRequirementInfo drv3d_vulkan::get_memory_requirements(VulkanDevice &device, VulkanBufferHandle buffer)
{
  MemoryRequirementInfo result = {};

#if VK_KHR_get_memory_requirements2
  if (device.hasExtension<GetMemoryRequirements2KHR>())
  {
    const VkBufferMemoryRequirementsInfo2KHR objectInfo = //
      {VK_STRUCTURE_TYPE_BUFFER_MEMORY_REQUIREMENTS_INFO_2_KHR, nullptr, buffer};
    VkMemoryRequirements2KHR memoryInfo = {VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2_KHR, nullptr};
#if VK_KHR_dedicated_allocation
    VkMemoryDedicatedRequirementsKHR dedicatedInfo = //
      {VK_STRUCTURE_TYPE_MEMORY_DEDICATED_REQUIREMENTS_KHR, nullptr, VK_FALSE, VK_FALSE};
    if (device.hasExtension<DedicatedAllocationKHR>())
      memoryInfo.pNext = &dedicatedInfo;
#endif

    VULKAN_LOG_CALL(device.vkGetBufferMemoryRequirements2KHR(device.get(), &objectInfo, &memoryInfo));

#if VK_KHR_dedicated_allocation
    if (device.hasExtension<DedicatedAllocationKHR>())
    {
      result.prefersDedicatedAllocation = dedicatedInfo.prefersDedicatedAllocation != VK_FALSE;
      result.requiresDedicatedAllocation = dedicatedInfo.requiresDedicatedAllocation != VK_FALSE;
    }
    else
#endif
    {
      result.prefersDedicatedAllocation = false;
      result.requiresDedicatedAllocation = false;
    }
    result.requirements = memoryInfo.memoryRequirements;
  }
  else
#endif
  {
    VULKAN_LOG_CALL(device.vkGetBufferMemoryRequirements(device.get(), buffer, &result.requirements));

    result.prefersDedicatedAllocation = false;
    result.requiresDedicatedAllocation = false;
  }

  return result;
}

MemoryRequirementInfo drv3d_vulkan::get_memory_requirements(VulkanDevice &device, VulkanImageHandle image)
{
  MemoryRequirementInfo result = {};

#if VK_KHR_get_memory_requirements2
  if (device.hasExtension<GetMemoryRequirements2KHR>())
  {
    const VkImageMemoryRequirementsInfo2KHR objectInfo = //
      {VK_STRUCTURE_TYPE_IMAGE_MEMORY_REQUIREMENTS_INFO_2_KHR, nullptr, image};
    VkMemoryRequirements2KHR memoryInfo = {VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2_KHR, nullptr};
#if VK_KHR_dedicated_allocation
    VkMemoryDedicatedRequirementsKHR dedicatedInfo = //
      {VK_STRUCTURE_TYPE_MEMORY_DEDICATED_REQUIREMENTS_KHR, nullptr, VK_FALSE, VK_FALSE};
    if (device.hasExtension<DedicatedAllocationKHR>())
      memoryInfo.pNext = &dedicatedInfo;
#endif

    VULKAN_LOG_CALL(device.vkGetImageMemoryRequirements2KHR(device.get(), &objectInfo, &memoryInfo));

#if VK_KHR_dedicated_allocation
    if (device.hasExtension<DedicatedAllocationKHR>())
    {
      result.prefersDedicatedAllocation = dedicatedInfo.prefersDedicatedAllocation != VK_FALSE;
      result.requiresDedicatedAllocation = dedicatedInfo.requiresDedicatedAllocation != VK_FALSE;
    }
    else
#endif
    {
      result.prefersDedicatedAllocation = false;
      result.requiresDedicatedAllocation = false;
    }
    result.requirements = memoryInfo.memoryRequirements;
  }
  else
#endif
  {
    VULKAN_LOG_CALL(device.vkGetImageMemoryRequirements(device.get(), image, &result.requirements));

    result.prefersDedicatedAllocation = false;
    result.requiresDedicatedAllocation = false;
  }

  return result;
}
