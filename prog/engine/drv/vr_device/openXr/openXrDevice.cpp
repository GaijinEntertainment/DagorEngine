#include "openXrDevice.h"

#include <3d/dag_drv3d.h>
#include <3d/dag_tex3d.h>
#include <3d/dag_drv3dCmd.h>
#include <debug/dag_log.h>
#include <openxr/openxr.h>
#include <util/dag_string.h>
#include <EASTL/string.h>
#include <3d/dag_drvDecl.h>
#include <osApiWrappers/dag_miscApi.h>
#include <shaders/dag_shaders.h>
#include <shaders/dag_shaderVar.h>
#include <shaders/dag_postFxRenderer.h>
#include <perfMon/dag_statDrv.h>
#include <perfMon/dag_cpuFreq.h>
#include <util/dag_convar.h>
#include <EASTL/algorithm.h>
#include <EASTL/shared_ptr.h>
#include <thread>

#include "openXrMath.h"

#include "openXrErrorReporting.h"
#define OXR_INSTANCE        oxrInstance
#define OXR_MODULE          "device"
#define OXR_RUNTIME_FAILURE &oxrHadRuntimeFailure

CONSOLE_BOOL_VAL("xr", enable_depth, true);
CONSOLE_BOOL_VAL("xr", enable_screen_mask, true);

bool oxrHadRuntimeFailure = false;

static constexpr VRDevice::RenderingAPI supported_rendering_APIs[] = {
#if _TARGET_PC_WIN
  VRDevice::RenderingAPI::D3D11,
  VRDevice::RenderingAPI::D3D12,
  VRDevice::RenderingAPI::Vulkan,
#elif _TARGET_ANDROID
  VRDevice::RenderingAPI::Vulkan,
#endif // PLATFORM
};

PFN_xrGetVisibilityMaskKHR pfnGetVisibilityMaskKHR = nullptr;
PFN_xrEnumerateDisplayRefreshRatesFB pfnEnumerateDisplayRefreshRatesFB = nullptr;
PFN_xrGetDisplayRefreshRateFB pfnGetDisplayRefreshRateFB = nullptr;
PFN_xrRequestDisplayRefreshRateFB pfnRequestDisplayRefreshRateFB = nullptr;

#if _TARGET_PC_WIN
PFN_xrConvertWin32PerformanceCounterToTimeKHR pfnConvertWin32PerformanceCounterToTimeKHR = nullptr;
#endif // _TARGET_PC_WIN

#ifdef XR_USE_GRAPHICS_API_D3D11
PFN_xrGetD3D11GraphicsRequirementsKHR pfnGetD3D11GraphicsRequirementsKHR = nullptr;
#endif // XR_USE_GRAPHICS_API_D3D11

#ifdef XR_USE_GRAPHICS_API_D3D12
PFN_xrGetD3D12GraphicsRequirementsKHR pfnGetD3D12GraphicsRequirementsKHR = nullptr;
#endif // XR_USE_GRAPHICS_API_D3D12

#ifdef XR_USE_GRAPHICS_API_VULKAN
PFN_xrGetVulkanGraphicsRequirementsKHR pfnGetVulkanGraphicsRequirementsKHR = nullptr;
PFN_xrGetVulkanDeviceExtensionsKHR pfnGetVulkanDeviceExtensionsKHR = nullptr;
PFN_xrGetVulkanInstanceExtensionsKHR pfnGetVulkanInstanceExtensionsKHR = nullptr;
PFN_xrGetVulkanGraphicsDeviceKHR pfnGetVulkanGraphicsDeviceKHR = nullptr;
#endif // XR_USE_GRAPHICS_API_VULKAN

PFN_xrCreateDebugUtilsMessengerEXT pfnCreateDebugUtilsMessengerEXT = nullptr;
PFN_xrDestroyDebugUtilsMessengerEXT pfnDestroyDebugUtilsMessengerEXT = nullptr;

XrBool32 XRAPI_PTR debug_callback(XrDebugUtilsMessageSeverityFlagsEXT severity, XrDebugUtilsMessageTypeFlagsEXT types,
  const XrDebugUtilsMessengerCallbackDataEXT *message, void *user_data)
{
  G_UNUSED(user_data);

  int level = 0;
  switch (severity)
  {
    case XR_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT: level = LOGLEVEL_REMARK; break;
    case XR_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT: level = LOGLEVEL_DEBUG; break;
    case XR_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT: level = LOGLEVEL_WARN; break;
    case XR_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT: level = LOGLEVEL_ERR; break;
  }

  char typeStr[128] = {0};
  if (types & XR_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT)
    strcat(typeStr, "General");
  if (types & XR_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT)
  {
    if (typeStr[0] != 0)
      strcat(typeStr, "|");
    strcat(typeStr, "Validation");
  }
  if (types & XR_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT)
  {
    if (typeStr[0] != 0)
      strcat(typeStr, "|");
    strcat(typeStr, "Performance");
  }
  if (types & XR_DEBUG_UTILS_MESSAGE_TYPE_CONFORMANCE_BIT_EXT)
  {
    if (typeStr[0] != 0)
      strcat(typeStr, "|");
    strcat(typeStr, "Conformance");
  }

  logmessage(level, "[XR][device] %s message from '%s' function: %s", typeStr, message->functionName, message->message);

  return XR_FALSE;
}

struct AutoExternalAccess
{
  AutoExternalAccess(bool own_device, bool own_context, VRDevice::RenderingAPI api) :
    device(own_device ? 1 : 0), context(own_context ? 1 : 0), api(api)
  {
    if (d3d::is_inited() && api == VRDevice::RenderingAPI::D3D11)
    {
      d3d::driver_command(DRV3D_COMMAND_BEGIN_EXTERNAL_ACCESS, &device, &context, nullptr);
      d3d::driver_command(DRV3D_COMMAND_ACQUIRE_OWNERSHIP, nullptr, nullptr, nullptr);
    }
  }

  ~AutoExternalAccess()
  {
    if (d3d::is_inited() && api == VRDevice::RenderingAPI::D3D11)
    {
      d3d::driver_command(DRV3D_COMMAND_END_EXTERNAL_ACCESS, &device, &context, nullptr);
      d3d::driver_command(DRV3D_COMMAND_RELEASE_OWNERSHIP, nullptr, nullptr, nullptr);
    }
  }

  int device;
  int context;
  VRDevice::RenderingAPI api;
};

static eastl::pair<int64_t, int> get_best_swapchain_format(const eastl::vector<int64_t> &swapchainFormats, bool depth,
  VRDevice::RenderingAPI api)
{
  G_ASSERT(!swapchainFormats.empty());
  if (swapchainFormats.empty())
    return {0, 0};

  if (depth)
  {
    float preferredDepth = ::dgs_get_settings()->getBlockByNameEx("xr")->getInt("preferredDepth", 0);

    logdbg("[XR][device] Selecting format for depth. Preferred bit count is %d", preferredDepth);

    // Find the first, less or equal to 32 bit depth format
    switch (api)
    {
#if defined XR_USE_GRAPHICS_API_D3D11 || defined XR_USE_GRAPHICS_API_D3D12
      case VRDevice::RenderingAPI::D3D11:
      case VRDevice::RenderingAPI::D3D12:
        for (auto format : swapchainFormats)
        {
          switch (format)
          {
            case DXGI_FORMAT_D24_UNORM_S8_UINT:
              if (preferredDepth == 24 || preferredDepth == 0)
              {
                logdbg("[XR][device] Selected format for depth is DXGI_FORMAT_D24_UNORM_S8_UINT");
                return {format, TEXFMT_DEPTH24};
              }
              break;
            case DXGI_FORMAT_D16_UNORM:
              if (preferredDepth == 16 || preferredDepth == 0)
              {
                logdbg("[XR][device] Selected format for depth is DXGI_FORMAT_D16_UNORM");
                return {format, TEXFMT_DEPTH16};
              }
              break;
            case DXGI_FORMAT_D32_FLOAT:
              if (preferredDepth == 32 || preferredDepth == 0)
              {
                logdbg("[XR][device] Selected format for depth is DXGI_FORMAT_D32_FLOAT");
                return {format, TEXFMT_DEPTH32};
              }
              break;
          }
        }
        logdbg("[XR][device] No suitable depth texture format was found for Dagor.");
        return {0, 0};
#endif // XR_USE_GRAPHICS_API_D3D11 || XR_USE_GRAPHICS_API_D3D12

#ifdef XR_USE_GRAPHICS_API_VULKAN
      case VRDevice::RenderingAPI::Vulkan:
        for (auto format : swapchainFormats)
        {
          switch (format)
          {
            case VK_FORMAT_D24_UNORM_S8_UINT:
              if (preferredDepth == 24 || preferredDepth == 0)
              {
                logdbg("[XR][device] Selected format for depth is VK_FORMAT_D24_UNORM_S8_UINT");
                return {format, TEXFMT_DEPTH24};
              }
              break;
            case VK_FORMAT_D16_UNORM:
              if (preferredDepth == 16 || preferredDepth == 0)
              {
                logdbg("[XR][device] Selected format for depth is VK_FORMAT_D16_UNORM");
                return {format, TEXFMT_DEPTH16};
              }
              break;
            case VK_FORMAT_D32_SFLOAT:
              if (preferredDepth == 32 || preferredDepth == 0)
              {
                logdbg("[XR][device] Selected format for depth is VK_FORMAT_D32_SFLOAT");
                return {format, TEXFMT_DEPTH32};
              }
              break;
          }
        }
        logdbg("[XR][device] No suitable depth texture format was found for Dagor.");
        return {0, 0};
#endif // XR_USE_GRAPHICS_API_VULKAN

      default: G_ASSERTF(false, "Implement swap chain format for this renderer!"); return {0, 0};
    }
  }
  else
  {
    // Find the first, 32 bit integer based sRGB format
    switch (api)
    {
#if defined XR_USE_GRAPHICS_API_D3D11 || defined XR_USE_GRAPHICS_API_D3D12
      case VRDevice::RenderingAPI::D3D11:
      case VRDevice::RenderingAPI::D3D12:
        for (auto format : swapchainFormats)
        {
          switch (format)
          {
            case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB: return {format, TEXFMT_R8G8B8A8};
            case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB: return {format, TEXFMT_A8R8G8B8};
          }
        }
        G_ASSERTF(false, "No suitable color texture format was found for Dagor.");
        return {0, 0};
#endif // XR_USE_GRAPHICS_API_D3D11 || XR_USE_GRAPHICS_API_D3D12

#ifdef XR_USE_GRAPHICS_API_VULKAN
      case VRDevice::RenderingAPI::Vulkan:
        for (auto format : swapchainFormats)
        {
          switch (format)
          {
            case VK_FORMAT_R8G8B8A8_SRGB: return {format, TEXFMT_R8G8B8A8};
            case VK_FORMAT_B8G8R8A8_SRGB: return {format, TEXFMT_A8R8G8B8};
          }
        }
        G_ASSERTF(false, "No suitable color texture format was found for Dagor.");
        return {0, 0};
#endif // XR_USE_GRAPHICS_API_VULKAN

      default: G_ASSERTF(false, "Implement swap chain format for this renderer!"); return {0, 0};
    }
  }
}

