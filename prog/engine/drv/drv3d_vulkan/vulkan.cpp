#include <3d/dag_drv3d.h>
#include <3d/dag_drv3d_res.h>
#include <3d/dag_drv3d_platform.h>
#include <3d/dag_tex3d.h>
#include <3d/dag_drv3dCmd.h>
#include <3d/ddsxTex.h>

#include <math/dag_Point2.h>
#include <math/dag_Point3.h>
#include <math/dag_TMatrix.h>
#include <math/dag_TMatrix4.h>
#include <math/integer/dag_IPoint2.h>
#include <math/dag_adjpow2.h>

#include <generic/dag_tab.h>
#include <generic/dag_staticTab.h>
#include <util/dag_string.h>

#include <startup/dag_globalSettings.h>
#include <perfMon/dag_cpuFreq.h>
#include <util/dag_globDef.h>

#include <ioSys/dag_dataBlock.h>
#include <ioSys/dag_memIo.h>
#include <ioSys/dag_genIo.h>
#include <ioSys/dag_zlibIo.h>

#include <debug/dag_debug.h>
#include <debug/dag_log.h>
#include <debug/dag_fatal.h>

#include <workCycle/dag_workCycle.h>
#include <osApiWrappers/dag_cpuJobs.h>
#include <osApiWrappers/dag_dynLib.h>
#include <osApiWrappers/dag_progGlobals.h>

#include <image/dag_texPixel.h> //for capture

#include <resourceActivationGeneric.h>

#include <EASTL/sort.h>

#include "../drv3d_commonCode/gpuConfig.h"

#include "driver.h"
#include "vulkan_loader.h"
#include "vulkan_instance.h"
#include "shader.h"
#include "device.h"
#include "texture.h"
#include "buffer.h"
#include "os.h"
#include "debug_ui.h"

#include "state.h"

#include "frameStateTM.inc.h"
#include <smolv.h>

#include "renderDoc_capture_module.h"

#if !_TARGET_D3D_MULTI
namespace d3d
{
bool get_vsync_enabled();
//! enables or disables strong VSYNC (flips only on VBLANK); returns true on success
bool enable_vsync(bool enable);
//! retrieve list of available display modes
void get_video_modes_list(Tab<String> &list);
} // namespace d3d
#endif

using namespace drv3d_vulkan;
namespace drv3d_vulkan
{
FrameStateTM g_frameState;
}
#define CHECK_MAIN_THREAD()
#include "frameStateTM.inc.cpp"

const bool d3d::HALF_TEXEL_OFS = false;
const float d3d::HALF_TEXEL_OFSFU = 0.0f;

namespace drv3d_vulkan
{
thread_local uint32_t tlsGlobalLockAcquired = 0;
thread_local VkResult tlsLastErrorCode = VK_SUCCESS;

struct GlobalDriverLock
{
  WinCritSec cs;

  void lock()
  {
    cs.lock();
    ++tlsGlobalLockAcquired;
  }

  void unlock()
  {
    --tlsGlobalLockAcquired;
    cs.unlock();
  }
};

struct ApiState
{
  bool isInitialized;
  VulkanLoader loader;
  VulkanInstance instance;
  WindowState windowState;
  Device device;
  String deviceName;
  String niceDeviceDriverVersionString;
  bool deviceWasLost;
  Driver3dDesc driverDesc; //-V730_NOINIT
  GlobalDriverLock globalLock;
  OSSpinlock bufferPoolGuard;
  ObjectPool<GenericBufferInterface> bufferPool;
  OSSpinlock texturePoolGuard;
  ObjectPool<TextureInterfaceBase> texturePool;
  Buffer *screenCaptureStagingBuffer = nullptr;
  Tab<uint8_t> screenCaptureBuffer;
  eastl::optional<RenderDocCaptureModule> docCaptureModule{eastl::nullopt};
#if VK_EXT_debug_report || VK_EXT_debug_utils
  Tab<int32_t> suppressedMessage;
#endif
#if VK_EXT_debug_report
  VulkanDebugReportCallbackEXTHandle debugHandle;
#endif
#if VK_EXT_debug_utils
  VulkanDebugUtilsMessengerEXTHandle debugHandle2;
#endif
  State state;
  ShaderProgramDatabase shaderProgramDatabase;
  BaseTex *swapchainColorProxyTexture = nullptr;
  BaseTex *swapchainDepthStencilProxyTexture = nullptr;
  int lastLimitingFactor = 0;
  bool allowAssertOnValidationFail = false;
  bool initVideoDone = false;
  uint64_t lastGpuTimeStamp = 0;

  void adjustCaps()
  {
    driverDesc.zcmpfunc = 0;
    driverDesc.acmpfunc = 0;
    driverDesc.sblend = 0;
    driverDesc.dblend = 0;
    driverDesc.mintexw = 1;
    driverDesc.mintexh = 1;
    driverDesc.maxtexw = 0x7FFFFFFF;
    driverDesc.maxtexh = 0x7FFFFFFF;
    driverDesc.mincubesize = 1;
    driverDesc.maxcubesize = 0x7FFFFFFF;
    driverDesc.minvolsize = 1;
    driverDesc.maxvolsize = 0x7FFFFFFF;
    driverDesc.maxtexaspect = 0;
    driverDesc.maxtexcoord = 0x7FFFFFFF;
    driverDesc.maxsimtex = spirv::T_REGISTER_INDEX_MAX;
    driverDesc.maxvertexsamplers = spirv::T_REGISTER_INDEX_MAX;
    driverDesc.maxclipplanes = 0x7FFFFFFF;
    driverDesc.maxstreams = 0x7FFFFFFF;
    driverDesc.maxstreamstr = 0x7FFFFFFF;
    driverDesc.maxvpconsts = 0x7FFFFFFF;
    driverDesc.maxprims = 0x7FFFFFFF;
    driverDesc.maxvertind = 0x7FFFFFFF;
    driverDesc.upixofs = 0.f;
    driverDesc.vpixofs = 0.f;
    driverDesc.shaderModel = 6.0_sm;
    driverDesc.maxSimRT = 0x7FFFFFFF;
    driverDesc.is20ArbitrarySwizzleAvailable = true;

    device.adjustCaps(driverDesc);
  }

#if VK_EXT_debug_report
  static VkBool32 VKAPI_PTR msgCallback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objectType, uint64_t object,
    size_t location, int32_t messageCode, const char *pLayerPrefix, const char *pMessage, void *pUserData);
#endif
#if VK_EXT_debug_utils
  static VkBool32 VKAPI_PTR msgCallback2(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, void *pUserData);
#endif

  ApiState()
  {
    isInitialized = false;
    deviceWasLost = false;
#if VK_EXT_debug_report
    debugHandle = VulkanNullHandle();
#endif
  }

  void releaseAll()
  {
    if (!initVideoDone)
      return;
    debug("vulkan: api_state::releaseAll");

    // reset pipeline state so nothing is still referenced on shutdown
    PipelineState &pipeState = device.getContext().getFrontend().pipelineState;
    pipeState.reset();
    pipeState.makeDirty();

    if (swapchainColorProxyTexture)
    {
      swapchainColorProxyTexture->destroyObject();
      swapchainColorProxyTexture = nullptr;
    }
    if (swapchainDepthStencilProxyTexture)
    {
      swapchainDepthStencilProxyTexture->destroyObject();
      swapchainDepthStencilProxyTexture = nullptr;
    }
    {
      OSSpinlockScopedLock lock{bufferPoolGuard};
      bufferPool.iterateAllocated([](GenericBufferInterface *i) { debug("vulkan: generic buffer %p %s leaked", i, i->getBufName()); });

      bufferPool.freeAll();
    }
    {
      OSSpinlockScopedLock lock{texturePoolGuard};
      texturePool.iterateAllocated([](TextureInterfaceBase *i) { debug("vulkan: texture %p %s leaked", i, i->getTexName()); });
      texturePool.freeAll();
    }

    {
      debug("vulkan: extracting & saving pipeline cache...");
      PipelineCacheOutFile pipelineCacheFile;
      device.storePipelineCache(pipelineCacheFile);
    }
    // otherwise we can crash on validation tracking optimized code in backend
    // if pending work contains some pipe rebinds
    device.getContext().processAllPendingWork();
    shaderProgramDatabase.clear(device.getContext());
    device.shutdown();

    clear_and_shrink(deviceName);
    clear_and_shrink(niceDeviceDriverVersionString);

#if VK_EXT_debug_report
    if (!is_null(debugHandle))
      VULKAN_LOG_CALL(instance.vkDestroyDebugReportCallbackEXT(instance.get(), debugHandle, nullptr));
#endif
#if VK_EXT_debug_utils
    if (!is_null(debugHandle2))
      VULKAN_LOG_CALL(instance.vkDestroyDebugUtilsMessengerEXT(instance.get(), debugHandle2, nullptr));
#endif

    instance.shutdown();
    loader.unload();
    windowState.closeWindow();
    shaderProgramDatabase.shutdown();
    isInitialized = false;

    docCaptureModule = eastl::nullopt;
  }
};

extern ApiState api_state;
} // namespace drv3d_vulkan

drv3d_vulkan::ApiState drv3d_vulkan::api_state;

#if VK_EXT_debug_report
VkBool32 VKAPI_PTR drv3d_vulkan::ApiState::msgCallback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objectType,
  uint64_t object, size_t location, int32_t messageCode, const char *pLayerPrefix, const char *pMessage, void *pUserData)
{
  (void)objectType;
  (void)object;
  (void)location;
  (void)pLayerPrefix;
  (void)pMessage;
  (void)pUserData;
  if (find_value_idx(api_state.suppressedMessage, messageCode) != -1)
    return VK_FALSE;
  if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT)
  {
    if ((objectType <= VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT) || !api_state.allowAssertOnValidationFail)
    {
      logerr("vulkan: Error: (objectType=%u, object=0x%X, location=%u, errorCode=%u) %s: %s", objectType, object, location,
        messageCode, pLayerPrefix, pMessage);
    }
    else
    {
      G_ASSERTF(0, "vulkan: Error: (objectType=%u, object=0x%X, location=%u, errorCode=%u) %s: %s", objectType, object, location,
        messageCode, pLayerPrefix, pMessage);
    }
  }
  else if (flags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT)
  {
    debug("vulkan: Perf Warning: (objectType=%u, object=0x%X, location=%u, errorCode=%u) %s: %s", objectType, object, location,
      messageCode, pLayerPrefix, pMessage);
  }
  else if (flags & VK_DEBUG_REPORT_WARNING_BIT_EXT)
  {
    debug("vulkan: Warning: (objectType=%u, object=0x%X, location=%u, errorCode=%u) %s: %s", objectType, object, location, messageCode,
      pLayerPrefix, pMessage);
  }
  else if (flags & VK_DEBUG_REPORT_DEBUG_BIT_EXT)
  {
    debug("vulkan: Debug: (objectType=%u, object=0x%X, location=%u, errorCode=%u) %s: %s", objectType, object, location, messageCode,
      pLayerPrefix, pMessage);
  }
  else
  {
    debug("vulkan: Info: (objectType=%u, object=0x%X, location=%u, errorCode=%u) %s: %s", objectType, object, location, messageCode,
      pLayerPrefix, pMessage);
  }
  return VK_FALSE;
}
#endif

