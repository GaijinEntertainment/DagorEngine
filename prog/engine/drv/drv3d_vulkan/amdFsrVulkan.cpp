// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "amdFsr.h"

#include <drv/3d/dag_driverDesc.h>
#include <drv/3d/dag_info.h>
#include <drv/3d/dag_commands.h>
#include <osApiWrappers/dag_dynLib.h>
#include <ffx_api/ffx_upscale.hpp>
#include <drv_log_defs.h>
#include <memory/dag_framemem.h>
#include <drv/3d/dag_resetDevice.h>

#define VK_NO_PROTOTYPES

#include <ffx_api/vk/ffx_api_vk.hpp>

#include "amdFsr3Helpers.h"

#include "driver.h"
#include "globals.h"
#include "vulkan_loader.h"

namespace
{
static VkResult VKAPI_PTR vkSetDebugUtilsObjectNameEXT(VkDevice, const VkDebugUtilsObjectNameInfoEXT *) { return VK_SUCCESS; }
static void VKAPI_PTR vkCmdBeginDebugUtilsLabelEXT(VkCommandBuffer, const VkDebugUtilsLabelEXT *) {}
static void VKAPI_PTR vkCmdEndDebugUtilsLabelEXT(VkCommandBuffer) {}
} // namespace

namespace amd
{

namespace
{
FSRVulkan *fsr_vulkan_instance = nullptr;
}

static inline FfxApiResource ffx_get_resource(void *texture, uint32_t state)
{
  FfxApiResource apiRes = {};
  if (!texture)
    return apiRes;

  auto dt = (eastl::pair<VkImage, VkImageCreateInfo> *)(texture);
  if (!dt->first)
    return apiRes;

  apiRes = ffxApiGetResourceVK(dt->first, ffxApiGetImageResourceDescriptionVK(dt->first, dt->second, 0), state);
  return apiRes;
}

// From https://github.com/GPUOpen-LibrariesAndSDKs/FidelityFX-SDK/issues/84
static PFN_vkVoidFunction VKAPI_ATTR override_vkGetDeviceProcAddr(VkDevice device, const char *pName)
{
  auto addr = drv3d_vulkan::Globals::VK::loader.vkGetDeviceProcAddr(device, pName);
  logdbg("[AMDFSR] vkGetDeviceProcAddr: %s -> 0x%p", pName, (void *)addr);

  if (!addr)
  {
#if _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4191)
#endif

    if (strcmp(pName, "vkSetDebugUtilsObjectNameEXT") == 0)
      addr = (PFN_vkVoidFunction)vkSetDebugUtilsObjectNameEXT;
    else if (strcmp(pName, "vkCmdBeginDebugUtilsLabelEXT") == 0)
      addr = (PFN_vkVoidFunction)vkCmdBeginDebugUtilsLabelEXT;
    else if (strcmp(pName, "vkCmdEndDebugUtilsLabelEXT") == 0)
      addr = (PFN_vkVoidFunction)vkCmdEndDebugUtilsLabelEXT;

#if _MSC_VER
#pragma warning(pop)
#endif

    G_ASSERTF(addr, "[AMDFSR] Failed to load VK function: %s", pName);
  }

  return addr;
}

void FSRVulkan::loadLib()
{
#if _TARGET_PC_WIN
  fsrModule.reset(os_dll_load("amd_fidelityfx_vk.dll"));
#elif _TARGET_PC_LINUX
  fsrModule.reset(os_dll_load("libamd_fidelityfx_vk.so"));
#endif

  if (fsrModule)
  {
    createContext = (PfnFfxCreateContext)os_dll_get_symbol(fsrModule.get(), "ffxCreateContext");
    destroyContext = (PfnFfxDestroyContext)os_dll_get_symbol(fsrModule.get(), "ffxDestroyContext");
    configure = (PfnFfxConfigure)os_dll_get_symbol(fsrModule.get(), "ffxConfigure");
    query = (PfnFfxQuery)os_dll_get_symbol(fsrModule.get(), "ffxQuery");
    dispatch = (PfnFfxDispatch)os_dll_get_symbol(fsrModule.get(), "ffxDispatch");
  }

  if (isLoadedImpl())
  {
    logdbg("[AMDFSR] FSRVulkan created.");
    uint64_t versionCount = 0;
    ffxQueryDescGetVersions versionsDesc{
      .header{
        .type = FFX_API_QUERY_DESC_TYPE_GET_VERSIONS,
      },
      .createDescType = FFX_API_CREATE_CONTEXT_DESC_TYPE_UPSCALE,
      .outputCount = &versionCount,
    };
    if (query(nullptr, &versionsDesc.header) == FFX_API_RETURN_OK && versionCount > 0)
    {
      dag::Vector<uint64_t, framemem_allocator> versionIds;
      dag::Vector<const char *, framemem_allocator> versionNames;
      versionIds.resize_noinit(versionCount);
      versionNames.resize_noinit(versionCount);
      versionsDesc.versionIds = versionIds.data();
      versionsDesc.versionNames = versionNames.data();

      if (query(nullptr, &versionsDesc.header) == FFX_API_RETURN_OK)
      {
        for (int verIx = 0; verIx < versionCount; ++verIx)
          logdbg("[AMDFSR] provider %llu - %s", versionIds[verIx], versionNames[verIx]);

        // The first returned provider is FSR upscaling
        upscalingVersionString = versionNames.front();
      }
    }
  }
  else
  {
    logdbg("[AMDFSR] Failed to create FSRVulkan.");
  }
}

bool FSRVulkan::initUpscaling(const FSR::ContextArgs &args)
{
  logdbg("[AMDFSR] Initializing upscaling...");

  if (!isLoaded())
  {
    logdbg("[AMDFSR] Library not loaded. Failed.");
    return false;
  }

  ffx::CreateBackendVKDesc backendDesc{};

  backendDesc.header.type = FFX_API_CREATE_CONTEXT_DESC_TYPE_BACKEND_VK;
  backendDesc.vkDevice = (VkDevice)d3d::get_device();
  d3d::driver_command(Drv3dCommand::GET_PHYSICAL_DEVICE, &backendDesc.vkPhysicalDevice);

  backendDesc.vkDeviceProcAddr = override_vkGetDeviceProcAddr;

  ffxCreateContextDescUpscale desc = convert(args, backendDesc.header);

  auto status = createContext(&upscalingContext, &desc.header, nullptr);
  if (status != FFX_API_RETURN_OK)
  {
    D3D_ERROR("[AMDFSR] Failed to create FSR context: %s", get_error_string(status));
    return false;
  }

  contextArgs = args;

  logdbg("[AMDFSR] Upscaling initialized.");

  return true;
}

void FSRVulkan::teardownUpscaling()
{
  logdbg("[AMDFSR] Teardown upscaling.");

  if (!upscalingContext)
    return;

  destroyContext(&upscalingContext, nullptr);
  upscalingContext = nullptr;
  contextArgs.mode = FSR::UpscalingMode::Off;
}

Point2 FSRVulkan::getNextJitter(uint32_t render_width, uint32_t output_width)
{
  return upscalingContext && query ? get_next_jitter(render_width, output_width, jitterIndex, upscalingContext, query) : Point2::ZERO;
}

bool FSRVulkan::doApplyUpscaling(const FSR::UpscalingPlatformArgs &args, void *command_list) const
{
  return upscalingContext && apply_upscaling(args, command_list, upscalingContext, dispatch, ffx_get_resource);
}

IPoint2 FSRVulkan::getRenderingResolution(FSR::UpscalingMode mode, const IPoint2 &target_resolution) const
{
  return isLoaded() ? get_rendering_resolution(mode, target_resolution, upscalingContext, query) : IPoint2::ZERO;
}

FSR::UpscalingMode FSRVulkan::getUpscalingMode() const { return contextArgs.mode; }

bool FSRVulkan::isLoadedImpl() const { return fsrModule && createContext && destroyContext && configure && query && dispatch; }

bool FSRVulkan::isLoaded() const { return isLoadedImpl(); }

bool FSRVulkan::isUpscalingSupported() const { return isLoaded() && d3d::get_driver_desc().shaderModel >= 6.2_sm; }

String FSRVulkan::getVersionString() const { return upscalingVersionString; }

void FSRVulkan::beforeReset()
{
  d3d::driver_command(Drv3dCommand::D3D_FLUSH);

  if (upscalingContext)
  {
    destroyContext(&upscalingContext, nullptr);
    upscalingContext = nullptr;
  }

  fsrModule.reset();
}

void FSRVulkan::afterReset()
{
  // The regular tear down resets the mode to off, but the preRecover keeps the mode
  // and the mode isn't off here have to restore the fsr context and reinitialize the object
  loadLib();
  if (getUpscalingMode() != amd::FSR::UpscalingMode::Off)
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
  logdbg("[AMDFSR] Creating upscaling FSRVulkan...");
  loadLib();
}

FSRVulkan::~FSRVulkan()
{
  teardownUpscaling();
  fsrModule.reset();

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