static XrTime get_current_xr_time(XrInstance oxrInstance)
{
  XrTime currentXRTime = -1;
#if _TARGET_PC_WIN
  LARGE_INTEGER qpc;
  QueryPerformanceCounter(&qpc);
  XR_REPORT(pfnConvertWin32PerformanceCounterToTimeKHR(oxrInstance, &qpc, &currentXRTime));
#elif _TARGET_ANDROID
#endif // PLATFORM
  return currentXRTime;
}


VRDevice *create_vr_device(VRDevice::RenderingAPI api, const VRDevice::ApplicationData &app_data)
{
  return new OpenXRDevice(api, app_data);
}

OpenXRDevice::OpenXRDevice(RenderingAPI rendering_api, const ApplicationData &application_data) :
  renderingAPI(rendering_api), oxrSessionState(XR_SESSION_STATE_UNKNOWN)
{
  d3d::driver_command(DRV3D_COMMAND_REGISTER_DEVICE_RESET_EVENT_HANDLER, static_cast<DeviceResetEventHandler *>(this), nullptr,
    nullptr);

#if _TARGET_ANDROID
  LoadPreInitOpenxrFuncs();
  android_app *state = (android_app *)win32_get_instance();
  JavaVM *jVM = state->activity->vm;
  jobject jClazz = state->activity->clazz;

  XrLoaderInitInfoAndroidKHR loaderInitInfoAndroid{XR_TYPE_LOADER_INIT_INFO_ANDROID_KHR, NULL, (void *)jVM, (void *)jClazz};

  xrInitializeLoaderKHR((XrLoaderInitInfoBaseHeaderKHR *)&loaderInitInfoAndroid);
#endif // _TARGET_ANDROID

  if (renderingAPI == RenderingAPI::Default)
  {
    d3d::get_driver_code()
      .map(d3d::windows && d3d::dx11, RenderingAPI::D3D11)
      .map(d3d::windows && d3d::dx12, RenderingAPI::D3D12)
      .map((d3d::windows || d3d::android) && d3d::vulkan, RenderingAPI::Vulkan)
      .get(renderingAPI);
  }

  G_ASSERTF_RETURN(renderingAPI != RenderingAPI::Default, , "Default rendering API is not specified for OpenXR on this platform!");
  G_ASSERTF_RETURN(eastl::find(eastl::begin(supported_rendering_APIs), eastl::end(supported_rendering_APIs), renderingAPI) !=
                     eastl::end(supported_rendering_APIs),
    , "The selected rendering api (%d) is not supported on this platform!", int(renderingAPI));

  uint32_t extensionCount = 0;
  if (!RUNTIME_CHECK(xrEnumerateInstanceExtensionProperties(nullptr, 0, &extensionCount, nullptr)))
    return;

  supportedExtensions.resize(extensionCount, {XR_TYPE_EXTENSION_PROPERTIES});

  if (!RUNTIME_CHECK(xrEnumerateInstanceExtensionProperties(nullptr, extensionCount, &extensionCount, supportedExtensions.data())))
    return;

  logdbg("[XR][device] OpenXR initialized supporting %u extensions", unsigned(supportedExtensions.size()));

  for (XrExtensionProperties &extension : supportedExtensions)
    logdbg("[XR][device]  %s v%u", extension.extensionName, extension.extensionVersion);

  switch (renderingAPI)
  {
#ifdef XR_USE_GRAPHICS_API_D3D11
    case RenderingAPI::D3D11:
      G_ASSERTF_RETURN(isExtensionSupported(XR_KHR_D3D11_ENABLE_EXTENSION_NAME), , "D3D11 rendering is not supported by OpenXR!");
      requiredExtensions.emplace_back(XR_KHR_D3D11_ENABLE_EXTENSION_NAME);
      break;
#endif // XR_USE_GRAPHICS_API_D3D11
#ifdef XR_USE_GRAPHICS_API_D3D12
    case RenderingAPI::D3D12:
      G_ASSERTF_RETURN(isExtensionSupported(XR_KHR_D3D12_ENABLE_EXTENSION_NAME), , "D3D12 rendering is not supported by OpenXR!");
      requiredExtensions.emplace_back(XR_KHR_D3D12_ENABLE_EXTENSION_NAME);
      break;
#endif // XR_USE_GRAPHICS_API_D3D11
#ifdef XR_USE_GRAPHICS_API_VULKAN
    case RenderingAPI::Vulkan:
      G_ASSERTF_RETURN(isExtensionSupported(XR_KHR_VULKAN_ENABLE_EXTENSION_NAME), , "Vulkan rendering is not supported by OpenXR!");
      requiredExtensions.emplace_back(XR_KHR_VULKAN_ENABLE_EXTENSION_NAME);
      break;
#endif // XR_USE_GRAPHICS_API_D3D11
    default: G_ASSERTF_RETURN(false, , "Implement required extensions for this renderer!"); break;
  }

#if _TARGET_PC_WIN
  if (isExtensionSupported(XR_KHR_WIN32_CONVERT_PERFORMANCE_COUNTER_TIME_EXTENSION_NAME))
    requiredExtensions.emplace_back(XR_KHR_WIN32_CONVERT_PERFORMANCE_COUNTER_TIME_EXTENSION_NAME);
#endif // _TARGET_PC_WIN

  if (isExtensionSupported(XR_EXT_DEBUG_UTILS_EXTENSION_NAME))
    requiredExtensions.emplace_back(XR_EXT_DEBUG_UTILS_EXTENSION_NAME);

  if (isExtensionSupported(XR_FB_DISPLAY_REFRESH_RATE_EXTENSION_NAME))
  {
    requiredExtensions.emplace_back(XR_FB_DISPLAY_REFRESH_RATE_EXTENSION_NAME);
    supportsChangingRefreshRate = true;
  }

#if _TARGET_ANDROID
  if (isExtensionSupported("XR_KHR_android_create_instance"))
    requiredExtensions.emplace_back("XR_KHR_android_create_instance");
#endif // _TARGET_ANDROID

  if (isExtensionSupported(XR_KHR_COMPOSITION_LAYER_DEPTH_EXTENSION_NAME))
    requiredExtensions.emplace_back(XR_KHR_COMPOSITION_LAYER_DEPTH_EXTENSION_NAME);

  if (isExtensionSupported(XR_KHR_VISIBILITY_MASK_EXTENSION_NAME))
    requiredExtensions.emplace_back(XR_KHR_VISIBILITY_MASK_EXTENSION_NAME);

  if (isExtensionSupported(XR_EXT_HAND_TRACKING_EXTENSION_NAME))
    requiredExtensions.emplace_back(XR_EXT_HAND_TRACKING_EXTENSION_NAME);

  applicationName = application_data.name;
  applicationVersion = application_data.version;

  setUpInstance();
}

OpenXRDevice::~OpenXRDevice()
{
  d3d::driver_command(DRV3D_COMMAND_UNREGISTER_DEVICE_RESET_EVENT_HANDLER, static_cast<DeviceResetEventHandler *>(this), nullptr,
    nullptr);

  tearDownSwapchainImages();
  tearDownSwapchains();
  tearDownSession();

  tearDownScreenMaskResources();

  if (oxrDebug != XR_NULL_HANDLE)
    pfnDestroyDebugUtilsMessengerEXT(oxrDebug);

  tearDownInstance();
}