#if VK_EXT_debug_utils
VkBool32 VKAPI_PTR drv3d_vulkan::ApiState::msgCallback2(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
  VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, void *pUserData)
{
  G_UNUSED(messageSeverity);
  G_UNUSED(messageType);
  G_UNUSED(pCallbackData);
  G_UNUSED(pUserData);
  if (!pCallbackData)
  {
    logerr("VK_EXT_debug_utils error: pCallbackData was nullptr!");
    return VK_FALSE;
  }

  // always 0, so breaks id list filter...
  if (find_value_idx(api_state.suppressedMessage, pCallbackData->messageIdNumber) != -1)
    return VK_FALSE;

  enum class ValidationAction
  {
    DO_ASSERT,
    DO_LOG_ERROR,
    DO_LOG_DEBUG,
  };

  ValidationAction action = ValidationAction::DO_LOG_DEBUG;
  const char *serverityName = "Error";
  const char *typeName = "Validation";

  if (VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT == messageSeverity)
  {
    serverityName = "Error";
    if (VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT == messageType)
    {
      action = ValidationAction::DO_ASSERT;
    }
    else if (VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT == messageType)
    {
      action = ValidationAction::DO_LOG_ERROR;
    }
    else if (VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT == messageType)
    {
      action = ValidationAction::DO_LOG_DEBUG; // -V1048
    }
  }
  else if (VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT == messageSeverity)
  {
    serverityName = "Warning";
    action = ValidationAction::DO_LOG_DEBUG; // -V1048
  }
  else if (VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT == messageSeverity)
  {
    serverityName = "Info";
    action = ValidationAction::DO_LOG_DEBUG; // -V1048
  }
  else if (VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT == messageSeverity)
  {
    serverityName = "Verbose";
    action = ValidationAction::DO_LOG_DEBUG; // -V1048
  }
  if (VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT == messageType)
  {
    typeName = "Validation";
  }
  else if (VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT == messageType)
  {
    typeName = "Performance";
  }
  else if (VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT == messageType)
  {
    typeName = "General";
  }

#if VULKAN_VALIDATION_COLLECT_CALLER > 0
  if (ExecutionContext::dbg_get_active_instance())
  {
    debug("vulkan: triggered in execution context by %s", ExecutionContext::dbg_get_active_instance()->getCurrentCmdCaller());
  }
#endif

  String message(2048, "vulkan: %s: %s: %s (%u): %s", typeName, serverityName, pCallbackData->pMessageIdName,
    pCallbackData->messageIdNumber, pCallbackData->pMessage);
  // TODO: use the extended data of pCallbackData to provide more information

  switch (action)
  {
    case ValidationAction::DO_ASSERT:
    {
      if (api_state.allowAssertOnValidationFail)
      {
        drv3d_vulkan::generateFaultReport();
        G_ASSERTF(!"VULKAN VALIDATION FAILED", "%s", message);
      }
      else
        logerr("%s", message);
    }
    break;
    case ValidationAction::DO_LOG_ERROR:
    {
      logerr("%s", message);
    }
    break;
    case ValidationAction::DO_LOG_DEBUG:
    {
      debug("%s", message);
    }
    break;
  }
  return VK_FALSE;
}
#endif

TextureInterfaceBase *drv3d_vulkan::allocate_texture(int res_type, uint32_t cflg)
{
  OSSpinlockScopedLock lock{api_state.texturePoolGuard};
  return api_state.texturePool.allocate(res_type, cflg);
}

void drv3d_vulkan::free_texture(TextureInterfaceBase *texture)
{
  OSSpinlockScopedLock lock{api_state.texturePoolGuard};
  api_state.texturePool.free(texture);
}

void drv3d_vulkan::free_buffer(GenericBufferInterface *buffer)
{
  OSSpinlockScopedLock lock{api_state.bufferPoolGuard};
  api_state.bufferPool.free(buffer);
}

Device &drv3d_vulkan::get_device() { return api_state.device; }
WindowState &drv3d_vulkan::get_window_state() { return api_state.windowState; }

bool drv3d_vulkan::is_global_lock_acquired() { return tlsGlobalLockAcquired > 0; }

void drv3d_vulkan::on_front_flush() { api_state.state.onFrontFlush(); }

ShaderProgramDatabase &drv3d_vulkan::get_shader_program_database() { return api_state.shaderProgramDatabase; }

void drv3d_vulkan::set_last_error(VkResult error) { tlsLastErrorCode = error; }

bool d3d::is_inited() { return api_state.isInitialized && api_state.initVideoDone; }

bool d3d::init_driver()
{
  if (d3d::is_inited())
  {
    logerr("Driver is already created");
    return false;
  }
  api_state.isInitialized = true;
  return true;
}

void d3d::release_driver()
{
  TEXQL_SHUTDOWN_TEX();
  tql::termTexStubs();
  api_state.releaseAll();
  api_state.isInitialized = false;
  // don't call this, releaseAll allready cleans this up
  // term_immediate_cb();
}

static bool create_output_window(void *hinst, main_wnd_f *wnd_proc, const char *wcname, int ncmdshow, void *&mainwnd, void *renderwnd,
  void *hicon, const char *title, Driver3dInitCallback *cb)
{
  api_state.windowState.set(hinst, wcname, ncmdshow, mainwnd, renderwnd, hicon, title, *(void **)&wnd_proc);
  api_state.windowState.getRenderWindowSettings(cb);

  if (dgs_get_window_mode() == WindowMode::FULLSCREEN_EXCLUSIVE)
    os_set_display_mode(api_state.windowState.settings.resolutionX, api_state.windowState.settings.resolutionY);

  if (!api_state.windowState.setRenderWindowParams())
    return false;
  mainwnd = api_state.windowState.getMainWindow();
  return true;
}

using UpdateGpuDriverConfigFunc = void (*)(GpuDriverConfig &);
extern UpdateGpuDriverConfigFunc update_gpu_driver_config;

void update_vulkan_gpu_driver_config(GpuDriverConfig &gpu_driver_config)
{
  auto &deviceProps = api_state.device.getDeviceProperties();

  gpu_driver_config.primaryVendor = deviceProps.vendorId;
  gpu_driver_config.deviceId = deviceProps.properties.deviceID;
  gpu_driver_config.integrated = deviceProps.properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU;
  for (int i = 0; i < 4; ++i)
    gpu_driver_config.driverVersion[i] = deviceProps.driverVersionDecoded[i];
}

