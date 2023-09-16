#include "vulkan_instance.h"
#include "vulkan_loader.h"
#include <EASTL/algorithm.h>
#include <EASTL/sort.h>
#include <ioSys/dag_dataBlock.h>

using namespace drv3d_vulkan;

namespace
{
StaticTab<const char *, drv3d_vulkan::VulkanInstance::ExtensionCount> get_all_supported_extension_names(
  const eastl::vector<VkExtensionProperties> &ext_list)
{
  StaticTab<const char *, VulkanInstance::ExtensionCount> result;
  for (auto &&ext : ext_list)
  {
    auto ref = eastl::find_if(eastl::begin(drv3d_vulkan::VulkanInstance::ExtensionNames),
      eastl::end(drv3d_vulkan::VulkanInstance::ExtensionNames),
      [extName = ext.extensionName](const char *name) { return 0 == strcmp(name, extName); });
    if (eastl::end(drv3d_vulkan::VulkanInstance::ExtensionNames) != ref)
      result.push_back(*ref);
  }
  return result;
}
} // namespace

// this way is needed or it will filter the extension out
typedef RequiresAnyExtension<ExtensionAdaptor<SurfaceKHR>> WindowingSystemSurfaceExtension;

typedef RequiresAnyExtension<ExtensionHasNever
#if VK_KHR_win32_surface
  ,
  ExtensionAdaptor<SurfaceWin32KHR>
#endif
#if VK_KHR_android_surface
  ,
  ExtensionAdaptor<SurfaceAndroidKHR>
#endif
#if VK_KHR_mir_surface
  ,
  ExtensionAdaptor<SurfaceMirKHR>
#endif
#if VK_KHR_wayland_surface
  ,
  ExtensionAdaptor<SurfaceWaylandKHR>
#endif
#if VK_KHR_xcb_surface
  ,
  ExtensionAdaptor<SurfaceXcbKHR>
#endif
#if VK_KHR_xlib_surface
  ,
  ExtensionAdaptor<SurfaceXlibKHR>
#endif
#if VK_NN_vi_surface
  ,
  ExtensionAdaptor<SurfaceViNN>
#endif
#if VK_MVK_ios_surface
  ,
  ExtensionAdaptor<SurfaceIosMVK>
#endif
#if VK_MVK_macos_surface
  ,
  ExtensionAdaptor<SurfaceMacosMVK>
#endif
  >
  WindowingSystemSpecificExtensions;

typedef MutualExclusiveExtensions<
#if VK_EXT_debug_utils
  ExtensionAdaptor<DebugUtilsEXT>,
#endif
#if VK_EXT_debug_report
  ExtensionAdaptor<DebugReport>,
#endif
  ExtensionHasNever>
  DebugExtensions;

bool drv3d_vulkan::has_all_required_instance_extension(dag::ConstSpan<const char *> ext_list, void (*clb)(const char *name))
{
  // don't early exit, list all possible missing extensions
  bool hasAll = true;
  if (!WindowingSystemSurfaceExtension::has(ext_list))
  {
    WindowingSystemSurfaceExtension::enumerate(clb);
    hasAll = false;
  }
  if (!WindowingSystemSpecificExtensions::has(ext_list))
  {
    WindowingSystemSpecificExtensions::enumerate(clb);
    hasAll = false;
  }

  return hasAll;
}

int drv3d_vulkan::remove_mutual_exclusive_instance_extensions(dag::Span<const char *> ext_list)
{
  eastl::sort(begin(ext_list), end(ext_list));
  uint32_t count = eastl::unique(begin(ext_list), end(ext_list)) - begin(ext_list);
  count = WindowingSystemSurfaceExtension::filter(ext_list.data(), count);
  count = WindowingSystemSpecificExtensions::filter(ext_list.data(), count);
  count = DebugExtensions::filter(ext_list.data(), count);
  return count;
}

int drv3d_vulkan::remove_debug_instance_extensions(dag::Span<const char *> ext_list)
{
  return DebugExtensions::remove(ext_list.data(), ext_list.size());
}

