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

class FSRVulkan;
namespace
{
FSRVulkan *vkFsrInstance = nullptr; // With this only one FSR instance is allowed. When this will became a problem, please fix it
}

class FSRVulkan : public FSR
{
public:
  FSRVulkan()
  {
    logdbg("[AMDFSR] Creating upscaling FSRVulkan...");
    G_ASSERT(!vkFsrInstance);
    vkFsrInstance = this;
    loadLib();
  }

  ~FSRVulkan()
  {
    fsrModule.reset();
    vkFsrInstance = nullptr;
  }

  void loadLib()
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

  void teardownUpscaling() override
  {
    logdbg("[AMDFSR] Teardown upscaling.");

    if (!upscalingContext)
      return;

    destroyContext(&upscalingContext, nullptr);
    upscalingContext = nullptr;
    contextArgs.mode = UpscalingMode::Off;
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

  UpscalingMode getUpscalingMode() const override { return contextArgs.mode; }

  bool isLoadedImpl() const { return fsrModule && createContext && destroyContext && configure && query && dispatch; }

  bool isLoaded() const override { return isLoadedImpl(); }

  bool isUpscalingSupported() const override { return isLoaded() && d3d::get_driver_desc().shaderModel >= 6.2_sm; }

  bool isFrameGenerationSupported() const override { return false; }

  void enableFrameGeneration(bool enable) override { G_UNUSED(enable); }

  void suppressFrameGeneration(bool suppress) override { G_UNUSED(suppress); }

  void doScheduleGeneratedFrames(const FrameGenPlatformArgs &args, void *command_list) override
  {
    G_UNUSED(args);
    G_UNUSED(command_list);
  }

  int getPresentedFrameCount() override { return 1; }

  bool isFrameGenerationActive() const override { return false; }

  bool isFrameGenerationSuppressed() const override { return false; }

  String getVersionString() const override { return upscalingVersionString; }

  void before_reset()
  {
    d3d::driver_command(Drv3dCommand::D3D_FLUSH);

    if (upscalingContext)
    {
      destroyContext(&upscalingContext, nullptr);
      upscalingContext = nullptr;
    }

    fsrModule.reset();
  }

  void after_reset()
  {
    // The regular tear down resets the mode to off, but the preRecover keeps the mode
    // and the mode isn't off here have to restore the fsr context and reinitialize the object
    loadLib();
    if (getUpscalingMode() != amd::FSR::UpscalingMode::Off)
      initUpscaling(contextArgs);
  }

private:
  DagorDynLibHolder fsrModule;

  PfnFfxCreateContext createContext = nullptr;
  PfnFfxDestroyContext destroyContext = nullptr;
  PfnFfxConfigure configure = nullptr;
  PfnFfxQuery query = nullptr;
  PfnFfxDispatch dispatch = nullptr;

  ffxContext upscalingContext = nullptr;

  ContextArgs contextArgs = {};

  int32_t jitterIndex = 0;
  String upscalingVersionString = String(8, "");
};

FSR *createVulkan() { return new FSRVulkan; }

} // namespace amd

static void fsr_vk_before_reset(bool full_reset)
{
  if (full_reset && amd::vkFsrInstance)
    amd::vkFsrInstance->before_reset();
}

static void fsr_vk_after_reset(bool full_reset)
{
  if (full_reset && amd::vkFsrInstance)
    amd::vkFsrInstance->after_reset();
}

REGISTER_D3D_BEFORE_RESET_FUNC(fsr_vk_before_reset);
REGISTER_D3D_AFTER_RESET_FUNC(fsr_vk_after_reset);