bool OpenXRDevice::setUpInstance()
{
  XrInstanceCreateInfo createInfo = {XR_TYPE_INSTANCE_CREATE_INFO};
  createInfo.enabledExtensionCount = requiredExtensions.size();
  createInfo.enabledExtensionNames = requiredExtensions.data();
  createInfo.applicationInfo.apiVersion = XR_CURRENT_API_VERSION;
  createInfo.applicationInfo.applicationVersion = applicationVersion;
  createInfo.applicationInfo.engineVersion = 4;
  strcpy_s(createInfo.applicationInfo.applicationName, applicationName.data());
  strcpy_s(createInfo.applicationInfo.engineName, "Dagor");

#if _TARGET_ANDROID
  android_app *state = (android_app *)win32_get_instance();
  JavaVM *jVM = state->activity->vm;
  jobject jClazz = state->activity->clazz;

  XrInstanceCreateInfoAndroidKHR createInfoAndroid = {XR_TYPE_INSTANCE_CREATE_INFO_ANDROID_KHR};
  createInfoAndroid.applicationVM = (void *)jVM;
  createInfoAndroid.applicationActivity = (void *)jClazz;

  createInfo.next = (XrBaseInStructure *)(&createInfoAndroid);
#endif // _TARGET_ANDROID

  if (!RUNTIME_CHECK(xrCreateInstance(&createInfo, &oxrInstance)))
    return false;

#if _TARGET_ANDROID
  LoadPostInitOpenxrFuncs(oxrInstance);
#endif // _TARGET_ANDROID

  XrInstanceProperties instanceProperties = {XR_TYPE_INSTANCE_PROPERTIES};
  XR_REPORT_RETURN(xrGetInstanceProperties(oxrInstance, &instanceProperties), false);
  logdbg("[XR][device] Runtime initialized: %s (%d.%d.%d)", instanceProperties.runtimeName,
    XR_VERSION_MAJOR(instanceProperties.runtimeVersion), XR_VERSION_MINOR(instanceProperties.runtimeVersion),
    XR_VERSION_PATCH(instanceProperties.runtimeVersion));

  XrSystemGetInfo systemInfo = {XR_TYPE_SYSTEM_GET_INFO};
  systemInfo.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;

  XrResult systemResult = xrGetSystem(oxrInstance, &systemInfo, &oxrSystemId);
  if (XR_FAILED(systemResult))
  {
    if (systemResult == XR_ERROR_FORM_FACTOR_UNAVAILABLE)
      G_ASSERTF(false, "No HMD is connected! Starting without VR.");
    else
      report_xr_result(OXR_INSTANCE, systemResult, "xrGetSystem(oxrInstance, &systemInfo, &oxrSystemId)", OXR_MODULE,
        &oxrHadRuntimeFailure);
    return false;
  }

  XrSystemHandTrackingPropertiesEXT handTrackingProperties = {XR_TYPE_SYSTEM_HAND_TRACKING_PROPERTIES_EXT};
  XrSystemProperties systemProperties = {XR_TYPE_SYSTEM_PROPERTIES, &handTrackingProperties};
  XR_REPORT_RETURN(xrGetSystemProperties(oxrInstance, oxrSystemId, &systemProperties), false);
  logdbg("[XR][device] System info. VendorId %d, SystemName: '%s'", systemProperties.vendorId, systemProperties.systemName);


  enableHandTracking =
    handTrackingProperties.supportsHandTracking && ::dgs_get_settings()->getBlockByNameEx("xr")->getBool("enableHandTracking", false);
  logdbg("[XR][device] System info. Hand tracking: %s", enableHandTracking ? "yes" : "no");

  bool disableDepthByConfig = false;
  auto depthBlacklist = ::dgs_get_settings()->getBlockByNameEx("xr")->getBlockByNameEx("depthBlacklist");
  for (int paramIx = 0; paramIx < depthBlacklist->paramCount() && !disableDepthByConfig; ++paramIx)
    if (stricmp(depthBlacklist->getParamName(paramIx), "vendorid") == 0)
      if (depthBlacklist->getInt(paramIx) == systemProperties.vendorId)
        disableDepthByConfig = true;

  if (disableDepthByConfig || !enable_depth)
    enableDepthLayerExtension = false;

  uint32_t blendCount = 0;
  XR_REPORT_RETURN(
    xrEnumerateEnvironmentBlendModes(oxrInstance, oxrSystemId, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO, 0, &blendCount, nullptr),
    false);

  supportedBlendModes.resize(blendCount);
  XR_REPORT_RETURN(xrEnumerateEnvironmentBlendModes(oxrInstance, oxrSystemId, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO,
                     supportedBlendModes.size(), &blendCount, supportedBlendModes.data()),
    false);

  G_ASSERTF_RETURN(
    eastl::find(supportedBlendModes.begin(), supportedBlendModes.end(), XR_ENVIRONMENT_BLEND_MODE_OPAQUE) != supportedBlendModes.end(),
    false, "Opaque blend mode not supported for this HMD.");

  if (hasScreenMask())
    XR_REPORT_RETURN(xrGetInstanceProcAddr(oxrInstance, "xrGetVisibilityMaskKHR", (PFN_xrVoidFunction *)(&pfnGetVisibilityMaskKHR)),
      false);


#if _TARGET_PC_WIN
  XR_REPORT_RETURN(xrGetInstanceProcAddr(oxrInstance, "xrConvertWin32PerformanceCounterToTimeKHR",
                     (PFN_xrVoidFunction *)(&pfnConvertWin32PerformanceCounterToTimeKHR)),
    false);
#endif // XR_USE_GRAPHICS_API_D3D11

  switch (renderingAPI)
  {
#ifdef XR_USE_GRAPHICS_API_D3D11
    case RenderingAPI::D3D11:
      XR_REPORT_RETURN(xrGetInstanceProcAddr(oxrInstance, "xrGetD3D11GraphicsRequirementsKHR",
                         (PFN_xrVoidFunction *)(&pfnGetD3D11GraphicsRequirementsKHR)),
        false);
      break;
#endif // XR_USE_GRAPHICS_API_D3D11
#ifdef XR_USE_GRAPHICS_API_D3D12
    case RenderingAPI::D3D12:
      XR_REPORT_RETURN(xrGetInstanceProcAddr(oxrInstance, "xrGetD3D12GraphicsRequirementsKHR",
                         (PFN_xrVoidFunction *)(&pfnGetD3D12GraphicsRequirementsKHR)),
        false);
      break;
#endif // XR_USE_GRAPHICS_API_D3D12
#ifdef XR_USE_GRAPHICS_API_VULKAN
    case RenderingAPI::Vulkan:
    {
      XR_REPORT_RETURN(xrGetInstanceProcAddr(oxrInstance, "xrGetVulkanGraphicsRequirementsKHR",
                         (PFN_xrVoidFunction *)(&pfnGetVulkanGraphicsRequirementsKHR)),
        false);
      XR_REPORT_RETURN(
        xrGetInstanceProcAddr(oxrInstance, "xrGetVulkanDeviceExtensionsKHR", (PFN_xrVoidFunction *)(&pfnGetVulkanDeviceExtensionsKHR)),
        false);
      XR_REPORT_RETURN(xrGetInstanceProcAddr(oxrInstance, "xrGetVulkanInstanceExtensionsKHR",
                         (PFN_xrVoidFunction *)(&pfnGetVulkanInstanceExtensionsKHR)),
        false);
      XR_REPORT_RETURN(
        xrGetInstanceProcAddr(oxrInstance, "xrGetVulkanGraphicsDeviceKHR", (PFN_xrVoidFunction *)(&pfnGetVulkanGraphicsDeviceKHR)),
        false);

      uint32_t reqExtCount = 0;
      XR_REPORT_RETURN(pfnGetVulkanDeviceExtensionsKHR(oxrInstance, oxrSystemId, 0, &reqExtCount, nullptr), false);

      oxrRequredDeviceExtensions.resize(reqExtCount + 1);
      XR_REPORT_RETURN(pfnGetVulkanDeviceExtensionsKHR(oxrInstance, oxrSystemId, oxrRequredDeviceExtensions.size(), &reqExtCount,
                         &oxrRequredDeviceExtensions[0]),
        false);

      // SteamVR requies VK_EXT_debug_marker which is a problem as it is only
      // available when the Vulkan SDK is installed and the validation layer
      // is enabled. This is not the case for the players. It turns out that
      // SteamVR works just fine without this extension, but we can't be sure
      // how stable this workaround is.
      // This will be needed to be deleted when this is solved.
      // https://steamcommunity.com/app/250820/discussions/8/2950376208547832257/
      static const char ext_VK_EXT_debug_marker[] = "VK_EXT_debug_marker";
      auto remove = oxrRequredDeviceExtensions.find(ext_VK_EXT_debug_marker);
      if (remove != eastl::string::npos)
        oxrRequredDeviceExtensions.erase(remove, sizeof(ext_VK_EXT_debug_marker));

      XR_REPORT_RETURN(pfnGetVulkanInstanceExtensionsKHR(oxrInstance, oxrSystemId, 0, &reqExtCount, nullptr), false);

      oxrRequredInstanceExtensions.resize(reqExtCount + 1);
      XR_REPORT_RETURN(pfnGetVulkanInstanceExtensionsKHR(oxrInstance, oxrSystemId, oxrRequredInstanceExtensions.size(), &reqExtCount,
                         &oxrRequredInstanceExtensions[0]),
        false);
      break;
    }
#endif // XR_USE_GRAPHICS_API_VULKAN
    default: G_ASSERTF_RETURN(false, false, "Implement getting GraphicsRequirements function for this renderer!"); break;
  }

  XR_REPORT_RETURN(
    xrGetInstanceProcAddr(oxrInstance, "xrCreateDebugUtilsMessengerEXT", (PFN_xrVoidFunction *)(&pfnCreateDebugUtilsMessengerEXT)),
    false);
  XR_REPORT_RETURN(
    xrGetInstanceProcAddr(oxrInstance, "xrDestroyDebugUtilsMessengerEXT", (PFN_xrVoidFunction *)(&pfnDestroyDebugUtilsMessengerEXT)),
    false);

  XrDebugUtilsMessengerCreateInfoEXT debug_info = {XR_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};
  debug_info.messageTypes = XR_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | XR_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                            XR_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT | XR_DEBUG_UTILS_MESSAGE_TYPE_CONFORMANCE_BIT_EXT;
  debug_info.messageSeverities = XR_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | XR_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
                                 XR_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | XR_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
  debug_info.userCallback = debug_callback;

  if (pfnCreateDebugUtilsMessengerEXT)
    pfnCreateDebugUtilsMessengerEXT(oxrInstance, &debug_info, &oxrDebug);

  uint32_t viewCount = 0;
  XR_REPORT_RETURN(
    xrEnumerateViewConfigurationViews(oxrInstance, oxrSystemId, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO, 0, &viewCount, nullptr),
    false);

  G_ASSERTF_RETURN(viewCount == 2, false, "VR device has %u views instead of 2!", viewCount);

  XR_REPORT_RETURN(
    xrEnumerateViewConfigurationViews(oxrInstance, oxrSystemId, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO, 2, &viewCount, configViews),
    false);

  initialized = true;

  return true;
}

void OpenXRDevice::tearDownInstance()
{
  if (oxrInstance != XR_NULL_HANDLE)
  {
    XR_REPORT(xrDestroyInstance(oxrInstance));
    oxrInstance = XR_NULL_HANDLE;
    gfxApiRangeCalled = false;
  }
}

