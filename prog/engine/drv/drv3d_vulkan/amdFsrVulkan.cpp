// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "amdFsr.h"

#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_info.h>
#include <drv/3d/dag_commands.h>
#include <osApiWrappers/dag_dynLib.h>
#include <osApiWrappers/dag_unicode.h>
#include <ioSys/dag_dataBlock.h>
#include <ffx_api/ffx_upscale.hpp>

#define VK_NO_PROTOTYPES

#include <ffx_api/vk/ffx_api_vk.hpp>

#include "amdFsr3Helpers.h"

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
  logdbg("[AMDFSR] vkGetDeviceProcAddr: %s -> 0x%p", pName, addr);

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

class FSRVulkan : public FSR
{
public:
  FSRVulkan()
  {
    logdbg("[AMDFSR] Creating upscaling FSRVulkan...");

#if _TARGET_PC_WIN
    fsrModule.reset(os_dll_load("amd_fidelityfx_vk.dll"));
#endif

    if (fsrModule)
    {
      createContext = (PfnFfxCreateContext)os_dll_get_symbol(fsrModule.get(), "ffxCreateContext");
      destroyContext = (PfnFfxDestroyContext)os_dll_get_symbol(fsrModule.get(), "ffxDestroyContext");
      configure = (PfnFfxConfigure)os_dll_get_symbol(fsrModule.get(), "ffxConfigure");
      query = (PfnFfxQuery)os_dll_get_symbol(fsrModule.get(), "ffxQuery");
      dispatch = (PfnFfxDispatch)os_dll_get_symbol(fsrModule.get(), "ffxDispatch");
    }

    if (isLoaded())
    {
      logdbg("[AMDFSR] FSRVulkan created.");
    }
    else
    {
      logdbg("[AMDFSR] Failed to create FSRVulkan.");
    }
  }

  bool initUpscaling(const ContextArgs &args)
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

    ffx::CreateContextDescUpscale desc = convert(args, backendDesc.header);

    auto status = createContext(&upscalingContext, &desc.header, nullptr);
    if (status != FFX_API_RETURN_OK)
    {
      logerr("[AMDFSR] Failed to create FSR context: %s", get_error_string(status));
      return false;
    }

    upscalingMode = args.mode;

    logdbg("[AMDFSR] Upscaling initialized.");

    return true;
  }

  void teardownUpscaling() override
  {
    logdbg("[AMDFSR] Teardown upscaling.");

    if (!upscalingContext)
      return;

    destroyContext(&upscalingContext, nullptr);
    upscalingContext = nullptr;
    upscalingMode = UpscalingMode::Off;
  }

  Point2 getNextJitter(uint32_t render_width, uint32_t output_width)
  {
    return get_next_jitter(render_width, output_width, jitterIndex, upscalingContext, query);
  }

  bool doApplyUpscaling(const UpscalingPlatformArgs &args, void *command_list) const override
  {
    return apply_upscaling(args, command_list, upscalingContext, dispatch, ffx_get_resource);
  }

  IPoint2 getRenderingResolution(UpscalingMode mode, const IPoint2 &target_resolution) const override
  {
    return get_rendering_resolution(mode, target_resolution, upscalingContext, query);
  }

  UpscalingMode getUpscalingMode() const override { return upscalingMode; }

  bool isLoaded() const override { return createContext && destroyContext && configure && query && dispatch; }

  bool isUpscalingSupported() const override { return isLoaded() && d3d::get_driver_desc().shaderModel >= 6.2_sm; }

  bool isFrameGenerationSupported() const override { return false; }

private:
  DagorDynLibHolder fsrModule;

  PfnFfxCreateContext createContext = nullptr;
  PfnFfxDestroyContext destroyContext = nullptr;
  PfnFfxConfigure configure = nullptr;
  PfnFfxQuery query = nullptr;
  PfnFfxDispatch dispatch = nullptr;

  ffxContext upscalingContext = nullptr;

  UpscalingMode upscalingMode = UpscalingMode::Off;

  uint32_t jitterIndex = 0;
};

FSR *createVulkan() { return new FSRVulkan; }

} // namespace amd