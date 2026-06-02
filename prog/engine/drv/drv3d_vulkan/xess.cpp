// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "xess.h"
#include <xess_sdk/inc/xess/xess_vk.h>
#include <xess_sdk/inc/xess/xess_debug.h>
#include <osApiWrappers/dag_dynLib.h>
#include <startup/dag_globalSettings.h>
#include <ioSys/dag_dataBlock.h>
#include <osApiWrappers/dag_localConv.h>
#include "globals.h"
#include "driver_config.h"
#include "physical_device_set.h"
#include "vulkan_instance.h"
#include "vulkan_device.h"
#include "os.h"
#include "backend/cmd/vendor_exts.h"
#include "backend.h"
#include "image_resource.h"
#include "backend/context.h"

namespace
{
DagorDynLibHolder libxess;
struct Funcs
{
  decltype(::xessVKGetRequiredInstanceExtensions) *xessVKGetRequiredInstanceExtensions = nullptr;
  decltype(::xessVKGetRequiredDeviceExtensions) *xessVKGetRequiredDeviceExtensions = nullptr;
  decltype(::xessVKGetRequiredDeviceFeatures) *xessVKGetRequiredDeviceFeatures = nullptr;
  decltype(::xessVKInit) *xessVKInit = nullptr;
  decltype(::xessVKExecute) *xessVKExecute = nullptr;
  decltype(::xessVKCreateContext) *xessVKCreateContext = nullptr;
  decltype(::xessVKBuildPipelines) *xessVKBuildPipelines = nullptr;

  decltype(::xessGetProperties) *xessGetProperties = nullptr;
  decltype(::xessGetInputResolution) *xessGetInputResolution = nullptr;
  decltype(::xessGetOptimalInputResolution) *xessGetOptimalInputResolution = nullptr;
  decltype(::xessDestroyContext) *xessDestroyContext = nullptr;
  decltype(::xessGetPipelineBuildStatus) *xessGetPipelineBuildStatus = nullptr;
  decltype(::xessSetVelocityScale) *xessSetVelocityScale = nullptr;
  decltype(::xessStartDump) *xessStartDump = nullptr;
  decltype(::xessGetVersion) *xessGetVersion = nullptr;
} libxessFn;

xess_quality_settings_t to_xess_quality(int quality)
{
  static constexpr xess_quality_settings_t xessQualities[] = {XESS_QUALITY_SETTING_PERFORMANCE, XESS_QUALITY_SETTING_BALANCED,
    XESS_QUALITY_SETTING_QUALITY, XESS_QUALITY_SETTING_ULTRA_QUALITY, XESS_QUALITY_SETTING_ULTRA_QUALITY_PLUS, XESS_QUALITY_SETTING_AA,
    XESS_QUALITY_SETTING_ULTRA_PERFORMANCE};
  G_ASSERT(quality < (int)eastl::extent<decltype(xessQualities)>::value);
  return xessQualities[quality];
}

drv3d_vulkan::Xess::ErrorKind to_error_kind(xess_result_t result)
{
  using ErrorKind = drv3d_vulkan::Xess::ErrorKind;
  switch (result)
  {
    case XESS_RESULT_SUCCESS: return ErrorKind::Unknown; // not an error
    case XESS_RESULT_ERROR_UNSUPPORTED_DEVICE: return ErrorKind::UnsupportedDevice;
    case XESS_RESULT_ERROR_UNSUPPORTED_DRIVER: return ErrorKind::UnsupportedDriver;
    case XESS_RESULT_ERROR_UNINITIALIZED: return ErrorKind::Uninitialized;
    case XESS_RESULT_ERROR_INVALID_ARGUMENT: return ErrorKind::InvalidArgument;
    case XESS_RESULT_ERROR_DEVICE_OUT_OF_MEMORY: return ErrorKind::DeviceOutOfMemory;
    case XESS_RESULT_ERROR_DEVICE: return ErrorKind::Device;
    case XESS_RESULT_ERROR_NOT_IMPLEMENTED: return ErrorKind::NotImplemented;
    case XESS_RESULT_ERROR_INVALID_CONTEXT: return ErrorKind::InvalidContext;
    case XESS_RESULT_ERROR_OPERATION_IN_PROGRESS: return ErrorKind::OperationInProgress;
    case XESS_RESULT_ERROR_UNSUPPORTED: return ErrorKind::Unsupported;
    case XESS_RESULT_ERROR_CANT_LOAD_LIBRARY: return ErrorKind::CantLoadLibrary;
    default: return ErrorKind::Unknown;
  }
}
}; // namespace