bool OpenXRDevice::setUpSession()
{
  void *device = d3d::get_device();
  G_ASSERT_RETURN(device, false);

  AutoExternalAccess gpuLock(true, true, renderingAPI);

#ifdef XR_USE_GRAPHICS_API_D3D11
  XrGraphicsBindingD3D11KHR d3d11Binding = {XR_TYPE_GRAPHICS_BINDING_D3D11_KHR};
#endif // XR_USE_GRAPHICS_API_D3D11

#ifdef XR_USE_GRAPHICS_API_D3D12
  XrGraphicsBindingD3D12KHR d3d12Binding = {XR_TYPE_GRAPHICS_BINDING_D3D12_KHR};
#endif // XR_USE_GRAPHICS_API_D3D12

#ifdef XR_USE_GRAPHICS_API_VULKAN
  XrGraphicsBindingVulkanKHR vulkanBinding = {XR_TYPE_GRAPHICS_BINDING_VULKAN_KHR};
#endif // XR_USE_GRAPHICS_API_VULKAN

  XrSessionCreateInfo sessionInfo = {XR_TYPE_SESSION_CREATE_INFO};

  switch (renderingAPI)
  {
#ifdef XR_USE_GRAPHICS_API_D3D11
    case RenderingAPI::D3D11:
      d3d11Binding.device = static_cast<ID3D11Device *>(device);
      sessionInfo.next = &d3d11Binding;
      break;
#endif // XR_USE_GRAPHICS_API_D3D11
#ifdef XR_USE_GRAPHICS_API_D3D12
    case RenderingAPI::D3D12:
      d3d::driver_command(DRV3D_COMMAND_GET_RENDERING_COMMAND_QUEUE, (void *)&d3d12Binding.queue, nullptr, nullptr);
      d3d12Binding.device = static_cast<ID3D12Device *>(device);
      sessionInfo.next = &d3d12Binding;
      break;
#endif // XR_USE_GRAPHICS_API_D3D12
#ifdef XR_USE_GRAPHICS_API_VULKAN
    case RenderingAPI::Vulkan:
    {
      vulkanBinding.device = static_cast<VkDevice>(device);
      d3d::driver_command(DRV3D_COMMAND_GET_INSTANCE, (void *)&vulkanBinding.instance, nullptr, nullptr);
      d3d::driver_command(DRV3D_COMMAND_GET_PHYSICAL_DEVICE, (void *)&vulkanBinding.physicalDevice, nullptr, nullptr);
      d3d::driver_command(DRV3D_COMMAND_GET_QUEUE_FAMILY_INDEX, (void *)&vulkanBinding.queueFamilyIndex, nullptr, nullptr);
      d3d::driver_command(DRV3D_COMMAND_GET_QUEUE_INDEX, (void *)&vulkanBinding.queueIndex, nullptr, nullptr);
      sessionInfo.next = &vulkanBinding;

      XR_REPORT_RETURN(pfnGetVulkanGraphicsDeviceKHR(oxrInstance, oxrSystemId, vulkanBinding.instance, &vulkanBinding.physicalDevice),
        false);

      break;
    }
#endif // XR_USE_GRAPHICS_API_VULKAN
    default: G_ASSERTF_RETURN(false, false, "Implement setRenderingDeviceImpl for this renderer!"); break;
  }

  if (!gfxApiRangeCalled)
    getRequiredGraphicsAPIRange();

  XrResult result;

  sessionInfo.systemId = oxrSystemId;
  XR_REPORT_RETURN(xrCreateSession(oxrInstance, &sessionInfo, &oxrSession), false);

  swapchainFormats.clear();
  uint32_t formatCount = 0;
  XR_REPORT_RETURN(xrEnumerateSwapchainFormats(oxrSession, 0, &formatCount, nullptr), false);

  swapchainFormats.resize(formatCount);
  XR_REPORT_RETURN(xrEnumerateSwapchainFormats(oxrSession, formatCount, &formatCount, swapchainFormats.data()), false);

  eastl::vector<float> refreshRates = getSupportedRefreshRates();
  if (!refreshRates.empty())
  {
    eastl::string ratesString;
    for (auto rate : refreshRates)
      ratesString += eastl::to_string(rate) + " ";
    debug("[XR][device] Supported refresh rates on this HMD are: %s", ratesString.data());
    debug("[XR][device] Running at %f Hz.", getRefreshRate());
  }
  else
    debug("[XR][device] Changing refresh rate is not supported for this HMD.");

  inputHandler.init(oxrInstance, oxrSession, enableHandTracking);

  return true;
}

void OpenXRDevice::tearDownSession()
{
  if (!oxrSession)
    return;

  inputHandler.shutdown();

  xrRequestExitSession(oxrSession); // it is OK for this to return errors
  xrEndSession(oxrSession);         // it is OK for this to return errors
  if (oxrRefSpace)
  {
    XR_REPORT(xrDestroySpace(oxrRefSpace));
    oxrRefSpace = XR_NULL_HANDLE;
  }

  if (oxrAppSpace)
  {
    XR_REPORT(xrDestroySpace(oxrAppSpace));
    oxrAppSpace = XR_NULL_HANDLE;
  }

  XR_REPORT(xrDestroySession(oxrSession));
  oxrSession = XR_NULL_HANDLE;

  sessionRunning = false;
}

bool OpenXRDevice::setUpSwapchains()
{
  switch (stereoMode)
  {
    case StereoMode::Multipass:
      return setUpSwapchain(swapchains[0], configViews[0].recommendedImageRectWidth, configViews[0].recommendedImageRectHeight) &&
             setUpSwapchain(swapchains[1], configViews[1].recommendedImageRectWidth, configViews[1].recommendedImageRectHeight);
    case StereoMode::SideBySideHorizontal:
      return setUpSwapchain(swapchains[0], configViews[0].recommendedImageRectWidth * 2, configViews[0].recommendedImageRectHeight);
    case StereoMode::SideBySideVertical:
      return setUpSwapchain(swapchains[0], configViews[0].recommendedImageRectWidth, configViews[0].recommendedImageRectHeight * 2);
  }

  G_ASSERT(false);
  return false;
}

bool OpenXRDevice::setUpSwapchain(SwapchainData &swapchain, int width, int height)
{
  XrSwapchainCreateInfo swapchainInfo = {XR_TYPE_SWAPCHAIN_CREATE_INFO};
  XrSwapchain handle;
  swapchainInfo.arraySize = 1;
  swapchainInfo.mipCount = 1;
  swapchainInfo.faceCount = 1;
  swapchainInfo.width = width;
  swapchainInfo.height = height;
  swapchainInfo.sampleCount = 1;
  swapchainInfo.usageFlags = XR_SWAPCHAIN_USAGE_SAMPLED_BIT | XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT;
  swapchainInfo.format = get_best_swapchain_format(swapchainFormats, false, renderingAPI).first;

  if (swapchainInfo.format == 0)
    return false;

  swapchain.width = swapchainInfo.width;
  swapchain.height = swapchainInfo.height;

  logdbg("[XR][device] Creating color swapchain using format %lld", swapchainInfo.format);

  XR_REPORT_RETURN(xrCreateSwapchain(oxrSession, &swapchainInfo, &swapchain.handle), false);

  if (isExtensionSupported(XR_KHR_COMPOSITION_LAYER_DEPTH_EXTENSION_NAME) && enableDepthLayerExtension)
  {
    swapchainInfo.usageFlags = XR_SWAPCHAIN_USAGE_SAMPLED_BIT | XR_SWAPCHAIN_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    swapchainInfo.format = get_best_swapchain_format(swapchainFormats, true, renderingAPI).first;

    if (swapchainInfo.format != 0)
    {
      swapchain.depth = eastl::make_unique<SwapchainData>();
      swapchain.depth->width = swapchain.width;
      swapchain.depth->height = swapchain.height;

      logdbg("[XR][device] Creating depth swapchain using format %lld", swapchainInfo.format);

      XR_REPORT_RETURN(xrCreateSwapchain(oxrSession, &swapchainInfo, &swapchain.depth->handle), false);
    }
  }

  return true;
}

bool OpenXRDevice::setUpSwapchainImage(SwapchainData &swapchain, int swap_chain_index, bool depth)
{
  if (!swapchain.imageViews.empty())
    return true;

  uint32_t surfaceCount = 0;

  XR_REPORT_RETURN(xrEnumerateSwapchainImages(swapchain.handle, 0, &surfaceCount, nullptr), false);

  eastl::unique_ptr<XrSwapchainImageBaseHeader[]> swapChainImages;

  switch (renderingAPI)
  {
#ifdef XR_USE_GRAPHICS_API_D3D11
    case RenderingAPI::D3D11:
    {
      XrSwapchainImageD3D11KHR *images = new XrSwapchainImageD3D11KHR[surfaceCount];
      eastl::fill_n(images, surfaceCount, XrSwapchainImageD3D11KHR{XR_TYPE_SWAPCHAIN_IMAGE_D3D11_KHR});
      swapChainImages.reset(reinterpret_cast<XrSwapchainImageBaseHeader *>(images));
      break;
    }
#endif // XR_USE_GRAPHICS_API_D3D11
#ifdef XR_USE_GRAPHICS_API_D3D12
    case RenderingAPI::D3D12:
    {
      XrSwapchainImageD3D12KHR *images = new XrSwapchainImageD3D12KHR[surfaceCount];
      eastl::fill_n(images, surfaceCount, XrSwapchainImageD3D12KHR{XR_TYPE_SWAPCHAIN_IMAGE_D3D12_KHR});
      swapChainImages.reset(reinterpret_cast<XrSwapchainImageBaseHeader *>(images));
      break;
    }
#endif // XR_USE_GRAPHICS_API_D3D12
#ifdef XR_USE_GRAPHICS_API_VULKAN
    case RenderingAPI::Vulkan:
    {
      XrSwapchainImageVulkanKHR *images = new XrSwapchainImageVulkanKHR[surfaceCount];
      eastl::fill_n(images, surfaceCount, XrSwapchainImageVulkanKHR{XR_TYPE_SWAPCHAIN_IMAGE_VULKAN_KHR});
      swapChainImages.reset(reinterpret_cast<XrSwapchainImageBaseHeader *>(images));
      break;
    }
#endif // XR_USE_GRAPHICS_API_VULKAN
    default: G_ASSERTF_RETURN(false, false, "Implement swap chain image retrieval for this renderer!"); break;
  }

  XR_REPORT_RETURN(xrEnumerateSwapchainImages(swapchain.handle, surfaceCount, &surfaceCount, swapChainImages.get()), false);

  for (int six = 0; six < surfaceCount; ++six)
  {
    eastl::string textureName;
    textureName.sprintf("OpenXR_%d_%d%s", swap_chain_index, six, depth ? "_depth" : "_color");
    Texture *wrappedTexture = nullptr;
    Drv3dMakeTextureParams makeParams;
    makeParams.name = textureName.c_str();
    makeParams.flg = get_best_swapchain_format(swapchainFormats, depth, renderingAPI).second | TEXCF_RTARGET;
    makeParams.w = swapchain.width;
    makeParams.h = swapchain.height;
    makeParams.layers = 1;
    makeParams.mips = 1;
    makeParams.currentState = depth ? ResourceBarrier::RB_RW_DEPTH_STENCIL_TARGET : ResourceBarrier::RB_RW_RENDER_TARGET;

    switch (renderingAPI)
    {
#ifdef XR_USE_GRAPHICS_API_D3D11
      case RenderingAPI::D3D11:
        makeParams.tex = reinterpret_cast<XrSwapchainImageD3D11KHR *>(swapChainImages.get())[six].texture;
        break;
#endif // XR_USE_GRAPHICS_API_D3D11
#ifdef XR_USE_GRAPHICS_API_D3D12
      case RenderingAPI::D3D12:
        makeParams.tex = reinterpret_cast<XrSwapchainImageD3D12KHR *>(swapChainImages.get())[six].texture;
        break;
#endif // XR_USE_GRAPHICS_API_D3D12
#ifdef XR_USE_GRAPHICS_API_VULKAN
      case RenderingAPI::Vulkan:
        makeParams.tex = &reinterpret_cast<XrSwapchainImageVulkanKHR *>(swapChainImages.get())[six].image;
        break;
#endif // XR_USE_GRAPHICS_API_VULKAN
      default: G_ASSERTF_RETURN(false, false, "Implement swap chain surface view creation for this renderer!"); break;
    }

    swapchain.imageViews.emplace_back(UniqueTex(dag::make_texture_raw(makeParams), textureName.data()));

    switch (renderingAPI)
    {
#ifdef XR_USE_GRAPHICS_API_D3D11
      case RenderingAPI::D3D11: reinterpret_cast<XrSwapchainImageD3D11KHR *>(swapChainImages.get())[six].texture->Release(); break;
#endif // XR_USE_GRAPHICS_API_D3D11
#ifdef XR_USE_GRAPHICS_API_D3D12
      case RenderingAPI::D3D12: reinterpret_cast<XrSwapchainImageD3D12KHR *>(swapChainImages.get())[six].texture->Release(); break;
#endif // XR_USE_GRAPHICS_API_D3D12
#ifdef XR_USE_GRAPHICS_API_VULKAN
      case RenderingAPI::Vulkan:
        // No need to delete ImageVulkan, because it dependant resource and will be deleted with swapchain
        break;
#endif // XR_USE_GRAPHICS_API_VULKAN
      default: G_ASSERTF_RETURN(false, false, "Implement image releasing for this renderer!"); break;
    }
  }

  if (swapchain.depth)
    return setUpSwapchainImage(*swapchain.depth, swap_chain_index, true);

  return true;
}

