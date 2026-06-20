// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "amdFsr.h"

#include <drv/3d/dag_driverDesc.h>
#include <drv/3d/dag_info.h>
#include <drv/3d/dag_commands.h>
#include <drv/3d/dag_resetDevice.h>
#include <drv_log_defs.h>
#include <debug/dag_assert.h>
#include <EASTL/utility.h>
#include <EASTL/vector.h>
#include <osApiWrappers/dag_unicode.h>

#include "driver.h"
#include "globals.h"
#include "vulkan_loader.h"
#include "vulkan_device.h"
#include "vulkan_instance.h"
#include "physical_device_set.h"

#include <host/ffxm_fsr2.h>
#include <host/backends/vk/ffxm_vk.h>

namespace amd
{

namespace
{
FSRVulkan *fsr_vulkan_instance = nullptr;

struct ArmAsrState
{
  arm::FfxmFsr2Context context = {};
  bool contextCreated = false;
  eastl::vector<uint8_t> scratchBuffer;
  arm::FfxmInterface backendInterface = {};
  arm::VkDeviceContext vkDeviceContext = {};
} g_state;

arm::FfxmSurfaceFormat vk_format_to_ffxm(VkFormat fmt)
{
  switch (fmt)
  {
    case VK_FORMAT_R8_UNORM: return arm::FFXM_SURFACE_FORMAT_R8_UNORM;
    case VK_FORMAT_R8_SNORM: return arm::FFXM_SURFACE_FORMAT_R8_SNORM;
    case VK_FORMAT_R8_UINT: return arm::FFXM_SURFACE_FORMAT_R8_UINT;
    case VK_FORMAT_R16_UNORM: return arm::FFXM_SURFACE_FORMAT_R16_UNORM;
    case VK_FORMAT_R16_SNORM: return arm::FFXM_SURFACE_FORMAT_R16_SNORM;
    case VK_FORMAT_R16_UINT: return arm::FFXM_SURFACE_FORMAT_R16_UINT;
    case VK_FORMAT_R16_SFLOAT: return arm::FFXM_SURFACE_FORMAT_R16_FLOAT;
    case VK_FORMAT_R32_UINT: return arm::FFXM_SURFACE_FORMAT_R32_UINT;
    case VK_FORMAT_R32_SFLOAT: return arm::FFXM_SURFACE_FORMAT_R32_FLOAT;
    case VK_FORMAT_D32_SFLOAT: return arm::FFXM_SURFACE_FORMAT_R32_FLOAT;
    case VK_FORMAT_R8G8_UNORM: return arm::FFXM_SURFACE_FORMAT_R8G8_UNORM;
    case VK_FORMAT_R16G16_SFLOAT: return arm::FFXM_SURFACE_FORMAT_R16G16_FLOAT;
    case VK_FORMAT_R16G16_UINT: return arm::FFXM_SURFACE_FORMAT_R16G16_UINT;
    case VK_FORMAT_R32G32_SFLOAT: return arm::FFXM_SURFACE_FORMAT_R32G32_FLOAT;
    case VK_FORMAT_B10G11R11_UFLOAT_PACK32: return arm::FFXM_SURFACE_FORMAT_R11G11B10_FLOAT;
    case VK_FORMAT_R8G8B8A8_UNORM: return arm::FFXM_SURFACE_FORMAT_R8G8B8A8_UNORM;
    case VK_FORMAT_R8G8B8A8_SNORM: return arm::FFXM_SURFACE_FORMAT_R8G8B8A8_SNORM;
    case VK_FORMAT_R8G8B8A8_SRGB: return arm::FFXM_SURFACE_FORMAT_R8G8B8A8_SRGB;
    case VK_FORMAT_B8G8R8A8_UNORM: return arm::FFXM_SURFACE_FORMAT_B8G8R8A8_UNORM;
    case VK_FORMAT_B8G8R8A8_SRGB: return arm::FFXM_SURFACE_FORMAT_B8G8R8A8_SRGB;
    case VK_FORMAT_R16G16B16A16_SFLOAT: return arm::FFXM_SURFACE_FORMAT_R16G16B16A16_FLOAT;
    case VK_FORMAT_R32G32B32A32_UINT: return arm::FFXM_SURFACE_FORMAT_R32G32B32A32_UINT;
    case VK_FORMAT_R32G32B32A32_SFLOAT: return arm::FFXM_SURFACE_FORMAT_R32G32B32A32_FLOAT;
    default: return arm::FFXM_SURFACE_FORMAT_UNKNOWN;
  }
}

arm::FfxmResource make_ffxm_resource(void *texture, bool is_uav, bool is_depth, arm::FfxmResourceStates state)
{
  if (!texture)
    return {};
  auto dt = (eastl::pair<VkImage, VkImageCreateInfo> *)(texture);
  if (!dt->first)
    return {};

  arm::FfxmResourceDescription desc = {};
  desc.type = arm::FFXM_RESOURCE_TYPE_TEXTURE2D;
  desc.format = vk_format_to_ffxm(dt->second.format);
  desc.width = dt->second.extent.width;
  desc.height = dt->second.extent.height;
  desc.depth = 1;
  desc.mipCount = dt->second.mipLevels ? dt->second.mipLevels : 1;
  desc.flags = arm::FFXM_RESOURCE_FLAGS_NONE;
  desc.usage =
    is_uav ? arm::FFXM_RESOURCE_USAGE_UAV : (is_depth ? arm::FFXM_RESOURCE_USAGE_DEPTHTARGET : arm::FFXM_RESOURCE_USAGE_READ_ONLY);

  return arm::ffxmGetResourceVK(reinterpret_cast<void *>(dt->first), desc, nullptr, state);
}

arm::FfxmFsr2ShaderQualityMode mode_to_shader_quality(FSR::UpscalingMode mode)
{
  switch (mode)
  {
    case FSR::UpscalingMode::NativeAA:
    case FSR::UpscalingMode::Quality: return arm::FFXM_FSR2_SHADER_QUALITY_MODE_QUALITY;
    case FSR::UpscalingMode::Balanced: return arm::FFXM_FSR2_SHADER_QUALITY_MODE_BALANCED;
    case FSR::UpscalingMode::Performance: return arm::FFXM_FSR2_SHADER_QUALITY_MODE_PERFORMANCE;
    case FSR::UpscalingMode::UltraPerformance: return arm::FFXM_FSR2_SHADER_QUALITY_MODE_ULTRA_PERFORMANCE;
    default: return arm::FFXM_FSR2_SHADER_QUALITY_MODE_QUALITY;
  }
}

arm::FfxmFsr2UpscalingRatio mode_to_upscaling_ratio(FSR::UpscalingMode mode)
{
  switch (mode)
  {
    case FSR::UpscalingMode::Quality: return arm::FFXM_FSR2_UPSCALING_RATIO_X1_5;
    case FSR::UpscalingMode::Balanced: return arm::FFXM_FSR2_UPSCALING_RATIO_X1_7;
    case FSR::UpscalingMode::Performance: return arm::FFXM_FSR2_UPSCALING_RATIO_X2;
    case FSR::UpscalingMode::NativeAA:
      G_ASSERTF(false, "[ARMASR] NativeAA (1x) has no equivalent upscaling ratio, falling back to X1.5");
      return arm::FFXM_FSR2_UPSCALING_RATIO_X1_5;
    case FSR::UpscalingMode::UltraPerformance:
      G_ASSERTF(false, "[ARMASR] UltraPerformance (3x) has no equivalent upscaling ratio, falling back to X2");
      return arm::FFXM_FSR2_UPSCALING_RATIO_X2;
    default: return arm::FFXM_FSR2_UPSCALING_RATIO_X1_5;
  }
}

void message_callback(arm::FfxmMsgType type, const wchar_t *message)
{
  char narrowMessage[1024];
  wcs_to_utf8(message, narrowMessage, sizeof(narrowMessage));
  if (type == arm::FfxmMsgType::FFXM_MESSAGE_TYPE_ERROR)
    D3D_ERROR("[ARMASR]: %s", narrowMessage);
  else
    logwarn("[ARMASR]: %s", narrowMessage);
}

void release_state()
{
  if (g_state.contextCreated)
  {
    arm::ffxmFsr2ContextDestroy(&g_state.context);
    g_state.contextCreated = false;
    g_state.context = {};
  }
  g_state.scratchBuffer.clear();
  g_state.scratchBuffer.shrink_to_fit();
  g_state.backendInterface = {};
  g_state.vkDeviceContext = {};
}
} // namespace

void FSRVulkan::loadLib()
{
  // ARM ASR is a static library; nothing to load.
  upscalingVersionString.printf(32, "ARM ASR %d.%d.%d", FFXM_FSR2_VERSION_MAJOR, FFXM_FSR2_VERSION_MINOR, FFXM_FSR2_VERSION_PATCH);
  logdbg("[ARMASR] FSRVulkan created. Version: %s", upscalingVersionString.c_str());
}

bool FSRVulkan::initUpscaling(const FSR::ContextArgs &args)
{
  logdbg("[ARMASR] Initializing upscaling...");

  G_ASSERTF(!args.enableFrameGeneration, "[ARMASR] Frame generation is not supported by ARM ASR");

  if (g_state.contextCreated)
  {
    logdbg("[ARMASR] Context already exists, recreating.");
    release_state();
  }

  VkPhysicalDevice physicalDevice = drv3d_vulkan::Globals::VK::phy.device;
  VkDevice device = drv3d_vulkan::Globals::VK::dev.get();

  g_state.vkDeviceContext.vkDevice = device;
  g_state.vkDeviceContext.vkPhysicalDevice = physicalDevice;
  g_state.vkDeviceContext.vkDeviceProcAddr = drv3d_vulkan::Globals::VK::loader.vkGetDeviceProcAddr;
  g_state.vkDeviceContext.vkInstance = drv3d_vulkan::Globals::VK::inst.get();
  g_state.vkDeviceContext.vkGetInstanceProcAddr = drv3d_vulkan::Globals::VK::loader.vkGetInstanceProcAddr;

  size_t scratchSize =
    arm::ffxmGetScratchMemorySizeVK(physicalDevice, 1, drv3d_vulkan::Globals::VK::inst.vkEnumerateDeviceExtensionProperties);
  g_state.scratchBuffer.resize(scratchSize);

  arm::FfxmDevice ffxmDevice = arm::ffxmGetDeviceVK(&g_state.vkDeviceContext);

  arm::FfxmErrorCode err =
    arm::ffxmGetInterfaceVK(&g_state.backendInterface, ffxmDevice, g_state.scratchBuffer.data(), g_state.scratchBuffer.size(), 1);
  if (err != arm::FFXM_OK)
  {
    D3D_ERROR("[ARMASR] ffxmGetInterfaceVK failed: 0x%x", err);
    release_state();
    return false;
  }

  arm::FfxmFsr2ContextDescription desc = {};
  desc.qualityMode = mode_to_shader_quality(args.mode);
  desc.flags = arm::FFXM_FSR2_ENABLE_AUTO_EXPOSURE;
  if (args.invertedDepth)
    desc.flags |= arm::FFXM_FSR2_ENABLE_DEPTH_INVERTED;
  if (args.enableHdr)
    desc.flags |= arm::FFXM_FSR2_ENABLE_HIGH_DYNAMIC_RANGE;
  if (args.enableDebug)
    desc.flags |= arm::FFXM_FSR2_ENABLE_DEBUG_CHECKING;
  desc.maxRenderSize.width = args.maxRenderWidth ? args.maxRenderWidth : args.outputWidth;
  desc.maxRenderSize.height = args.maxRenderHeight ? args.maxRenderHeight : args.outputHeight;
  desc.displaySize.width = args.outputWidth;
  desc.displaySize.height = args.outputHeight;
  desc.backendInterface = g_state.backendInterface;
  desc.fpMessage = message_callback;

  err = arm::ffxmFsr2ContextCreate(&g_state.context, &desc);
  if (err != arm::FFXM_OK)
  {
    D3D_ERROR("[ARMASR] ffxmFsr2ContextCreate failed: 0x%x", err);
    release_state();
    return false;
  }

  g_state.contextCreated = true;
  contextArgs = args;
  logdbg("[ARMASR] Upscaling initialized.");
  return true;
}

void FSRVulkan::teardownUpscaling()
{
  logdbg("[ARMASR] Teardown upscaling.");
  release_state();
  contextArgs.mode = FSR::UpscalingMode::Off;
}

Point2 FSRVulkan::getNextJitter(uint32_t render_width, uint32_t output_width)
{
  if (!g_state.contextCreated)
    return Point2::ZERO;
  ++jitterIndex;
  Point2 jitter = Point2::ZERO;
  int32_t phaseCount = arm::ffxmFsr2GetJitterPhaseCount(int32_t(render_width), int32_t(output_width));
  if (phaseCount > 0)
    arm::ffxmFsr2GetJitterOffset(&jitter.x, &jitter.y, jitterIndex, phaseCount);
  return jitter;
}

bool FSRVulkan::doApplyUpscaling(const FSR::UpscalingPlatformArgs &args, void *command_list) const
{
  if (!g_state.contextCreated)
    return false;

  arm::FfxmFsr2DispatchDescription desc = {};
  desc.commandList = arm::ffxmGetCommandListVK(static_cast<VkCommandBuffer>(command_list));
  desc.color = make_ffxm_resource(args.colorTexture, false, false, arm::FFXM_RESOURCE_STATE_PIXEL_COMPUTE_READ);
  desc.depth = make_ffxm_resource(args.depthTexture, false, true, arm::FFXM_RESOURCE_STATE_PIXEL_COMPUTE_READ);
  desc.motionVectors = make_ffxm_resource(args.motionVectors, false, false, arm::FFXM_RESOURCE_STATE_PIXEL_COMPUTE_READ);
  desc.exposure = make_ffxm_resource(args.exposureTexture, false, false, arm::FFXM_RESOURCE_STATE_PIXEL_COMPUTE_READ);
  desc.reactive = make_ffxm_resource(args.reactiveTexture, false, false, arm::FFXM_RESOURCE_STATE_PIXEL_COMPUTE_READ);
  desc.transparencyAndComposition =
    make_ffxm_resource(args.transparencyAndCompositionTexture, false, false, arm::FFXM_RESOURCE_STATE_PIXEL_COMPUTE_READ);
  desc.output = make_ffxm_resource(args.outputTexture, false, false, arm::FFXM_RESOURCE_STATE_PIXEL_WRITE);
  desc.jitterOffset.x = args.jitter.x;
  desc.jitterOffset.y = args.jitter.y;
  desc.motionVectorScale.x = args.motionVectorScale.x;
  desc.motionVectorScale.y = args.motionVectorScale.y;
  desc.renderSize.width = uint32_t(args.inputResolution.x);
  desc.renderSize.height = uint32_t(args.inputResolution.y);
  desc.enableSharpening = args.sharpness > 0;
  desc.sharpness = args.sharpness;
  desc.frameTimeDelta = args.timeElapsed * 1000.0f; // ARM ASR expects milliseconds
  desc.preExposure = args.preExposure;
  desc.reset = args.reset;
  desc.cameraNear = args.nearPlane;
  desc.cameraFar = args.farPlane;
  desc.cameraFovAngleVertical = args.fovY;
  desc.viewSpaceToMetersFactor = 1.0f;

  arm::FfxmErrorCode err = arm::ffxmFsr2ContextDispatch(const_cast<arm::FfxmFsr2Context *>(&g_state.context), &desc);
  if (err != arm::FFXM_OK)
  {
    D3D_ERROR("[ARMASR] ffxmFsr2ContextDispatch failed: 0x%x", err);
    return false;
  }
  return true;
}

IPoint2 FSRVulkan::getRenderingResolution(FSR::UpscalingMode mode, const IPoint2 &target_resolution) const
{
  if (mode == FSR::UpscalingMode::Off)
    return target_resolution;

  uint32_t renderWidth = 0;
  uint32_t renderHeight = 0;
  arm::FfxmErrorCode err = arm::ffxmFsr2GetRenderResolutionFromUpscalingRatio(&renderWidth, &renderHeight,
    uint32_t(target_resolution.x), uint32_t(target_resolution.y), mode_to_upscaling_ratio(mode));
  if (err != arm::FFXM_OK)
    return target_resolution;
  return IPoint2(int(renderWidth), int(renderHeight));
}

FSR::UpscalingMode FSRVulkan::getUpscalingMode() const { return contextArgs.mode; }

bool FSRVulkan::isLoadedImpl() const { return true; }

bool FSRVulkan::isLoaded() const { return true; }

bool FSRVulkan::isUpscalingSupported() const
{
  // ARM ASR FSR2 shaders rely on half support with subgroup extended types and subgroup quad shuffles
  const auto &phy = drv3d_vulkan::Globals::VK::phy;
  if (!phy.hasShaderFloat16)
    return false;
  if (!phy.hasShaderSubgroupExtendedTypes)
    return false;
  if (0 == (phy.subgroupProperties.supportedOperations & VK_SUBGROUP_FEATURE_QUAD_BIT))
    return false;
  if (0 == (phy.subgroupProperties.supportedStages & (VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT)))
    return false;
  return true;
}

String FSRVulkan::getVersionString() const { return upscalingVersionString; }

void FSRVulkan::beforeReset()
{
  d3d::driver_command(Drv3dCommand::D3D_FLUSH);
  release_state();
}

void FSRVulkan::afterReset()
{
  loadLib();
  if (getUpscalingMode() != FSR::UpscalingMode::Off)
    initUpscaling(contextArgs);
}

FSRVulkan &FSRVulkan::getInstance()
{
  static FSRVulkan instance;
  fsr_vulkan_instance = &instance;
  return instance;
}

FSRVulkan *FSRVulkan::getExistingInstance() { return fsr_vulkan_instance; }

FSRVulkan::FSRVulkan()
{
  logdbg("[ARMASR] Creating upscaling FSRVulkan...");
  loadLib();
}

FSRVulkan::~FSRVulkan()
{
  teardownUpscaling();
  if (fsr_vulkan_instance == this)
    fsr_vulkan_instance = nullptr;
}

} // namespace amd

static void fsr_vk_before_reset(bool full_reset)
{
  if (full_reset)
    if (amd::FSRVulkan *fsr = amd::FSRVulkan::getExistingInstance())
      fsr->beforeReset();
}

static void fsr_vk_after_reset(bool full_reset)
{
  if (full_reset)
    if (amd::FSRVulkan *fsr = amd::FSRVulkan::getExistingInstance())
      fsr->afterReset();
}

REGISTER_D3D_BEFORE_RESET_FUNC(fsr_vk_before_reset);
REGISTER_D3D_AFTER_RESET_FUNC(fsr_vk_after_reset);