bool d3d::init_video(void *hinst, main_wnd_f *wnd_proc, const char *wcname, int ncmdshow, void *&mainwnd, void *renderwnd, void *hicon,
  const char *title, Driver3dInitCallback *cb)
{
  api_state.initVideoDone = false;
  const DataBlock *videoCfg = ::dgs_get_settings()->getBlockByNameEx("video");
  const DataBlock *vkCfg = ::dgs_get_settings()->getBlockByNameEx("vulkan");
#if VK_EXT_debug_report || VK_EXT_debug_utils
  api_state.suppressedMessage = compile_supressed_message_set(vkCfg);
#endif

  const char *lib_names[] = {
    vkCfg->getStr("driverName", VULKAN_LIB_NAME),
#if _TARGET_ANDROID
    "libvulkan.so",
    "libvulkan.so.1",
#endif
  };

  bool lib_loaded = false;
  for (const char *lib_nm : lib_names)
  {
    lib_loaded = api_state.loader.load(lib_nm, vkCfg->getBool("validateLoaderLibrary", true));
    if (lib_loaded)
      break;
    api_state.loader.unload();
  }
  if (!lib_loaded)
  {
    tlsLastErrorCode = VK_ERROR_INITIALIZATION_FAILED;
    return false;
  }

  debug("vulkan: pipeline state size %u", sizeof(PipelineState));
  debug("vulkan: execution state size %u", sizeof(ExecutionState));

  int debugLevel = vkCfg->getInt("debugLevel", 0);
  api_state.allowAssertOnValidationFail = vkCfg->getBool("allowAssertOnValidationFail", false);
  const DataBlock *extensionListNames = vkCfg->getBlockByName("extensions");

  auto instanceExtensionSet = build_instance_extension_list(api_state.loader, extensionListNames, debugLevel, cb);

  bool hasAllInstExt = has_all_required_instance_extension(instanceExtensionSet, [](const char *name) //
    { debug("vulkan: Missing required instance extension %s", name); });
  if (!hasAllInstExt)
  {
    tlsLastErrorCode = VK_ERROR_INCOMPATIBLE_DRIVER;
    debug("vulkan: One ore more required instance extensions are missing");
    return false;
  }

  debug("vulkan: Used Instance Extensions...");
  for (auto &&name : instanceExtensionSet)
  {
    debug("vulkan: ...%s...", name);
  }

  auto enableRenderLayer = vkCfg->getBool("enableRenderDocLayer", false);
  auto layerList = build_instance_layer_list(api_state.loader, vkCfg->getBlockByName("layers"), debugLevel, enableRenderLayer);
  eastl::vector<const char *> layerNames;
  layerNames.reserve(layerList.size());
  for (auto &&layer : layerList)
    layerNames.push_back(layer.layerName);

  auto appInfo = create_app_info(vkCfg);

  debug("vulkan: Reporting to Vulkan: pAppName=%s appVersion=0x%08X pEngineName=%s engineVersion=0x%08X "
        "apiVersion=0x%08X...",
    appInfo.pApplicationName, appInfo.applicationVersion, appInfo.pEngineName, appInfo.engineVersion, appInfo.apiVersion);

  VkInstanceCreateInfo instanceCreateInfo;
  instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  instanceCreateInfo.pNext = nullptr;
  instanceCreateInfo.flags = 0;
  instanceCreateInfo.pApplicationInfo = &appInfo;
  instanceCreateInfo.ppEnabledExtensionNames = instanceExtensionSet.data();
  instanceCreateInfo.enabledExtensionCount = instanceExtensionSet.size();
  instanceCreateInfo.enabledLayerCount = static_cast<uint32_t>(layerNames.size());
  instanceCreateInfo.ppEnabledLayerNames = layerNames.data();

  if (!api_state.instance.init(api_state.loader, instanceCreateInfo))
  {
    debug("vulkan: Failed to create Vulkan instance");
    return false;
  }

  Tab<VulkanPhysicalDeviceHandle> physicalDevices = api_state.instance.getAllDevices();
  if (physicalDevices.empty())
  {
    debug("vulkan: No devices found, trying with VK_LAYER_NV_optimus");
    layerNames.push_back("VK_LAYER_NV_optimus");
    instanceCreateInfo.enabledLayerCount = static_cast<uint32_t>(layerNames.size());
    instanceCreateInfo.ppEnabledLayerNames = layerNames.data();
    api_state.instance.shutdown();
    if (!api_state.instance.init(api_state.loader, instanceCreateInfo))
    {
      debug("vulkan: Failed to create Vulkan instance");
      return false;
    }

    physicalDevices = api_state.instance.getAllDevices();
  }


  // check if all extensions that are required where loaded correctly
  // usually this should never happen as the loaded exposes those
  bool hasAllRequiredExtensionss = instance_has_all_required_extensions(api_state.instance, [](const char *name) //
    { debug("vulkan: Missing required instance extension %s", name); });

  if (!hasAllRequiredExtensionss)
  {
    tlsLastErrorCode = VK_ERROR_INCOMPATIBLE_DRIVER;
    debug("vulkan: Some required instance extensions where not loaded (broken loader)");
    return false;
  }

#if VK_EXT_debug_report
  api_state.debugHandle = create_debug_sink(api_state.instance, debugLevel, api_state.msgCallback, &api_state);
#endif
#if VK_EXT_debug_utils
  api_state.debugHandle2 = create_debug_sink_2(api_state.instance, debugLevel, api_state.msgCallback2, &api_state);
#endif

  if (physicalDevices.empty())
  {
    tlsLastErrorCode = VK_ERROR_INCOMPATIBLE_DRIVER;
    debug("vulkan: No Vulkan compatible devices found");
    return false;
  }

  eastl::vector<PhysicalDeviceSet> physicalDeviceSet;
  physicalDeviceSet.resize(physicalDevices.size());
  for (int i = 0; i < physicalDeviceSet.size(); ++i)
    physicalDeviceSet[i].init(api_state.instance, physicalDevices[i]);

  eastl::sort(begin(physicalDeviceSet), end(physicalDeviceSet),
    [preferIGpu = videoCfg->getBool("preferiGPU", false)](const PhysicalDeviceSet &l, const PhysicalDeviceSet &r) {
      if (preferIGpu && l.properties.deviceType != r.properties.deviceType)
        return l.properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU;
      return r.score < l.score;
    });

#if _TARGET_ANDROID
  // On Android window is created by OS and set on APP_CMD_INIT_WINDOW.
  // Window can be temporarily destroyed due to losing app focus.
  // Therefore we are waiting either until we get a window to work with
  // or app terminate.
  const bool initialWindowIsNull = !::win32_get_main_wnd();
  if (initialWindowIsNull)
  {
    debug("vulkan: Android window is null, waiting for new window");
    while (!::win32_get_main_wnd())
    {
      dagor_idle_cycle(false);
      sleep_msec(250);
    }
    debug("vulkan: Android window obtained, continuing initialization");
  }
#endif

  // create draw window now, so we can determine which device
  // is able to render to the window surface
  if (!create_output_window(hinst, wnd_proc, wcname, ncmdshow, mainwnd, renderwnd, hicon, title, cb))
  {
    tlsLastErrorCode = VK_ERROR_INITIALIZATION_FAILED;
    debug("vulkan: Failed to create output window");
    return false;
  }

  SwapchainMode swapchainMode;
  swapchainMode.surface = init_window_surface(api_state.instance);
  if (is_null(swapchainMode.surface))
  {
    debug("vulkan: Unable to create surface definition of the output window");
    return false;
  }

  swapchainMode.enableSrgb = false;
  swapchainMode.extent.width = api_state.windowState.settings.resolutionX;
  swapchainMode.extent.height = api_state.windowState.settings.resolutionY;
  swapchainMode.setPresentModeFromConfig();

  VkPhysicalDeviceFeatures featureList;
  VkPhysicalDeviceFeatures2KHR featureListExt = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2_KHR, nullptr};
  Tab<const char *> usedDeviceExtension;
  StaticTab<VkDeviceQueueCreateInfo, uint32_t(DeviceQueueType::COUNT)> queueCreateInfo;

  VkDeviceCreateInfo deviceCreateInfo;
  deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  deviceCreateInfo.pNext = nullptr;
  deviceCreateInfo.flags = 0;
  deviceCreateInfo.pQueueCreateInfos = queueCreateInfo.data();
  deviceCreateInfo.pEnabledFeatures = &featureList;
  deviceCreateInfo.enabledLayerCount = instanceCreateInfo.enabledLayerCount;
  deviceCreateInfo.ppEnabledLayerNames = instanceCreateInfo.ppEnabledLayerNames;

  auto deviceCfg = get_device_config(vkCfg);
  auto enabledDeviceExtensions = get_enabled_device_extensions(extensionListNames);

  DeviceQueueGroup::Info queueGroupInfo;
  carray<float, uint32_t(DeviceQueueType::COUNT)> prio;
  for (int i = 0; i < prio.size(); ++i)
    prio[i] = 1.f;

  debug("vulkan: listing %u available devices", physicalDeviceSet.size());

  for (auto &&device : physicalDeviceSet)
  {
    debug("vulkan: device %u [score %u] : %s, %s", &device - &physicalDeviceSet[0], device.score, device.properties.deviceName,
      PhysicalDeviceSet::nameOf(device.properties.deviceType));
  }

  for (auto &&device : physicalDeviceSet)
  {
    debug("vulkan: === detailed info dump for device %u", &device - &physicalDeviceSet[0]);
    device.print();
    debug("vulkan: === detailed info dump for device %u end", &device - &physicalDeviceSet[0]);
  }

  uint32_t usePhysicalDeviceIndex = vkCfg->getInt("usePhysicalDeviceIndex", -1);
  if (usePhysicalDeviceIndex != -1)
  {
    if (usePhysicalDeviceIndex >= physicalDeviceSet.size())
    {
      debug("vulkan: configuration asked for non existent device index %u, fallback to auto", usePhysicalDeviceIndex);
      usePhysicalDeviceIndex = -1;
    }
    else
      debug("vulkan: using config supplied device %u", usePhysicalDeviceIndex);
  }
  else
    debug("vulkan: selecting device by internal score order");

  for (auto &&device : physicalDeviceSet)
  {
    uint32_t index = &device - &physicalDeviceSet[0];
    if ((usePhysicalDeviceIndex != -1) && (usePhysicalDeviceIndex != index))
      continue;

    debug("vulkan: considering %u: %s", index, device.properties.deviceName);

    featureList = device.features;
    if (!check_and_update_device_features(featureList))
    {
      debug("vulkan: Device rejected, does not support the required features");
      continue;
    }

    // update structs chain data & enabled features as we reusing them for multiple devices
    deviceCreateInfo.pNext = nullptr;
    featureListExt.pNext = nullptr;
    deviceCreateInfo.pEnabledFeatures = &featureList;
    // enable features that live in extended descriptions
    device.enableExtendedFeatures(deviceCreateInfo, featureListExt);

    if (!queueGroupInfo.init(api_state.instance, device.device, swapchainMode.surface, device.queueFamilyProperties))
    {
      debug("vulkan: Device rejected, does not support the required queue types");
      continue;
    }

    auto dsFormat = get_default_depth_stencil_format(api_state.instance, device.device);
    if (VK_FORMAT_UNDEFINED == dsFormat)
    {
      debug("vulkan: Device rejected, no suitable depth stencil format supported for depth "
            "stencil attachments");
      continue;
    }
    FormatStore::setRemapDepth24BitsToDepth32Bits(dsFormat == VK_FORMAT_D32_SFLOAT_S8_UINT);

    deviceCreateInfo.enabledExtensionCount =
      fill_device_extension_list(usedDeviceExtension, enabledDeviceExtensions, device.extensions);

    usedDeviceExtension.resize(deviceCreateInfo.enabledExtensionCount);
    usedDeviceExtension.shrink_to_fit();

    auto injectExtensions = [&](const char *injection) {
      char *desiredRendererExtensions = const_cast<char *>(injection);
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

        bool dupe = eastl::find_if(usedDeviceExtension.cbegin(), usedDeviceExtension.cend(), [currentExtension](const char *ext) {
          return strcmp(ext, currentExtension) == 0;
        }) != usedDeviceExtension.cend();

        if (!dupe)
          usedDeviceExtension.push_back(currentExtension);
      }

      deviceCreateInfo.enabledExtensionCount = usedDeviceExtension.size();
    };

    if (cb)
      injectExtensions(cb->desiredRendererDeviceExtensions());

    eastl::string lateOsSpecificExts = os_get_additional_ext_requirements(device.device, device.extensions);
    if (!lateOsSpecificExts.empty())
      injectExtensions(lateOsSpecificExts.data());

    deviceCreateInfo.ppEnabledExtensionNames = usedDeviceExtension.data();

    // reject device if not all essential extensions are supported
    if (!has_all_required_extension(make_span_const(usedDeviceExtension).first(deviceCreateInfo.enabledExtensionCount)))
    {
      debug("vulkan: Device rejected, not all required extensions are supported");
      continue;
    }

    queueCreateInfo = build_queue_create_info(queueGroupInfo, prio.data());
    deviceCreateInfo.queueCreateInfoCount = queueCreateInfo.size();

    DeviceMemoryReport::CallbackDesc deviceMemoryReportCb;
    if (api_state.device.getMemoryReport().init(device, deviceMemoryReportCb))
      chain_structs(deviceCreateInfo, deviceMemoryReportCb);

    debug("vulkan: Device requirements check passed, creating device...");
    if (api_state.device.init(api_state.instance, deviceCfg, device, deviceCreateInfo, swapchainMode, queueGroupInfo))
    {
      debug("vulkan: ...device created");
      api_state.deviceName = device.properties.deviceName;
      if (videoCfg->getBool("preferiGPU", false) && device.properties.deviceType != VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
        logwarn("vulkan: Despite the preferiGPU flag being enabled, the dedicated GPU is used!");
      break;
    }
  }

  if (!api_state.device.isInitialized())
  {
    VULKAN_LOG_CALL(api_state.instance.vkDestroySurfaceKHR(api_state.instance.get(), swapchainMode.surface, nullptr));
    swapchainMode.surface = VulkanNullHandle();
    debug("vulkan: Unable to create any device");
    // don't forget to set the error code to something else than success
    tlsLastErrorCode = VK_ERROR_INCOMPATIBLE_DRIVER;
    return false;
  }

  {
    // load the driver pipeline cache from disk
    PipelineCacheInFile pipelineCacheFile;
    api_state.device.loadPipelineCache(pipelineCacheFile);
  }

  api_state.adjustCaps();
  api_state.shaderProgramDatabase.init(api_state.device.getContext());

  {
    // vulkan have limited memory swapping support now
    // going over quota is most of time an out of memory crash
    // put overdraft in driver reserve to decrease total quota
    // while keeping overdraft correctly working
    DataBlock *writableCfg = const_cast<DataBlock *>(::dgs_get_settings());
    if (writableCfg && !writableCfg->isEmpty())
    {
      const DataBlock *texStreamingCfg = writableCfg->getBlockByNameEx("texStreaming");
      int overdraftMB = texStreamingCfg->getInt("maxQuotaOverdraftMB", 256);
      int driverReserveKB = texStreamingCfg->getInt("driverReserveKB", overdraftMB << 10);

      writableCfg->addBlock("texStreaming")->setInt("driverReserveKB", driverReserveKB);

      debug("vulkan: Override texStreaming/driverReserveKB:i = %@", driverReserveKB);
    }
    else
      debug("vulkan: Cannot override texStreaming/driverReserveKB. Settings is not inited yet.");
  }

  update_gpu_driver_config = update_vulkan_gpu_driver_config;

  api_state.initVideoDone = true;

  // tex streaming need to know that driver is inited
  tql::initTexStubs();
  // we don't care was they used or not
  for (int i = 0; i < tql::texStub.size(); i++)
    ((BaseTex *)tql::texStub[i])->setUsed();

  // state inited to NULL target by default, reset to BB
  d3d::set_render_target();

  if (enableRenderLayer)
    api_state.docCaptureModule = eastl::make_optional<RenderDocCaptureModule>();

  debug("vulkan: init_video done");
  return true;
}

void d3d::window_destroyed(void *handle)
{
  // may be called after shutdown of device, so make sure we don't crash
  if (api_state.device.isInitialized())
  {
    api_state.globalLock.lock();

    if (handle == api_state.windowState.getMainWindow())
    {
      api_state.windowState.closeWindow();
      SwapchainMode newMode(api_state.device.swapchain.getMode());
      newMode.surface = VulkanNullHandle();
      api_state.device.swapchain.setMode(newMode);
    }
    api_state.globalLock.unlock();
  }
}

void d3d::reserve_res_entries(bool strict_max, int max_tex, int max_vs, int max_ps, int max_vdecl, int max_vb, int max_ib,
  int max_stblk)
{
  {
    OSSpinlockScopedLock lock{api_state.texturePoolGuard};
    api_state.texturePool.reserve(max_tex);
  }
  {
    OSSpinlockScopedLock lock{api_state.bufferPoolGuard};
    api_state.bufferPool.reserve(max_vb + max_ib);
  }
  G_UNUSED(strict_max);
  G_UNUSED(max_tex);
  G_UNUSED(max_vs);
  G_UNUSED(max_ps);
  G_UNUSED(max_vdecl);
  G_UNUSED(max_stblk);
}

void d3d::get_max_used_res_entries(int &max_tex, int &max_vs, int &max_ps, int &max_vdecl, int &max_vb, int &max_ib, int &max_stblk)
{
  {
    OSSpinlockScopedLock lock{api_state.texturePoolGuard};
    max_tex = api_state.texturePool.capacity();
  }
  {
    max_vb = max_ib = 0;
    OSSpinlockScopedLock lock{api_state.bufferPoolGuard};
    api_state.bufferPool.iterateAllocated([&](auto buffer) {
      int flags = buffer->getFlags();
      if ((flags & SBCF_BIND_MASK) == SBCF_BIND_INDEX)
        ++max_ib;
      else
        ++max_vb;
    });
  }

  auto total = max_vb + max_ib;
  max_vb *= api_state.bufferPool.capacity();
  max_ib *= api_state.bufferPool.capacity();
  max_vb /= total;
  max_ib /= total;
  G_UNUSED(max_vs);
  G_UNUSED(max_ps);
  G_UNUSED(max_vdecl);
  G_UNUSED(max_stblk);
}

void d3d::get_cur_used_res_entries(int &max_tex, int &max_vs, int &max_ps, int &max_vdecl, int &max_vb, int &max_ib, int &max_stblk)
{
  {
    OSSpinlockScopedLock lock{api_state.texturePoolGuard};
    max_tex = api_state.texturePool.size();
  }
  {
    max_vb = max_ib = 0;
    OSSpinlockScopedLock lock{api_state.bufferPoolGuard};
    api_state.bufferPool.iterateAllocated([&](auto buffer) {
      int flags = buffer->getFlags();
      if ((flags & SBCF_BIND_MASK) == SBCF_BIND_INDEX)
        ++max_ib;
      else
        ++max_vb;
    });
  }
  G_UNUSED(max_vs);
  G_UNUSED(max_ps);
  G_UNUSED(max_vdecl);
  G_UNUSED(max_stblk);
}

const char *d3d::get_driver_name() { return "Vulkan"; }

DriverCode d3d::get_driver_code() { return DriverCode::make(d3d::vulkan); }

const char *d3d::get_device_name() { return drv3d_vulkan::api_state.deviceName.str(); }
const char *d3d::get_last_error() { return drv3d_vulkan::vulkan_error_string(tlsLastErrorCode); }

uint32_t d3d::get_last_error_code() { return tlsLastErrorCode; }

const char *d3d::get_device_driver_version() { return drv3d_vulkan::api_state.niceDeviceDriverVersionString.str(); }

void *d3d::get_device() { return drv3d_vulkan::api_state.device.getDevice(); }

const Driver3dDesc &d3d::get_driver_desc() { return drv3d_vulkan::api_state.driverDesc; }

#include "raytrace/interface.inc.cpp"

