// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <generic/dag_tab.h>
#include <EASTL/bitset.h>
#include <drv/3d/dag_info.h>

#include "driver.h"
#include "vulkan_loader.h"
#include "vk_entry_points.h"
#include "extension_utils.h"

namespace drv3d_vulkan
{
#if VK_EXT_debug_utils
DEFINE_VULKAN_NON_DISPATCHABLE_HANDLE(VulkanDebugUtilsMessengerEXTHandle, VkDebugUtilsMessengerEXT);

#define VK_EXT_DEBUG_UTILS_FUNCTION_LIST   \
  VK_FUNC(vkSetDebugUtilsObjectNameEXT)    \
  VK_FUNC(vkSetDebugUtilsObjectTagEXT)     \
  VK_FUNC(vkQueueBeginDebugUtilsLabelEXT)  \
  VK_FUNC(vkQueueEndDebugUtilsLabelEXT)    \
  VK_FUNC(vkQueueInsertDebugUtilsLabelEXT) \
  VK_FUNC(vkCmdBeginDebugUtilsLabelEXT)    \
  VK_FUNC(vkCmdEndDebugUtilsLabelEXT)      \
  VK_FUNC(vkCmdInsertDebugUtilsLabelEXT)   \
  VK_FUNC(vkCreateDebugUtilsMessengerEXT)  \
  VK_FUNC(vkDestroyDebugUtilsMessengerEXT) \
  VK_FUNC(vkSubmitDebugUtilsMessageEXT)

#define VK_FUNC VULKAN_MAKE_EXTENSION_FUNCTION_DEF
VK_EXT_DEBUG_UTILS_FUNCTION_LIST
#undef VK_FUNC

VULKAN_BEGIN_EXTENSION_FUNCTION_PACK
#define VK_FUNC VULKAN_EXTENSION_FUNCTION_PACK_ENTRY
VK_EXT_DEBUG_UTILS_FUNCTION_LIST
#undef VK_FUNC
VULKAN_END_EXTENSION_FUCTION_PACK(DebugUtilsEXT);

#undef VK_EXT_DEBUG_UTILS_FUNCTION_LIST

VULKAN_DECLARE_EXTENSION(DebugUtilsEXT, EXT_DEBUG_UTILS);
#endif

#if VK_MVK_ios_surface
// only one entry point no need for a list
VULKAN_MAKE_CORE_FUNCTION_DEF(vkCreateIOSSurfaceMVK);

VULKAN_BEGIN_EXTENSION_FUNCTION_PACK
VULKAN_EXTENSION_FUNCTION_PACK_ENTRY(vkCreateIOSSurfaceMVK)
VULKAN_END_EXTENSION_FUCTION_PACK(SurfaceIosMVK);

VULKAN_DECLARE_EXTENSION(SurfaceIosMVK, MVK_IOS_SURFACE);
#endif
#if VK_MVK_macos_surface
// only one entry point no need for a list
VULKAN_MAKE_CORE_FUNCTION_DEF(vkCreateMacOSSurfaceMVK);

VULKAN_BEGIN_EXTENSION_FUNCTION_PACK
VULKAN_EXTENSION_FUNCTION_PACK_ENTRY(vkCreateMacOSSurfaceMVK)
VULKAN_END_EXTENSION_FUCTION_PACK(SurfaceMacosMVK);

VULKAN_DECLARE_EXTENSION(SurfaceMacosMVK, MVK_MACOS_SURFACE);
#endif

#if VK_KHR_get_physical_device_properties2
#define VK_FUNC_LIST                                    \
  VK_FUNC(vkGetPhysicalDeviceFeatures2KHR)              \
  VK_FUNC(vkGetPhysicalDeviceProperties2KHR)            \
  VK_FUNC(vkGetPhysicalDeviceFormatProperties2KHR)      \
  VK_FUNC(vkGetPhysicalDeviceImageFormatProperties2KHR) \
  VK_FUNC(vkGetPhysicalDeviceQueueFamilyProperties2KHR) \
  VK_FUNC(vkGetPhysicalDeviceMemoryProperties2KHR)      \
  VK_FUNC(vkGetPhysicalDeviceSparseImageFormatProperties2KHR)

#define VK_FUNC(name) VULKAN_MAKE_EXTENSION_FUNCTION_DEF(name)
VK_FUNC_LIST
#undef VK_FUNC

VULKAN_BEGIN_EXTENSION_FUNCTION_PACK
#define VK_FUNC(name) VULKAN_EXTENSION_FUNCTION_PACK_ENTRY(name)
VK_FUNC_LIST
#undef VK_FUNC
VULKAN_END_EXTENSION_FUCTION_PACK(GetPhysicalDeviceProperties2KHR);

#undef VK_FUNC_LIST

VULKAN_DECLARE_EXTENSION(GetPhysicalDeviceProperties2KHR, KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2);
#endif

DEFINE_VULKAN_NON_DISPATCHABLE_HANDLE(VulkanSurfaceKHRHandle, VkSurfaceKHR);

VULKAN_MAKE_CORE_FUNCTION_DEF(vkDestroySurfaceKHR)
VULKAN_MAKE_CORE_FUNCTION_DEF(vkGetPhysicalDeviceSurfaceSupportKHR)
VULKAN_MAKE_CORE_FUNCTION_DEF(vkGetPhysicalDeviceSurfaceCapabilitiesKHR)
VULKAN_MAKE_CORE_FUNCTION_DEF(vkGetPhysicalDeviceSurfaceFormatsKHR)
VULKAN_MAKE_CORE_FUNCTION_DEF(vkGetPhysicalDeviceSurfacePresentModesKHR)

VULKAN_BEGIN_EXTENSION_FUNCTION_PACK
VULKAN_EXTENSION_FUNCTION_PACK_ENTRY(vkDestroySurfaceKHR)
VULKAN_EXTENSION_FUNCTION_PACK_ENTRY(vkGetPhysicalDeviceSurfaceSupportKHR)
VULKAN_EXTENSION_FUNCTION_PACK_ENTRY(vkGetPhysicalDeviceSurfaceCapabilitiesKHR)
VULKAN_EXTENSION_FUNCTION_PACK_ENTRY(vkGetPhysicalDeviceSurfaceFormatsKHR)
VULKAN_EXTENSION_FUNCTION_PACK_ENTRY(vkGetPhysicalDeviceSurfacePresentModesKHR)
VULKAN_END_EXTENSION_FUCTION_PACK(SurfaceKHR);

VULKAN_DECLARE_EXTENSION(SurfaceKHR, KHR_SURFACE);

#if VK_KHR_win32_surface
VULKAN_MAKE_CORE_FUNCTION_DEF(vkCreateWin32SurfaceKHR)
VULKAN_MAKE_CORE_FUNCTION_DEF(vkGetPhysicalDeviceWin32PresentationSupportKHR)

VULKAN_BEGIN_EXTENSION_FUNCTION_PACK
VULKAN_EXTENSION_FUNCTION_PACK_ENTRY(vkCreateWin32SurfaceKHR)
// VULKAN_EXTENSION_FUNCTION_PACK_ENTRY(vkGetPhysicalDeviceWin32PresentationSupportKHR)
VULKAN_END_EXTENSION_FUCTION_PACK(SurfaceWin32KHR);

VULKAN_DECLARE_EXTENSION(SurfaceWin32KHR, KHR_WIN32_SURFACE);
#endif

#if VK_KHR_android_surface
VULKAN_MAKE_CORE_FUNCTION_DEF(vkCreateAndroidSurfaceKHR)

VULKAN_BEGIN_EXTENSION_FUNCTION_PACK
VULKAN_EXTENSION_FUNCTION_PACK_ENTRY(vkCreateAndroidSurfaceKHR)
VULKAN_END_EXTENSION_FUCTION_PACK(SurfaceAndroidKHR);

VULKAN_DECLARE_EXTENSION(SurfaceAndroidKHR, KHR_ANDROID_SURFACE);
#endif

#if VK_KHR_mir_surface
VULKAN_MAKE_CORE_FUNCTION_DEF(vkCreateMirSurfaceKHR)
VULKAN_MAKE_CORE_FUNCTION_DEF(vkGetPhysicalDeviceMirPresentationSupportKHR)

VULKAN_BEGIN_EXTENSION_FUNCTION_PACK
VULKAN_EXTENSION_FUNCTION_PACK_ENTRY(vkCreateMirSurfaceKHR)
// VULKAN_EXTENSION_FUNCTION_PACK_ENTRY(vkGetPhysicalDeviceMirPresentationSupportKHR)
VULKAN_END_EXTENSION_FUCTION_PACK(SurfaceMirKHR);

VULKAN_DECLARE_EXTENSION(SurfaceMirKHR, KHR_MIR_SURFACE);
#endif

#if VK_KHR_wayland_surface
VULKAN_MAKE_CORE_FUNCTION_DEF(vkCreateWaylandSurfaceKHR)
VULKAN_MAKE_CORE_FUNCTION_DEF(vkGetPhysicalDeviceWaylandPresentationSupportKHR)

VULKAN_BEGIN_EXTENSION_FUNCTION_PACK
VULKAN_EXTENSION_FUNCTION_PACK_ENTRY(vkCreateWaylandSurfaceKHR)
// VULKAN_EXTENSION_FUNCTION_PACK_ENTRY(vkGetPhysicalDeviceWaylandPresentationSupportKHR)
VULKAN_END_EXTENSION_FUCTION_PACK(SurfaceWaylandKHR);

VULKAN_DECLARE_EXTENSION(SurfaceWaylandKHR, KHR_WAYLAND_SURFACE);
#endif

#if VK_KHR_xcb_surface
VULKAN_MAKE_CORE_FUNCTION_DEF(vkCreateXcbSurfaceKHR)
VULKAN_MAKE_CORE_FUNCTION_DEF(vkGetPhysicalDeviceXcbPresentationSupportKHR)

VULKAN_BEGIN_EXTENSION_FUNCTION_PACK
VULKAN_EXTENSION_FUNCTION_PACK_ENTRY(vkCreateXcbSurfaceKHR)
// VULKAN_EXTENSION_FUNCTION_PACK_ENTRY(vkGetPhysicalDeviceXcbPresentationSupportKHR)
VULKAN_END_EXTENSION_FUCTION_PACK(SurfaceXcbKHR);

VULKAN_DECLARE_EXTENSION(SurfaceXcbKHR, KHR_XCB_SURFACE);
#endif

#if VK_KHR_xlib_surface
VULKAN_MAKE_CORE_FUNCTION_DEF(vkCreateXlibSurfaceKHR)
VULKAN_MAKE_CORE_FUNCTION_DEF(vkGetPhysicalDeviceXlibPresentationSupportKHR)

VULKAN_BEGIN_EXTENSION_FUNCTION_PACK
VULKAN_EXTENSION_FUNCTION_PACK_ENTRY(vkCreateXlibSurfaceKHR)
// VULKAN_EXTENSION_FUNCTION_PACK_ENTRY(vkGetPhysicalDeviceXlibPresentationSupportKHR)
VULKAN_END_EXTENSION_FUCTION_PACK(SurfaceXlibKHR);

VULKAN_DECLARE_EXTENSION(SurfaceXlibKHR, KHR_XLIB_SURFACE);
#endif

#if VK_NN_vi_surface
VULKAN_MAKE_CORE_FUNCTION_DEF(vkCreateViSurfaceNN)

VULKAN_BEGIN_EXTENSION_FUNCTION_PACK
VULKAN_EXTENSION_FUNCTION_PACK_ENTRY(vkCreateViSurfaceNN)
VULKAN_END_EXTENSION_FUCTION_PACK(SurfaceViNN);

VULKAN_DECLARE_EXTENSION(SurfaceViNN, NN_VI_SURFACE);
#endif

#if VK_KHR_display

DEFINE_VULKAN_NON_DISPATCHABLE_HANDLE(VulkanDisplayKHRHandle, VkDisplayKHR);
DEFINE_VULKAN_NON_DISPATCHABLE_HANDLE(VulkanDisplayModeKHRHandle, VkDisplayModeKHR);

#define VK_FUNC_LIST                                    \
  VK_FUNC(vkGetPhysicalDeviceDisplayPropertiesKHR)      \
  VK_FUNC(vkGetPhysicalDeviceDisplayPlanePropertiesKHR) \
  VK_FUNC(vkGetDisplayPlaneSupportedDisplaysKHR)        \
  VK_FUNC(vkGetDisplayModePropertiesKHR)                \
  VK_FUNC(vkCreateDisplayModeKHR)                       \
  VK_FUNC(vkGetDisplayPlaneCapabilitiesKHR)             \
  VK_FUNC(vkCreateDisplayPlaneSurfaceKHR)

#define VK_FUNC(name) VULKAN_MAKE_EXTENSION_FUNCTION_DEF(name)
VK_FUNC_LIST
#undef VK_FUNC

VULKAN_BEGIN_EXTENSION_FUNCTION_PACK
#define VK_FUNC(name) VULKAN_EXTENSION_FUNCTION_PACK_ENTRY(name)
VK_FUNC_LIST
#undef VK_FUNC
VULKAN_END_EXTENSION_FUCTION_PACK(DisplayKHR);

#undef VK_FUNC_LIST

VULKAN_DECLARE_EXTENSION(DisplayKHR, KHR_DISPLAY);
#endif

#if VK_EXT_debug_report
DEFINE_VULKAN_NON_DISPATCHABLE_HANDLE(VulkanDebugReportCallbackEXTHandle, VkDebugReportCallbackEXT);

VULKAN_MAKE_EXTENSION_FUNCTION_DEF(vkCreateDebugReportCallbackEXT)
VULKAN_MAKE_EXTENSION_FUNCTION_DEF(vkDestroyDebugReportCallbackEXT)
VULKAN_MAKE_EXTENSION_FUNCTION_DEF(vkDebugReportMessageEXT)

VULKAN_BEGIN_EXTENSION_FUNCTION_PACK
VULKAN_EXTENSION_FUNCTION_PACK_ENTRY(vkCreateDebugReportCallbackEXT)
VULKAN_EXTENSION_FUNCTION_PACK_ENTRY(vkDestroyDebugReportCallbackEXT)
VULKAN_EXTENSION_FUNCTION_PACK_ENTRY(vkDebugReportMessageEXT)
VULKAN_END_EXTENSION_FUCTION_PACK(DebugReport);

VULKAN_DECLARE_EXTENSION(DebugReport, EXT_DEBUG_REPORT);
#endif

#if VK_GOOGLE_surfaceless_query
VULKAN_BEGIN_EXTENSION_FUNCTION_PACK
VULKAN_END_EXTENSION_FUCTION_PACK(SurfacelessQueryGOOGLE);
VULKAN_DECLARE_EXTENSION(SurfacelessQueryGOOGLE, GOOGLE_SURFACELESS_QUERY);
#endif

#if VK_KHR_get_surface_capabilities2

VULKAN_MAKE_EXTENSION_FUNCTION_DEF(vkGetPhysicalDeviceSurfaceCapabilities2KHR)
VULKAN_MAKE_EXTENSION_FUNCTION_DEF(vkGetPhysicalDeviceSurfaceFormats2KHR)

VULKAN_BEGIN_EXTENSION_FUNCTION_PACK
VULKAN_EXTENSION_FUNCTION_PACK_ENTRY(vkGetPhysicalDeviceSurfaceCapabilities2KHR)
VULKAN_EXTENSION_FUNCTION_PACK_ENTRY(vkGetPhysicalDeviceSurfaceFormats2KHR)
VULKAN_END_EXTENSION_FUCTION_PACK(GetSurfaceCapabilities2KHR);
VULKAN_DECLARE_EXTENSION(GetSurfaceCapabilities2KHR, KHR_GET_SURFACE_CAPABILITIES_2);
#endif

// Implements vulkan core instance handling
template <typename... Extensions>
class VulkanInstanceCore : public Extensions...
{
  typedef TypePack<Extensions...> ExtensionTypePack;
  typedef eastl::bitset<sizeof...(Extensions)> ExtensionBitSetType;