int drv3d_vulkan::filter_instance_extension(dag::Span<const char *> ext_list, const char *filter)
{
  for (auto &&name : ext_list)
  {
    if (0 == strcmp(name, filter))
    {
      name = ext_list.back();
      return ext_list.size() - 1;
    }
  }
  return ext_list.size();
}

bool drv3d_vulkan::instance_has_all_required_extensions(VulkanInstance &instance, void (*clb)(const char *name))
{
  // don't early exit, list all possible missing extensions
  bool hasAll = true;
  if (!WindowingSystemSurfaceExtension::hasExtensions(instance))
  {
    WindowingSystemSurfaceExtension::enumerate(clb);
    hasAll = false;
  }
  if (!WindowingSystemSpecificExtensions::hasExtensions(instance))
  {
    WindowingSystemSpecificExtensions::enumerate(clb);
    hasAll = false;
  }

  return hasAll;
}

Tab<const char *> drv3d_vulkan::build_instance_extension_list(VulkanLoader &loader, const DataBlock *extension_list_names,
  int debug_level, Driver3dInitCallback *cb)
{
  debug("vulkan: Querying Instance Extensions...");
  auto extensionList = loader.getExtensions();
  for (auto &&ext : extensionList)
  {
    debug("vulkan: ...%s v %u...", ext.extensionName, ext.specVersion);
  }
  debug("vulkan: ...%u instance extensions found...", extensionList.size());

  auto instanceExtensionSet = get_all_supported_extension_names(extensionList);
  if (debug_level < 1)
  {
    instanceExtensionSet.resize(remove_debug_instance_extensions(make_span(instanceExtensionSet)));
    debug("vulkan: Debug turned off, ignoring related extensions");
  }
  if (extension_list_names)
  {
    for (int i = 0; i < extension_list_names->paramCount(); ++i)
    {
      if (!extension_list_names->getBool(i))
      {
        debug("vulkan: Extension %s disabled via configuration", extension_list_names->getParamName(i));
        instanceExtensionSet.resize(filter_instance_extension(make_span(instanceExtensionSet), extension_list_names->getParamName(i)));
      }
    }
  }

  instanceExtensionSet.resize(remove_mutual_exclusive_instance_extensions(make_span(instanceExtensionSet)));

  Tab<const char *> allRequiredInstanceExtensions;
  allRequiredInstanceExtensions = instanceExtensionSet;

  if (cb)
  {
    char *desiredRendererExtensions = const_cast<char *>(cb->desiredRendererInstanceExtensions());
    while (desiredRendererExtensions && *desiredRendererExtensions)
    {
      char *currentExtension = desiredRendererExtensions;

      char *separator = strchr(desiredRendererExtensions, ' ');
      if (separator && *separator)
      {
        *separator = 0;
        desiredRendererExtensions = separator + 1;
      }
      else
        desiredRendererExtensions = separator;

      bool dupe =
        eastl::find_if(allRequiredInstanceExtensions.cbegin(), allRequiredInstanceExtensions.cend(),
          [currentExtension](const char *ext) { return strcmp(ext, currentExtension) == 0; }) != allRequiredInstanceExtensions.cend();

      if (!dupe)
        allRequiredInstanceExtensions.push_back(currentExtension);
    }
  }

  return allRequiredInstanceExtensions;
}

eastl::vector<VkLayerProperties> drv3d_vulkan::build_instance_layer_list(VulkanLoader &loader, const DataBlock *layer_list,
  int debug_level, bool enable_render_doc_layer)
{
  debug("vulkan: Query Instance Layers...");
  auto layerList = loader.getLayers();
  Tab<const char *> layerWhiteList;

  // build up a white list defined by the config block vulkan.layers
  if (layer_list)
    for (uint32_t i = 0; i < layer_list->paramCount(); ++i)
      if (layer_list->getBool(i))
        layerWhiteList.push_back(layer_list->getParamName(i));

#if VK_EXT_debug_report
  if (debug_level > 1)
  {
    const char *defaultLayer = "VK_LAYER_KHRONOS_validation";
    if (!(layer_list && !layer_list->getBool(defaultLayer, true)))
      layerWhiteList.push_back(defaultLayer);
  }
#else
  G_UNUSED(debug_level);
#endif
  if (enable_render_doc_layer)
  {
    layerWhiteList.push_back("VK_LAYER_RENDERDOC_Capture");
  }

  auto newEnd = eastl::remove_if(begin(layerList), end(layerList), [&](const VkLayerProperties &layer) {
    auto whiteRef = eastl::find_if(layerWhiteList.cbegin(), layerWhiteList.cend(),
      [&](const char *name) { return 0 == strcmp(name, layer.layerName); });
    if (whiteRef != layerWhiteList.cend())
    {
      debug("vulkan: ...using %s v%u: %s...", layer.layerName, layer.implementationVersion, layer.description);
      return false;
    }
    else
    {
      debug("vulkan: ...ignoring %s v%u: %s...", layer.layerName, layer.implementationVersion, layer.description);
      return true;
    }
  });
  layerList.erase(newEnd, end(layerList));
  return layerList;
}