bool OpenXRDevice::setUpSwapchainImages()
{
  if (!setUpSwapchainImage(swapchains[0], 0, false))
    return false;

  if (swapchains[1].handle != XR_NULL_HANDLE && !setUpSwapchainImage(swapchains[1], 1, false))
    return false;

  return true;
}

void OpenXRDevice::tearDownSwapchains()
{
  tearDownSwapchain(swapchains[0]);
  tearDownSwapchain(swapchains[1]);
}

void OpenXRDevice::tearDownSwapchain(SwapchainData &swapchain)
{
  if (swapchain.handle == XR_NULL_HANDLE)
    return;

  XR_REPORT(xrDestroySwapchain(swapchain.handle));
  swapchain.handle = XR_NULL_HANDLE;

  if (swapchain.depth)
  {
    XR_REPORT(xrDestroySwapchain(swapchain.depth->handle));
    swapchain.depth.reset();
  }
}

void OpenXRDevice::tearDownSwapchainImages()
{
  // With D3D12 and VK, the swapchain textures wrapped inside dagor textures will not be released
  // during teardown, only scheduled for deletion at the end of the frame. We wait for that
  // to happen before moving forward, and probably make the OpenXR runtime to destroy them.

#if defined(XR_USE_GRAPHICS_API_VULKAN)
  if (renderingAPI == RenderingAPI::Vulkan && d3d::is_inited())
    d3d::driver_command(DRV3D_COMMAND_D3D_FLUSH, 0, 0, 0);
#endif // XR_USE_GRAPHICS_API_VULKAN

  tearDownSwapchainImages(swapchains[0]);
  tearDownSwapchainImages(swapchains[1]);

#if defined(XR_USE_GRAPHICS_API_D3D12)
  if (renderingAPI == RenderingAPI::D3D12 && d3d::is_inited())
  {
    auto query = d3d::create_event_query();
    d3d::issue_event_query(query);
    while (!d3d::get_event_query_status(query, true))
      sleep_msec(0);
    d3d::release_event_query(query);
  }
#endif // XR_USE_GRAPHICS_API_D3D12
}

void OpenXRDevice::tearDownSwapchainImages(SwapchainData &swapchain)
{
  if (swapchain.handle == XR_NULL_HANDLE)
    return;

  swapchain.imageViews.clear();

  if (swapchain.depth)
    swapchain.depth->imageViews.clear();
}

OpenXRDevice::RenderingAPI OpenXRDevice::getRenderingAPI() const { return renderingAPI; }

OpenXRDevice::AdapterID OpenXRDevice::getAdapterId() const
{
  switch (renderingAPI)
  {
#ifdef XR_USE_GRAPHICS_API_D3D11
    case RenderingAPI::D3D11:
    {
      static_assert(sizeof(AdapterID) == sizeof(LUID), "AdapterID size mismatch!");
      XrGraphicsRequirementsD3D11KHR requirement = {XR_TYPE_GRAPHICS_REQUIREMENTS_D3D11_KHR};
      XR_REPORT(pfnGetD3D11GraphicsRequirementsKHR(oxrInstance, oxrSystemId, &requirement));
      return *reinterpret_cast<AdapterID *>(&requirement.adapterLuid);
    }
#endif // XR_USE_GRAPHICS_API_D3D11
#ifdef XR_USE_GRAPHICS_API_D3D12
    case RenderingAPI::D3D12:
    {
      static_assert(sizeof(AdapterID) == sizeof(LUID), "AdapterID size mismatch!");
      XrGraphicsRequirementsD3D12KHR requirement = {XR_TYPE_GRAPHICS_REQUIREMENTS_D3D12_KHR};
      XR_REPORT(pfnGetD3D12GraphicsRequirementsKHR(oxrInstance, oxrSystemId, &requirement));
      return *reinterpret_cast<AdapterID *>(&requirement.adapterLuid);
    }
#endif // XR_USE_GRAPHICS_API_D3D12
#ifdef XR_USE_GRAPHICS_API_VULKAN
    case RenderingAPI::Vulkan:
    {
      // XrGraphicsRequirementsVulkanKHR doesn't specify a required adapter.
      return 0;
    }
#endif // XR_USE_GRAPHICS_API_VULKAN
    default: G_ASSERTF_RETURN(false, 0, "Implement getAdapterId for this renderer!");
  }
}

const char *OpenXRDevice::getRequiredDeviceExtensions() const { return oxrRequredDeviceExtensions.data(); }

const char *OpenXRDevice::getRequiredInstanceExtensions() const { return oxrRequredInstanceExtensions.data(); }

eastl::pair<uint64_t, uint64_t> OpenXRDevice::getRequiredGraphicsAPIRange() const
{
  gfxApiRangeCalled = true;

#ifdef XR_USE_GRAPHICS_API_VULKAN
  if (renderingAPI == RenderingAPI::Vulkan)
  {
    XrGraphicsRequirementsVulkanKHR vreq = {XR_TYPE_GRAPHICS_REQUIREMENTS_VULKAN_KHR};
    if (XR_SUCCEEDED(pfnGetVulkanGraphicsRequirementsKHR(oxrInstance, oxrSystemId, &vreq)))
      return {vreq.minApiVersionSupported, vreq.maxApiVersionSupported};
  }
#endif // XR_USE_GRAPHICS_API_VULKAN
  return {0, 0};
}

void OpenXRDevice::getViewResolution(int &width, int &height) const
{
  width = configViews[0].recommendedImageRectWidth;
  height = configViews[0].recommendedImageRectHeight;
}

float OpenXRDevice::getViewAspect() const
{
  int w, h;
  getViewResolution(w, h);
  return (w != 0 && h != 0) ? w / (float)h : 1.0f;
}


bool OpenXRDevice::isActive() const { return sessionRunning; }

void OpenXRDevice::setZoom(float zoom) { this->zoom = fmax(zoom, 0.01f); }

bool OpenXRDevice::setRenderingDeviceImpl()
{
  if (!setUpSession())
    return false;

  return setUpSwapchains() && setUpSwapchainImages();
}

void OpenXRDevice::setUpSpace() { shouldResetAppSpace = true; }

bool OpenXRDevice::isReadyForRender() { return sessionRunning && !oxrHadRuntimeFailure && oxrAppSpace; }

void OpenXRDevice::setUpApplicationSpace()
{
  if (!oxrRefSpace || !shouldResetAppSpace)
    return;

#if _TARGET_PC_WIN
  uint32_t dummy;
  XrView views[] = {{XR_TYPE_VIEW}, {XR_TYPE_VIEW}};
  XrViewState viewState = {XR_TYPE_VIEW_STATE};
  XrViewLocateInfo locateInfo = {XR_TYPE_VIEW_LOCATE_INFO};

  locateInfo.viewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
  locateInfo.displayTime = get_current_xr_time(oxrInstance);
  locateInfo.space = oxrRefSpace;

  XR_REPORT_RETURN(xrLocateViews(oxrSession, &locateInfo, &viewState, 2, &dummy, views), );

  for (const auto &v : views)
    if (!is_orientation_valid(v.pose.orientation))
      return;

  // as_quat() from algo.h cannot be used here: it converts to Dagor's space
  const Quat leftEyeOrientation = Quat{0, views[0].pose.orientation.y, 0, views[0].pose.orientation.w}.normalize();
  const Quat rightEyeOrientation = Quat{0, views[1].pose.orientation.y, 0, views[1].pose.orientation.w}.normalize();
  const Quat centerViewOrientation = qinterp(leftEyeOrientation, rightEyeOrientation, 0.5f);

  XrReferenceSpaceCreateInfo refSpace = {XR_TYPE_REFERENCE_SPACE_CREATE_INFO};
  refSpace.poseInReferenceSpace.orientation.x = centerViewOrientation.x;
  refSpace.poseInReferenceSpace.orientation.y = centerViewOrientation.y;
  refSpace.poseInReferenceSpace.orientation.z = centerViewOrientation.z;
  refSpace.poseInReferenceSpace.orientation.w = centerViewOrientation.w;
  refSpace.poseInReferenceSpace.position.x = (views[0].pose.position.x + views[1].pose.position.x) / 2;
  refSpace.poseInReferenceSpace.position.y = (views[0].pose.position.y + views[1].pose.position.y) / 2;
  refSpace.poseInReferenceSpace.position.z = (views[0].pose.position.z + views[1].pose.position.z) / 2;
  refSpace.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL;
#elif _TARGET_ANDROID
  XrReferenceSpaceCreateInfo refSpace = {XR_TYPE_REFERENCE_SPACE_CREATE_INFO};
  refSpace.poseInReferenceSpace.orientation = {0, 0, 0, 1};
  refSpace.poseInReferenceSpace.position = {0, 0, 0};
  refSpace.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL;
#else
// On standalone devices, app space should be the ref space.
// Otherwise it should be like windows, but get_current_xr_time needs to be implemented on that platform.
#error "Implement getting the application space for this platform."
#endif // PLATFORM

  if (oxrAppSpace)
  {
    XR_REPORT(xrDestroySpace(oxrAppSpace));
    oxrAppSpace = 0;
  }

  XR_REPORT_RETURN(xrCreateReferenceSpace(oxrSession, &refSpace, &oxrAppSpace), );

  shouldResetAppSpace = false;
}