int d3d::driver_command(int command, void *par1, void *par2, void *par3)
{
  switch (command)
  {
    case DRV3D_COMMAND_SET_APP_INFO:
    {
      const char *appName = static_cast<const char *>(par1);
      const uint32_t *appVer = static_cast<const uint32_t *>(par2);
      drv3d_vulkan::set_app_info(appName, *appVer);
    }
      return 1;
    case DRV3D_COMMAND_DEBUG_MESSAGE:
      api_state.device.getContext().writeDebugMessage(static_cast<const char *>(par1), reinterpret_cast<intptr_t>(par2),
        reinterpret_cast<intptr_t>(par3));
      return 1;
    case DRV3D_COMMAND_GET_TIMINGS:
#if VULKAN_RECORD_TIMING_DATA
      *reinterpret_cast<Drv3dTimings *>(par1) = api_state.device.getContext().getTiming(reinterpret_cast<uintptr_t>(par2));
      return FRAME_TIMING_HISTORY_LENGTH;
#else
      return 0;
#endif
    case DRV3D_GET_SHADER_CACHE_UUID:
      if (par1)
      {
        auto &properties = api_state.device.getDeviceProperties().properties;
        G_STATIC_ASSERT(sizeof(uint64_t) * 2 == sizeof(properties.pipelineCacheUUID));
        memcpy(par1, properties.pipelineCacheUUID, sizeof(properties.pipelineCacheUUID));
        return 1;
      }
      break;
    case DRV3D_COMMAND_AFTERMATH_MARKER: api_state.device.getContext().placeAftermathMarker((const char *)par1); break;
    case DRV3D_COMMAND_SET_VS_DEBUG_INFO:
    case DRV3D_COMMAND_SET_PS_DEBUG_INFO:
      api_state.shaderProgramDatabase.setShaderDebugName(ShaderID(*(int *)par1), (const char *)par2);
      break;
    case DRV3D_COMMAND_D3D_FLUSH:
      // driver can merge multiple queue submits
      // and GPU will timeout itself if workload is too long
      // so use wait instead of simple flush
      {
        TIME_PROFILE(vulkan_d3d_flush);
        int64_t flushTime = 0;
        {
          ScopedTimer timer(flushTime);
          api_state.device.getContext().wait();
        }
        if (par1)
          debug("vulkan: flush %u:%s taken %u us", (uintptr_t)par2, (const char *)par1, flushTime);
      }
      break;
    case DRV3D_COMMAND_FLUSH_STATES:
      api_state.state.flushGraphics(api_state.device.getContext());
      api_state.state.flushCompute(api_state.device.getContext());
      break;
      //  case DRV3D_COMMAND_GETALLTEXS:
      //  case DRV3D_COMMAND_SET_STAT3D_HANDLER:
      //  case DRV3D_COMMAND_GETTEXTUREMEM:
      //    break;
      //  case DRV3D_COMMAND_GETVISIBILITYBEGIN:  // not supported
      //  case DRV3D_COMMAND_GETVISIBILITYEND:    // not supported
      //  case DRV3D_COMMAND_GETVISIBILITYCOUNT:  // not supported
      //    break;
      //  case DRV3D_COMMAND_ENABLEDEBUGTEXTURES:
      //  case DRV3D_COMMAND_DISABLEDEBUGTEXTURES:
      //    break;
      //  case DRV3D_COMMAND_ENABLE_MT: // ignore
      //  case DRV3D_COMMAND_DISABLE_MT:// ignore
      //    break;
    case DRV3D_COMMAND_COMPILE_PIPELINE:
    {
      G_ASSERTF(is_global_lock_acquired(), "pipeline compilation requires gpu lock");
      const ShaderStage pipelineType = (ShaderStage)(uintptr_t)par1;

      switch (pipelineType)
      {
        case STAGE_PS:
        {
          const VkPrimitiveTopology topology = translate_primitive_topology_to_vulkan((uintptr_t)par2);

          api_state.state.flushGraphics(api_state.device.getContext());

          api_state.device.getContext().compileGraphicsPipeline(topology);
          break;
        }

        case STAGE_CS:
        {
          api_state.state.flushCompute(api_state.device.getContext());

          api_state.device.getContext().compileComputePipeline();
          break;
        }

        default:
        {
          G_ASSERTF(false, "Unsupported pipeline type(%u) for compilation", (unsigned)pipelineType);
          break;
        }
      }

      break;
    }
    case DRV3D_COMMAND_ENTER_RESOURCE_LOCK_CS:
    case DRV3D_COMMAND_LEAVE_RESOURCE_LOCK_CS: break;
    case DRV3D_COMMAND_SET_PIPELINE_COMPILATION_TIME_BUDGET:
      api_state.device.getContext().setPipelineCompilationTimeBudget(reinterpret_cast<uintptr_t>(par1));
      break;
    case DRV3D_COMMAND_GET_PIPELINE_COMPILATION_QUEUE_LENGTH:
      if (par1)
      {
        uint32_t *frameLatency = static_cast<uint32_t *>(par1);
        *frameLatency = MAX_RETIREMENT_QUEUE_ITEMS;
      }
      return api_state.device.getContext().getPiplineCompilationQueueLength();
      break;
    case DRV3D_COMMAND_ACQUIRE_OWNERSHIP: api_state.globalLock.lock(); break;
    case DRV3D_COMMAND_RELEASE_OWNERSHIP: api_state.globalLock.unlock(); break;

    // TriggerMultiFrameCapture causes crashes on some android devices but 1 frame capture works correctly
    // so free command slot it used for that
    case DRV3D_COMMAND_PIX_GPU_CAPTURE_NEXT_FRAMES:
      if (api_state.docCaptureModule)
        api_state.docCaptureModule->triggerCapture(reinterpret_cast<uintptr_t>(par3));
      return 0;
      break;

    // Must be used in conjuction with DRV3D_COMMAND_PIX_GPU_END_CAPTURE
    case DRV3D_COMMAND_PIX_GPU_BEGIN_CAPTURE:
      if (api_state.docCaptureModule)
        api_state.docCaptureModule->beginCapture();
      return 0;
      break;

    // Must be used in conjuction with DRV3D_COMMAND_PIX_GPU_BEGIN_CAPTURE
    case DRV3D_COMMAND_PIX_GPU_END_CAPTURE:
      if (api_state.docCaptureModule)
        api_state.docCaptureModule->endCapture();
      return 0;
      break;

      //  case DRV3D_COMMAND_PREALLOCATE_RT:
      //    break;
      //  case DRV3D_COMMAND_SUSPEND:   // ignore
      //    break;
      //  case DRV3D_COMMAND_RESUME:  // ignore
      //    break;
      //  case DRV3D_COMMAND_GET_SUSPEND_COUNT: // ignore
      //    break;
      //  case DRV3D_COMMAND_GET_MEM_USAGE:
      //  case DRV3D_COMMAND_GET_AA_LEVEL:
      //    break;
      //  case DRV3D_COMMAND_THREAD_ENTER:
      //    break;
      //  case DRV3D_COMMAND_THREAD_LEAVE:
      //    break;
      //  case DRV3D_COMMAND_GET_GPU_FRAME_TIME:  // ignore
      //    break;
      //  case DRV3D_COMMAND_ENTER_RESOURCE_LOCK_CS:
      //  case DRV3D_COMMAND_LEAVE_RESOURCE_LOCK_CS:
      //    break;
      //  case DRV3D_COMMAND_INVALIDATE_STATES: // ignore
      //    break;
    case D3V3D_COMMAND_TIMESTAMPFREQ:
      if (api_state.device.hasGpuTimestamps())
      {
        *reinterpret_cast<uint64_t *>(par1) = api_state.device.getGpuTimestampFrequency();
        return 1;
      }
      break;
    case D3V3D_COMMAND_TIMESTAMPISSUE:
      if (api_state.device.hasGpuTimestamps())
      {
        TimestampQuery **q = static_cast<TimestampQuery **>(par1);
        if (!*q)
        {
          *q = api_state.device.timestamps.allocate();
        }
        api_state.device.getContext().insertTimestampQuery(*q);
      }
      return 1;
      break;
    case D3V3D_COMMAND_TIMESTAMPGET:
      if (par1 && api_state.device.timestamps.isReady(static_cast<TimestampQuery *>(par1)))
      {
        *reinterpret_cast<uint64_t *>(par2) = (static_cast<TimestampQuery *>(par1))->result;

// cache last timestamp result to fake timestamp calibration if it is not available
#if VK_EXT_calibrated_timestamps
        if (!api_state.device.getVkDevice().hasExtension<CalibratedTimestampsEXT>())
#endif
        {
          api_state.lastGpuTimeStamp = (static_cast<TimestampQuery *>(par1))->result;
        }

        return 1;
      }
      break;
    case D3V3D_COMMAND_TIMECLOCKCALIBRATION:
    {
      uint64_t pTimestamps[2] = {0, 0};
#if VK_EXT_calibrated_timestamps
      if (api_state.device.getVkDevice().hasExtension<CalibratedTimestampsEXT>())
      {
        VkCalibratedTimestampInfoEXT timestampsInfos[2];
        timestampsInfos[0].sType = timestampsInfos[1].sType = VK_STRUCTURE_TYPE_CALIBRATED_TIMESTAMP_INFO_EXT;
        timestampsInfos[0].pNext = timestampsInfos[1].pNext = NULL;
        timestampsInfos[0].timeDomain = VK_TIME_DOMAIN_DEVICE_EXT;

#if _TARGET_PC_WIN
        timestampsInfos[1].timeDomain = VK_TIME_DOMAIN_QUERY_PERFORMANCE_COUNTER_EXT;
#else
        timestampsInfos[1].timeDomain = VK_TIME_DOMAIN_CLOCK_MONOTONIC_EXT;
#endif

        uint64_t pMaxDeviation[2];

        VULKAN_LOG_CALL(api_state.device.getVkDevice().vkGetCalibratedTimestampsEXT(api_state.device.getDevice(), 2, timestampsInfos,
          pTimestamps, pMaxDeviation));

        // report fake CPU timestamp in case of missing result for CPU time domain
        if (pTimestamps[1] == 0)
          pTimestamps[1] = ref_time_ticks();
      }
      else
#endif
      {
        // report fake CPU & GPU timestamp if calibrated timestamps missing fully
        pTimestamps[1] = ref_time_ticks();
        pTimestamps[0] = api_state.lastGpuTimeStamp;
      }

      *reinterpret_cast<uint64_t *>(par1) = pTimestamps[1]; // cpu
      *reinterpret_cast<uint64_t *>(par2) = pTimestamps[0];

#if _TARGET_PC_WIN
      if (par3)
        *(int *)par3 = DRV3D_CPU_FREQ_TYPE_QPC;
#else
      // Make notice that CPU Timestamp is same as dagor's `ref_time_ticks` function on switch, android and linux
      // ref_time_ticks is same as profile_ref_ticks on Switch/iOS
      // however, to clarify we use explicit DRV3D_CPU_FREQ_NSEC, rather than DRV3D_CPU_FREQ_REF (but it will be practically same!)
      if (par3)
        *(int *)par3 = DRV3D_CPU_FREQ_NSEC;
#endif

      return 1;
      break;
    }
    case DRV3D_COMMAND_RELEASE_QUERY:
      if (par1 && *static_cast<TimestampQuery **>(par1))
      {
        api_state.device.timestamps.release(*static_cast<TimestampQuery **>(par1));
        *static_cast<TimestampQuery **>(par1) = 0;
      }
      break;

      //  case DRV3D_COMMAND_GET_SECONDARY_BACKBUFFER:
      //    break;
    case DRV3D_COMMAND_GET_VENDOR: return drv3d_vulkan::api_state.device.getDeviceVendor(); break;
    case DRV3D_COMMAND_GET_RESOLUTION:
      if (par1 && par2)
      {
        *((int *)par1) = api_state.windowState.settings.resolutionX;
        *((int *)par2) = api_state.windowState.settings.resolutionY;
        return 1;
      }
      break;
    case DRV3D_COMMAND_GET_FRAMERATE_LIMITING_FACTOR:
    {
      return api_state.lastLimitingFactor;
    }
    case DRV3D_COMMAND_LOAD_PIPELINE_CACHE:
    {
      PipelineCacheInFile cache((const char *)par1);
      api_state.device.loadPipelineCache(cache);
      break;
    }
    case DRV3D_COMMAND_SAVE_PIPELINE_CACHE:
    {
      api_state.globalLock.lock();
      api_state.device.getContext().wait();
      PipelineCacheOutFile pipelineCacheFile;
      api_state.device.storePipelineCache(pipelineCacheFile);
      api_state.globalLock.unlock();
      break;
    }
    case DRV3D_COMMAND_GET_VIDEO_MEMORY_BUDGET:
    {
      uint32_t totalMemory = api_state.device.getDeviceProperties().getAvailableVideoMemoryKb();
      if (par1)
        *reinterpret_cast<uint32_t *>(par1) = totalMemory;
      if (par2 || par3)
      {
        uint32_t availableMemory = api_state.device.getCurrentAvailableMemoryKb();
        availableMemory = min<uint32_t>(totalMemory, availableMemory);
        if (availableMemory == 0)
          return 0;
        if (par2)
          *reinterpret_cast<uint32_t *>(par2) = availableMemory;
        if (par3)
          *reinterpret_cast<uint32_t *>(par3) = totalMemory - availableMemory; // it is always positive
      }
      return 1;
    }

    case DRV3D_COMMAND_GET_INSTANCE: *(VkInstance *)par1 = drv3d_vulkan::api_state.device.getInstance(); return 1;
    case DRV3D_COMMAND_GET_PHYSICAL_DEVICE: *(VkPhysicalDevice *)par1 = drv3d_vulkan::api_state.device.getPhysicalDevice(); return 1;
    case DRV3D_COMMAND_GET_QUEUE_FAMILY_INDEX:
      *(uint32_t *)par1 = drv3d_vulkan::api_state.device.getQueue(DeviceQueueType::GRAPHICS).getFamily();
      return 1;
    case DRV3D_COMMAND_GET_QUEUE_INDEX:
      *(uint32_t *)par1 = drv3d_vulkan::api_state.device.getQueue(DeviceQueueType::GRAPHICS).getIndex();
      return 1;
    case DRV3D_COMMAND_MAKE_TEXTURE:
    {
      const Drv3dMakeTextureParams *makeParams = (const Drv3dMakeTextureParams *)par1;
      *(Texture **)par2 = drv3d_vulkan::api_state.device.wrapVKImage(*(VkImage *)makeParams->tex, makeParams->currentState,
        makeParams->w, makeParams->h, makeParams->layers, makeParams->mips, makeParams->name, makeParams->flg);
      return 1;
    }
    case DRV3D_COMMAND_REGISTER_ONE_TIME_FRAME_EXECUTION_EVENT_CALLBACKS:
      drv3d_vulkan::api_state.device.getContext().registerFrameEventsCallback(static_cast<FrameEvents *>(par1), par2);
      return 1;

    case DRV3D_COMMAND_PROCESS_APP_INACTIVE_UPDATE:
    {
      // keep finishing frames when app is minimized/inactive
      // as app can generate resource creation/deletion even if minimized
      // and we must process them (by finishing frames), otherwise we will run out of memory
      // also not finishing frames can cause swapchain to bug out either by not updating on restore,
      // or hanging somewhere inside system (on android)
      api_state.globalLock.lock();
      // keep pre rotated state to avoid glitching system preview
      drv3d_vulkan::api_state.device.getContext().holdPreRotateStateForOneFrame();
      d3d::update_screen();
      api_state.globalLock.unlock();
      return 1;
    }
    case DRV3D_COMMAND_PRE_ROTATE_PASS:
    {
      uintptr_t v = (uintptr_t)par1;
      api_state.device.getContext().startPreRotate((uint32_t)v);
      return api_state.device.swapchain.getPreRotationAngle();
    }
    case DRV3D_COMMAND_PROCESS_PENDING_RESOURCE_UPDATED:
    {
      drv3d_vulkan::Device &device = drv3d_vulkan::get_device();
      // flush draws only if we under global lock, otherwise just use extra memory
      // as it is quite unsafe to flushDraws outside of global lock
      if (is_global_lock_acquired())
        device.getContext().flushDraws();
      else
        device.allocated_upload_buffer_size = 0;
      return 1;
    }
    case DRV3D_COMMAND_GET_WORKER_CPU_CORE:
    {
      drv3d_vulkan::api_state.device.getContext().getWorkerCpuCore((int *)par1, (int *)par2);
      return 1;
    }
    case DRV3D_COMMAND_SWAPPY_SET_TARGET_FRAME_RATE:
    {
      api_state.device.getContext().setSwappyTargetRate(*(int *)par1);
      return 1;
    }
    case DRV3D_COMMAND_SWAPPY_GET_STATUS:
    {
      api_state.device.getContext().getSwappyStatus((int *)par1);
      return 1;
    }
  };

  // silence compiler - we may not use it when some extensions are not compiled in
  G_UNUSED(par3);
  return 0;
}