  VulkanLoader *loader = nullptr;
  VulkanInstanceHandle instance;
  ExtensionBitSetType extensionEnabled;

public:
  VulkanInstanceCore(const VulkanInstanceCore &) = delete;
  VulkanInstanceCore &operator=(const VulkanInstanceCore &) = delete;

  VulkanInstanceCore() = default;

private:
  uint32_t activeApiVersion;

  template <typename T, typename T2, typename... L>
  void extensionInit(const VkInstanceCreateInfo &ici)
  {
    extensionInit<T>(ici);
    extensionInit<T2, L...>(ici);
  }
  template <typename T>
  void extensionInit(const VkInstanceCreateInfo &ici)
  {
    auto listStart = ici.ppEnabledExtensionNames;
    auto listEnd = listStart + ici.enabledExtensionCount;
    auto extRef = eastl::find_if(listStart, listEnd, [](const char *e_name) { return 0 == strcmp(e_name, extension_name<T>()); });
    bool loaded = false;
    if (extRef != listEnd)
    {
      loaded = true;
      this->T::enumerateFunctions([this, &loaded](const char *f_name, PFN_vkVoidFunction &function) {
        function = VULKAN_LOG_CALL_R(loader->vkGetInstanceProcAddr(instance, f_name));
        if (!function)
          loaded = false;
      });
    }
    extensionEnabled.set(TypeIndexOf<T, ExtensionTypePack>::value, loaded);
  }
  template <typename T, typename T2, typename... L>
  void extensionShutdown()
  {
    extensionShutdown<T2, L...>();
    extensionShutdown<T>();
  }
  template <typename T>
  void extensionShutdown()
  {
    if (extensionEnabled.test(TypeIndexOf<T, ExtensionTypePack>::value))
    {
      this->T::enumerateFunctions([](const char *, PFN_vkVoidFunction &function) { function = nullptr; });
      extensionEnabled.reset(TypeIndexOf<T, ExtensionTypePack>::value);
    }
  }

public:
  int getAPIVersion() { return activeApiVersion; }