void OpenXRDevice::retrieveScreenMaskTriangles(const TMatrix4 &projection, eastl::vector<Point4> &visibilityMaskVertices,
  eastl::vector<uint16_t> &visibilityMaskIndices, int view_index)
{
  XrVisibilityMaskKHR mask = {XR_TYPE_VISIBILITY_MASK_KHR};
  XR_REPORT_RETURN(pfnGetVisibilityMaskKHR(oxrSession, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO, view_index,
                     XR_VISIBILITY_MASK_TYPE_HIDDEN_TRIANGLE_MESH_KHR, &mask), );

  // This is normal. The mask is not yet available to the runtime, but it may be later
  if (mask.vertexCountOutput == 0 || mask.indexCountOutput == 0)
  {
    screenMaskAvailable = false;
    return;
  }

  eastl::vector<XrVector2f> visibilityMaskVerticesXR(mask.vertexCountOutput);
  eastl::vector<uint32_t> visibilityMaskIndices32(mask.indexCountOutput);
  visibilityMaskVertices.resize(mask.vertexCountOutput);
  visibilityMaskIndices.resize(mask.indexCountOutput);

  mask.vertexCapacityInput = visibilityMaskVerticesXR.size();
  mask.indexCapacityInput = visibilityMaskIndices32.size();
  mask.vertices = visibilityMaskVerticesXR.data();
  mask.indices = visibilityMaskIndices32.data();

  XR_REPORT_RETURN(pfnGetVisibilityMaskKHR(oxrSession, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO, view_index,
                     XR_VISIBILITY_MASK_TYPE_HIDDEN_TRIANGLE_MESH_KHR, &mask), );

  eastl::copy(visibilityMaskIndices32.begin(), visibilityMaskIndices32.end(), visibilityMaskIndices.begin());

  eastl::transform(visibilityMaskVerticesXR.begin(), visibilityMaskVerticesXR.end(), visibilityMaskVertices.begin(),
    [&projection](XrVector2f v) {
      Point4 result;
      projection.transform(Point3(v.x, v.y, 1), result);
      result.z = result.w;
      return result;
    });
}

bool OpenXRDevice::recreateOpenXr()
{
  auto query = d3d::create_event_query();
  d3d::issue_event_query(query);
  for (int startTime = ::get_time_msec(); !d3d::get_event_query_status(query, true);)
  {
    if (::get_time_msec() > startTime + 2000)
    {
      fatal("[XR][device] Flushing the rendering pipeline is failed within 2 seconds!");
      return false;
    }

    std::this_thread::yield();
  }
  d3d::release_event_query(query);

  auto tearDown = [this]() {
    tearDownSwapchainImages();
    tearDownScreenMaskResources();
    tearDownSwapchains();
    tearDownSession();
    tearDownInstance();
  };

  tearDown();

  if (!setUpInstance() || !getAdapterId() || !setUpSession() || !setUpSwapchains() || !setUpSwapchainImages())
  {
    logerr("[XR][device] OpenXR was not able to recreate the XR facilities after a runtime error. Trying later.");
    tearDown();
    return false;
  }

  return true;
}

void OpenXRDevice::tick(const SessionCallback &cb)
{
  TIME_D3D_PROFILE(OpenXRDevice__tick);

  if (oxrHadRuntimeFailure)
  {
    if (!recreateOpenXr())
      return;

    oxrHadRuntimeFailure = false;
  }

  // We expect that all effects of a state transition to happen within one frame
  inHmdStateTransition = false;

  XrEventDataBuffer eventBuffer = {XR_TYPE_EVENT_DATA_BUFFER};

  while (xrPollEvent(oxrInstance, &eventBuffer) == XR_SUCCESS)
  {
    switch (eventBuffer.type)
    {
      case XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED:
      {
        XrEventDataSessionStateChanged *event = reinterpret_cast<XrEventDataSessionStateChanged *>(&eventBuffer); //-V641
        oxrSessionState = event->state;

        switch (oxrSessionState)
        {
          case XR_SESSION_STATE_READY:
          {
            debug("[XR] XR_SESSION_STATE_READY happened.");

            XrSessionBeginInfo beginInfo = {XR_TYPE_SESSION_BEGIN_INFO};
            beginInfo.primaryViewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
            XR_REPORT(xrBeginSession(oxrSession, &beginInfo));
            setUpReferenceSpace();
            sessionRunning = true;
            inHmdStateTransition = true;
            callSessionStartedCallbackIfAvailable = true;
            break;
          }

          case XR_SESSION_STATE_STOPPING:
          {
            debug("[XR] XR_SESSION_STATE_STOPPING happened.");

            sessionRunning = false;

            XR_REPORT(xrEndSession(oxrSession));
            XR_REPORT(xrDestroySpace(oxrRefSpace));
            oxrRefSpace = 0;

            if (oxrAppSpace)
            {
              XR_REPORT(xrDestroySpace(oxrAppSpace));
              oxrAppSpace = 0;
            }

            inHmdStateTransition = true;
            callSessionEndedCallbackIfAvailable = true;

            break;
          }

          case XR_SESSION_STATE_EXITING:
          {
            debug("[XR] XR_SESSION_STATE_EXITING happened.");
            apiRequestedQuit = true;
            break;
          }

          case XR_SESSION_STATE_LOSS_PENDING:
          {
            debug("[XR] XR_SESSION_STATE_LOSS_PENDING happened.");
            oxrHadRuntimeFailure = true;
            break;
          }

          default: debug("[XR] default case in switch(oxrSessionState): %d", int(oxrSessionState)); break;
        }
        break;
      }

      case XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING: apiRequestedQuit = true; return;

      case XR_TYPE_EVENT_DATA_VISIBILITY_MASK_CHANGED_KHR:
        screenMaskVertexBuffer[0].close();
        screenMaskVertexBuffer[1].close();
        screenMaskIndexBuffer[0].close();
        screenMaskIndexBuffer[1].close();
        screenMaskAvailable = true;
        break;

      case XR_TYPE_EVENT_DATA_REFERENCE_SPACE_CHANGE_PENDING: setUpReferenceSpace(); break;

      case XR_TYPE_EVENT_DATA_INTERACTION_PROFILE_CHANGED: inputHandler.updateCurrentBindings(); break;

      default: debug("default case in switch(eventBuffer.type)"); break;
    }

    eventBuffer = {XR_TYPE_EVENT_DATA_BUFFER};
  }

  inputHandler.updateActionsState();

  if (callSessionStartedCallbackIfAvailable && callSessionEndedCallbackIfAvailable)
    callSessionStartedCallbackIfAvailable = callSessionEndedCallbackIfAvailable = false;

  if (callSessionStartedCallbackIfAvailable && cb)
  {
    inHmdStateTransition = true;
    if (cb(Session::Start))
      callSessionStartedCallbackIfAvailable = false;
  }
  if (callSessionEndedCallbackIfAvailable && cb)
  {
    inHmdStateTransition = true;
    if (cb(Session::End))
      callSessionEndedCallbackIfAvailable = false;
  }

  setUpApplicationSpace();
}