extern bool dagor_d3d_force_driver_mode_reset;
bool d3d::device_lost(bool *can_reset_now)
{
  if (can_reset_now)
    *can_reset_now = dagor_d3d_force_driver_mode_reset;
  return dagor_d3d_force_driver_mode_reset;
}
static bool device_is_being_reset = false;
bool d3d::is_in_device_reset_now() { return /*device_is_lost != S_OK || */ device_is_being_reset; }

bool d3d::reset_device()
{
  struct RaiiReset
  {
    RaiiReset()
    {
      api_state.globalLock.lock();
      device_is_being_reset = true;
    }
    ~RaiiReset()
    {
      device_is_being_reset = false;
      api_state.globalLock.unlock();
    }
  } raii_reset;

  debug("vulkan: reset device");

  os_restore_display_mode();
  void *oldWindow = api_state.windowState.getMainWindow();
  api_state.windowState.getRenderWindowSettings();

  if (!api_state.windowState.setRenderWindowParams())
    return false;

  SwapchainMode newMode(api_state.device.swapchain.getMode());

  VkExtent2D cext;
  cext.width = api_state.windowState.settings.resolutionX;
  cext.height = api_state.windowState.settings.resolutionY;
  newMode.extent = cext;
  newMode.setPresentModeFromConfig();

  // linux returns pointer to static object field
  // treat this as always changed instead as always same
#if !_TARGET_PC_LINUX
  // reset changed window, we need to update surface
  if (oldWindow != api_state.windowState.getMainWindow())
#endif
  {
    newMode.surface = init_window_surface(api_state.instance);
  }

  // empty mode change will be filtered automatically
  api_state.device.swapchain.setMode(newMode);

  if (api_state.swapchainColorProxyTexture)
  {
    api_state.swapchainColorProxyTexture->setParams(cext.width, cext.height, 1, 1, api_state.swapchainColorProxyTexture->getTexName());
  }
  if (api_state.swapchainDepthStencilProxyTexture)
  {
    api_state.swapchainDepthStencilProxyTexture->setParams(cext.width, cext.height, 1, 1,
      api_state.swapchainDepthStencilProxyTexture->getTexName());
  }

  if (dgs_get_window_mode() == WindowMode::FULLSCREEN_EXCLUSIVE)
    os_set_display_mode(api_state.windowState.settings.resolutionX, api_state.windowState.settings.resolutionY);

  dagor_d3d_force_driver_mode_reset = false;
  return true;
}

static VkImageUsageFlags usage_flags_from_cfg(int cflg, FormatStore fmt)
{
  VkImageUsageFlags flags = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
  if (cflg & TEXCF_RTARGET)
  {
    if (fmt.getAspektFlags() & VK_IMAGE_ASPECT_COLOR_BIT)
      flags |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    else
      flags |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
  }
  if (cflg & TEXCF_UNORDERED)
  {
    flags |= VK_IMAGE_USAGE_STORAGE_BIT;
  }
  if (!(cflg & TEXCF_SYSMEM))
  {
    flags |= VK_IMAGE_USAGE_SAMPLED_BIT;
  }
  return flags;
}

bool d3d::check_texformat(int cflg)
{
  auto fmt = FormatStore::fromCreateFlags(cflg);
  return api_state.device.checkFormatSupport(fmt.asVkFormat(), VK_IMAGE_TYPE_2D, VK_IMAGE_TILING_OPTIMAL,
    usage_flags_from_cfg(cflg, fmt), 0, (cflg & TEXCF_MULTISAMPLED) ? api_state.device.calcMSAAQuality() : VK_SAMPLE_COUNT_1_BIT);
}

bool d3d::issame_texformat(int cflg1, int cflg2)
{
  auto formatA = FormatStore::fromCreateFlags(cflg1);
  auto formatB = FormatStore::fromCreateFlags(cflg2);
  return formatA.asVkFormat() == formatB.asVkFormat();
}

bool d3d::check_cubetexformat(int cflg)
{
  auto fmt = FormatStore::fromCreateFlags(cflg);
  return api_state.device.checkFormatSupport(fmt.asVkFormat(), VK_IMAGE_TYPE_2D, VK_IMAGE_TILING_OPTIMAL,
    usage_flags_from_cfg(cflg, fmt), VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT,
    (cflg & TEXCF_MULTISAMPLED) ? api_state.device.calcMSAAQuality() : VK_SAMPLE_COUNT_1_BIT);
}

bool d3d::issame_cubetexformat(int cflg1, int cflg2) { return issame_texformat(cflg1, cflg2); }

bool d3d::check_voltexformat(int cflg)
{
  auto fmt = FormatStore::fromCreateFlags(cflg);
  VkImageCreateFlags flags = 0;
  if (api_state.device.canRenderTo3D())
  {
    if (cflg & TEXCF_RTARGET)
    {
      flags = VK_IMAGE_CREATE_2D_ARRAY_COMPATIBLE_BIT_KHR;
    }
  }
  else
  {
    // remove flag, as we emulate it
    cflg &= ~TEXCF_RTARGET;
  }
  return api_state.device.checkFormatSupport(fmt.asVkFormat(), VK_IMAGE_TYPE_3D, VK_IMAGE_TILING_OPTIMAL,
    usage_flags_from_cfg(cflg, fmt), flags, (cflg & TEXCF_MULTISAMPLED) ? api_state.device.calcMSAAQuality() : VK_SAMPLE_COUNT_1_BIT);
}

bool d3d::issame_voltexformat(int cflg1, int cflg2) { return issame_texformat(cflg1, cflg2); }

bool d3d::stretch_rect(BaseTexture *src, BaseTexture *dst, RectInt *rsrc, RectInt *rdst)
{
  TextureInterfaceBase *srcTex = cast_to_texture_base(src);
  TextureInterfaceBase *dstTex = cast_to_texture_base(dst);

  Image *srcImg = nullptr;
  Image *dstImg = nullptr;

  if (srcTex)
  {
    srcImg = srcTex->getDeviceImage(false);
    srcTex->setUsed();
  }

  if (dstTex)
    dstImg = dstTex->getDeviceImage(false);

  VkImageBlit blit;
  blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  blit.srcSubresource.mipLevel = 0;
  blit.srcSubresource.baseArrayLayer = 0;
  blit.srcSubresource.layerCount = 1;
  blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  blit.dstSubresource.mipLevel = 0;
  blit.dstSubresource.baseArrayLayer = 0;
  blit.dstSubresource.layerCount = 1;

  if (rsrc)
  {
    blit.srcOffsets[0].x = rsrc->left;
    blit.srcOffsets[0].y = rsrc->top;
    blit.srcOffsets[0].z = 0;
    blit.srcOffsets[1].x = rsrc->right;
    blit.srcOffsets[1].y = rsrc->bottom;
    blit.srcOffsets[1].z = 1;
  }
  else if (srcTex)
  {
    blit.srcOffsets[0].x = 0;
    blit.srcOffsets[0].y = 0;
    blit.srcOffsets[0].z = 0;
    blit.srcOffsets[1] = toOffset(srcTex->getMipmapExtent(0));
    if (blit.dstOffsets[1].z < 1)
      blit.dstOffsets[1].z = 1;
  }
  else
  {
    VkExtent2D size = api_state.device.swapchain.getMode().extent;
    blit.srcOffsets[0].x = 0;
    blit.srcOffsets[0].y = 0;
    blit.srcOffsets[0].z = 0;
    blit.srcOffsets[1].x = size.width;
    blit.srcOffsets[1].y = size.height;
    blit.srcOffsets[1].z = 1;
  }

  if (rdst)
  {
    blit.dstOffsets[0].x = rdst->left;
    blit.dstOffsets[0].y = rdst->top;
    blit.dstOffsets[0].z = 0;
    blit.dstOffsets[1].x = rdst->right;
    blit.dstOffsets[1].y = rdst->bottom;
    blit.dstOffsets[1].z = 1;
  }
  else if (dstTex)
  {
    blit.dstOffsets[0].x = 0;
    blit.dstOffsets[0].y = 0;
    blit.dstOffsets[0].z = 0;
    blit.dstOffsets[1] = toOffset(dstTex->getMipmapExtent(0));
    if (blit.dstOffsets[1].z < 1)
      blit.dstOffsets[1].z = 1;
  }
  else
  {
    VkExtent2D size = api_state.device.swapchain.getMode().extent;
    blit.dstOffsets[0].x = 0;
    blit.dstOffsets[0].y = 0;
    blit.dstOffsets[0].z = 0;
    blit.dstOffsets[1].x = size.width;
    blit.dstOffsets[1].y = size.height;
    blit.dstOffsets[1].z = 1;
  }

  api_state.device.getContext().blitImage(srcImg, dstImg, blit);
  return true;
}