namespace drv3d_vulkan
{

eastl::string Xess::errorKindToString(ErrorKind) { return {}; }

void Xess::load()
{
  reqApiVersion = VULKAN_USED_VERSION;
  if (!Globals::cfg.bits.allowXess)
  {
    debug("vulkan: XeSS: disabled by config, libxess.dll will not be loaded");
    return;
  }
  libxess.reset(os_dll_load("libxess.dll"));
  if (!libxess)
  {
    debug("vulkan: XeSS: Failed to load libxess.dll, xess will be disabled");
    return;
  }

#define LOAD_FUNC(x)                                                                \
  do                                                                                \
  {                                                                                 \
    libxessFn.x = static_cast<decltype(x) *>(os_dll_get_symbol(libxess.get(), #x)); \
    if (libxessFn.x == nullptr)                                                     \
    {                                                                               \
      libxess.reset();                                                              \
      return;                                                                       \
    }                                                                               \
  } while (0)

  LOAD_FUNC(xessVKGetRequiredInstanceExtensions);
  LOAD_FUNC(xessVKGetRequiredDeviceExtensions);
  LOAD_FUNC(xessVKGetRequiredDeviceFeatures);
  LOAD_FUNC(xessVKInit);
  LOAD_FUNC(xessVKExecute);
  LOAD_FUNC(xessVKCreateContext);
  LOAD_FUNC(xessVKBuildPipelines);
  LOAD_FUNC(xessGetProperties);
  LOAD_FUNC(xessGetInputResolution);
  LOAD_FUNC(xessGetOptimalInputResolution);
  LOAD_FUNC(xessDestroyContext);
  LOAD_FUNC(xessGetPipelineBuildStatus);
  LOAD_FUNC(xessSetVelocityScale);
  LOAD_FUNC(xessStartDump);
  LOAD_FUNC(xessGetVersion);

  debug("vulkan: XeSS: loaded");
}

void Xess::unload() { libxess.reset(); }

bool Xess::init()
{
  if (!libxess)
    return false;

  if (state != XessState::DISABLED)
    return false;

  if (!requiredInstanceExtPass || !requiredDeviceExtPass)
  {
    logwarn("vulkan: XeSS: disabled due to missing %s exts", requiredInstanceExtPass ? "instance" : "device");
    state = XessState::DISABLED; //-V1048
    return false;
  }

  if (!Globals::cfg.bits.allowXess)
  {
    state = XessState::DISABLED; //-V1048
    return false;
  }

  xess_context_handle_t ctx = nullptr;
  xess_result_t result = libxessFn.xessVKCreateContext(Globals::VK::inst.get(), Globals::VK::phy.device, Globals::VK::dev.get(), &ctx);
  if (result != XESS_RESULT_SUCCESS)
  {
    logwarn("vulkan: XeSS: xessVKCreateContext failed with %d", static_cast<int>(result));
    switch (result)
    {
      case XESS_RESULT_ERROR_UNSUPPORTED_DEVICE: state = XessState::UNSUPPORTED_DEVICE; break;
      case XESS_RESULT_ERROR_UNSUPPORTED_DRIVER: state = XessState::UNSUPPORTED_DRIVER; break;
      default: state = XessState::INIT_ERROR_UNKNOWN; break;
    }
    return false;
  }

  context = ctx;
  state = XessState::SUPPORTED;
  debug("vulkan: XeSS: context created");

  const DataBlock &blk_video = *::dgs_get_settings()->getBlockByNameEx("video");
  int xessQuality = -1;
  if (blk_video.getNameId("antialiasing_mode") > -1 && blk_video.getNameId("antialiasing_upscaling") > -1)
  {
    if (dd_stricmp(blk_video.getStr("antialiasing_mode", "off"), "xess") == 0)
    {
      const char *quality = blk_video.getStr("antialiasing_upscaling", "native");
      if (dd_stricmp(quality, "native") == 0)
        xessQuality = 5;
      else if (dd_stricmp(quality, "ultra_quality_plus") == 0)
        xessQuality = 4;
      else if (dd_stricmp(quality, "ultra_quality") == 0)
        xessQuality = 3;
      else if (dd_stricmp(quality, "quality") == 0)
        xessQuality = 2;
      else if (dd_stricmp(quality, "balanced") == 0)
        xessQuality = 1;
      else if (dd_stricmp(quality, "performance") == 0)
        xessQuality = 0;
      else if (dd_stricmp(quality, "ultra_performance") == 0)
        xessQuality = 6;
    }
  }
  else
    xessQuality = blk_video.getInt("xessQuality", -1);

  if (xessQuality >= 0)
    return createFeature(xessQuality, Globals::window.settings.resolutionX, Globals::window.settings.resolutionY);

  return true;
}

bool Xess::createFeature(int quality, uint32_t target_width, uint32_t target_height)
{
  if (!libxess || !context)
    return false;

  desiredOutputResolutionX = target_width;
  desiredOutputResolutionY = target_height;

  xess_vk_init_params_t initParams{};
  initParams.outputResolution = {target_width, target_height};
  initParams.qualitySetting = to_xess_quality(quality);
  initParams.initFlags = XESS_INIT_FLAG_INVERTED_DEPTH;

  xess_result_t result = libxessFn.xessVKInit(context, &initParams);
  if (result != XESS_RESULT_SUCCESS)
  {
    logwarn("vulkan: XeSS: xessVKInit failed with %d", static_cast<int>(result));
    switch (result)
    {
      case XESS_RESULT_ERROR_UNSUPPORTED_DEVICE: state = XessState::UNSUPPORTED_DEVICE; break;
      case XESS_RESULT_ERROR_UNSUPPORTED_DRIVER: state = XessState::UNSUPPORTED_DRIVER; break;
      default: state = XessState::INIT_ERROR_UNKNOWN; break;
    }
    return false;
  }

  xess_2d_t outRes = {target_width, target_height};
  xess_2d_t optimal{}, minRes{}, maxRes{};
  result = libxessFn.xessGetOptimalInputResolution(context, &outRes, initParams.qualitySetting, &optimal, &minRes, &maxRes);
  if (result != XESS_RESULT_SUCCESS)
  {
    logwarn("vulkan: XeSS: xessGetOptimalInputResolution failed with %d", static_cast<int>(result));
    state = XessState::INIT_ERROR_UNKNOWN;
    return false;
  }

  // xessGetOptimalInputResolution gives optimal == min, which is wrong
  result = libxessFn.xessGetInputResolution(context, &outRes, to_xess_quality(quality), &optimal);
  if (result != XESS_RESULT_SUCCESS)
  {
    logwarn("vulkan: XeSS: xessGetInputResolution failed with %d", static_cast<int>(result));
    state = XessState::INIT_ERROR_UNKNOWN;
    return false;
  }

  optimalRenderResolutionX = optimal.x;
  optimalRenderResolutionY = optimal.y;
  minRenderResolutionX = minRes.x;
  minRenderResolutionY = minRes.y;
  maxRenderResolutionX = maxRes.x;
  maxRenderResolutionY = maxRes.y;

  state = XessState::READY;
  debug("vulkan: XeSS: feature ready, output %ux%u, optimal input %ux%u (range %ux%u..%ux%u)", target_width, target_height, optimal.x,
    optimal.y, minRes.x, minRes.y, maxRes.x, maxRes.y);
  return true;
}

void Xess::failDeviceExt() { requiredDeviceExtPass = false; }

void Xess::failInstanceExt() { requiredInstanceExtPass = false; }

bool Xess::shutdown()
{
  requiredInstanceExtPass = false;
  requiredDeviceExtPass = false;
  if (!context)
    return false;

  xess_result_t result = libxessFn.xessDestroyContext(context);
  context = nullptr;
  state = XessState::DISABLED;
  desiredOutputResolutionX = desiredOutputResolutionY = 0;
  optimalRenderResolutionX = optimalRenderResolutionY = 0;
  minRenderResolutionX = minRenderResolutionY = 0;
  maxRenderResolutionX = maxRenderResolutionY = 0;

  if (result != XESS_RESULT_SUCCESS)
  {
    logwarn("vulkan: XeSS: xessDestroyContext failed with %d", static_cast<int>(result));
    return false;
  }
  debug("vulkan: XeSS: context destroyed");
  return true;
}

XessState Xess::getState() const { return state; }

void Xess::getRenderResolution(int &w, int &h, int &minw, int &minh, int &maxw, int &maxh) const
{
  w = optimalRenderResolutionX;
  h = optimalRenderResolutionY;
  minw = minRenderResolutionX;
  minh = minRenderResolutionY;
  maxw = maxRenderResolutionX;
  maxh = maxRenderResolutionY;
}

bool Xess::evaluate(const CmdExecuteXESS &params)
{
  if (!libxess || !context || state != XessState::READY)
    return false;

  auto toTuple = [](Image *src, bool out = false) -> eastl::tuple<VkImage, VkImageCreateInfo, VkImageView> {
    if (!src)
      return eastl::make_tuple(VkImage(0), VkImageCreateInfo{}, VkImageView(0));

    ImageViewState ivs;
    ivs.setMipBase(0);
    ivs.setMipCount(1);
    ivs.setArrayBase(0);
    ivs.setArrayCount(1);
    ivs.isArray = 0;
    ivs.isCubemap = 0;
    ivs.isUAV = out ? 1 : 0;
    ivs.setFormat(src->getFormat());
    auto ici = src->getDescription().ici.toVk();
    ici.format = src->getDescription().format.asVkFormat();
    return eastl::make_tuple(src->getHandle(), ici, src->getImageView(ivs));
  };

  auto toImageView = [&](Image *src, bool out = false) -> xess_vk_image_view_info {
    if (!src)
      return {};
    auto t = toTuple(src, out);
    const VkImageCreateInfo &ici = eastl::get<1>(t);
    xess_vk_image_view_info info{};
    info.imageView = eastl::get<2>(t);
    info.image = eastl::get<0>(t);
    info.subresourceRange = {src->getFormat().getAspektFlags(), 0u, 1u, 0u, 1u};
    info.format = ici.format;
    info.width = ici.extent.width;
    info.height = ici.extent.height;
    return info;
  };

  xess_vk_execute_params_t exec_params{};
  exec_params.colorTexture = toImageView(params.inColor);
  exec_params.velocityTexture = toImageView(params.inMotionVectors);
  exec_params.depthTexture = toImageView(params.inDepth);
  exec_params.outputTexture = toImageView(params.outColor, true);
  exec_params.jitterOffsetX = params.inJitterOffsetX;
  exec_params.jitterOffsetY = params.inJitterOffsetY;
  exec_params.exposureScale = 1.0f;
  exec_params.resetHistory = params.inReset ? 1u : 0u;
  exec_params.inputWidth = params.inInputWidth;
  exec_params.inputHeight = params.inInputHeight;
  exec_params.inputColorBase = {params.inColorDepthOffsetX, params.inColorDepthOffsetY};
  exec_params.inputDepthBase = exec_params.inputColorBase;

  xess_result_t result = libxessFn.xessVKExecute(context, Backend::ctx.frameCore, &exec_params);
  if (result != XESS_RESULT_SUCCESS)
  {
    logwarn("vulkan: XeSS: xessVKExecute failed with %d", static_cast<int>(result));
    return false;
  }
  return true;
}

void Xess::setVelocityScale(float x, float y)
{
  if (!context)
    return;

  libxessFn.xessSetVelocityScale(context, x, y);
}

bool Xess::isQualityAvailableAtResolution(uint32_t target_width, uint32_t target_height, int xess_quality) const
{
  if (!context)
    return false;

  xess_2d_t targetResolution = {target_width, target_height};
  xess_2d_t inputResolution{};
  xess_result_t result = libxessFn.xessGetInputResolution(context, &targetResolution, to_xess_quality(xess_quality), &inputResolution);

  return result == XESS_RESULT_SUCCESS && inputResolution.x > 0 && inputResolution.y > 0;
}

dag::Expected<eastl::string, Xess::ErrorKind> Xess::getVersion() const
{
  if (!libxess)
    return dag::Unexpected(ErrorKind::CantLoadLibrary);

  xess_version_t version{};
  xess_result_t result = libxessFn.xessGetVersion(&version);
  if (result != XESS_RESULT_SUCCESS)
    return dag::Unexpected(to_error_kind(result));

  return eastl::string(eastl::string::CtorSprintf(), "%u.%u.%u", version.major, version.minor, version.patch);
}

Xess::Xess() = default;

Xess::~Xess() = default;

void Xess::startDump(const char *, uint32_t) {}

bool Xess::isFrameGenerationSupported() const { return false; }

bool Xess::isFrameGenerationEnabled() const { return false; }

void Xess::enableFrameGeneration(bool) {}

void Xess::suppressFrameGeneration(bool) {}

void Xess::doScheduleGeneratedFrames(const XessFgParamsVulkan &, const XessFgParamsVulkanResourceStates &) {}

int Xess::getPresentedFrameCount() { return 1; }

uint64_t Xess::getMemoryUsage() const { return 0; }

const eastl::vector<eastl::string> &Xess::getRequiredDeviceExtensions() const
{
  requiredDeviceExtensions.clear();
  if (!libxess)
    return requiredDeviceExtensions;

  uint32_t count = 0;
  const char *const *exts = nullptr;
  if (libxessFn.xessVKGetRequiredDeviceExtensions(Globals::VK::inst.get(), Globals::VK::phy.device, &count, &exts) !=
      XESS_RESULT_SUCCESS)
    return requiredDeviceExtensions;

  requiredDeviceExtensions.reserve(count);
  for (uint32_t i = 0; i < count; ++i)
    requiredDeviceExtensions.emplace_back(exts[i]);

  requiredDeviceExtPass = true;
  return requiredDeviceExtensions;
}

const eastl::vector<eastl::string> &Xess::getRequiredInstanceExtensions() const
{
  requiredInstanceExtensions.clear();
  if (!libxess)
    return requiredInstanceExtensions;

  uint32_t count = 0;
  const char *const *exts = nullptr;
  if (libxessFn.xessVKGetRequiredInstanceExtensions(&count, &exts, &reqApiVersion) != XESS_RESULT_SUCCESS)
    return requiredInstanceExtensions;

  requiredInstanceExtensions.reserve(count);
  for (uint32_t i = 0; i < count; ++i)
    requiredInstanceExtensions.emplace_back(exts[i]);

  requiredInstanceExtPass = true;
  return requiredInstanceExtensions;
}

void Xess::injectRequiredFeatures(VkPhysicalDeviceFeatures2KHR &features2) const
{
  if (!libxess || !requiredInstanceExtPass || !requiredDeviceExtPass)
    return;

  void *features = &features2;
  xess_result_t result = libxessFn.xessVKGetRequiredDeviceFeatures(Globals::VK::inst.get(), Globals::VK::phy.device, &features);
  if (result != XESS_RESULT_SUCCESS)
  {
    logwarn("vulkan: XeSS: xessVKGetRequiredDeviceFeatures failed with %d", static_cast<int>(result));
    return;
  }

  // XeSS may inject VkPhysicalDeviceVulkan1{2,3}Features into the chain and enable
  // VK_KHR_sampler_mirror_clamp_to_edge / VK_EXT_descriptor_indexing. The existing chain
  // already contains the standalone (now-promoted) timelineSemaphore / synchronization2
  // feature structs - having both in one chain is invalid. Merge the promoted feature
  // into Vulkan1xFeatures, unlink the standalone struct, and enable Vulkan12 features
  // matching the extensions XeSS adds.
  VkPhysicalDeviceVulkan12Features *v12 = nullptr;
  VkPhysicalDeviceVulkan13Features *v13 = nullptr;
  for (VkBaseOutStructure *c = reinterpret_cast<VkBaseOutStructure *>(&features2); c; c = c->pNext)
  {
    if (c->sType == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES)
      v12 = reinterpret_cast<VkPhysicalDeviceVulkan12Features *>(c);
    else if (c->sType == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES)
      v13 = reinterpret_cast<VkPhysicalDeviceVulkan13Features *>(c);
  }

  auto unlinkByType = [&](VkStructureType sType) -> VkBaseOutStructure * {
    VkBaseOutStructure *prev = reinterpret_cast<VkBaseOutStructure *>(&features2);
    while (prev->pNext)
    {
      if (prev->pNext->sType == sType)
      {
        VkBaseOutStructure *found = prev->pNext;
        prev->pNext = found->pNext;
        found->pNext = nullptr;
        return found;
      }
      prev = prev->pNext;
    }
    return nullptr;
  };

  if (v12)
  {
    if (auto *ts = reinterpret_cast<VkPhysicalDeviceTimelineSemaphoreFeatures *>(
          unlinkByType(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES)))
      v12->timelineSemaphore |= ts->timelineSemaphore;
    if (auto *f16i8 = reinterpret_cast<VkPhysicalDeviceShaderFloat16Int8Features *>(
          unlinkByType(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_FLOAT16_INT8_FEATURES)))
    {
      v12->shaderFloat16 |= f16i8->shaderFloat16;
      v12->shaderInt8 |= f16i8->shaderInt8;
    }
    if (auto *di = reinterpret_cast<VkPhysicalDeviceDescriptorIndexingFeatures *>(
          unlinkByType(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES)))
    {
      v12->shaderInputAttachmentArrayDynamicIndexing |= di->shaderInputAttachmentArrayDynamicIndexing;
      v12->shaderUniformTexelBufferArrayDynamicIndexing |= di->shaderUniformTexelBufferArrayDynamicIndexing;
      v12->shaderStorageTexelBufferArrayDynamicIndexing |= di->shaderStorageTexelBufferArrayDynamicIndexing;
      v12->shaderUniformBufferArrayNonUniformIndexing |= di->shaderUniformBufferArrayNonUniformIndexing;
      v12->shaderSampledImageArrayNonUniformIndexing |= di->shaderSampledImageArrayNonUniformIndexing;
      v12->shaderStorageBufferArrayNonUniformIndexing |= di->shaderStorageBufferArrayNonUniformIndexing;
      v12->shaderStorageImageArrayNonUniformIndexing |= di->shaderStorageImageArrayNonUniformIndexing;
      v12->shaderInputAttachmentArrayNonUniformIndexing |= di->shaderInputAttachmentArrayNonUniformIndexing;
      v12->shaderUniformTexelBufferArrayNonUniformIndexing |= di->shaderUniformTexelBufferArrayNonUniformIndexing;
      v12->shaderStorageTexelBufferArrayNonUniformIndexing |= di->shaderStorageTexelBufferArrayNonUniformIndexing;
      v12->descriptorBindingUniformBufferUpdateAfterBind |= di->descriptorBindingUniformBufferUpdateAfterBind;
      v12->descriptorBindingSampledImageUpdateAfterBind |= di->descriptorBindingSampledImageUpdateAfterBind;
      v12->descriptorBindingStorageImageUpdateAfterBind |= di->descriptorBindingStorageImageUpdateAfterBind;
      v12->descriptorBindingStorageBufferUpdateAfterBind |= di->descriptorBindingStorageBufferUpdateAfterBind;
      v12->descriptorBindingUniformTexelBufferUpdateAfterBind |= di->descriptorBindingUniformTexelBufferUpdateAfterBind;
      v12->descriptorBindingStorageTexelBufferUpdateAfterBind |= di->descriptorBindingStorageTexelBufferUpdateAfterBind;
      v12->descriptorBindingUpdateUnusedWhilePending |= di->descriptorBindingUpdateUnusedWhilePending;
      v12->descriptorBindingPartiallyBound |= di->descriptorBindingPartiallyBound;
      v12->descriptorBindingVariableDescriptorCount |= di->descriptorBindingVariableDescriptorCount;
      v12->runtimeDescriptorArray |= di->runtimeDescriptorArray;
    }
    v12->descriptorIndexing = VK_TRUE;
    v12->samplerMirrorClampToEdge = VK_TRUE;
  }
  if (v13)
  {
    if (auto *s2 = reinterpret_cast<VkPhysicalDeviceSynchronization2Features *>(
          unlinkByType(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES)))
      v13->synchronization2 |= s2->synchronization2;
    if (auto *pcc = reinterpret_cast<VkPhysicalDevicePipelineCreationCacheControlFeatures *>(
          unlinkByType(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PIPELINE_CREATION_CACHE_CONTROL_FEATURES)))
      v13->pipelineCreationCacheControl |= pcc->pipelineCreationCacheControl;
  }
}

void Xess::patchApiVersion(uint32_t &api_ver)
{
  if (!libxess)
    return;
  api_ver = reqApiVersion;
}

} // namespace drv3d_vulkan