bool OpenXRDevice::prepareFrame(FrameData &frameData, float zNear, float zFar)
{
  TIME_D3D_PROFILE(OpenXRDevice__prepareFrame);

  if (!sessionRunning)
  {
    debug("[xr][device] prepareFrame failed, due to not running session.");
    return false;
  }

  if (oxrHadRuntimeFailure)
  {
    debug("[xr][device] prepareFrame failed, due to a recent runtime failure.");
    return false;
  }

  if (!oxrAppSpace)
  {
    debug("[xr][device] prepareFrame failed, due to missing app space.");
    return false;
  }

  XrFrameState frameState = {XR_TYPE_FRAME_STATE};
  XR_REPORT_RETURN_RTF(xrWaitFrame(oxrSession, nullptr, &frameState), false);

  uint32_t dummy;
  XrView oxrCurrentViews[] = {{XR_TYPE_VIEW}, {XR_TYPE_VIEW}};

  XrViewState viewState = {XR_TYPE_VIEW_STATE};
  XrViewLocateInfo locateInfo = {XR_TYPE_VIEW_LOCATE_INFO};
  locateInfo.viewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
  locateInfo.displayTime = frameState.predictedDisplayTime;
  locateInfo.space = oxrAppSpace;
  XR_REPORT_RETURN_RTF(xrLocateViews(oxrSession, &locateInfo, &viewState, 2, &dummy, oxrCurrentViews), false);

  static bool once = true;
  if (once)
  {
    // The Oculus runtime can provide a zero quaternion for orientation during
    // initialization when the HMD is put on for the first time.
    if (is_orientation_valid(frameData.views[0].orientation))
    {
      float separation = (Point3(&oxrCurrentViews[0].pose.position.x, Point3::CTOR_FROM_PTR) -
                          Point3(&oxrCurrentViews[1].pose.position.x, Point3::CTOR_FROM_PTR))
                           .length();

      Quat leftOrientation = as_quat(oxrCurrentViews[0].pose.orientation);
      Quat rightOrientation = as_quat(oxrCurrentViews[1].pose.orientation);
      Quat vergence = rightOrientation * inverse(qinterp(leftOrientation, rightOrientation, 0.5));

      int emuWidth = swapchains[0].width;
      int emuHeight = swapchains[0].height;

      switch (stereoMode)
      {
        case VRDevice::StereoMode::SideBySideHorizontal: emuWidth /= 2; break;
        case VRDevice::StereoMode::SideBySideVertical: emuHeight /= 2; break;
        default: break;
      }

      logdbg("[XR][device] Emulator profile: -config:xr/emulatorProfile:t=%d,%d,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f", emuWidth,
        emuHeight, separation, oxrCurrentViews[0].fov.angleLeft, oxrCurrentViews[0].fov.angleRight, oxrCurrentViews[0].fov.angleUp,
        oxrCurrentViews[0].fov.angleDown, oxrCurrentViews[1].fov.angleLeft, oxrCurrentViews[1].fov.angleRight,
        oxrCurrentViews[1].fov.angleUp, oxrCurrentViews[1].fov.angleDown, vergence.x, vergence.y, vergence.z, vergence.w);
      once = false;
    }
  }

  frameData.displayTime = frameState.predictedDisplayTime;
  frameData.zoom = zoom;

  {
    for (int viewIx = 0; viewIx < 2; ++viewIx)
    {
      frameData.views[viewIx].fovLeft = oxrCurrentViews[viewIx].fov.angleLeft;
      frameData.views[viewIx].fovRight = oxrCurrentViews[viewIx].fov.angleRight;
      frameData.views[viewIx].fovUp = oxrCurrentViews[viewIx].fov.angleUp;
      frameData.views[viewIx].fovDown = oxrCurrentViews[viewIx].fov.angleDown;
      frameData.views[viewIx].position = as_dpoint3(oxrCurrentViews[viewIx].pose.position);
      frameData.views[viewIx].orientation = as_quat(oxrCurrentViews[viewIx].pose.orientation);

      // The Oculus runtime can provide a zero quaternion for orientation during
      // initialization when the HMD is put on for the first time.
      if (!is_orientation_valid(frameData.views[viewIx].orientation))
        frameData.views[viewIx].orientation.identity();

      calcViewTransforms(frameData.views[viewIx], zNear, zFar, frameData.zoom);
    }
  }

  const float extraZFar = calcBoundingView(frameData);
  calcViewTransforms(frameData.boundingView, zNear, zFar + extraZFar, frameData.zoom);
  if (!is_equal_float(frameData.zoom, 1.0f) && frameData.views[0].orientation != frameData.views[1].orientation)
    for (auto &v : frameData.views)
    {
      // HACK: We have to adjust views for slanted (angled) hmd displays, otherwise they point
      // in different directions and can't see the objects in the center. For usual (plane-aligned)
      // displays the orientation is not affected since all three views look in the same direction.
      const float k = zoom * 0.25f; // Empirical value based on Pimax Artisan
      const auto originalPosition = v.position;
      const auto originalOrientation = v.orientation;

      v.position = frameData.boundingView.position;
      v.orientation = qinterp(v.orientation, frameData.boundingView.orientation, k);
      calcViewTransforms(v, zNear, zFar, frameData.zoom);

      // Restore original view properties to avoid shrinking viewport with SteamXr
      v.position = originalPosition;
      v.orientation = originalOrientation;
    }

  auto getSwapchainImage = [this](const SwapchainData &swapchain) -> ManagedTexView {
    uint32_t imageId;
    XrSwapchainImageAcquireInfo acquireInfo = {XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO};
    XR_REPORT(xrAcquireSwapchainImage(swapchain.handle, &acquireInfo, &imageId));

    return swapchain.imageViews[imageId];
  };

  switch (stereoMode)
  {
    case StereoMode::Multipass:
      frameData.frameRects[0] = RectInt{0, 0, swapchains[0].width, swapchains[0].height};
      frameData.frameRects[1] = RectInt{0, 0, swapchains[0].width, swapchains[0].height};
      frameData.frameOffsets[0] = IPoint2(0, 0);
      frameData.frameOffsets[1] = IPoint2(0, 0);
      frameData.frameTargets[0] = getSwapchainImage(swapchains[0]);
      frameData.frameTargets[1] = getSwapchainImage(swapchains[1]);
      frameData.frameDepths[0] = (swapchains[0].depth && enable_depth) ? getSwapchainImage(*swapchains[0].depth) : ManagedTexView{};
      frameData.frameDepths[1] = (swapchains[1].depth && enable_depth) ? getSwapchainImage(*swapchains[1].depth) : ManagedTexView{};
      frameData.frameTargetSwapchainHandles[0] = uint64_t(swapchains[0].handle);
      frameData.frameTargetSwapchainHandles[1] = uint64_t(swapchains[1].handle);
      frameData.frameDepthSwapchainHandles[0] = (swapchains[0].depth && enable_depth) ? uint64_t(swapchains[0].depth->handle) : 0;
      frameData.frameDepthSwapchainHandles[1] = (swapchains[1].depth && enable_depth) ? uint64_t(swapchains[1].depth->handle) : 0;
      break;
    case StereoMode::SideBySideHorizontal:
      frameData.frameRects[0] = RectInt{0, 0, swapchains[0].width / 2, swapchains[0].height};
      frameData.frameRects[1] = RectInt{swapchains[0].width / 2, 0, swapchains[0].width, swapchains[0].height};
      frameData.frameOffsets[0] = IPoint2(0, 0);
      frameData.frameOffsets[1] = IPoint2(1, 0);
      frameData.frameTargets[0] = getSwapchainImage(swapchains[0]);
      frameData.frameTargets[1] = frameData.frameTargets[0];
      frameData.frameDepths[0] = (swapchains[0].depth && enable_depth) ? getSwapchainImage(*swapchains[0].depth) : ManagedTexView{};
      frameData.frameDepths[1] = frameData.frameDepths[0];
      frameData.frameTargetSwapchainHandles[0] = uint64_t(swapchains[0].handle);
      frameData.frameTargetSwapchainHandles[1] = frameData.frameTargetSwapchainHandles[0];
      frameData.frameDepthSwapchainHandles[0] = (swapchains[0].depth && enable_depth) ? uint64_t(swapchains[0].depth->handle) : 0;
      frameData.frameDepthSwapchainHandles[1] = frameData.frameDepthSwapchainHandles[0];
      break;
    case StereoMode::SideBySideVertical:
      frameData.frameRects[0] = RectInt{0, 0, swapchains[0].width, swapchains[0].height / 2};
      frameData.frameRects[1] = RectInt{0, swapchains[0].height / 2, swapchains[0].width, swapchains[0].height};
      frameData.frameOffsets[0] = IPoint2(0, 0);
      frameData.frameOffsets[1] = IPoint2(0, 1);
      frameData.frameTargets[0] = getSwapchainImage(swapchains[0]);
      frameData.frameTargets[1] = frameData.frameTargets[0];
      frameData.frameDepths[0] = (swapchains[0].depth && enable_depth) ? getSwapchainImage(*swapchains[0].depth) : ManagedTexView{};
      frameData.frameDepths[1] = frameData.frameDepths[0];
      frameData.frameTargetSwapchainHandles[0] = uint64_t(swapchains[0].handle);
      frameData.frameTargetSwapchainHandles[1] = frameData.frameTargetSwapchainHandles[0];
      frameData.frameDepthSwapchainHandles[0] = (swapchains[0].depth && enable_depth) ? uint64_t(swapchains[0].depth->handle) : 0;
      frameData.frameDepthSwapchainHandles[1] = frameData.frameDepthSwapchainHandles[0];
      break;
  }

  frameData.depthFilled = false;

  frameData.nearZ = zNear;
  frameData.farZ = zFar;

  frameData.frameStartedSuccessfully = false;
  frameData.frameId = frameId++;

  inputHandler.updatePoseSpaces(oxrAppSpace, frameState.predictedDisplayTime);
  inputHandler.updateLegacyControllerPoses(oxrAppSpace, frameState.predictedDisplayTime);
  inputHandler.updateHandJoints(oxrAppSpace, frameState.predictedDisplayTime);

  return true;
}

void OpenXRDevice::beginFrame(FrameData &frameData)
{
  TIME_D3D_PROFILE(OpenXRDevice__beginFrame);

  struct FrameEventCallbacks : public FrameEvents
  {
    FrameEventCallbacks(OpenXRDevice *thiz, FrameData &frameData) : thiz(thiz), frameData(frameData) {}
    virtual void beginFrameEvent() override { thiz->beginRender(frameData); }
    virtual void endFrameEvent() override
    {
      thiz->endRender(frameData);
      delete this;
    }

    OpenXRDevice *thiz;
    FrameData &frameData;
  };

  const bool useFront = false;

  d3d::driver_command(DRV3D_COMMAND_REGISTER_ONE_TIME_FRAME_EXECUTION_EVENT_CALLBACKS, new FrameEventCallbacks(this, frameData),
    reinterpret_cast<void *>(static_cast<intptr_t>(useFront)), nullptr);
}

void OpenXRDevice::beginRender(FrameData &frameData)
{
  TIME_D3D_PROFILE(OpenXRDevice__beginRender);

  AutoExternalAccess gpuLock(false, true, renderingAPI);

  if (!XR_REPORT_RTF(xrBeginFrame(oxrSession, nullptr)))
  {
    frameData.frameStartedSuccessfully = false;
    return;
  }

  XrSwapchainImageWaitInfo waitInfo = {XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO};
  waitInfo.timeout = XR_INFINITE_DURATION;

  bool colorWait =
    XR_REPORT_RTF(xrWaitSwapchainImage(swapchains[0].handle, &waitInfo)) &&
    (swapchains[1].handle != XR_NULL_HANDLE ? XR_REPORT_RTF(xrWaitSwapchainImage(swapchains[1].handle, &waitInfo)) : true);

  bool depthWait =
    ((swapchains[0].depth && enable_depth) ? XR_REPORT_RTF(xrWaitSwapchainImage(swapchains[0].depth->handle, &waitInfo)) : true) &&
    ((swapchains[1].depth && enable_depth) ? XR_REPORT_RTF(xrWaitSwapchainImage(swapchains[1].depth->handle, &waitInfo)) : true);

  if (!colorWait || !depthWait)
  {
    XrFrameEndInfo endInfo{XR_TYPE_FRAME_END_INFO};
    endInfo.displayTime = frameData.displayTime;
    endInfo.environmentBlendMode = supportedBlendModes.front();
    endInfo.layerCount = 0;
    endInfo.layers = nullptr;
    XR_REPORT_RTF(xrEndFrame(oxrSession, &endInfo));

    frameData.frameStartedSuccessfully = false;
    return;
  }

  frameData.frameStartedSuccessfully = true;
}