// Texture states setup
unsigned d3d::get_texformat_usage(int cflg, int restype)
{
  auto fmt = FormatStore::fromCreateFlags(cflg);
  VkFormatFeatureFlags features = api_state.device.getFormatFeatures(fmt.asVkFormat());

  unsigned result = 0;
  switch (restype)
  {
    case RES3D_TEX:
    case RES3D_CUBETEX:
    case RES3D_ARRTEX:
    case RES3D_VOLTEX:
      if (features & VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT)
      {
        result |= USAGE_RTARGET;
      }
      if (features & VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BLEND_BIT)
      {
        result |= USAGE_BLEND;
      }
      if (features & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
      {
        result |= USAGE_DEPTH;
      }
      if (features & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT)
      {
        result |= USAGE_TEXTURE;
        result |= USAGE_VERTEXTEXTURE;
        if (features & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)
          result |= USAGE_FILTER;
      }
      if (features & (VK_FORMAT_FEATURE_STORAGE_TEXEL_BUFFER_BIT | VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT))
        result |= USAGE_UNORDERED;
      break;
    case RES3D_SBUF: break;
  }
  return result | USAGE_PIXREADWRITE;
}

static void decode_shader_binary(const uint32_t *native_code, uint32_t, VkShaderStageFlags, Tab<spirv::ChunkHeader> &chunks,
  Tab<uint8_t> &chunk_data)
{
  if (spirv::SPIR_V_BLOB_IDENT == native_code[0])
  {
    InPlaceMemLoadCB crd(native_code + 2, native_code[1]);
    ZlibLoadCB zlib_crd(crd, native_code[1]);
    zlib_crd.readTab(chunks);
    zlib_crd.readTab(chunk_data);
    zlib_crd.ceaseReading();
  }
  else if (spirv::SPIR_V_BLOB_IDENT_UNCOMPRESSED == native_code[0])
  {
    InPlaceMemLoadCB crd(native_code + 2, native_code[1]);
    crd.readTab(chunks);
    crd.readTab(chunk_data);
    crd.ceaseReading();
  }
  else
    fatal("vulkan: unknown shader binary ident %llX", native_code[0]);
}

static int create_shader_for_stage(const uint32_t *native_code, VkShaderStageFlagBits stage, uintptr_t size)
{
  if (spirv::SPIR_V_COMBINED_BLOB_IDENT == native_code[0])
  {
    uint32_t count = native_code[1];
    auto comboHeaderChunks = reinterpret_cast<const spirv::CombinedChunk *>(native_code + 2);
    auto comboData = reinterpret_cast<const uint8_t *>(comboHeaderChunks + count);

    eastl::vector<VkShaderStageFlagBits> comboStages;
    eastl::vector<Tab<spirv::ChunkHeader>> comboChunks;
    eastl::vector<Tab<uint8_t>> comboChunkData;
    comboStages.reserve(count);
    comboChunks.reserve(count);
    comboChunkData.reserve(count);

    for (auto &&chunk : make_span(comboHeaderChunks, count))
    {
      Tab<spirv::ChunkHeader> chunks;
      Tab<uint8_t> chunkData;
      decode_shader_binary(reinterpret_cast<const uint32_t *>(comboData + chunk.offset), chunk.size, chunk.stage, chunks, chunkData);
      comboStages.push_back(chunk.stage);
      comboChunks.push_back(eastl::move(chunks));
      comboChunkData.push_back(eastl::move(chunkData));
    }
    return api_state.shaderProgramDatabase
      .newShader(api_state.device.getContext(), eastl::move(comboStages), eastl::move(comboChunks), eastl::move(comboChunkData))
      .get();
  }
  else
  {
    Tab<spirv::ChunkHeader> chunks;
    Tab<uint8_t> chunkData;
    decode_shader_binary(native_code, size, stage, chunks, chunkData);
    return api_state.shaderProgramDatabase.newShader(api_state.device.getContext(), stage, chunks, chunkData).get();
  }
}

VPROG d3d::create_vertex_shader(const uint32_t *native_code)
{
  return create_shader_for_stage(native_code, VK_SHADER_STAGE_VERTEX_BIT, 0);
}

void d3d::delete_vertex_shader(VPROG vs) { api_state.shaderProgramDatabase.deleteShader(api_state.device.getContext(), ShaderID(vs)); }

int d3d::set_cs_constbuffer_size(int required_size)
{
  G_ASSERTF(required_size >= 0, "Negative register count?");
  return api_state.state.setComputeConstRegisterCount(required_size);
}

int d3d::set_vs_constbuffer_size(int required_size)
{
  G_ASSERTF(required_size >= 0, "Negative register count?");
  return api_state.state.setVertexConstRegisterCount(required_size);
}

FSHADER d3d::create_pixel_shader(const uint32_t *native_code)
{
  return create_shader_for_stage(native_code, VK_SHADER_STAGE_FRAGMENT_BIT, 0);
}

void d3d::delete_pixel_shader(FSHADER ps)
{
  api_state.shaderProgramDatabase.deleteShader(api_state.device.getContext(), ShaderID(ps));
}

PROGRAM d3d::get_debug_program() { return api_state.shaderProgramDatabase.getDebugProgram().get(); }

PROGRAM d3d::create_program(VPROG vs, FSHADER fs, VDECL vdecl, unsigned *, unsigned)
{
  return api_state.shaderProgramDatabase
    .newGraphicsProgram(api_state.device.getContext(), InputLayoutID(vdecl), ShaderID(vs), ShaderID(fs))
    .get();
}

PROGRAM d3d::create_program(const uint32_t *vs, const uint32_t *ps, VDECL vdecl, unsigned *strides, unsigned streams)
{
  VPROG vprog = create_vertex_shader(vs);
  FSHADER fshad = create_pixel_shader(ps);
  return create_program(vprog, fshad, vdecl, strides, streams);
}

PROGRAM d3d::create_program_cs(const uint32_t *cs_native)
{
  Tab<spirv::ChunkHeader> chunks;
  Tab<uint8_t> chunkData;
  decode_shader_binary(cs_native, 0, VK_SHADER_STAGE_COMPUTE_BIT, chunks, chunkData);
  return api_state.shaderProgramDatabase.newComputeProgram(api_state.device.getContext(), chunks, chunkData).get();
}

bool d3d::set_program(PROGRAM prog_id)
{
  ProgramID prog{prog_id};
  if (ProgramID::Null() != prog)
  {
    auto &pipeState = api_state.device.getContext().getFrontend().pipelineState;

    switch (get_program_type(prog))
    {
#if D3D_HAS_RAY_TRACING
      case program_type_raytrace: pipeState.set<StateFieldRaytraceProgram, ProgramID, FrontRaytraceState>(prog); break;
#endif
      case program_type_graphics:
      {
        pipeState.set<StateFieldGraphicsInputLayoutOverride, InputLayoutID, FrontGraphicsState>(InputLayoutID::Null());
        pipeState.set<StateFieldGraphicsProgram, ProgramID, FrontGraphicsState>(prog);
        Stat3D::updateProgram();
      }
      break;
      case program_type_compute: pipeState.set<StateFieldComputeProgram, ProgramID, FrontComputeState>(prog); break;
      default: G_ASSERTF(false, "Broken program type"); return false;
    }
  }
  return true;
}

void d3d::delete_program(PROGRAM prog)
{
  ProgramID pid{prog};
  api_state.shaderProgramDatabase.removeProgram(api_state.device.getContext(), pid);
}

#if _TARGET_PC
VPROG d3d::create_vertex_shader_dagor(const VPRTYPE * /*tokens*/, int /*len*/)
{
  G_ASSERT(false);
  return BAD_PROGRAM;
}

VPROG d3d::create_vertex_shader_asm(const char * /*asm_text*/)
{
  G_ASSERT(false);
  return BAD_PROGRAM;
}

FSHADER d3d::create_pixel_shader_dagor(const FSHTYPE * /*tokens*/, int /*len*/)
{
  G_ASSERT(false);
  return BAD_PROGRAM;
}

FSHADER d3d::create_pixel_shader_asm(const char * /*asm_text*/)
{
  G_ASSERT(false);
  return BAD_PROGRAM;
}

// NOTE: entry point should be removed from the interface
bool d3d::set_pixel_shader(FSHADER /*shader*/)
{
  G_ASSERT(false);
  return true;
}

// NOTE: entry point should be removed from the interface
bool d3d::set_vertex_shader(VPROG /*shader*/)
{
  G_ASSERT(false);
  return true;
}

VDECL d3d::get_program_vdecl(PROGRAM prog)
{
  return api_state.shaderProgramDatabase.getGraphicsProgInputLayout(ProgramID(prog)).get();
}
#endif // _TARGET_PC

bool d3d::set_const(unsigned stage, unsigned first, const void *data, unsigned count)
{
  G_ASSERT(stage < STAGE_MAX);
  api_state.state.setConstRegisters(stage, first * SHADER_REGISTER_ELEMENTS,
    make_span((const uint32_t *)data, count * SHADER_REGISTER_ELEMENTS));

  return true;
}

bool d3d::set_blend_factor(E3DCOLOR color)
{
  auto &pipeState = api_state.device.getContext().getFrontend().pipelineState;
  pipeState.set<StateFieldGraphicsBlendConstantFactor, E3DCOLOR, FrontGraphicsState>(color);
  return true;
}

bool d3d::set_tex(unsigned shader_stage, unsigned unit, BaseTexture *tex, bool /*use_sampler*/)
{
  BaseTex *texture = cast_to_texture_base(tex);

  PipelineState &pipeState = api_state.device.getContext().getFrontend().pipelineState;
  auto &resBinds = pipeState.getStageResourceBinds((ShaderStage)shader_stage);
  TRegister tReg(texture);
  if (resBinds.set<StateFieldTRegisterSet, StateFieldTRegister::Indexed>({unit, tReg}))
  {
    if (texture)
      texture->setUsed();
    pipeState.markResourceBindDirty((ShaderStage)shader_stage);
  }

  return true;
}

bool d3d::set_rwtex(unsigned shader_stage, unsigned unit, BaseTexture *tex, uint32_t face, uint32_t mip_level, bool as_uint)
{
  PipelineState &pipeState = api_state.device.getContext().getFrontend().pipelineState;
  auto &resBinds = pipeState.getStageResourceBinds((ShaderStage)shader_stage);
  URegister uReg(tex, face, mip_level, as_uint);
  if (resBinds.set<StateFieldURegisterSet, StateFieldURegister::Indexed>({unit, uReg}))
  {
    BaseTex *texture = cast_to_texture_base(tex);
    if (texture)
      texture->setUsed();
    pipeState.markResourceBindDirty((ShaderStage)shader_stage);
  }

  return true;
}

bool d3d::clear_rwtexi(BaseTexture *tex, const unsigned val[4], uint32_t face, uint32_t mip_level)
{
  BaseTex *texture = cast_to_texture_base(tex);
  if (texture)
  {
    Image *image = texture->getDeviceImage(true);
    VkClearColorValue ccv;
    G_STATIC_ASSERT(sizeof(ccv) == sizeof(unsigned) * 4);
    memcpy(&ccv, val, sizeof(ccv));
    VkImageSubresourceRange area = makeImageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT, mip_level, 1, face, 1);
    api_state.device.getContext().clearColorImage(image, area, ccv);
  }
  return true;
}

bool d3d::clear_rwtexf(BaseTexture *tex, const float val[4], uint32_t face, uint32_t mip_level)
{
  BaseTex *texture = cast_to_texture_base(tex);
  if (texture)
  {
    Image *image = texture->getDeviceImage(true);
    VkClearColorValue ccv;
    G_STATIC_ASSERT(sizeof(ccv) == sizeof(float) * 4);
    memcpy(&ccv, val, sizeof(ccv));
    VkImageSubresourceRange area = makeImageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT, mip_level, 1, face, 1);
    api_state.device.getContext().clearColorImage(image, area, ccv);
  }
  return true;
}

bool d3d::clear_rwbufi(Sbuffer *buffer, const unsigned values[4])
{
  if (buffer)
  {
    G_ASSERT(buffer->getFlags() & SBCF_BIND_UNORDERED);
    GenericBufferInterface *vbuf = (GenericBufferInterface *)buffer;
    api_state.device.getContext().clearBufferInt(vbuf->getDeviceBuffer(), values);
  }
  return true;
}

bool d3d::clear_rwbuff(Sbuffer *buffer, const float values[4])
{
  if (buffer)
  {
    G_ASSERT(buffer->getFlags() & SBCF_BIND_UNORDERED);
    GenericBufferInterface *vbuf = (GenericBufferInterface *)buffer;

    api_state.device.getContext().clearBufferFloat(vbuf->getDeviceBuffer(), values);
  }
  return true;
}

bool d3d::set_buffer(unsigned shader_stage, unsigned unit, Sbuffer *buffer)
{
  G_ASSERT((nullptr == buffer) || buffer->getFlags() & SBCF_BIND_SHADER_RES || buffer->getFlags() & SBCF_BIND_UNORDERED);

  PipelineState &pipeState = api_state.device.getContext().getFrontend().pipelineState;
  auto &resBinds = pipeState.getStageResourceBinds((ShaderStage)shader_stage);
  TRegister tReg(buffer);
  if (resBinds.set<StateFieldTRegisterSet, StateFieldTRegister::Indexed>({unit, tReg}))
    pipeState.markResourceBindDirty((ShaderStage)shader_stage);
  // TODO: redo sets to precise per class overrides like this
  // BufferRef tRegB{((GenericBufferInterface *)buffer)->getDeviceBuffer(), buffer->ressize()};
  // resBinds.set<StateFieldTRegisterSet, TrackedStateIndexedDataRORef<BufferRef>>({unit, tRegB});
  return true;
}

