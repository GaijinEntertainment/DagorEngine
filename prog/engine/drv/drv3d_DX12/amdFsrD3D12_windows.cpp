// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "amdFsr.h"
#include <amdFsr3Helpers.h>

#include "driver_win.h"
#include "swapchain.h"

#include <drv/3d/dag_commands.h>
#include <drv/3d/dag_driverDesc.h>
#include <drv/3d/dag_info.h>
#include <drv/3d/dag_resetDevice.h>
#include <osApiWrappers/dag_dynLib.h>
#include <osApiWrappers/dag_versionQuery.h>
#include <memory/dag_framemem.h>
#include <drv_log_defs.h>
#include <3d/gpuLatency.h>
#include <supp/dag_comPtr.h>
#include <util/dag_finally.h>

#include <dx12/ffx_api_dx12.h>
#include <dx12/ffx_api_framegeneration_dx12.h>
#include <ffx_framegeneration.h>
#include <ffx_upscale.h>

#include <EASTL/fixed_vector.h>

ComPtr<DXGISwapChain> get_fg_swapchain();

namespace amd
{
namespace
{
FfxApiResource ffx_get_resource(void *texture, uint32_t state)
{
  if (texture)
    return ffxApiGetResourceDX12((ID3D12Resource *)texture, state);

  return {};
}

uint32_t convert(DXGI_FORMAT format)
{
  switch (format)
  {
    case DXGI_FORMAT_B8G8R8A8_UNORM: return FFX_API_SURFACE_FORMAT_B8G8R8A8_UNORM;
    default: G_ASSERT(false); return FFX_API_SURFACE_FORMAT_UNKNOWN;
  }
}

bool verify_dll_version(const char *dll, LibraryVersion version)
{
  if (auto queriedVersion = get_library_version(dll))
  {
    return version == queriedVersion;
  }

  return true;
};

} // namespace

FSRD3D12 *g_fsrD3D12 = nullptr; // With this only one FSR instance is allowed. When this will became a problem, please fix it

// On windows, we use FSR3.1
class FSRD3D12::Impl
{
public:
  using ContextArgs = FSR::ContextArgs;
  using UpscalingMode = FSR::UpscalingMode;
  using UpscalingPlatformArgs = FSR::UpscalingPlatformArgs;
  using FrameGenPlatformArgs = FSR::FrameGenPlatformArgs;

  Impl()
  {
    logdbg("[AMDFSR] Creating upscaling FSRD3D12...");

    fsrModule.reset(os_dll_load("amd_fidelityfx_loader_dx12.dll"));

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
      logdbg("[AMDFSR] FSRD3D12 created.");
    }
    else
    {
      logdbg("[AMDFSR] Failed to create FSRD3D12.");
    }
  }

  ~Impl()
  {
    d3d::driver_command(Drv3dCommand::ACQUIRE_OWNERSHIP);
    FINALLY([] { d3d::driver_command(Drv3dCommand::RELEASE_OWNERSHIP); });

    G_VERIFY(destroyContextIfNeeded(upscalingContext));
    G_VERIFY(destroyContextIfNeeded(framegenContext));
    G_VERIFY(destroyContextIfNeeded(swapchainContext));
  }