#if VK_EXT_debug_report
VulkanDebugReportCallbackEXTHandle drv3d_vulkan::create_debug_sink(VulkanInstance &instance, int debug_level,
  PFN_vkDebugReportCallbackEXT clb, void *data)
{
  if (instance.hasExtension<DebugReport>() && debug_level > 0)
  {
    VkDebugReportCallbackCreateInfoEXT drcci;
    drcci.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
    drcci.pNext = nullptr;
    drcci.pfnCallback = clb;
    drcci.pUserData = data;
    drcci.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT;

    if (debug_level > 1)
      drcci.flags |= VK_DEBUG_REPORT_WARNING_BIT_EXT;
    if (debug_level > 2)
      drcci.flags |= VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
    if (debug_level > 3)
      drcci.flags |= VK_DEBUG_REPORT_INFORMATION_BIT_EXT;
    if (debug_level > 4)
      drcci.flags |= VK_DEBUG_REPORT_DEBUG_BIT_EXT;

    VulkanDebugReportCallbackEXTHandle result;
    VULKAN_LOG_CALL(instance.vkCreateDebugReportCallbackEXT(instance.get(), &drcci, nullptr, ptr(result)));
    return result;
  }
  return {};
}
#endif

#if VK_EXT_debug_utils
VulkanDebugUtilsMessengerEXTHandle drv3d_vulkan::create_debug_sink_2(VulkanInstance &instance, int debug_level,
  PFN_vkDebugUtilsMessengerCallbackEXT clb, void *data)
{
  if (instance.hasExtension<DebugUtilsEXT>() && debug_level > 0)
  {
    VkDebugUtilsMessengerCreateInfoEXT dumci{VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};
    dumci.flags = 0;
    dumci.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    dumci.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
    dumci.pfnUserCallback = clb;
    dumci.pUserData = data;
    if (debug_level > 1)
    {
      dumci.messageSeverity |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
    }
    if (debug_level > 2)
    {
      dumci.messageType |= VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    }
    if (debug_level > 3)
    {
      dumci.messageSeverity |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;
    }
    if (debug_level > 4)
    {
      dumci.messageType |= VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT;
      dumci.messageSeverity |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT;
    }
    VulkanDebugUtilsMessengerEXTHandle result;
    VULKAN_LOG_CALL(instance.vkCreateDebugUtilsMessengerEXT(instance.get(), &dumci, nullptr, ptr(result)));
    return result;
  }
  return {};
}
#endif

VkFormat drv3d_vulkan::get_default_depth_stencil_format(VulkanInstance &instance, VulkanPhysicalDeviceHandle device)
{
  constexpr VkFormatFeatureFlags depth_stencil_flags_needed = VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;
  // ordering is important here
  // should be from most desired to last desired
  static const VkFormat viable_formats[] = //
    {VK_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_D32_SFLOAT_S8_UINT};

  for (auto &&fmt : viable_formats)
  {
    VkFormatProperties fp;
    VULKAN_LOG_CALL(instance.vkGetPhysicalDeviceFormatProperties(device, fmt, &fp));
    if ((fp.optimalTilingFeatures & depth_stencil_flags_needed) == depth_stencil_flags_needed)
      return fmt;
  }

  return VK_FORMAT_UNDEFINED;
}