void OpenXRDevice::endRender(FrameData &frameData)
{
  TIME_D3D_PROFILE(OpenXRDevice__endRender);

  if (!frameData.frameStartedSuccessfully)
    return;

  if (!oxrSession)
    return;

  AutoExternalAccess gpuLock(false, true, renderingAPI);

  XrSwapchainImageReleaseInfo releaseInfo = {XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO};
  for (int viewIx = 0; viewIx < 2; ++viewIx)
  {
    if (swapchains[viewIx].handle == XR_NULL_HANDLE)
      break;

    XR_REPORT_RTF(xrReleaseSwapchainImage(swapchains[viewIx].handle, &releaseInfo));
    if (swapchains[viewIx].depth && enable_depth)
      XR_REPORT_RTF(xrReleaseSwapchainImage(swapchains[viewIx].depth->handle, &releaseInfo));
  }

  if (oxrHadRuntimeFailure)
  {
    frameData.frameStartedSuccessfully = false;
    return;
  }

  XrCompositionLayerDepthInfoKHR depthSubmissions[2] = {
    {XR_TYPE_COMPOSITION_LAYER_DEPTH_INFO_KHR}, {XR_TYPE_COMPOSITION_LAYER_DEPTH_INFO_KHR}};

  XrCompositionLayerProjectionView views[2] = {};
  for (int viewIx = 0; viewIx < 2; ++viewIx)
  {
    const auto &rc = frameData.frameRects[viewIx];

    views[viewIx] = {XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW};
    views[viewIx].pose.position.x = frameData.views[viewIx].position.x;
    views[viewIx].pose.position.y = frameData.views[viewIx].position.y;
    views[viewIx].pose.position.z = -frameData.views[viewIx].position.z;
    views[viewIx].pose.orientation.x = -frameData.views[viewIx].orientation.x;
    views[viewIx].pose.orientation.y = -frameData.views[viewIx].orientation.y;
    views[viewIx].pose.orientation.z = frameData.views[viewIx].orientation.z;
    views[viewIx].pose.orientation.w = frameData.views[viewIx].orientation.w;
    views[viewIx].fov.angleLeft = frameData.views[viewIx].fovLeft;
    views[viewIx].fov.angleRight = frameData.views[viewIx].fovRight;
    views[viewIx].fov.angleUp = frameData.views[viewIx].fovUp;
    views[viewIx].fov.angleDown = frameData.views[viewIx].fovDown;
    views[viewIx].subImage.swapchain = (XrSwapchain)frameData.frameTargetSwapchainHandles[viewIx];
    views[viewIx].subImage.imageArrayIndex = 0;
    views[viewIx].subImage.imageRect.offset.x = rc.left;
    views[viewIx].subImage.imageRect.offset.y = rc.top;
    views[viewIx].subImage.imageRect.extent.width = rc.right - rc.left;
    views[viewIx].subImage.imageRect.extent.height = rc.bottom - rc.top;

    if (frameData.frameDepths[viewIx] && frameData.depthFilled && enable_depth)
    {
      depthSubmissions[viewIx].subImage.swapchain = (XrSwapchain)frameData.frameDepthSwapchainHandles[viewIx];
      depthSubmissions[viewIx].subImage.imageRect = views[viewIx].subImage.imageRect;
      depthSubmissions[viewIx].minDepth = 0;
      depthSubmissions[viewIx].maxDepth = 1;

      // Z near and far swapped, meaning that reverse depth is in action
      // https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#XR_KHR_composition_layer_depth
      // "nearZ is the positive distance in meters of the minDepth value in the depth swapchain. Applications
      //  may use a nearZ that is greater than farZ to indicate depth values are reversed. nearZ can be infinite."
      depthSubmissions[viewIx].nearZ = frameData.farZ;
      depthSubmissions[viewIx].farZ = frameData.nearZ;

      views[viewIx].next = &depthSubmissions[viewIx];
    }
  }

  XrCompositionLayerProjection layerProj = {XR_TYPE_COMPOSITION_LAYER_PROJECTION};
  layerProj.space = oxrAppSpace;
  layerProj.viewCount = 2;
  layerProj.views = views;

  XrCompositionLayerBaseHeader *layer = (XrCompositionLayerBaseHeader *)&layerProj;

  XrFrameEndInfo endInfo{XR_TYPE_FRAME_END_INFO};
  endInfo.displayTime = frameData.displayTime;
  endInfo.environmentBlendMode = supportedBlendModes.front();
  endInfo.layerCount = 1;
  endInfo.layers = &layer;
  XrResult result = xrEndFrame(oxrSession, &endInfo);
  if (result == XR_ERROR_SESSION_NOT_RUNNING)
    logdbg("[XR][device] OpenXR session ended during frame rendering. This is normal, but according to the specs, "
           "every successful xrBeginFrame call has to have a matching xrEndFrame call anyway.");
  else if (result == XR_ERROR_RUNTIME_FAILURE)
  {
    logdbg("[XR][device] OpenXR reported a runtime failure. Recreating OpenXR.");
    oxrHadRuntimeFailure = true;
  }
  else
    XR_REPORT(result);

  frameData.frameStartedSuccessfully = false;
}

bool OpenXRDevice::hasQuitRequest() const { return apiRequestedQuit; }

bool OpenXRDevice::isInitialized() const { return initialized; }

bool OpenXRDevice::isExtensionSupported(const char *name) const
{
  for (const XrExtensionProperties &extension : supportedExtensions)
    if (strcmp(extension.extensionName, name) == 0)
      return true;

  return false;
}

void OpenXRDevice::setUpReferenceSpace()
{
  XrReferenceSpaceCreateInfo refSpace = {XR_TYPE_REFERENCE_SPACE_CREATE_INFO};
  refSpace.poseInReferenceSpace.orientation = {0, 0, 0, 1};
  refSpace.poseInReferenceSpace.position = {0, 0, 0};
  refSpace.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL;

  if (oxrRefSpace)
  {
    XR_REPORT(xrDestroySpace(oxrRefSpace));
    oxrRefSpace = 0;
  }

  XR_REPORT_RETURN(xrCreateReferenceSpace(oxrSession, &refSpace, &oxrRefSpace), );

  shouldResetAppSpace = true;
}

void OpenXRDevice::preRecovery()
{
  logdbg("[XR][device] preRecovery");
  tearDownScreenMaskResources();
  tearDownSwapchains();
  tearDownSession();
  tearDownInstance();
}

void OpenXRDevice::recovery()
{
  logdbg("[XR][device] recovery");
  setUpInstance();
  getAdapterId();
  setUpSession();
  setUpSwapchains();
  setUpSwapchainImages();
}

bool OpenXRDevice::hasScreenMask()
{
  return isExtensionSupported(XR_KHR_VISIBILITY_MASK_EXTENSION_NAME) && enable_screen_mask && !disableScreenMask;
}

void OpenXRDevice::beforeSoftDeviceReset() { tearDownSwapchainImages(); }

void OpenXRDevice::afterSoftDeviceReset() { setUpSwapchainImages(); }

bool OpenXRDevice::isInStateTransition() const { return inHmdStateTransition; }

eastl::vector<float> OpenXRDevice::getSupportedRefreshRates() const
{
  eastl::vector<float> refreshRates;

  if (!supportsChangingRefreshRate)
    return {};

  if (!pfnEnumerateDisplayRefreshRatesFB)
    XR_REPORT_RETURN(xrGetInstanceProcAddr(oxrInstance, "xrEnumerateDisplayRefreshRatesFB",
                       (PFN_xrVoidFunction *)(&pfnEnumerateDisplayRefreshRatesFB)),
      {});

  uint32_t numberOfRefreshRates;
  XR_REPORT_RETURN(pfnEnumerateDisplayRefreshRatesFB(oxrSession, 0, &numberOfRefreshRates, nullptr), {});

  refreshRates.resize(numberOfRefreshRates);
  XR_REPORT_RETURN(pfnEnumerateDisplayRefreshRatesFB(oxrSession, refreshRates.size(), &numberOfRefreshRates, refreshRates.data()), {});

  return refreshRates;
}

bool OpenXRDevice::setRefreshRate(float rate) const
{
  debug("[XR][device] Trying to switch the refresh rate to %f Hz.", rate);

  if (!supportsChangingRefreshRate)
  {
    debug("[XR][device] No device support for changing the refresh rate.");
    return false;
  }

  if (!pfnRequestDisplayRefreshRateFB)
    XR_REPORT_RETURN(
      xrGetInstanceProcAddr(oxrInstance, "xrRequestDisplayRefreshRateFB", (PFN_xrVoidFunction *)(&pfnRequestDisplayRefreshRateFB)),
      false);

  XR_REPORT_RETURN(pfnRequestDisplayRefreshRateFB(oxrSession, rate), false);

  debug("[XR][device] Switch succeeded.");
  return true;
}

float OpenXRDevice::getRefreshRate() const
{
  if (!supportsChangingRefreshRate)
    return 0;

  if (!pfnGetDisplayRefreshRateFB)
    XR_REPORT_RETURN(
      xrGetInstanceProcAddr(oxrInstance, "xrGetDisplayRefreshRateFB", (PFN_xrVoidFunction *)(&pfnGetDisplayRefreshRateFB)), 0);

  float rate = 0;
  XR_REPORT_RETURN(pfnGetDisplayRefreshRateFB(oxrSession, &rate), 0);

  return rate;
}

bool OpenXRDevice::isMirrorDisabled() const
{
#if _TARGET_ANDROID
  return true;
#else
  return false;
#endif // _TARGET_ANDROID
}