  bool init(VulkanLoader &vk_loader, const VkInstanceCreateInfo &ici)
  {
    activeApiVersion = ici.pApplicationInfo->apiVersion;
    loader = &vk_loader;

    VkResult rc = vk_loader.vkCreateInstance(&ici, nullptr, ptr(instance));
    if (VULKAN_CHECK_FAIL(rc))
    {
      logwarn("vulkan: instance create failed with error %08lX:%s", rc, vulkan_error_string(rc));
      return false;
    }

#if !_TARGET_C3
#define VK_DEFINE_ENTRY_POINT(name)                                                                                     \
  *reinterpret_cast<PFN_vkVoidFunction *>(&name) = VULKAN_LOG_CALL_R(vk_loader.vkGetInstanceProcAddr(instance, #name)); \
  if (!name)                                                                                                            \
  {                                                                                                                     \
    logwarn("vulkan: no instance entrypoint for %s", #name);                                                            \
    shutdown();                                                                                                         \
    return false;                                                                                                       \
  }
    VK_INSTANCE_ENTRY_POINT_LIST
#undef VK_DEFINE_ENTRY_POINT
#endif

    extensionInit<Extensions...>(ici);
    return true;
  }
  void shutdown()
  {
    extensionShutdown<Extensions...>();

    VULKAN_LOG_CALL(vkDestroyInstance(instance, nullptr));
    instance = VulkanNullHandle();
    loader = nullptr;

#if !_TARGET_C3
#define VK_DEFINE_ENTRY_POINT(name) *reinterpret_cast<PFN_vkVoidFunction *>(&name) = nullptr;
    VK_INSTANCE_ENTRY_POINT_LIST
#undef VK_DEFINE_ENTRY_POINT
#endif
  }
  inline VulkanInstanceHandle get() const { return instance; }
  inline bool isValid() const { return !is_null(instance); }
  inline VulkanLoader &getLoader() { return *loader; }

  Tab<VulkanPhysicalDeviceHandle> getAllDevices()
  {
    Tab<VulkanPhysicalDeviceHandle> result;
    uint32_t count = 0;
    if (VULKAN_CHECK_OK(vkEnumeratePhysicalDevices(instance, &count, nullptr)))
    {
      result.resize(count);
      if (VULKAN_CHECK_FAIL(vkEnumeratePhysicalDevices(instance, &count, ary(result.data()))))
        clear_and_shrink(result);
    }
    return result;
  }
  Tab<VkQueueFamilyProperties> getDeviceQFamilyProperties(VulkanPhysicalDeviceHandle device)
  {
    Tab<VkQueueFamilyProperties> result;
    uint32_t count = 0;
    VULKAN_LOG_CALL(vkGetPhysicalDeviceQueueFamilyProperties(device, &count, nullptr));
    result.resize(count);
    VULKAN_LOG_CALL(vkGetPhysicalDeviceQueueFamilyProperties(device, &count, result.data()));
    return result;
  }

#define VK_DEFINE_ENTRY_POINT(name) VULKAN_MAKE_CORE_FUNCTION_POINTER_MEMBER(name);
  VK_INSTANCE_ENTRY_POINT_LIST
#undef VK_DEFINE_ENTRY_POINT

  template <typename T>
  void disableExtension()
  {
    if (hasExtension<T>())
      extensionShutdown<T>();
  }
  template <typename T>
  bool hasExtension() const
  {
    return extensionEnabled.test(TypeIndexOf<T, ExtensionTypePack>::value);
  }
  static constexpr size_t ExtensionCount = sizeof...(Extensions);
  static const char *ExtensionNames[ExtensionCount];
};

template <typename... E>
const char *VulkanInstanceCore<E...>::ExtensionNames[VulkanInstanceCore<E...>::ExtensionCount] = //
  {extension_name<E>()...};

class VulkanInstance : public VulkanInstanceCore<SurfaceKHR
#if VK_KHR_display
                         ,
                         DisplayKHR
#endif
#if VK_EXT_debug_report
                         ,
                         DebugReport
#endif
#if VK_KHR_win32_surface
                         ,
                         SurfaceWin32KHR
#endif
#if VK_KHR_android_surface
                         ,
                         SurfaceAndroidKHR
#endif
#if VK_KHR_mir_surface
                         ,
                         SurfaceMirKHR
#endif
#if VK_KHR_wayland_surface
                         ,
                         SurfaceWaylandKHR
#endif
#if VK_KHR_xcb_surface
                         ,
                         SurfaceXcbKHR
#endif
#if VK_KHR_xlib_surface
                         ,
                         SurfaceXlibKHR
#endif
#if VK_NN_vi_surface
                         ,
                         SurfaceViNN
#endif
#if VK_KHR_get_physical_device_properties2
                         ,
                         GetPhysicalDeviceProperties2KHR
#endif
#if VK_MVK_ios_surface
                         ,
                         SurfaceIosMVK
#endif
#if VK_MVK_macos_surface
                         ,
                         SurfaceMacosMVK
#endif
#if VK_EXT_debug_utils
                         ,
                         DebugUtilsEXT
#endif
#if VK_GOOGLE_surfaceless_query
                         ,
                         SurfacelessQueryGOOGLE
#endif
#if VK_KHR_get_surface_capabilities2
                         ,
                         GetSurfaceCapabilities2KHR
#endif
                         >
{
public:
  VulkanInstance(const VulkanInstance &) = delete;
  VulkanInstance &operator=(const VulkanInstance &) = delete;

  VulkanInstance() = default;
};

// checks if all required extensions are in the given list and returns true if they are all
// included.
bool has_all_required_instance_extension(dag::ConstSpan<const char *> ext_list, void (*clb)(const char *name));
// removes mutual exclusive extensions from the list and returns the new list length
int remove_mutual_exclusive_instance_extensions(dag::Span<const char *> ext_list);
// removes all debug extensions from the list and returns the new list length
int remove_debug_instance_extensions(dag::Span<const char *> ext_list);
// removes the given extension from the list and returns the new list length
int filter_instance_extension(dag::Span<const char *> ext_list, const char *filter);

bool instance_has_all_required_extensions(VulkanInstance &instance, void (*clb)(const char *name));
Tab<const char *> build_instance_extension_list(VulkanLoader &loader, const DataBlock *extension_list_names, int debug_level,
  Driver3dInitCallback *cb);
eastl::vector<VkLayerProperties> build_instance_layer_list(VulkanLoader &loader, const DataBlock *layer_list, int debug_level,
  bool enable_render_doc_layer);
#if VK_EXT_debug_report
VulkanDebugReportCallbackEXTHandle create_debug_sink(VulkanInstance &instance, int debug_level, PFN_vkDebugReportCallbackEXT clb,
  void *data);
#endif
#if VK_EXT_debug_utils
VulkanDebugUtilsMessengerEXTHandle create_debug_sink_2(VulkanInstance &instance, int debug_level,
  PFN_vkDebugUtilsMessengerCallbackEXT clb, void *data);
#endif
VkFormat get_default_depth_stencil_format(VulkanInstance &instance, VulkanPhysicalDeviceHandle device);
} // namespace drv3d_vulkan