// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <amdFsr.h>
#include <amdFsr3Helpers.h>

#include "driver_win.h"

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


eastl::tuple<ComPtr<DXGIFactory>, ComPtr<DXGISwapChain>, ComPtr<ID3D12CommandQueue>> get_fg_initializers();
ComPtr<DXGISwapChain> get_fg_swapchain();
void shutdown_internal_swapchain();
bool adopt_external_swapchain(DXGISwapChain *swapchain);
void create_default_swapchain();

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

uint32_t parse_fsr_major_version(const char *version_string)
{
  uint32_t majorVersion;
  sscanf(version_string, "%d", &majorVersion);
  return majorVersion;
}

} // namespace

class FSRD3D12Win;

FSRD3D12Win *fSRD3D12Win = nullptr; // With this only one FSR instance is allowed. When this will became a problem, please fix it

// On windows, we use FSR3.1
class FSRD3D12Win final : public FSR
{
public:
  FSRD3D12Win()
  {
    logdbg("[AMDFSR] Creating upscaling FSRD3D12Win...");

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
      uint64_t versionCount = 0;
      ffxQueryDescGetVersions versionsDesc{
        .header{
          .type = FFX_API_QUERY_DESC_TYPE_GET_VERSIONS,
        },
        .createDescType = FFX_API_CREATE_CONTEXT_DESC_TYPE_UPSCALE,
        .device = d3d::get_device(),
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
          upscalingMajorVersion = parse_fsr_major_version(versionNames.front());
          upscalingVersionString = versionNames.front();
        }
      }

