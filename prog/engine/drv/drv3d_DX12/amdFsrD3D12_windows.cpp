// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <amdFsr.h>
#include <amdFsr3Helpers.h>

#include "driver_win.h"

#include <drv/3d/dag_commands.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_info.h>
#include <drv/3d/dag_resetDevice.h>
#include <ioSys/dag_dataBlock.h>
#include <osApiWrappers/dag_dynLib.h>
#include <memory/dag_framemem.h>
#include <drv_log_defs.h>
#include <3d/gpuLatency.h>
#include <supp/dag_comPtr.h>
#include <util/dag_finally.h>

#include <dx12/ffx_api_dx12.hpp>
#include <dx12/ffx_api_framegeneration_dx12.hpp>
#include <ffx_framegeneration.hpp>
#include <ffx_upscale.hpp>

void get_fg_initializers(DXGIFactory *&factory, DXGISwapChain *&swapchain, ID3D12CommandQueue *&graphicsQueue);
void shutdown_internal_swapchain();
bool adopt_external_swapchain(DXGISwapChain *swapchain);
void create_default_swapchain();


namespace amd
{

static inline FfxApiResource ffx_get_resource(void *texture, uint32_t state)
{
  FfxApiResource apiRes = {};
  if (!texture)
    return apiRes;

  apiRes = ffxApiGetResourceDX12((ID3D12Resource *)texture, state);
  return apiRes;
}

inline FfxApiSurfaceFormat convert(DXGI_FORMAT format)
{
  switch (format)
  {
    case DXGI_FORMAT_B8G8R8A8_UNORM: return FFX_API_SURFACE_FORMAT_B8G8R8A8_UNORM;
    default: G_ASSERT(false); return FFX_API_SURFACE_FORMAT_UNKNOWN;
  }
}

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
      ffx::QueryDescGetVersions versionsDesc{};
      versionsDesc.createDescType = FFX_API_CREATE_CONTEXT_DESC_TYPE_UPSCALE;
      versionsDesc.device = d3d::get_device();
      versionsDesc.outputCount = &versionCount;
      if (query(nullptr, &versionsDesc.header) == FFX_API_RETURN_OK)
      {
        dag::Vector<uint64_t, framemem_allocator> versionIds;
        dag::Vector<const char *, framemem_allocator> versionNames;
        versionIds.resize(versionCount);
        versionNames.resize(versionCount);
        versionsDesc.versionIds = versionIds.data();
        versionsDesc.versionNames = versionNames.data();

        if (versionCount > 0 && query(nullptr, &versionsDesc.header) == FFX_API_RETURN_OK)
        {
          for (int verIx = 0; verIx < versionCount; ++verIx)
            logdbg("[AMDFSR] provider %llu - %s", versionIds[verIx], versionNames[verIx]);

          // The first returned provider is FSR3 upscaling
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

    ffx::CreateBackendDX12Desc backendDesc{};
    backendDesc.header.type = FFX_API_CREATE_CONTEXT_DESC_TYPE_BACKEND_DX12;
    backendDesc.device = static_cast<ID3D12Device *>(d3d::get_device());

    ffx::CreateContextDescUpscale desc = convert(args, backendDesc.header);

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
    if (enable)
    {
      if (framegenContext)
        return;

      d3d::driver_command(Drv3dCommand::ACQUIRE_OWNERSHIP);
      FINALLY([] { d3d::driver_command(Drv3dCommand::RELEASE_OWNERSHIP); });

      ComPtr<DXGIFactory> dxgiFactory;
      ComPtr<DXGISwapChain> dxgiSwapchain;
      ComPtr<ID3D12CommandQueue> d3dGraphicsQueue;
      get_fg_initializers(*dxgiFactory.GetAddressOf(), *dxgiSwapchain.GetAddressOf(), *d3dGraphicsQueue.GetAddressOf());

      G_ASSERT_RETURN(dxgiSwapchain, );

      DXGI_SWAP_CHAIN_DESC swapchainDesc = {};
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
      ffx::CreateContextDescFrameGenerationSwapChainForHwndDX12 createSwapChainDesc{};
      createSwapChainDesc.swapchain = newDxgiSwapchain.GetAddressOf();
      createSwapChainDesc.hwnd = hwnd;
      createSwapChainDesc.desc = &desc1;
      createSwapChainDesc.fullscreenDesc = &fullscreenDesc;
      createSwapChainDesc.dxgiFactory = dxgiFactory.Get();
      createSwapChainDesc.gameQueue = d3dGraphicsQueue.Get();

      [[maybe_unused]] auto retCode = createContext(&swapchainContext, &createSwapChainDesc.header, nullptr);
      G_ASSERT(retCode == FFX_API_RETURN_OK);

      if (!adopt_external_swapchain(newDxgiSwapchain.Get()))
      {
        create_default_swapchain();
        return;
      }

      // Make the framegen context
      ffx::CreateContextDescFrameGeneration createFg{};
      if (contextArgs.invertedDepth)
        createFg.flags |= FFX_FRAMEGENERATION_ENABLE_DEPTH_INVERTED;
      if (contextArgs.enableHdr)
        createFg.flags |= FFX_FRAMEGENERATION_ENABLE_HIGH_DYNAMIC_RANGE;
      createFg.displaySize.width = contextArgs.outputWidth;
      createFg.displaySize.height = contextArgs.outputHeight;
      createFg.maxRenderSize.width = contextArgs.outputWidth;
      createFg.maxRenderSize.height = contextArgs.outputHeight;
      createFg.backBufferFormat = convert(swapchainDesc.BufferDesc.Format);

      ffx::CreateBackendDX12Desc backendDesc{};
      backendDesc.header.type = FFX_API_CREATE_CONTEXT_DESC_TYPE_BACKEND_DX12;
      backendDesc.device = static_cast<ID3D12Device *>(d3d::get_device());

      createFg.header.pNext = &backendDesc.header;

      retCode = createContext(&framegenContext, &createFg.header, nullptr);
      G_ASSERT(retCode == FFX_API_RETURN_OK);
    }
    else
    {
      d3d::driver_command(Drv3dCommand::ACQUIRE_OWNERSHIP);
      FINALLY([] { d3d::driver_command(Drv3dCommand::RELEASE_OWNERSHIP); });

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

        // Reset the swapchain from the backend!
        shutdown_internal_swapchain();
        create_default_swapchain();
      }
    }
  }

