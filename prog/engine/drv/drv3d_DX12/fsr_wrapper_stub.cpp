// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "fsr_wrapper.h"

namespace amd
{
class FSRD3D12
{};
} // namespace amd

namespace drv3d_dx12
{

bool FsrWrapper::fsrInit() { return false; }
bool FsrWrapper::fsrCreateFeature(const ContextArgs &) { return false; }
bool FsrWrapper::fsrShutdown() { return true; }
void FsrWrapper::teardownUpscaling() {}

bool FsrWrapper::evaluateFsr(void *, const FsrParamsDx12 &) { return false; }
Point2 FsrWrapper::getNextJitter(uint32_t, uint32_t) { return Point2::ZERO; }
IPoint2 FsrWrapper::getFsrRenderResolution(UpscalingMode, const IPoint2 &target_resolution) const { return target_resolution; }
FsrWrapper::UpscalingMode FsrWrapper::getUpscalingMode() const { return UpscalingMode::Off; }

bool FsrWrapper::isLoaded() const { return false; }
bool FsrWrapper::isUpscalingSupported() const { return false; }
bool FsrWrapper::isFrameGenerationSupported() const { return false; }
String FsrWrapper::getVersion() const { return {}; }

bool FsrWrapper::isFrameGenerationEnabled() const { return false; }
bool FsrWrapper::isFrameGenerationSuppressed() const { return false; }
void FsrWrapper::enableFrameGeneration(bool) {}
void FsrWrapper::suppressFrameGeneration(bool) {}
void FsrWrapper::doScheduleGeneratedFrames(const amd::FSR::FrameGenPlatformArgs &, void *) {}
int FsrWrapper::getPresentedFrameCount() { return 0; }
uint64_t FsrWrapper::getMemoryUsage() const { return 0; }

#if _TARGET_PC_WIN
bool FsrWrapper::createFrameGenerationSwapchain(DXGIFactory *, ID3D12CommandQueue *, const SwapchainCreateInfo &,
  const DXGI_SWAP_CHAIN_DESC1 &, const DXGI_SWAP_CHAIN_FULLSCREEN_DESC &, DXGISwapChain **swapchain)
{
  if (swapchain)
    *swapchain = nullptr;
  return false;
}

void FsrWrapper::releaseFrameGenerationSwapchainContext() {}

bool create_fsrfg_swapchain(DXGIFactory *, ID3D12CommandQueue *, const SwapchainCreateInfo &, const DXGI_SWAP_CHAIN_DESC1 &,
  const DXGI_SWAP_CHAIN_FULLSCREEN_DESC &, DXGISwapChain **)
{
  return false;
}
#endif

FsrWrapper::FsrWrapper() = default;
FsrWrapper::~FsrWrapper() = default;

} // namespace drv3d_dx12