      logdbg("[AMDFSR] FSRD3D12Win created. Upscaling use %s.", getVersionString().data());
    }
    else
    {
      logdbg("[AMDFSR] Failed to create FSRD3D12Win.");
    }

    G_ASSERT(!fSRD3D12Win);
    fSRD3D12Win = this;
  }

  ~FSRD3D12Win()
  {
    d3d::driver_command(Drv3dCommand::ACQUIRE_OWNERSHIP);
    FINALLY([] { d3d::driver_command(Drv3dCommand::RELEASE_OWNERSHIP); });

    if (upscalingContext)
    {
      [[maybe_unused]] auto retCode = destroyContext(&upscalingContext, nullptr);
      G_ASSERT(retCode == FFX_API_RETURN_OK);
    }

    if (framegenContext)
    {
      [[maybe_unused]] auto retCode = destroyContext(&framegenContext, nullptr);
      G_ASSERT(retCode == FFX_API_RETURN_OK);
    }

    if (swapchainContext)
    {
      [[maybe_unused]] auto retCode = destroyContext(&swapchainContext, nullptr);
      G_ASSERT(retCode == FFX_API_RETURN_OK);
      shutdown_internal_swapchain();
    }

    fSRD3D12Win = nullptr;
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

  void teardownUpscaling() override
  {
    enableFrameGeneration(false);

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

  bool isLoaded() const override { return createContext && destroyContext && configure && query && dispatch; }

  bool isUpscalingSupported() const override { return isLoaded() && d3d::get_driver_desc().shaderModel >= 6.2_sm; }

  bool isFrameGenerationSupported() const override { return isLoaded() && d3d::get_driver_desc().shaderModel >= 6.2_sm; }

  String getVersionString() const override { return upscalingVersionString; }

  void enableFrameGeneration(bool enable) override
  {
    if (enable == bool(framegenContext)) [[likely]]
      return;

    d3d::driver_command(Drv3dCommand::ACQUIRE_OWNERSHIP);
    FINALLY([] { d3d::driver_command(Drv3dCommand::RELEASE_OWNERSHIP); });

    d3d::driver_command(Drv3dCommand::D3D_FLUSH);

    if (enable)
    {
      auto [dxgiFactory, dxgiSwapchain, d3dGraphicsQueue] = get_fg_initializers();

      G_ASSERT_RETURN(dxgiSwapchain, );

      DXGI_SWAP_CHAIN_DESC swapchainDesc;
      dxgiSwapchain->GetDesc(&swapchainDesc);
      HWND hwnd;
      dxgiSwapchain->GetHwnd(&hwnd);
      DXGI_SWAP_CHAIN_DESC1 desc1;
      dxgiSwapchain->GetDesc1(&desc1);
      DXGI_SWAP_CHAIN_FULLSCREEN_DESC fullscreenDesc;
      dxgiSwapchain->GetFullscreenDesc(&fullscreenDesc);
      dxgiSwapchain.Reset();

      // Remove the old swapchain first, because only one swapchain can exist for the window with flip model
      shutdown_internal_swapchain();

      ComPtr<IDXGISwapChain4> newDxgiSwapchain;
      // Make the framegen swapchain and replace the driver one with it
      ffxCreateContextDescFrameGenerationSwapChainForHwndDX12 createSwapChainDesc{
        .header{
          .type = FFX_API_CREATE_CONTEXT_DESC_TYPE_FRAMEGENERATIONSWAPCHAIN_FOR_HWND_DX12,
        },
        .swapchain = newDxgiSwapchain.GetAddressOf(),
        .hwnd = hwnd,
        .desc = &desc1,
        .fullscreenDesc = &fullscreenDesc,
        .dxgiFactory = dxgiFactory.Get(),
        .gameQueue = d3dGraphicsQueue.Get(),
      };

      [[maybe_unused]] auto retCode = createContext(&swapchainContext, &createSwapChainDesc.header, nullptr);
      G_ASSERT(retCode == FFX_API_RETURN_OK);

      if (!adopt_external_swapchain(newDxgiSwapchain.Get()))
      {
        create_default_swapchain();
        return;
      }

      // Make the framegen context
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
        .flags = (contextArgs.invertedDepth ? FFX_FRAMEGENERATION_ENABLE_DEPTH_INVERTED : 0u) |
                 (contextArgs.enableHdr ? FFX_FRAMEGENERATION_ENABLE_HIGH_DYNAMIC_RANGE : 0u),
        .displaySize{
          .width = contextArgs.outputWidth,
          .height = contextArgs.outputHeight,
        },
        .maxRenderSize{
          .width = contextArgs.outputWidth,
          .height = contextArgs.outputHeight,
        },
        .backBufferFormat = convert(swapchainDesc.BufferDesc.Format),
      };

      retCode = createContext(&framegenContext, &createFg.header, nullptr);
      G_ASSERT(retCode == FFX_API_RETURN_OK);
    }
    else if (swapchainContext)
    {
      // A flush with wait and then reset the swapchain from the backend!
      shutdown_internal_swapchain();

      [[maybe_unused]] auto retCode = destroyContext(&swapchainContext, nullptr);
      G_ASSERT(retCode == FFX_API_RETURN_OK);
      swapchainContext = nullptr;

      if (framegenContext)
      {
        retCode = destroyContext(&framegenContext, nullptr);
        G_ASSERT(retCode == FFX_API_RETURN_OK);
        framegenContext = nullptr;
      }

      create_default_swapchain();
    }
  }

  void suppressFrameGeneration(bool suppress) override { isFrameGenSuppressed = suppress; }

  void doScheduleGeneratedFrames(const FrameGenPlatformArgs &args, void *command_list) override
  {
    G_ASSERT_RETURN(framegenContext, );

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
      .frameGenerationEnabled = !isFrameGenSuppressed,
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

  int getPresentedFrameCount() override { return (framegenContext && !isFrameGenSuppressed) ? 2 : 1; }

  bool isFrameGenerationActive() const override { return !!framegenContext; }

  bool isFrameGenerationSuppressed() const override { return isFrameGenSuppressed; }

  void preRecover()
  {
    d3d::driver_command(Drv3dCommand::D3D_FLUSH);

    if (framegenContext)
    {
      [[maybe_unused]] auto retCode = destroyContext(&framegenContext, nullptr);
      G_ASSERT(retCode == FFX_API_RETURN_OK);
      framegenContext = nullptr;
    }

    if (swapchainContext)
    {
      [[maybe_unused]] auto retCode = destroyContext(&swapchainContext, nullptr);
      G_ASSERT(retCode == FFX_API_RETURN_OK);
      swapchainContext = nullptr;
    }

    if (upscalingContext)
    {
      destroyContext(&upscalingContext, nullptr);
      upscalingContext = nullptr;
    }
  }

  void recover()
  {
    // The regular tear down resets the mode to off, but the preRecover keeps the mode
    // and the mode isn't off here have to restore the fsr context and reinitialize the object
    if (getUpscalingMode() != amd::FSR::UpscalingMode::Off)
      initUpscaling(contextArgs);
    if (contextArgs.enableFrameGeneration)
      enableFrameGeneration(true);
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

  ffxConfigureDescFrameGeneration frameGenerationConfig;

  ContextArgs contextArgs = {};

  int32_t jitterIndex = 0;

  bool isFrameGenSuppressed = false;

  uint32_t upscalingMajorVersion = 0;
  String upscalingVersionString = String(8, "");
};

FSR *createD3D12Win() { return new FSRD3D12Win; }

uint64_t getFSRMemorySize()
{
  if (!fSRD3D12Win)
  {
    return 0;
  }
  return fSRD3D12Win->getMemorySize();
}

} // namespace amd

static void fsr3_before_reset(bool full_reset)
{
  if (full_reset && amd::fSRD3D12Win)
    amd::fSRD3D12Win->preRecover();
}

static void fsr3_after_reset(bool full_reset)
{
  if (full_reset && amd::fSRD3D12Win)
    amd::fSRD3D12Win->recover();
}

REGISTER_D3D_BEFORE_RESET_FUNC(fsr3_before_reset);
REGISTER_D3D_AFTER_RESET_FUNC(fsr3_after_reset);