  void suppressFrameGeneration(bool suppress) override { isFrameGenSuppressed = suppress; }

  void doScheduleGeneratedFrames(const FrameGenPlatformArgs &args, void *command_list) override
  {
    G_ASSERT_RETURN(framegenContext, );

    ComPtr<DXGIFactory> dxgiFactory;
    ComPtr<DXGISwapChain> dxgiSwapchain;
    ComPtr<ID3D12CommandQueue> d3dGraphicsQueue;
    get_fg_initializers(*dxgiFactory.GetAddressOf(), *dxgiSwapchain.GetAddressOf(), *d3dGraphicsQueue.GetAddressOf());

    if (auto latency = GpuLatency::getInstance(); latency && latency->getVendor() == GpuVendor::AMD)
    {
      // Anti Lag 2 data injection

      // {5083ae5b-8070-4fca-8ee5-3582dd367d13}
      static const GUID IID_IFfxAntiLag2Data = {0x5083ae5b, 0x8070, 0x4fca, {0x8e, 0xe5, 0x35, 0x82, 0xdd, 0x36, 0x7d, 0x13}};

      struct AntiLag2Data
      {
        void *context;
        bool enabled;
      } data;

      data.context = latency->getHandle();
      data.enabled = latency->isEnabled();
      dxgiSwapchain->SetPrivateData(IID_IFfxAntiLag2Data, sizeof(data), &data);
    }

    frameGenerationConfig.swapChain = dxgiSwapchain.Get();
    frameGenerationConfig.presentCallback = nullptr;
    frameGenerationConfig.presentCallbackUserContext = nullptr;
    frameGenerationConfig.frameGenerationCallback = nullptr;
    frameGenerationConfig.frameGenerationCallbackUserContext = nullptr;
    frameGenerationConfig.frameGenerationEnabled = !isFrameGenSuppressed;
    frameGenerationConfig.allowAsyncWorkloads = true;
    frameGenerationConfig.HUDLessColor = FfxApiResource({});
    frameGenerationConfig.flags = 0; // FfxApiDispatchFrameGenerationFlags
    frameGenerationConfig.onlyPresentGenerated = false;
    frameGenerationConfig.generationRect.left = 0;
    frameGenerationConfig.generationRect.top = 0;
    frameGenerationConfig.generationRect.width = args.outputResolution.x;
    frameGenerationConfig.generationRect.height = args.outputResolution.y;
    frameGenerationConfig.frameID = args.frameIndex;

    auto retCode = configure(&framegenContext, &frameGenerationConfig.header);
    G_ASSERT(retCode == FFX_API_RETURN_OK);

    ffx::DispatchDescFrameGenerationPrepare dispatchFgPrep{};
    dispatchFgPrep.frameID = args.frameIndex;
    dispatchFgPrep.flags = args.debugView ? FFX_FRAMEGENERATION_FLAG_DRAW_DEBUG_TEAR_LINES : 0;
    dispatchFgPrep.commandList = command_list;
    dispatchFgPrep.renderSize.width = args.inputResolution.x;
    dispatchFgPrep.renderSize.height = args.inputResolution.y;
    dispatchFgPrep.jitterOffset.x = args.jitter.x;
    dispatchFgPrep.jitterOffset.y = args.jitter.y;
    dispatchFgPrep.motionVectorScale.x = args.motionVectorScale.x;
    dispatchFgPrep.motionVectorScale.y = args.motionVectorScale.y;
    dispatchFgPrep.frameTimeDelta = args.timeElapsed * 1000.0f; // FSR expects milliseconds
    dispatchFgPrep.unused_reset = args.reset;
    dispatchFgPrep.cameraNear = args.nearPlane;
    dispatchFgPrep.cameraFar = args.farPlane;
    dispatchFgPrep.cameraFovAngleVertical = args.fovY;
    dispatchFgPrep.viewSpaceToMetersFactor = 1;
    dispatchFgPrep.depth = ffx_get_resource(args.depthTexture, FFX_API_RESOURCE_STATE_PIXEL_COMPUTE_READ);
    dispatchFgPrep.motionVectors = ffx_get_resource(args.motionVectors, FFX_API_RESOURCE_STATE_PIXEL_COMPUTE_READ);

    retCode = dispatch(&framegenContext, &dispatchFgPrep.header);
    G_ASSERT(retCode == FFX_API_RETURN_OK);

    ffx::ConfigureDescFrameGenerationSwapChainRegisterUiResourceDX12 uiConfig{};
    uiConfig.uiResource = ffx_get_resource(args.uiTexture, FFX_API_RESOURCE_STATE_PIXEL_COMPUTE_READ);
    uiConfig.flags = FFX_FRAMEGENERATION_UI_COMPOSITION_FLAG_ENABLE_INTERNAL_UI_DOUBLE_BUFFERING;
    retCode = configure(&swapchainContext, &uiConfig.header);
    G_ASSERT(retCode == FFX_API_RETURN_OK);

    if (!isFrameGenSuppressed)
    {
      ffx::DispatchDescFrameGeneration dispatchFg{};
      dispatchFg.presentColor = ffx_get_resource(args.colorTexture, FFX_API_RESOURCE_STATE_COMMON);
      dispatchFg.numGeneratedFrames = 1;
      dispatchFg.generationRect.left = 0;
      dispatchFg.generationRect.top = 0;
      dispatchFg.generationRect.width = args.outputResolution.x;
      dispatchFg.generationRect.height = args.outputResolution.y;
      dispatchFg.frameID = args.frameIndex;
      dispatchFg.reset = args.reset;
      dispatchFg.minMaxLuminance[0] = 0;
      dispatchFg.minMaxLuminance[1] = 0;
      dispatchFg.backbufferTransferFunction = 0;
      for (auto &t : dispatchFg.outputs)
        t = FfxApiResource({});

      ffx::QueryDescFrameGenerationSwapChainInterpolationCommandListDX12 queryCmdList{};
      queryCmdList.pOutCommandList = &dispatchFg.commandList;
      retCode = query(&swapchainContext, &queryCmdList.header);
      G_ASSERT(retCode == FFX_API_RETURN_OK);

      ffx::QueryDescFrameGenerationSwapChainInterpolationTextureDX12 queryFiTexture{};
      queryFiTexture.pOutTexture = &dispatchFg.outputs[0];
      retCode = query(&swapchainContext, &queryFiTexture.header);
      G_ASSERT(retCode == FFX_API_RETURN_OK);

      retCode = dispatch(&framegenContext, &dispatchFg.header);
      G_ASSERT(retCode == FFX_API_RETURN_OK);

      G_UNUSED(retCode);
    }
  }

  int getPresentedFrameCount() override { return (framegenContext && !isFrameGenSuppressed) ? 2 : 1; }

  bool isFrameGenerationActive() const override { return !!framegenContext; }

  bool isFrameGenerationSuppressed() const override { return isFrameGenSuppressed; }

  void preRecover()
  {
    auto tmpContextArgs = contextArgs;

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
      contextArgs.mode = UpscalingMode::Off;
    }

    contextArgs = tmpContextArgs;
  }

  void recover()
  {
    // The regular tear down resets the mode to off, but the preRecover keeps the mode
    // and the mode isn't off here have to restore the fsr context and reinitialize the object
    if (amd::fSRD3D12Win->getUpscalingMode() != amd::FSR::UpscalingMode::Off)
      initUpscaling(contextArgs);
    if (contextArgs.enableFrameGeneration)
      enableFrameGeneration(true);
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

  ffx::ConfigureDescFrameGeneration frameGenerationConfig;

  ContextArgs contextArgs = {};

  uint32_t jitterIndex = 0;

  bool isFrameGenSuppressed = false;

  String upscalingVersionString = String(8, "");
};

FSR *createD3D12Win() { return new FSRD3D12Win; }

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