  bool initUpscaling(const ContextArgs &args)
  {
    logdbg("[AMDFSR] Initializing upscaling...");

    if (!isLoaded())
    {
      logdbg("[AMDFSR] Library not loaded. Failed.");
      return false;
    }

    ffxCreateBackendDX12Desc backendDesc{
      .header{
        .type = FFX_API_CREATE_CONTEXT_DESC_TYPE_BACKEND_DX12,
      },
      .device = static_cast<ID3D12Device *>(d3d::get_device()),
    };

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

  void teardownUpscaling()
  {
    enableFrameGeneration(false);

    if (!upscalingContext)
      return;

    G_VERIFY(destroyContextIfNeeded(upscalingContext));
    contextArgs.mode = UpscalingMode::Off;
  }

  Point2 getNextJitter(uint32_t render_width, uint32_t output_width)
  {
    return get_next_jitter(render_width, output_width, jitterIndex, upscalingContext, query);
  }

  bool doApplyUpscaling(const UpscalingPlatformArgs &args, void *command_list) const
  {
    return apply_upscaling(args, command_list, upscalingContext, dispatch, ffx_get_resource);
  }

  IPoint2 getRenderingResolution(UpscalingMode mode, const IPoint2 &target_resolution) const
  {
    return get_rendering_resolution(mode, target_resolution, upscalingContext, query);
  }

  UpscalingMode getUpscalingMode() const { return contextArgs.mode; }

  bool isLoaded() const { return createContext && destroyContext && configure && query && dispatch; }

  bool isUpscalingSupported() const { return isLoaded() && d3d::get_driver_desc().shaderModel >= 6.2_sm; }

  bool isFrameGenerationSupported() const { return isLoaded() && d3d::get_driver_desc().shaderModel >= 6.2_sm; }

  String getVersionString() const
  {
    initVersionString();
    return upscalingVersionString;
  }

  bool createFrameGenerationSwapchain(DXGIFactory *factory, ID3D12CommandQueue *queue,
    const drv3d_dx12::SwapchainCreateInfo &create_info, const DXGI_SWAP_CHAIN_DESC1 &swapchain_desc,
    const DXGI_SWAP_CHAIN_FULLSCREEN_DESC &fullscreen_desc, DXGISwapChain **swapchain)
  {
    G_ASSERT_RETURN(swapchain, false);

    if (!isFrameGenerationSupported())
      return false;

    if (!destroyContextIfNeeded(framegenContext) || !destroyContextIfNeeded(swapchainContext))
      return false;

    DXGI_SWAP_CHAIN_DESC1 mutableSwapchainDesc = swapchain_desc;
    DXGI_SWAP_CHAIN_FULLSCREEN_DESC mutableFullscreenDesc = fullscreen_desc;

    ComPtr<IDXGISwapChain4> newDxgiSwapchain;
    ffxCreateContextDescFrameGenerationSwapChainForHwndDX12 createSwapChainDesc{
      .header{
        .type = FFX_API_CREATE_CONTEXT_DESC_TYPE_FRAMEGENERATIONSWAPCHAIN_FOR_HWND_DX12,
      },
      .swapchain = newDxgiSwapchain.GetAddressOf(),
      .hwnd = create_info.window,
      .desc = &mutableSwapchainDesc,
      .fullscreenDesc = &mutableFullscreenDesc,
      .dxgiFactory = factory,
      .gameQueue = queue,
    };

    [[maybe_unused]] auto retCode = createContext(&swapchainContext, &createSwapChainDesc.header, nullptr);
    G_ASSERT_RETURN(retCode == FFX_API_RETURN_OK, false);

    ffxCreateBackendDX12Desc backendDesc{
      .header{
        .type = FFX_API_CREATE_CONTEXT_DESC_TYPE_BACKEND_DX12,
      },
      .device = static_cast<ID3D12Device *>(d3d::get_device()),
    };

    ffxCreateContextDescFrameGeneration createFg{
      .header{
        .type = FFX_API_CREATE_CONTEXT_DESC_TYPE_FRAMEGENERATION,
        .pNext = &backendDesc.header,
      },
      .flags =
        (contextArgs.invertedDepth ? FFX_FRAMEGENERATION_ENABLE_DEPTH_INVERTED : 0u) |
        ((contextArgs.enableHdr || create_info.enableHdr || create_info.forceHdr) ? FFX_FRAMEGENERATION_ENABLE_HIGH_DYNAMIC_RANGE
                                                                                  : 0u),
      .displaySize{
        .width = swapchain_desc.Width,
        .height = swapchain_desc.Height,
      },
      .maxRenderSize{
        .width = swapchain_desc.Width,
        .height = swapchain_desc.Height,
      },
      .backBufferFormat = convert(swapchain_desc.Format),
    };

    retCode = createContext(&framegenContext, &createFg.header, nullptr);
    if (retCode != FFX_API_RETURN_OK)
    {
      G_ASSERT_RETURN(destroyContextIfNeeded(swapchainContext), false);
      return false;
    }

    *swapchain = newDxgiSwapchain.Detach();
    return true;
  }

  void releaseFrameGenerationSwapchainContext()
  {
    enableFrameGeneration(false);

    G_VERIFY(destroyContextIfNeeded(framegenContext));
    G_VERIFY(destroyContextIfNeeded(swapchainContext));
  }

  void enableFrameGeneration(bool enable)
  {
    if (!framegenContext || !swapchainContext)
      return;

    if (enable == isFrameGenEnabled)
      return;

    d3d::driver_command(Drv3dCommand::ACQUIRE_OWNERSHIP);
    FINALLY([] { d3d::driver_command(Drv3dCommand::RELEASE_OWNERSHIP); });

    d3d::driver_command(Drv3dCommand::D3D_FLUSH);
    isFrameGenEnabled = enable;
  }

  void suppressFrameGeneration(bool suppress) { isFrameGenSuppressed = suppress; }

  void doScheduleGeneratedFrames(const FrameGenPlatformArgs &args, void *command_list)
  {
    G_ASSERT_RETURN(framegenContext && swapchainContext, );

    ComPtr<DXGISwapChain> dxgiSwapchain = get_fg_swapchain();

    if (auto latency = GpuLatency::getInstance(); latency && latency->getVendor() == GpuVendor::AMD)
    {
      // Anti Lag 2 data injection

      // {5083ae5b-8070-4fca-8ee5-3582dd367d13}
      static const GUID IID_IFfxAntiLag2Data{0x5083ae5b, 0x8070, 0x4fca, {0x8e, 0xe5, 0x35, 0x82, 0xdd, 0x36, 0x7d, 0x13}};

      struct AntiLag2Data
      {
        void *context;
        bool enabled;
      } data{
        .context = latency->getHandle(),
        .enabled = latency->isEnabled(),
      };
      dxgiSwapchain->SetPrivateData(IID_IFfxAntiLag2Data, sizeof(data), &data);
    }

    frameGenerationConfig = {
      .header{
        .type = FFX_API_CONFIGURE_DESC_TYPE_FRAMEGENERATION,
      },
      .swapChain = dxgiSwapchain.Get(),
      .frameGenerationEnabled = isFrameGenEnabled && !isFrameGenSuppressed,
      .allowAsyncWorkloads = true,
      .flags = 0, // FfxApiDispatchFrameGenerationFlags
      .generationRect{
        .width = args.outputResolution.x,
        .height = args.outputResolution.y,
      },
      .frameID = args.frameIndex,
    };
    [[maybe_unused]] auto retCode = configure(&framegenContext, &frameGenerationConfig.header);
    G_ASSERT(retCode == FFX_API_RETURN_OK);

    ffxDispatchDescFrameGenerationPrepare dispatchFgPrep{
      .header{
        .type = FFX_API_DISPATCH_DESC_TYPE_FRAMEGENERATION_PREPARE,
      },
      .frameID = args.frameIndex,
      .flags = args.debugView ? FFX_FRAMEGENERATION_FLAG_DRAW_DEBUG_TEAR_LINES : 0u,
      .commandList = command_list,
      .renderSize{
        .width = uint32_t(args.inputResolution.x),
        .height = uint32_t(args.inputResolution.y),
      },
      .jitterOffset{
        .x = args.jitter.x,
        .y = args.jitter.y,
      },
      .motionVectorScale{
        .x = args.motionVectorScale.x,
        .y = args.motionVectorScale.y,
      },
      .frameTimeDelta = args.timeElapsed * 1000.0f, // FSR expects milliseconds
      .unused_reset = args.reset,
      .cameraNear = args.nearPlane,
      .cameraFar = args.farPlane,
      .cameraFovAngleVertical = args.fovY,
      .viewSpaceToMetersFactor = 1,
      .depth = ffx_get_resource(args.depthTexture, FFX_API_RESOURCE_STATE_PIXEL_COMPUTE_READ),
      .motionVectors = ffx_get_resource(args.motionVectors, FFX_API_RESOURCE_STATE_PIXEL_COMPUTE_READ),
    };
    retCode = dispatch(&framegenContext, &dispatchFgPrep.header);
    G_ASSERT(retCode == FFX_API_RETURN_OK);

    ffxConfigureDescFrameGenerationSwapChainRegisterUiResourceDX12 uiConfig{
      .header{
        .type = FFX_API_CONFIGURE_DESC_TYPE_FRAMEGENERATIONSWAPCHAIN_REGISTERUIRESOURCE_DX12,
      },
      .uiResource = ffx_get_resource(args.uiTexture, FFX_API_RESOURCE_STATE_PIXEL_COMPUTE_READ),
      .flags = FFX_FRAMEGENERATION_UI_COMPOSITION_FLAG_ENABLE_INTERNAL_UI_DOUBLE_BUFFERING |
               FFX_FRAMEGENERATION_UI_COMPOSITION_FLAG_USE_PREMUL_ALPHA,
    };
    retCode = configure(&swapchainContext, &uiConfig.header);
    G_ASSERT(retCode == FFX_API_RETURN_OK);

    if (!isFrameGenSuppressed)
    {
      ffxDispatchDescFrameGeneration dispatchFg{
        .header{
          .type = FFX_API_DISPATCH_DESC_TYPE_FRAMEGENERATION,
        },
        .presentColor = ffx_get_resource(args.colorTexture, FFX_API_RESOURCE_STATE_COMMON),
        .numGeneratedFrames = 1,
        .reset = args.reset,
        .generationRect{
          .width = args.outputResolution.x,
          .height = args.outputResolution.y,
        },
        .frameID = args.frameIndex,
      };

      ffxQueryDescFrameGenerationSwapChainInterpolationCommandListDX12 queryCmdList{
        .header{
          .type = FFX_API_QUERY_DESC_TYPE_FRAMEGENERATIONSWAPCHAIN_INTERPOLATIONCOMMANDLIST_DX12,
        },
        .pOutCommandList = &dispatchFg.commandList,
      };
      retCode = query(&swapchainContext, &queryCmdList.header);
      G_ASSERT(retCode == FFX_API_RETURN_OK);

      ffxQueryDescFrameGenerationSwapChainInterpolationTextureDX12 queryFiTexture{
        .header{
          .type = FFX_API_QUERY_DESC_TYPE_FRAMEGENERATIONSWAPCHAIN_INTERPOLATIONTEXTURE_DX12,
        },
        .pOutTexture = &dispatchFg.outputs[0],
      };
      retCode = query(&swapchainContext, &queryFiTexture.header);
      G_ASSERT(retCode == FFX_API_RETURN_OK);

      retCode = dispatch(&framegenContext, &dispatchFg.header);
      G_ASSERT(retCode == FFX_API_RETURN_OK);
    }
  }

  int getPresentedFrameCount() { return (framegenContext && isFrameGenEnabled && !isFrameGenSuppressed) ? 2 : 1; }

  bool isFrameGenerationActive() const { return isFrameGenEnabled; }

  bool isFrameGenerationSuppressed() const { return isFrameGenSuppressed; }

  void preRecover()
  {
    d3d::driver_command(Drv3dCommand::D3D_FLUSH);

    G_VERIFY(destroyContextIfNeeded(framegenContext));
    G_VERIFY(destroyContextIfNeeded(swapchainContext));
    G_VERIFY(destroyContextIfNeeded(upscalingContext));
  }

  void recover()
  {
    // The regular tear down resets the mode to off, but the preRecover keeps the mode
    // and the mode isn't off here have to restore the fsr context and reinitialize the object
    if (getUpscalingMode() != amd::FSR::UpscalingMode::Off)
      initUpscaling(contextArgs);
  }

  uint64_t getMemorySize()
  {
    FfxApiEffectMemoryUsage usageInfo{};
    if (upscalingContext)
    {
      ffxQueryDescUpscaleGetGPUMemoryUsage usageQuery{
        .header =
          {
            .type = FFX_API_QUERY_DESC_TYPE_UPSCALE_GPU_MEMORY_USAGE,
          },
        .gpuMemoryUsageUpscaler = &usageInfo,
      };
      query(&upscalingContext, &usageQuery.header);
    }
    uint64_t totalSize = usageInfo.totalUsageInBytes;

    if (framegenContext)
    {
      usageInfo = {};
      ffxQueryDescFrameGenerationGetGPUMemoryUsage usageQuery{
        .header =
          {
            .type = FFX_API_QUERY_DESC_TYPE_FRAMEGENERATION_GPU_MEMORY_USAGE,
          },
        .gpuMemoryUsageFrameGeneration = &usageInfo,
      };
      query(&framegenContext, &usageQuery.header);
    }
    totalSize += usageInfo.totalUsageInBytes;

    if (swapchainContext)
    {
      usageInfo = {};
      ffxQueryFrameGenerationSwapChainGetGPUMemoryUsageDX12 usageQuery{
        .header = {.type = FFX_API_QUERY_DESC_TYPE_FRAMEGENERATIONSWAPCHAIN_GPU_MEMORY_USAGE_DX12},
        .gpuMemoryUsageFrameGenerationSwapchain = &usageInfo,
      };
      query(&swapchainContext, &usageQuery.header);
    }
    totalSize += usageInfo.totalUsageInBytes;

    return totalSize;
  }

private:
  DagorDynLibHolder fsrModule;

  PfnFfxCreateContext createContext = nullptr;
  PfnFfxDestroyContext destroyContext = nullptr;
  PfnFfxConfigure configure = nullptr;
  PfnFfxQuery query = nullptr;
  PfnFfxDispatch dispatch = nullptr;

  ffxContext upscalingContext = nullptr;
  ffxContext framegenContext = nullptr;
  ffxContext swapchainContext = nullptr;

  ffxConfigureDescFrameGeneration frameGenerationConfig = {};

  ContextArgs contextArgs = {};

  int32_t jitterIndex = 0;

  bool isFrameGenEnabled = false;
  bool isFrameGenSuppressed = false;

  mutable String upscalingVersionString = String(8, "");

  bool destroyContextIfNeeded(ffxContext &context)
  {
    if (!context)
      return true;

    [[maybe_unused]] auto retCode = destroyContext(&context, nullptr);
    G_ASSERT_RETURN(retCode == FFX_API_RETURN_OK, false);
    context = nullptr;
    return true;
  }

  void initVersionString() const
  {
    if (!upscalingVersionString.empty() || !query)
      return;

    auto device = d3d::get_device();
    if (!device)
      return;

    uint64_t versionCount = 0;
    ffxQueryDescGetVersions versionsDesc{
      .header{
        .type = FFX_API_QUERY_DESC_TYPE_GET_VERSIONS,
      },
      .createDescType = FFX_API_CREATE_CONTEXT_DESC_TYPE_UPSCALE,
      .device = device,
      .outputCount = &versionCount,
    };
    if (query(nullptr, &versionsDesc.header) != FFX_API_RETURN_OK || versionCount == 0)
      return;

    const int MAX_VERSION_COUNT = 32;
    eastl::fixed_vector<uint64_t, MAX_VERSION_COUNT> versionIds;
    eastl::fixed_vector<const char *, MAX_VERSION_COUNT> versionNames;
    versionIds.resize(versionCount);
    versionNames.resize(versionCount);
    versionsDesc.versionIds = versionIds.data();
    versionsDesc.versionNames = versionNames.data();

    if (query(nullptr, &versionsDesc.header) != FFX_API_RETURN_OK)
      return;

    for (int verIx = 0; verIx < versionCount; ++verIx)
      logdbg("[AMDFSR] provider %llu - %s", versionIds[verIx], versionNames[verIx]);

    upscalingVersionString = versionNames.front();
  }
};

FSRD3D12::FSRD3D12() : pImpl(eastl::make_unique<Impl>())
{
  G_ASSERT(!g_fsrD3D12);
  g_fsrD3D12 = this;
}

FSRD3D12::~FSRD3D12() { g_fsrD3D12 = nullptr; }

bool FSRD3D12::initUpscaling(const FSR::ContextArgs &args) { return pImpl->initUpscaling(args); }

void FSRD3D12::teardownUpscaling() { pImpl->teardownUpscaling(); }

Point2 FSRD3D12::getNextJitter(uint32_t render_width, uint32_t output_width)
{
  return pImpl->getNextJitter(render_width, output_width);
}

bool FSRD3D12::doApplyUpscaling(const FSR::UpscalingPlatformArgs &args, void *command_list) const
{
  return pImpl->doApplyUpscaling(args, command_list);
}

IPoint2 FSRD3D12::getRenderingResolution(FSR::UpscalingMode mode, const IPoint2 &target_resolution) const
{
  return pImpl->getRenderingResolution(mode, target_resolution);
}

FSR::UpscalingMode FSRD3D12::getUpscalingMode() const { return pImpl->getUpscalingMode(); }

bool FSRD3D12::isLoaded() const { return pImpl->isLoaded(); }

bool FSRD3D12::isUpscalingSupported() const { return pImpl->isUpscalingSupported(); }

bool FSRD3D12::isFrameGenerationSupported() const { return pImpl->isFrameGenerationSupported(); }

String FSRD3D12::getVersionString() const { return pImpl->getVersionString(); }

void FSRD3D12::enableFrameGeneration(bool enable) { pImpl->enableFrameGeneration(enable); }

void FSRD3D12::suppressFrameGeneration(bool suppress) { pImpl->suppressFrameGeneration(suppress); }

void FSRD3D12::doScheduleGeneratedFrames(const FSR::FrameGenPlatformArgs &args, void *command_list)
{
  pImpl->doScheduleGeneratedFrames(args, command_list);
}

int FSRD3D12::getPresentedFrameCount() { return pImpl->getPresentedFrameCount(); }

bool FSRD3D12::isFrameGenerationActive() const { return pImpl->isFrameGenerationActive(); }

bool FSRD3D12::isFrameGenerationSuppressed() const { return pImpl->isFrameGenerationSuppressed(); }

void FSRD3D12::preRecover() { pImpl->preRecover(); }

void FSRD3D12::recover() { pImpl->recover(); }

bool FSRD3D12::createFrameGenerationSwapchain(DXGIFactory *factory, ID3D12CommandQueue *queue,
  const drv3d_dx12::SwapchainCreateInfo &create_info, const DXGI_SWAP_CHAIN_DESC1 &swapchain_desc,
  const DXGI_SWAP_CHAIN_FULLSCREEN_DESC &fullscreen_desc, DXGISwapChain **swapchain)
{
  return pImpl->createFrameGenerationSwapchain(factory, queue, create_info, swapchain_desc, fullscreen_desc, swapchain);
}

void FSRD3D12::releaseFrameGenerationSwapchainContext() { pImpl->releaseFrameGenerationSwapchainContext(); }

uint64_t FSRD3D12::getMemorySize() { return pImpl->getMemorySize(); }

} // namespace amd

static void fsr3_before_reset(bool full_reset)
{
  if (full_reset && amd::g_fsrD3D12)
    amd::g_fsrD3D12->preRecover();
}

static void fsr3_after_reset(bool full_reset)
{
  if (full_reset && amd::g_fsrD3D12)
    amd::g_fsrD3D12->recover();
}

REGISTER_D3D_BEFORE_RESET_FUNC(fsr3_before_reset);
REGISTER_D3D_AFTER_RESET_FUNC(fsr3_after_reset);