bool d3d::set_rwbuffer(unsigned shader_stage, unsigned unit, Sbuffer *buffer)
{
  PipelineState &pipeState = api_state.device.getContext().getFrontend().pipelineState;
  auto &resBinds = pipeState.getStageResourceBinds((ShaderStage)shader_stage);
  URegister uReg(buffer);
  if (resBinds.set<StateFieldURegisterSet, StateFieldURegister::Indexed>({unit, uReg}))
    pipeState.markResourceBindDirty((ShaderStage)shader_stage);

  return true;
}

bool d3d::setview(int x, int y, int w, int h, float minz, float maxz)
{
  // it is ok for other APIs, but we will crash GPU due to RP render area dependency
  // clamp to 0 with wh fixup for now, as it does not make sense to draw with negative offset
  if (x < 0)
  {
    w += x;
    x = 0;
  }
  if (y < 0)
  {
    h += y;
    y = 0;
  }

  auto &pipeState = api_state.device.getContext().getFrontend().pipelineState;
  ViewportState viewport = {{{x, y}, {(uint32_t)w, (uint32_t)h}}, minz, maxz};

  pipeState.set<StateFieldGraphicsViewport, ViewportState, FrontGraphicsState>(viewport);
  return true;
}

bool d3d::getview(int &x, int &y, int &w, int &h, float &minz, float &maxz)
{
  auto &pipeState = api_state.device.getContext().getFrontend().pipelineState;
  const ViewportState &viewport = pipeState.getRO<StateFieldGraphicsViewport, ViewportState, FrontGraphicsState>();

  x = viewport.rect2D.offset.x;
  y = viewport.rect2D.offset.y;
  w = viewport.rect2D.extent.width;
  h = viewport.rect2D.extent.height;
  minz = viewport.minZ;
  maxz = viewport.maxZ;
  return true;
}

bool d3d::setscissor(int x, int y, int w, int h)
{
  VkRect2D scissor = {{x, y}, {(uint32_t)w, (uint32_t)h}};
  auto &pipeState = api_state.device.getContext().getFrontend().pipelineState;
  pipeState.set<StateFieldGraphicsScissorRect, VkRect2D, FrontGraphicsState>(scissor);
  return true;
}

bool d3d::setstencil(uint32_t ref)
{
  // this will override stencil ref from render state id
  // until new render state is setted up
  auto &pipeState = api_state.device.getContext().getFrontend().pipelineState;
  pipeState.set<StateFieldGraphicsStencilRefOverride, uint16_t, FrontGraphicsState>(uint16_t(ref));
  return true;
}

bool d3d::update_screen(bool /*app_active*/)
{
  api_state.lastLimitingFactor = api_state.device.getContext().getFramerateLimitingFactor();

  api_state.device.getContext().present();

  // restore BB
  d3d::set_render_target();

  if (::dgs_on_swap_callback)
    ::dgs_on_swap_callback();

  return true;
}

bool d3d::setvsrc_ex(int stream, Vbuffer *vb, int ofs, int stride_bytes)
{
  G_ASSERT(stream < MAX_VERTEX_INPUT_STREAMS);
  using Bind = StateFieldGraphicsVertexBufferBind;
  using StrideBind = StateFieldGraphicsVertexBufferStride;
  auto &pipeState = api_state.device.getContext().getFrontend().pipelineState;

  Buffer *devBuf = nullptr;
  if (vb)
  {
    devBuf = ((GenericBufferInterface *)vb)->getDeviceBuffer();
    G_ASSERTF(((GenericBufferInterface *)vb)->getFlags() & SBCF_BIND_VERTEX,
      "vulkan: using non vertex buffer %p:%s in vertex bind slot %u", devBuf, devBuf->getDebugName(), stream);
  }

  BufferRef bRef(devBuf);
  Bind bind{bRef, (uint32_t)ofs};
  pipeState.set<StateFieldGraphicsVertexBuffers, Bind::Indexed, FrontGraphicsState>({(uint32_t)stream, bind});
  pipeState.set<StateFieldGraphicsVertexBufferStrides, StrideBind::Indexed, FrontGraphicsState>(
    {(uint32_t)stream, (uint32_t)stride_bytes});

  return true;
}

bool d3d::setind(Ibuffer *ib)
{
  GenericBufferInterface *gb = ((GenericBufferInterface *)ib); //-V522
  auto &pipeState = api_state.device.getContext().getFrontend().pipelineState;

  Buffer *devBuf = nullptr;
  if (gb)
    devBuf = gb->getDeviceBuffer();

  BufferRef bRef(devBuf);
  pipeState.set<StateFieldGraphicsIndexBuffer, BufferRef, FrontGraphicsState>(bRef); //-V522
  if (gb)
    pipeState.set<StateFieldGraphicsIndexBuffer, VkIndexType, FrontGraphicsState>(gb->getIndexType()); //-V522

  return true;
}

VDECL d3d::create_vdecl(VSDTYPE *vsd)
{
  drv3d_vulkan::InputLayout layout;
  layout.fromVdecl(vsd);
  return api_state.shaderProgramDatabase.registerInputLayout(api_state.device.getContext(), layout).get();
}

void d3d::delete_vdecl(VDECL vdecl)
{
  (void)vdecl;
  // ignore delete request, we keep it as a 'optimization'
}

bool d3d::setvdecl(VDECL vdecl)
{
  auto &pipeState = api_state.device.getContext().getFrontend().pipelineState;
  pipeState.set<StateFieldGraphicsInputLayoutOverride, InputLayoutID, FrontGraphicsState>(InputLayoutID(vdecl));
  return true;
}

// stolen from dx11 backend
static inline uint32_t nprim_to_nverts(uint32_t prim_type, uint32_t numprim)
{
  // table look-up: 4 bits per entry [2b mul 2bit add]
  static const uint64_t table = (0x0ULL << (4 * PRIM_POINTLIST))          //*1+0 00/00
                                | (0x4ULL << (4 * PRIM_LINELIST))         //*2+0 01/00
                                | (0x1ULL << (4 * PRIM_LINESTRIP))        //*1+1 00/01
                                | (0x8ULL << (4 * PRIM_TRILIST))          //*3+0 10/00
                                | (0x2ULL << (4 * PRIM_TRISTRIP))         //*1+2 00/10
                                | (0x2ULL << (4 * PRIM_TRIFAN))           //*1+2 00/10
                                | (0xcULL << (4 * PRIM_4_CONTROL_POINTS)) //*4+0 11/00
    ;

  const uint32_t code = uint32_t((table >> (prim_type * 4)) & 0x0f);
  return numprim * ((code >> 2) + 1) + (code & 3);
}

bool d3d::draw_base(int type, int start, int numprim, uint32_t num_instances, uint32_t start_instance)
{
  VkPrimitiveTopology topology = translate_primitive_topology_to_vulkan(type);
  auto &state = api_state.state;
  state.flushGraphics(api_state.device.getContext());
  api_state.device.getContext().draw(topology, start, nprim_to_nverts(type, numprim), start_instance, num_instances);

  Stat3D::updateDrawPrim();
  Stat3D::updateTriangles(numprim * num_instances);
  if (num_instances > 1)
    Stat3D::updateInstances(num_instances);
  return true;
}

bool d3d::drawind_base(int type, int startind, int numprim, int base_vertex, uint32_t num_instances, uint32_t start_instance)
{
  VkPrimitiveTopology topology = translate_primitive_topology_to_vulkan(type);
  api_state.state.flushGraphics(api_state.device.getContext());
  G_ASSERT(num_instances > 0);
  api_state.device.getContext().drawIndexed(topology, startind, nprim_to_nverts(type, numprim), max(base_vertex, 0), start_instance,
    num_instances);

  Stat3D::updateDrawPrim();
  Stat3D::updateTriangles(numprim * num_instances);
  if (num_instances > 1)
    Stat3D::updateInstances(num_instances);
  return true;
}

bool d3d::draw_up(int type, int numprim, const void *ptr, int stride_bytes)
{
  VkPrimitiveTopology topology = translate_primitive_topology_to_vulkan(type);
  auto primCount = nprim_to_nverts(type, numprim);

  auto vBuffer = api_state.device.getContext().copyToTempBuffer(TempBufferManager::TYPE_USER_VERTEX, primCount * stride_bytes, ptr);
  api_state.state.flushGraphics(api_state.device.getContext());
  api_state.device.getContext().drawUserData(topology, primCount, stride_bytes, vBuffer.get());

  Stat3D::updateDrawPrim();
  Stat3D::updateTriangles(numprim);
  return true;
}

bool d3d::drawind_up(int type, int minvert, int numvert, int numprim, const uint16_t *ind, const void *ptr, int stride_bytes)
{
  (void)minvert;
  VkPrimitiveTopology topology = translate_primitive_topology_to_vulkan(type);
  auto primCount = nprim_to_nverts(type, numprim);

  auto vBuffer = api_state.device.getContext().copyToTempBuffer(TempBufferManager::TYPE_USER_VERTEX, numvert * stride_bytes, ptr);
  auto iBuffer = api_state.device.getContext().copyToTempBuffer(TempBufferManager::TYPE_USER_INDEX, primCount * sizeof(uint16_t), ind);
  api_state.state.flushGraphics(api_state.device.getContext());
  api_state.device.getContext().drawIndexedUserData(topology, primCount, stride_bytes, vBuffer.get(), iBuffer.get());

  Stat3D::updateDrawPrim();
  Stat3D::updateTriangles(numprim);
  return true;
}

bool d3d::dispatch(uint32_t x, uint32_t y, uint32_t z, GpuPipeline gpu_pipeline)
{
  G_UNUSED(gpu_pipeline);
  api_state.state.flushCompute(api_state.device.getContext());
  api_state.device.getContext().dispatch(x, y, z);
  return true;
}

bool d3d::draw_indirect(int prim_type, Sbuffer *args, uint32_t byte_offset)
{
  return d3d::multi_draw_indirect(prim_type, args, 1, sizeof(VkDrawIndirectCommand), byte_offset);
}

bool d3d::draw_indexed_indirect(int prim_type, Sbuffer *args, uint32_t byte_offset)
{
  return d3d::multi_draw_indexed_indirect(prim_type, args, 1, sizeof(VkDrawIndexedIndirectCommand), byte_offset);
}

bool d3d::multi_draw_indirect(int prim_type, Sbuffer *args, uint32_t draw_count, uint32_t stride_bytes, uint32_t byte_offset)
{
  G_ASSERTF(args != nullptr, "multi_draw_indirect with nullptr buffer is invalid");
  GenericBufferInterface *buffer = (GenericBufferInterface *)args;
  G_ASSERTF(args->getFlags() & SBCF_MISC_DRAWINDIRECT, "multi_draw_indirect buffer is not usable as indirect buffer");

  VkPrimitiveTopology topology = translate_primitive_topology_to_vulkan(prim_type);
  api_state.state.flushGraphics(api_state.device.getContext());
  api_state.device.getContext().drawIndirect(topology, draw_count, buffer->getDeviceBuffer(), byte_offset, stride_bytes);

  Stat3D::updateDrawPrim();
  return true;
}

bool d3d::multi_draw_indexed_indirect(int prim_type, Sbuffer *args, uint32_t draw_count, uint32_t stride_bytes, uint32_t byte_offset)
{
  G_ASSERTF(args != nullptr, "multi_draw_indexed_indirect with nullptr buffer is invalid");
  GenericBufferInterface *buffer = (GenericBufferInterface *)args;
  G_ASSERTF(args->getFlags() & SBCF_MISC_DRAWINDIRECT, "multi_draw_indexed_indirect buffer is not usable as indirect buffer");

  VkPrimitiveTopology topology = translate_primitive_topology_to_vulkan(prim_type);
  api_state.state.flushGraphics(api_state.device.getContext());
  api_state.device.getContext().drawIndexedIndirect(topology, draw_count, buffer->getDeviceBuffer(), byte_offset, stride_bytes);

  Stat3D::updateDrawPrim();
  return true;
}

