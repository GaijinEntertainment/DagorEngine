// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "fsr_wrapper.h"

#include "amdFsr.h"
#include "swapchain.h"
#include "resource_manager/image.h"

namespace drv3d_dx12
{
namespace
{
FsrWrapper *active_fsr_wrapper = nullptr;

amd::FSR::UpscalingPlatformArgs to_platform_args(const FsrParamsDx12 &params)
{
  amd::FSR::UpscalingPlatformArgs result = params;
  result.colorTexture = get_handle(params.colorTexture);
  result.depthTexture = get_handle(params.depthTexture);
  result.motionVectors = get_handle(params.motionVectors);
  result.exposureTexture = get_handle(params.exposureTexture);
  result.outputTexture = get_handle(params.outputTexture);
  result.reactiveTexture = get_handle(params.reactiveTexture);
  result.transparencyAndCompositionTexture = get_handle(params.transparencyAndCompositionTexture);
  return result;
}

amd::FSR::FrameGenPlatformArgs to_platform_args(const FsrFgParamsDx12 &params)
{
  amd::FSR::FrameGenPlatformArgs result = params;
  result.colorTexture = get_handle(params.colorTexture);
  result.depthTexture = get_handle(params.depthTexture);
  result.motionVectors = get_handle(params.motionVectors);
  result.uiTexture = get_handle(params.uiTexture);
  return result;
}
} // namespace

bool FsrWrapper::fsrInit()
{
  if (!pImpl)
    pImpl = eastl::make_unique<amd::FSRD3D12>();
  return pImpl->isLoaded();
}

bool FsrWrapper::fsrCreateFeature(const ContextArgs &args)
{
  pImpl->teardownUpscaling();
  return pImpl->initUpscaling(args);
}

bool FsrWrapper::fsrShutdown()
{
  pImpl->teardownUpscaling();
  pImpl.reset();
  return true;
}

void FsrWrapper::teardownUpscaling() { pImpl->teardownUpscaling(); }

bool FsrWrapper::evaluateFsr(void *command_list, const FsrParamsDx12 &params)
{
  return pImpl->doApplyUpscaling(to_platform_args(params), command_list);
}

Point2 FsrWrapper::getNextJitter(uint32_t render_width, uint32_t output_width)
{
  return pImpl->getNextJitter(render_width, output_width);
}

IPoint2 FsrWrapper::getFsrRenderResolution(UpscalingMode mode, const IPoint2 &target_resolution) const
{
  return pImpl->getRenderingResolution(mode, target_resolution);
}

FsrWrapper::UpscalingMode FsrWrapper::getUpscalingMode() const { return pImpl->getUpscalingMode(); }

bool FsrWrapper::isLoaded() const { return pImpl->isLoaded(); }

bool FsrWrapper::isUpscalingSupported() const { return pImpl->isUpscalingSupported(); }

bool FsrWrapper::isFrameGenerationSupported() const { return pImpl->isFrameGenerationSupported(); }

String FsrWrapper::getVersion() const { return pImpl->getVersionString(); }

bool FsrWrapper::isFrameGenerationEnabled() const { return pImpl->isFrameGenerationActive(); }

bool FsrWrapper::isFrameGenerationSuppressed() const { return pImpl->isFrameGenerationSuppressed(); }

void FsrWrapper::enableFrameGeneration(bool enable) { pImpl->enableFrameGeneration(enable); }

void FsrWrapper::suppressFrameGeneration(bool suppress) { pImpl->suppressFrameGeneration(suppress); }

void FsrWrapper::doScheduleGeneratedFrames(const amd::FSR::FrameGenPlatformArgs &args, void *command_list)
{
  pImpl->doScheduleGeneratedFrames(args, command_list);
}

int FsrWrapper::getPresentedFrameCount() { return pImpl->getPresentedFrameCount(); }

uint64_t FsrWrapper::getMemoryUsage() const
{
#if _TARGET_PC_WIN && _TARGET_64BIT && !_M_ARM64
  return pImpl->getMemorySize();
#else
  return 0;
#endif
}

#if _TARGET_PC_WIN
bool FsrWrapper::createFrameGenerationSwapchain(DXGIFactory *factory, ID3D12CommandQueue *queue,
  const SwapchainCreateInfo &create_info, const DXGI_SWAP_CHAIN_DESC1 &swapchain_desc,
  const DXGI_SWAP_CHAIN_FULLSCREEN_DESC &fullscreen_desc, DXGISwapChain **swapchain)
{
  if (pImpl == nullptr)
    fsrInit();

  return pImpl->createFrameGenerationSwapchain(factory, queue, create_info, swapchain_desc, fullscreen_desc, swapchain);
}

void FsrWrapper::releaseFrameGenerationSwapchainContext()
{
  // may be called when shut down already
  if (pImpl)
  {
    pImpl->releaseFrameGenerationSwapchainContext();
  }
}

bool create_fsrfg_swapchain(DXGIFactory *factory, ID3D12CommandQueue *queue, const SwapchainCreateInfo &create_info,
  const DXGI_SWAP_CHAIN_DESC1 &swapchain_desc, const DXGI_SWAP_CHAIN_FULLSCREEN_DESC &fullscreen_desc, DXGISwapChain **swapchain)
{
  return active_fsr_wrapper &&
         active_fsr_wrapper->createFrameGenerationSwapchain(factory, queue, create_info, swapchain_desc, fullscreen_desc, swapchain);
}
#endif

FsrWrapper::FsrWrapper() { active_fsr_wrapper = this; }

FsrWrapper::~FsrWrapper()
{
  if (active_fsr_wrapper == this)
    active_fsr_wrapper = nullptr;
}

} // namespace drv3d_dx12