bool d3d::dispatch_indirect(Sbuffer *args, uint32_t byte_offset, GpuPipeline gpu_pipeline)
{
  G_UNUSED(gpu_pipeline);
  G_ASSERTF(args != nullptr, "dispatch_indirect with nullptr buffer is invalid");
  GenericBufferInterface *buffer = (GenericBufferInterface *)args;
  G_ASSERTF(args->getFlags() & SBCF_BIND_UNORDERED, "dispatch_indirect buffer without SBCF_BIND_UNORDERED flag");
  G_ASSERTF(args->getFlags() & SBCF_MISC_DRAWINDIRECT, "dispatch_indirect buffer is not usable as indirect buffer");
  api_state.state.flushCompute(api_state.device.getContext());
  api_state.device.getContext().dispatchIndirect(buffer->getDeviceBuffer(), byte_offset);
  return true;
}

bool d3d::set_const_buffer(uint32_t stage, uint32_t unit, Sbuffer *buffer, uint32_t consts_offset, uint32_t consts_size)
{
  G_ASSERT_RETURN(!consts_offset && !consts_size, false); // not implemented
  G_ASSERT((nullptr == buffer) || (buffer->getFlags() & SBCF_BIND_CONSTANT));

  PipelineState &pipeState = api_state.device.getContext().getFrontend().pipelineState;
  auto &resBinds = pipeState.getStageResourceBinds((ShaderStage)stage);
  BufferRef bReg{};
  if (buffer)
    bReg = BufferRef{((GenericBufferInterface *)buffer)->getDeviceBuffer()};

  if (resBinds.set<StateFieldBRegisterSet, StateFieldBRegister::Indexed>({unit, bReg}))
    pipeState.markResourceBindDirty((ShaderStage)stage);
  return true;
}

bool d3d::setwire(bool wire)
{
  if (api_state.device.getDeviceProperties().features.fillModeNonSolid != VK_FALSE)
  {
    auto &pipeState = api_state.device.getContext().getFrontend().pipelineState;
    pipeState.set<StateFieldGraphicsPolygonLine, bool, FrontGraphicsState>(wire);
    return true;
  }
  return false;
}

float d3d::get_screen_aspect_ratio() { return api_state.windowState.settings.aspect; }

void *d3d::fast_capture_screen(int &w, int &h, int &stride_bytes, int &format)
{
  ApiState &as = api_state;
  Device &device = as.device;

  VkExtent2D extent = as.device.swapchain.getMode().extent;
  FormatStore fmt = as.device.swapchain.getMode().colorFormat;
  uint32_t bufferSize = fmt.calculateSlicePich(extent.width, extent.height);

  w = extent.width;
  h = extent.height;
  stride_bytes = fmt.calculateRowPitch(extent.width);
  format = CAPFMT_X8R8G8B8;

  as.screenCaptureStagingBuffer =
    device.createBuffer(bufferSize, DeviceMemoryClass::HOST_RESIDENT_HOST_READ_ONLY_BUFFER, 1, BufferMemoryFlags::TEMP);
  device.setBufName(as.screenCaptureStagingBuffer, "screen cap staging buf");

  device.getContext().captureScreen(api_state.screenCaptureStagingBuffer);
  device.getContext().wait();
  return api_state.screenCaptureStagingBuffer->dataPointer(0);
}

void d3d::end_fast_capture_screen()
{
  api_state.device.getContext().destroyBuffer(api_state.screenCaptureStagingBuffer);
  api_state.screenCaptureStagingBuffer = nullptr;
}

TexPixel32 *d3d::capture_screen(int &w, int &h, int &stride_bytes)
{
  int fmt;
  void *ptr = fast_capture_screen(w, h, stride_bytes, fmt);
  api_state.screenCaptureBuffer.resize(w * h * sizeof(TexPixel32));
  if (CAPFMT_X8R8G8B8 == fmt)
  {
    memcpy(api_state.screenCaptureBuffer.data(), ptr, data_size(api_state.screenCaptureBuffer));
  }
  else
    G_ASSERT(false);
  end_fast_capture_screen();
  return (TexPixel32 *)api_state.screenCaptureBuffer.data();
}

void d3d::release_capture_buffer() { clear_and_shrink(api_state.screenCaptureBuffer); }

void d3d::get_screen_size(int &w, int &h)
{
  ApiState &as = api_state;

  VkExtent2D extent = as.device.swapchain.getMode().extent;

  w = extent.width;
  h = extent.height;
}

void d3d::beginEvent(const char *name)
{
#if VK_EXT_debug_marker || VK_EXT_debug_utils
  api_state.device.getContext().pushEvent(name);
#else
  (void)name;
#endif

  d3dhang::hang_if_requested(name);
}

void d3d::endEvent()
{
#if VK_EXT_debug_marker || VK_EXT_debug_utils
  api_state.device.getContext().popEvent();
#endif
}

bool d3d::set_depth_bounds(float zmin, float zmax)
{
  using FieldName = StateFieldGraphicsDepthBounds;

  zmin = clamp(zmin, 0.f, 1.f);
  zmax = clamp(zmax, zmin, 1.f);
  auto &pipeState = api_state.device.getContext().getFrontend().pipelineState;
  pipeState.set<FieldName, FieldName::DataType, FrontGraphicsState>({zmin, zmax});
  return true;
}

bool d3d::supports_depth_bounds() { return api_state.driverDesc.caps.hasDepthBoundsTest; }

bool d3d::begin_survey(int name)
{
  if (-1 == name)
    return false;

  api_state.device.getContext().beginSurvey(name);
  return true;
}

void d3d::end_survey(int name)
{
  if (-1 != name)
    api_state.device.getContext().endSurvey(name);
}

int d3d::create_predicate()
{
  if (api_state.device.hasConditionalRender())
    return api_state.device.surveys.add();
  return -1;
}

void d3d::free_predicate(int name)
{
  if (-1 != name)
    api_state.device.surveys.remove(name);
}

void d3d::begin_conditional_render(int name)
{
  if (-1 != name)
  {
    G_ASSERTF(name >= 0, "vlukan: driver received an invalid survey name '%d' in begin_conditional_rendering", name);

    auto &pipeState = api_state.device.getContext().getFrontend().pipelineState;
    pipeState.set<StateFieldGraphicsConditionalRenderingState, ConditionalRenderingState, FrontGraphicsState>(
      api_state.device.surveys.startCondRender(static_cast<uint32_t>(name)));
  }
}

void d3d::end_conditional_render(int name)
{
  if (-1 != name)
  {
    G_ASSERTF(name >= 0, "vlukan: driver received an invalid survey name '%d' in end_conditional_rendering", name);

    auto &pipeState = api_state.device.getContext().getFrontend().pipelineState;
    pipeState.set<StateFieldGraphicsConditionalRenderingState, ConditionalRenderingState, FrontGraphicsState>(
      ConditionalRenderingState{});
  }
}

bool d3d::get_vsync_enabled() { return api_state.device.swapchain.getMode().isVsyncOn(); }

bool d3d::enable_vsync(bool enable)
{
  auto videoCfg = ::dgs_get_settings()->getBlockByNameEx("video");
  {
    SwapchainMode newMode(api_state.device.swapchain.getMode());
    newMode.presentMode = get_requested_present_mode(enable, videoCfg->getBool("adaptive_vsync", false));
    api_state.device.swapchain.setMode(newMode);
  }
  return true;
}

d3d::EventQuery *d3d::create_event_query()
{
  auto event = eastl::make_unique<AsyncCompletionState>();
  event->completed();
  return (d3d::EventQuery *)(event.release());
}

void d3d::release_event_query(d3d::EventQuery *fence)
{
  if (fence)
  {
    auto tf = (AsyncCompletionState *)fence;
    api_state.device.getContext().deleteAsyncCompletionStateOnFinish(*tf);
  }
}

bool d3d::issue_event_query(d3d::EventQuery *fence)
{
  if (fence)
  {
    auto tf = (AsyncCompletionState *)fence;
    if (tf->isPending())
      api_state.device.getContext().waitFor(*tf);
    api_state.device.getContext().addSyncEvent(*tf);
  }
  return true;
}

bool d3d::get_event_query_status(d3d::EventQuery *fence, bool flush)
{
  // TODO: may use flush as a hint?
  G_UNUSED(flush);
  if (fence)
  {
    auto tf = (AsyncCompletionState *)fence;
    return tf->isCompleted();
  }
  return true;
}

#if _TARGET_PC
void d3d::get_video_modes_list(Tab<String> &list) { drv3d_vulkan::get_video_modes_list(list); }
#else
void d3d::get_video_modes_list(Tab<String> &list) { clear_and_shrink(list); }
#endif

Vbuffer *d3d::create_vb(int size, int flg, const char *name)
{
  OSSpinlockScopedLock lock{api_state.bufferPoolGuard};
  return api_state.bufferPool.allocate(1, size, flg | SBCF_BIND_VERTEX | SBCF_BIND_SHADER_RES, FormatStore(), name);
}

Ibuffer *d3d::create_ib(int size, int flg, const char *stat_name)
{
  OSSpinlockScopedLock lock{api_state.bufferPoolGuard};
  return api_state.bufferPool.allocate(1, size, flg | SBCF_BIND_INDEX | SBCF_BIND_SHADER_RES, FormatStore(), stat_name);
}

Vbuffer *d3d::create_sbuffer(int struct_size, int elements, unsigned flags, unsigned format, const char *name)
{
  OSSpinlockScopedLock lock{api_state.bufferPoolGuard};
  return api_state.bufferPool.allocate(struct_size, elements, flags, FormatStore::fromCreateFlags(format), name);
}

Texture *d3d::get_backbuffer_tex()
{
  // lazy create
  if (!api_state.swapchainColorProxyTexture)
  {
    api_state.swapchainColorProxyTexture = allocate_texture(RES3D_TEX, TEXFMT_DEFAULT | TEXCF_RTARGET);
    api_state.swapchainColorProxyTexture->setParams(api_state.device.swapchain.getMode().extent.width,
      api_state.device.swapchain.getMode().extent.height, 1, 1, "swapchain color target");
  }
  return api_state.swapchainColorProxyTexture;
}

Texture *d3d::get_backbuffer_tex_depth()
{
  // lazy create
  if (!api_state.swapchainDepthStencilProxyTexture)
  {
    api_state.swapchainDepthStencilProxyTexture = allocate_texture(RES3D_TEX, TEXFMT_DEPTH24 | TEXCF_RTARGET);
    api_state.swapchainDepthStencilProxyTexture->setParams(api_state.device.swapchain.getMode().extent.width,
      api_state.device.swapchain.getMode().extent.height, 1, 1, "swapchain depth stencil target");
  }
  return api_state.swapchainDepthStencilProxyTexture;
}

shaders::DriverRenderStateId d3d::create_render_state(const shaders::RenderState &state)
{
  return api_state.device.getRenderStateSystem().registerState(api_state.device.getContext(), state);
}

bool d3d::set_render_state(shaders::DriverRenderStateId state_id)
{
  auto &pipeState = api_state.device.getContext().getFrontend().pipelineState;
  pipeState.set<StateFieldGraphicsRenderState, shaders::DriverRenderStateId, FrontGraphicsState>(state_id);
  pipeState.set<StateFieldGraphicsStencilRefOverride, uint16_t, FrontGraphicsState>(STENCIL_REF_OVERRIDE_DISABLE);
  return true;
}

void d3d::clear_render_states()
{
  // do nothing
}

void d3d::resource_barrier(ResourceBarrierDesc desc, GpuPipeline gpu_pipeline /*= GpuPipeline::GRAPHICS*/)
{
  if (is_global_lock_acquired())
    api_state.device.getContext().resourceBarrier(desc, gpu_pipeline);
}

IMPLEMENT_D3D_RESOURCE_ACTIVATION_API_USING_GENERIC()

bool d3d::set_immediate_const(unsigned stage, const uint32_t *data, unsigned num_words)
{
  PipelineState &pipeState = api_state.device.getContext().getFrontend().pipelineState;
  auto &resBinds = pipeState.getStageResourceBinds((ShaderStage)stage);
  if (resBinds.set<StateFieldImmediateConst, StateFieldImmediateConst::SrcData>({(uint8_t)num_words, data}))
    pipeState.markResourceBindDirty((ShaderStage)stage);

  return true;
}

#if DAGOR_DBGLEVEL > 0
#include <gui/dag_imgui.h>
REGISTER_IMGUI_WINDOW("VULK", "Memory##VULK-memory", drv3d_vulkan::debug_ui_memory);
REGISTER_IMGUI_WINDOW("VULK", "Frame##VULK-frame", drv3d_vulkan::debug_ui_frame);
REGISTER_IMGUI_WINDOW("VULK", "Timeline##VULK-timeline", drv3d_vulkan::debug_ui_timeline);
REGISTER_IMGUI_WINDOW("VULK", "Resources##VULK-resources", drv3d_vulkan::debug_ui_resources);
REGISTER_IMGUI_WINDOW("VULK", "Pipelines##VULK-pipelines", drv3d_vulkan::debug_ui_pipelines);
REGISTER_IMGUI_WINDOW("VULK", "Misc##VULK-misc-debug-features", drv3d_vulkan::debug_ui_misc);
#endif
